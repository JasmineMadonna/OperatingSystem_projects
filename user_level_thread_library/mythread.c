#include "queue.h"
#include "mythread.h"
#include "ucontext.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>

#define STACK_SIZE 1024*32

struct _MyThread
{
	ucontext_t context;
	int id;
	int parent_id;
	int child_count;
	int join_count;
	bool running;
	bool exited;
	bool join; //for child
	bool joinall; //for parent
};

struct Semaphore
{
	int value;
	int id;
	struct queue *thread_list;
};

ucontext_t main_context;
struct queue *ready_q;
struct queue *blocked_q;
static int thread_id = 0;
struct _MyThread *running_thread;

static int sem_id = 0;

void MyThreadInit(void(*start_funct)(void *), void *args)
{
	//Initialize the ready and blocked queue
	ready_q = createQueue();
	blocked_q = createQueue();
	
	struct _MyThread *main_thread = (struct _MyThread*)malloc(sizeof(struct _MyThread));
	getcontext(&main_thread->context);

	main_thread->context.uc_link = 0;
	main_thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	main_thread->context.uc_stack.ss_size = STACK_SIZE;
	main_thread->context.uc_stack.ss_flags = 0;
	
	main_thread->id = ++thread_id;  //thread_id is 1 for the main thread
	main_thread->parent_id = 0;
	main_thread->child_count = 0;
	main_thread->join_count = 0;
	main_thread->exited = false;
	main_thread->join = false;
	main_thread->joinall = false;

	makecontext(&main_thread->context, (void(*)(void))start_funct, 1, args);
	main_thread->running = true;
	
	running_thread = main_thread;
	//Start running the thread pointed by start_func
	//setcontext(&running_thread->context);
	swapcontext(&main_context, &running_thread->context);
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	struct _MyThread *thread = (struct _MyThread*)malloc(sizeof(struct _MyThread));
	getcontext(&thread->context);

	thread->context.uc_link = 0;
	thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	thread->context.uc_stack.ss_size = STACK_SIZE;
	thread->context.uc_stack.ss_flags = 0;

	thread->id = ++thread_id;
	thread->parent_id = running_thread->id;
	thread->child_count = 0;
	thread->join_count = 0;
	thread->exited = false;
	thread->join = false;
	thread->joinall = false;

	running_thread->child_count++;

	//Creating a new context
	makecontext(&thread->context, (void(*)(void))start_funct, 1, args);
	//Adding the thread to the ready queue
	enqueue(ready_q,thread);
	return (MyThread)thread;
}

void MyThreadYield(void)
{
	//Taking the thread at the front of the queue to execute next
	struct _MyThread *next_thread = (struct _MyThread*)dequeue(ready_q);
	if(next_thread == NULL)
	{
		fprintf(stdout,"No more threads to execute. Exiting\n");
		return;
	}
	
	//adding the current running thread to the queue
	struct _MyThread *current_thread = running_thread;
	enqueue(ready_q,current_thread);
	running_thread = next_thread;
	swapcontext(&current_thread->context,&running_thread->context);
}

int MyThreadJoin(MyThread thread)
{
	struct _MyThread *child = (struct _MyThread*)thread; 
	//thread is not the immediate child of invoking thread
	if(running_thread->id != child->parent_id)
	{
		fprintf(stderr,"thread %d is not the immediate child of thread %d\n",child->id,running_thread->id);
		return -1;
	}
	//child has exited
	if(child->exited)
		return 0;
	
	child->join = true;
	running_thread->join_count++;
	
	struct _MyThread *parent = running_thread;
	if(find(blocked_q,parent->id) == NULL)
		enqueue(blocked_q, parent);
	
	struct _MyThread *head_ready_q = (struct _MyThread*)dequeue(ready_q);
	if(head_ready_q == NULL)
	{
		fprintf(stdout,"No more threads to execute. Exiting\n");
		return 0;
	}
	head_ready_q->running = true;
	running_thread = head_ready_q;
	return swapcontext(&parent->context,&running_thread->context);	
}

void MyThreadJoinAll(void)
{
	struct _MyThread *parent = running_thread;
	if(find(blocked_q,parent->id) == NULL)
		enqueue(blocked_q, parent);
	
	parent->join_count = parent->child_count;
	parent->joinall = true;
	struct _MyThread *head_ready_q = (struct _MyThread*)dequeue(ready_q);
	if(head_ready_q == NULL)
	{
		fprintf(stdout,"No more threads to execute. Exiting\n");
		return;
	}
	head_ready_q->running = true;
	running_thread = head_ready_q;
	swapcontext(&parent->context,&running_thread->context);	
}

