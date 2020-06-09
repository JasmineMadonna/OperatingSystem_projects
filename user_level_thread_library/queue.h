// A linked list (LL) node to store a queue entry
struct node
{
    void *data;
    struct node *next;
};

struct queue
{
    struct node *front, *rear;
};

// Creates a new linked list node.
struct node* newNode(void *data);

// Creates an empty queue
struct queue *createQueue();

// Adds a data to queue
void enqueue(struct queue *q, void *data);

// Removes node from given queue q
void *dequeue(struct queue *q);

//This is not a generic function of queue.c . Given a thread ID, it will return the _MyThread for that id if found otherwise returns NULL
void *find(struct queue *q, int id);
