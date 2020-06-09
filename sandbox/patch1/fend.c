#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <signal.h>

char *get_filename(pid_t child, long addr); 
char *get_permission(char *config, char *file);
char *get_parent_dir(char *pathname);
int isRelative(char *pathname);

int main(int argc, char **argv)
{
	long orig_rax, rax;
	struct user_regs_struct regs;
	int status;
	int first_exec = 0;
	if(argc < 4)
	{
		fprintf(stderr,"Usage : ./fend -c config command");
		exit(1);
	}
	pid_t child = fork();
	if (child < 0 )
	{
		fprintf(stderr,"Fork failed\n");
		exit(1);
	}
	else if(child == 0)
	{
		ptrace(PTRACE_TRACEME,0,NULL,NULL);
		kill(getpid(), SIGSTOP);
		execvp(argv[3],argv+3);//must do a check whether we really have more than 3 arguments
		//printf("This line shouldn't get printed\n");
	}
	else
	{
		while(1)
		{
          		wait(&status);
          		if(WIFEXITED(status))
              			break;
          		orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, NULL);

          		if((int)orig_rax == SYS_open || (int)orig_rax == SYS_openat)
			{
                 		ptrace(PTRACE_GETREGS, child,NULL, &regs);
				unsigned int flag = regs.rsi;
				char *filename = get_filename(child,regs.rdi);
				if((int)orig_rax == SYS_openat)
				{
					filename = get_filename(child,regs.rsi);
					flag = regs.rdx;
					//if(strcmp(filename,".")==0)
					//	filename = get_current_dir_name();
				}
				if(regs.rdi || regs.rsi)
				{
				char *permission = get_permission(argv[2], filename); //argv[2] is the config file
				if(permission != NULL)
				{
				
				int no_read = (strcmp(permission,"000") == 0)||(strcmp(permission,"001")==0)||(strcmp(permission, "010")==0)||(strcmp(permission,"011")==0);
				int no_write = (strcmp(permission,"000") == 0)||(strcmp(permission,"100")==0)||(strcmp(permission, "101")==0)||(strcmp(permission,"001")==0);
				if((flag & 1) == 1)
				{
					if(no_write)
					{
					  fprintf(stderr,"Terminating %s: Unauthorized write to file %s\n",argv[3],filename);
					  kill(child, SIGKILL);
					  break;
					}
				}
				if((flag & 2) == 2)
				{
					if(no_read || no_write)
					{
					  fprintf(stderr,"Terminating %s: Unauthorized access to file %s\n",argv[3],filename);
					  kill(child, SIGKILL);
					  break;
					}
				}
				if((flag & 1) == 0)
				{
					if(no_read)
					{
					  fprintf(stderr,"Terminating %s: Unauthorized read to file %s\n",argv[3],filename);
					  kill(child, SIGKILL);
					 break;
					}
				}
				}
				}
          		}
			else if(orig_rax == SYS_mkdir)
			{
                 		ptrace(PTRACE_GETREGS, child,NULL, &regs);
				if(regs.rdi)
				{
				char *filename = get_filename(child,regs.rdi);
				char *permission = get_permission(argv[2], filename); //argv[2] is the config file
				if(permission != NULL)
				{
					int no_permission = (strcmp(permission,"000")==0)||(strcmp(permission,"100")==0)||(strcmp(permission, "101")==0)||(strcmp(permission,"001")==0);
					
					if(no_permission)
					{
						fprintf(stderr,"Terminating %s: Cannot create directory  %s\n",argv[3],filename);
						kill(child, SIGKILL);
						break;
					}
				}
				}
			}
			else if(orig_rax == SYS_unlink)
			{
                 		ptrace(PTRACE_GETREGS, child,NULL, &regs);
				if(regs.rsi)
				{
				char *filename = get_filename(child,regs.rdi);
				char *permission = get_permission(argv[2], filename); //argv[2] is the config file
				if(permission != NULL)
				{
					int no_permission = (strcmp(permission,"000")==0)||(strcmp(permission,"100")==0)||(strcmp(permission, "101")==0)||(strcmp(permission,"001")==0);
					
					if(no_permission)
					{
						fprintf(stderr,"Terminating %s: Cannot remove directory  %s\n",argv[3],filename);
						kill(child, SIGKILL);
						break;
					}
				}
				}
			}
			else if(orig_rax == SYS_execve)
			{
				ptrace(PTRACE_GETREGS, child,NULL, &regs);
				if(regs.rsi)
				{
				  char *filename = get_filename(child,regs.rdi);
				  if(fnmatch(filename,argv[3],FNM_PATHNAME) == FNM_NOMATCH)
				  {
					char *permission = get_permission(argv[2], filename); //argv[2] is the config file
					if(permission != NULL)
					{
					int no_execute = (strcmp(permission,"000")==0)||(strcmp(permission,"100")==0)||(strcmp(permission,"110")==0)||(strcmp(permission, "010")==0);
					if(no_execute)
					{
						fprintf(stderr,"Terminating %s: Permission denied for  %s\n",argv[3],filename);
						kill(child, SIGKILL);
						break;
					}
				  	}
				  }
				}
			}
			else if(orig_rax == SYS_fchmodat)
			{
                 		ptrace(PTRACE_GETREGS, child,NULL, &regs);
				char *filename = get_filename(child,regs.rsi);
				if(regs.rsi)
				{
				char *permission = get_permission(argv[2], filename); //argv[2] is the config file
				if(permission != NULL)
				{
					int no_execute = (strcmp(permission,"000")==0)||(strcmp(permission,"100")==0)||(strcmp(permission,"110")==0)||(strcmp(permission, "010")==0);
					
					if(no_execute)
					{
						fprintf(stderr,"Terminating %s: Permission denied for  %s\n",argv[3],filename);
						kill(child, SIGKILL);
						break;
					}
				}
				}
			}
          		ptrace(PTRACE_SYSCALL, child,NULL, NULL);
       		}
	}
	return 0;
}