void MyThreadExit(void)
{
	running_thread->exited = true;
	int parent_id = running_thread->parent_id;
	struct _MyThread *parent = find(blocked_q,parent_id);
	if(parent != NULL)
	{
		parent->child_count--;
		if(running_thread->join || parent->joinall)
		{
			parent->join_count--;
			if(parent->join_count == 0)
				enqueue(ready_q,parent);
		}
	} 
	//thread at the head of queue becomes the next running thread
	running_thread = dequeue(ready_q);
	if(running_thread != NULL)
	{
		running_thread->running = true;
		setcontext(&running_thread->context);
	}
	else
		setcontext(&main_context);
}

MySemaphore MySemaphoreInit(int initialValue)
{
	if(initialValue < 0)
		return NULL;
	struct Semaphore *sem = (struct Semaphore*)malloc(sizeof(struct Semaphore));
	sem->value = initialValue;
	sem->id = ++sem_id;
	sem->thread_list = createQueue();

	if(ready_q == NULL)
		ready_q = createQueue();
	return (MySemaphore)sem;
}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem)
{
	if(sem == NULL)
	{
		fprintf(stderr, "Semaphore NULL, Call MySemaphoreInit first\n");
		return;
	}
	struct Semaphore *s = (struct Semaphore*)sem;
	s->value++;
	if(s->value <= 0)
	{
		struct _MyThread *thread = (struct _MyThread*)dequeue(s->thread_list);
		if(thread != NULL)
			enqueue(ready_q, thread);
	}
}

// Wait on a semaphore
void MySemaphoreWait(MySemaphore sem)
{
	if(sem == NULL)
	{
		fprintf(stderr, "Semaphore NULL, Call MySemaphoreInit first\n");
		return;
	}
	struct Semaphore *s = (struct Semaphore*)sem;
	s->value--;
	if(s->value < 0)
	{	
		//This possibility is less
		if(running_thread == NULL)
		{
			running_thread = (struct _MyThread*)malloc(sizeof(struct _MyThread));
			getcontext(&running_thread->context);
			running_thread->context.uc_link = 0;
			running_thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
			running_thread->context.uc_stack.ss_size = STACK_SIZE;
			running_thread->context.uc_stack.ss_flags = 0;

			running_thread->id = ++thread_id;
			running_thread->child_count = 0;
			running_thread->join_count = 0;
			running_thread->exited = false;
			running_thread->join = false;
			running_thread->joinall = false;
		}
		
		//Add the running thread to the thread_list;
		enqueue(s->thread_list,running_thread);
		struct _MyThread *current_thread = running_thread;
		current_thread->running = false;
		
		//Run the next thread from the head of ready q
		struct _MyThread *head = (struct _MyThread*)dequeue(ready_q);
		if(head != NULL)
		{
			head->running = true;
			running_thread = head;
			swapcontext(&current_thread->context,&running_thread->context);
		}
	} 
}

// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore sem)
{
	if(sem == NULL)
	{
		fprintf(stderr, "Semaphore NULL, Nothing to do\n");
		return -1;
	}
	struct Semaphore *s = (struct Semaphore*)sem;
	if(dequeue(s->thread_list) != NULL)
		return -1;
	free(s->thread_list);
	free(s);
	return 0;
}

void *find(struct queue *q, int id)
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
}

void *find_semaphore(struct queue *q, int id)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
       return NULL;

    bool found = false;
    struct node *temp = q->front;
    while(temp != NULL)
    {
        struct Semaphore *sem = (struct Semaphore*)temp->data;
        if(sem->id == id)
        {
            found = true;
            break;
        }
        else
            temp = temp->next;
    }

    if(found == true)
        return (struct Semaphore*)temp->data;
    else
        return NULL;
}

int delete_semaphore(struct queue *q, int id)
{
    if(q->front == NULL)
	return -1;

    bool found = false;
    struct node *del_node = q->front;
    struct node *prev_node = NULL;
    struct Semaphore *sem;
    while(del_node != NULL)
    {
        sem = (struct Semaphore*)del_node->data;
        if(sem->id == id)
        {
            found = true;
            break;
        }
        else
	{
	    prev_node = del_node;
            del_node = del_node->next;
	}
    }
    if(found == true)
    {
	if(prev_node != NULL)
		prev_node->next = del_node->next;
	if(del_node == q->front)
		q->front = del_node->next;
	if(del_node == q->rear)
		q->rear = prev_node;
	free(sem->thread_list);
	free(sem);
	free(del_node);
	return 0;
    }
    return -1;
}
