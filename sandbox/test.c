#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fnmatch.h>

char *get_permission(char *config, char *file);
main()
{
	//printf("permission = %s\n",(char *)get_permission("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/config", "/afs/unity.ncsu.edu/users/j/jsabari/sandbox/test.txt"));
	//int open("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/tmp",O_WRONLY,S_IXGRP);
	execl("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/fw.out","fw.out",NULL);
	//printf("%d\n",fnmatch("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/d","/afs/unity.ncsu.edu/users/j/jsabari/sandbox/*",FNM_PATHNAME));
}

char *get_permission(char *config, char *file)
{
        FILE *fp;
        fp = fopen(config,"r");
	char *p = malloc(20);
        /*char *parent = get_parent_dir(file);
        parent = realloc(parent,strlen(parent)+2);
        parent = strcat(parent,"/*");
        printf("parent = %s\n",parent);*/
        if(fp)
        {
                //char permission[4], path[120];
		char *permission = malloc(20);
		char *path = malloc(120);
                while(fscanf(fp,"%s %s",permission, path) != EOF)
                {
                        printf("%s %s\n",permission,path);
                        if(fnmatch(file,path,FNM_PATHNAME) == 0)
                        {
                                p = permission;
				//p[4]='\0';
                                //break;
                        }
                }

                fclose(fp);
        }
        return p;
}
