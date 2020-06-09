/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Author...........: Vincent W. Freeh
 *
 *  The file is modified to implement ush for OS project P3 by Jasmine.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

/*Function declarations*/
int execute(Cmd c);
int getPathname(char *cmd);
int spawn_proc (int in, int out, Cmd c, int err);

char host[1024];
extern char **environ;

void handle_signal(int signo)
{
  printf("\n%s%% ",host);
  fflush(stdout);
}

//int std_out_tmp;
int std_in_tmp;

static void prCmd(Cmd c)
{
  int i, std_out, std_in, out_file, in_file;
  //save the current open input and output file description
  std_in = dup(0);	
  std_out = dup(1);	

  if ( c ) {
  //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    if ( c->in == Tin )
    {
      in_file = open(c->infile, O_RDONLY);
      if(in_file < 0)
      	printf("%s doesn't exist\n",c->infile);
      else
	dup2(in_file,0);
    }
    if ( c->out != Tnil )
      switch ( c->out ) {
      case Tout:
	out_file = open(c->outfile, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	dup2(out_file,1);
	break;
      case Tapp:
    	//std_out = dup(1);	
	out_file = open(c->outfile, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	dup2(out_file,1);
	break;
      case ToutErr:
	out_file = open(c->outfile, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	dup2(out_file,1);
	dup2(out_file,2);
	break;
      case TappErr:
	out_file = open(c->outfile, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	dup2(out_file,1);
	dup2(out_file,2);
	break;
      case Tpipe:
	//printf("| ");
	break;
      case TpipeErr:
	//printf("|& ");
	break;
      default:
	fprintf(stderr, "Shouldn't get here\n");
	exit(-1);
      }

    // this driver understands one command
    if ( !strcmp(c->args[0], "end") )
    {
	//printf ("In end\n");	
      exit(0);
    }
    //Added
    execute(c);
    if(c->outfile)
    {
      dup2(std_out,1);
      if(c->out == ToutErr || c->out == TappErr)
	dup2(std_out, 2);
      close(std_out);
    }
    if(c->in == Tin)
    {
      dup2(std_in,0);
      close(std_in);
    }
  }
}

int spawn_proc (int in, int out, Cmd c, int err)
{
  int status;
  pid_t pid = fork();

  if (pid == 0)
  {
    if (in != 0)
    {
      dup2 (in, 0);
      close (in);
    }
    if (out != 1)
    {
      dup2 (out, 1);
      if(err)
      	dup2 (out, 2);
      close (out);
    }
    execvp (c->args[0], c->args);
   }
   else if(pid == 1)
	waitpid(pid,&status, 0);

  return pid;
}

static void prPipe(Pipe p)
{
  int i = 0;
  int err = 0;
  int in, fd[2];
  Cmd c;
  
  if ( p == NULL )
    return;

  Cmd first_cmd = p->head;
  in = 0;

  if(first_cmd != NULL && (first_cmd->out == Tpipe || first_cmd->out == TpipeErr))
  {
    for ( c = p->head; c != NULL; c = c->next )
    {
     //printf("Inside pipe = %s\n",c->args[0]);
      if(c->out == TpipeErr)
	err = 1;
      else 
	err = 0;

      if(c->next == NULL)
      {
	if (in != 0){
    	  dup2 (in, 0);
    	  close(in);
	  //close(fd[1]);
	}
  	/* Execute the last stage with the current process. */
	//execute(c);
	prCmd(c);
	dup2(std_in_tmp,0);
	//close(std_in_tmp);
	//dup2(std_out_tmp,1);
	//close(std_out_tmp);
      }
      else
      {
      	pipe (fd);
      	spawn_proc (in, fd[1], c, err);
      	close (fd[1]);
      	in = fd[0];
      }
    }
  }
  else
  {
    for ( c = p->head; c != NULL; c = c->next ) {
      prCmd(c);
    }
  }

  prPipe(p->next);
}

//Execute the Cmd c
int execute(Cmd c)
{
  int i;
  int status;

  if(strcmp(c->args[0],"cd") == 0)
  {
    if(c->args[1])
      chdir(c->args[1]);
    else
      chdir("/home/jasmine");
  }
  else if(strcmp(c->args[0],"pwd") == 0)
  {
    char cwd[1024];
    getcwd(cwd,1024);
    printf("%s\n",cwd);
  }
  else if(strcmp(c->args[0],"echo") == 0)
  {
    for(i = 1; i < c->nargs; i++)
    {
      if(strncmp(c->args[i],"$",1) == 0)
      {
        char var[strlen(c->args[i])];
     	int j;
	for(j=0; j < strlen(c->args[i]); j++)
	  var[j] = c->args[i][j+1]; 
	printf("%s ",getenv(var));
      }
      else
	printf("%s ",c->args[i]);
    }
    printf("\n");
  }
  else if(strcmp(c->args[0],"setenv") == 0)
  {
    if(c->nargs == 1)
    {
      char *s = *environ;

      for (i=1; s; i++) {
   	 printf("%s\n", s);
    	s = *(environ+i);
      }
    }
    else if(c->nargs == 2)
      setenv(c->args[1],"",1);
    else if(c->nargs == 3)
      setenv(c->args[1],c->args[2],1);
  }
  else if(strcmp(c->args[0],"unsetenv") == 0)
  { //do I need to do a check here also?
    if(c->nargs == 2)
      unsetenv(c->args[1]);
  }
  else if(strcmp(c->args[0],"nice") == 0)
  {
   if(c->nargs == 1)
     setpriority(PRIO_PROCESS, 0, 4); 
   else if(c->nargs == 2)
     setpriority(PRIO_PROCESS, 0, atoi(c->args[1])); 
   else if(c->nargs >= 3) {
     setpriority(PRIO_PROCESS, 0, atoi(c->args[1])); 
     int s;
     pid_t ch_pid = fork();
     if(ch_pid == 0)
     {
      if(execvp(c->args[2],c->args+2) == -1)
      {
        if((access(c->args[0],F_OK) == 0) && (access(c->args[0],X_OK) == -1))
          printf("permission denied\n");
        else if(strchr(c->args[0],'/') == NULL)
        {
          if(getPathname(strdup(c->args[0])) == 0)
            printf("command not found\n");
        }
      }
     }
     else if(ch_pid == 1)
        waitpid(ch_pid, &s, 0);
   }
  }
  else if(strcmp(c->args[0],"where") == 0)
  {
    if(c->nargs != 2) {
	printf("wrong syntax for where\n");
	return;
    }

    if((strcmp(c->args[1],"cd") == 0) || (strcmp(c->args[1],"echo") == 0) || (strcmp(c->args[1],"nice") == 0) || (strcmp(c->args[1],"pwd") == 0) || (strcmp(c->args[1],"setenv") == 0) || (strcmp(c->args[1],"unsetenv") == 0) || (strcmp(c->args[1],"where") == 0) || (strcmp(c->args[1],"logout") == 0))
	printf("%s is a built in command\n",c->args[1]);
  
    char *tmp_cmd = strdup(c->args[1]);
    char *path = getenv("PATH");
    const char delim[2] = ":";
    char *token;
    char cmd[strlen(tmp_cmd)+2];
    strcpy(cmd,"/");
    strcat(cmd,tmp_cmd);

    int size = 200 * sizeof(char) + strlen(cmd);
    char full_path[size];

    token = strtok(strdup(path),delim);
    while (token != NULL)
    {
      strcpy(full_path,strdup(token));
      strcat(full_path,cmd);

      if(access(full_path, F_OK) == 0) {
        printf("%s\n",full_path);
      }

     token = strtok(NULL, delim);
    }
  }
  else if(strcmp(c->args[0],"logout") == 0)
    exit(0);
  else 
  {
    pid_t pid = fork();

    if(pid < 0)
    {
      fprintf(stderr,"Fork failed\n");
      //exit(1);
    }
    else if(pid == 0) //start of child : pid == 0
    {
      if(execvp(c->args[0],c->args) == -1)
      {
  	if((access(c->args[0],F_OK) == 0) && (access(c->args[0],X_OK) == -1))
      	  printf("permission denied\n");
	else if(strchr(c->args[0],'/') == NULL)
	{
      	  if(getPathname(strdup(c->args[0])) == 0)
	    printf("command not found\n");
	}
      }
    } //end of child pid == 0
    else {
      waitpid(pid,&status,0);
    }
  }
}

int getPathname(char *cmd)
{
  char slash_cmd[strlen(cmd)+2]; 
  strcpy(slash_cmd,"/");
  strcat(slash_cmd,cmd);

  char *path = getenv("PATH");
  int size = (200 * sizeof(char)) + strlen(slash_cmd);
  const char delim[2] = ":";
  char *token;
  char full_path[size];

  token = strtok(strdup(path),delim);
  while (token != NULL)
  {
    strcpy(full_path,strdup(token));
    strcat(full_path,slash_cmd);

    if(access(full_path, X_OK) == 0)
        return 1;

    token = strtok(NULL, delim);
  }
  return 0;
}


int main(int argc, char *argv[])
{
  Pipe p;
  //char host[1024];
  size_t buf = 1024;
  gethostname(host, 1024);

  signal(SIGINT, SIG_IGN);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, SIG_IGN);
  
  //std_out_tmp = dup(1);
  std_in_tmp = dup(0);

  while ( 1 ) {
    printf("%s%% ", host);
    p = parse();
    prPipe(p);
    freePipe(p);
  }
}

/*........................ end of main.c ....................................*/
