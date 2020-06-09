#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
 
struct node* newNode(void *data)
{
    struct node *temp = (struct node*)malloc(sizeof(struct node));
    temp->data = data;
    temp->next = NULL;
    return temp; 
}
 
struct queue *createQueue()
{
    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    q->front = q->rear = NULL;
    return q;
}
 
void enqueue(struct queue *q, void *data)
{
    struct node *temp = newNode(data);
 
    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL)
    {
       q->front = q->rear = temp;
       return;
    }
 
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}
 
void *dequeue(struct queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
       return NULL;
 
    // Store previous front and move front one node ahead
    struct node *temp = q->front;
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
       q->rear = NULL;
    return temp->data;
}

//This is not a generic function of queue.c . Given a thread ID, it will return the _MyThread for that id if found otherwise returns NULL
/*void *find(struct queue *q, int id)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
       return NULL;
    
    bool found = false;
    struct node *temp = q->front;
    while(temp != NULL)
    {
	struct _MyThread *thread = (struct _MyThread*)temp->data;
	if(thread->id == id)
	{
	    found = true;
	    break;
	}
	else
	    temp = temp->next;
    }

    if(found == true)
	return (struct _MyThread*)temp->data;
    else
        return NULL;
}*/

/*int main()
{
    struct queue *q = createQueue();
    struct _MyThread *main_thread = (struct _MyThread*)malloc(sizeof(struct _MyThread));
    main_thread->id = 1;
    main_thread->parent_id = 0;
    struct _MyThread *child_thread = (struct _MyThread*)malloc(sizeof(struct _MyThread));
    child_thread->id = 2;
    child_thread->parent_id = 1;
    enqueue(q, main_thread);
    enqueue(q, child_thread);
    struct _MyThread *thread = (struct _MyThread*)find(q,3);
    if(thread == NULL)
	printf("Thread not found in q\n");
    else
	printf("parent id of thread = %d\n",thread->parent_id);
	int a = 10;
	int b = 20;
    enqueue(q, &a);
    //enqueue(q, &b);
    dequeue(q);
    //dequeue(q);
    //enqueue(q, 40);
    //enqueue(q, 50);
    int *data = (int *)dequeue(q);
    if (data != NULL)
    {
      printf("Dequeued item is %d\n", *data);
    }	
    return 0;
}*/
