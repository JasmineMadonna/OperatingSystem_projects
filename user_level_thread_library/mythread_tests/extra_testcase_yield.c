#include <stdio.h>
#include "mythread.h"
#include "assert.h" 

void t1(void * who)
{
  int i;
  int re; 

  printf("t1\n");
}

int main(int argc, char *argv[])
{
  
  printf("main: pre\n");
  MyThreadInitExtra();
  printf("main: before\n");
  MyThreadCreate(t1,(void*)0);
  printf("main: after\n");
  MyThreadYield();
  printf("main: end\n");
} 