char *get_filename(pid_t child, long addr) 
{
    char *val = malloc(4096);
    int allocated = 4096;
    int read = 0;
    unsigned long tmp;
    while (1) {
        if (read + sizeof tmp > allocated) {
            allocated *= 2;
            val = realloc(val, allocated);
        }
        tmp = ptrace(PTRACE_PEEKDATA, child, addr + read);
        if(errno != 0) {
            val[read] = 0;
            break;
        }
        memcpy(val + read, &tmp, sizeof tmp);
        if (memchr(&tmp, 0, sizeof tmp) != NULL)
            break;
        read += sizeof tmp;
    }
    return val;
}

char *get_permission(char *config, char *file)
{
        FILE *fp;
        fp = fopen(config,"r");
        char *p = malloc(20);
        //char *parent = get_parent_dir(file);
        //parent = realloc(parent,strlen(parent)+2);
        //parent = strcat(parent,"/*");
	int match = 0;
        if(fp)
        {
                char *permission = malloc(20);
		char *path = malloc(200);
                while(fscanf(fp,"%s %s",permission, path) != EOF)
                {
                        if(fnmatch(path,file,FNM_PATHNAME) == 0)
                        {
                                p = permission;
				match = 1;
                                //break;
                        }
                }

                fclose(fp);
        }
	if(match)
        	return p;

	return NULL;
}

char *get_parent_dir(char *pathname)
{
	int len = strlen(pathname);
        int i,new_len;
        for(i=len-1; i>=0; i--)
        {
                if(pathname[i] == '/')
                {
                        if(i == len-1)
                                continue;
                        else
                        {
                                new_len=i+1;
                                break;
                        }
                }
        }
        char *parent_dir = malloc(new_len);
        strncpy(parent_dir,pathname,new_len);
        parent_dir[new_len-1] = '\0';
	return parent_dir;
}

int isRelative(char *pathname)
{
        if(pathname == NULL)
                return -1;
        if(pathname[0]=='/')
                return 0;
        else
                return 1;
}
