#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
	FILE *fp;
	int res;
	res = creat("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/testr.txt", O_RDONLY|O_CREAT);
	printf("res = %d\n",res);
   	//fp = fopen("/afs/unity.ncsu.edu/users/j/jsabari/sandbox/testw1.txt", "w");
   	//fp1 = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/3.txt", "r");
	//printf("File opened\n");
	//system("lsof");
//	printf("file descriptor = %d\n",fp);
	//printf("after fpen 1.txt\n");
   	//fp = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/2.txt", "r+");
	//printf("after fpen 2.txt\n");
   	//fp = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/3.txt", "w");
	//printf("after fpen 3.txt\n");
   	//fp = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/4.txt", "w+");
   	//fp = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/5.txt", "a");
   	//fp = fopen("/home/jasmine/NCSU/Classes/OS/test_programs/6.txt", "a+");
   	//fprintf(fp, "This is testing for fprintf...\n");
   	//fputs("This is testing for fputs...\n", fp);
   //	fclose(fp);
   	return 0;
}
