
struct queuenode{
	int val;
	struct queuenode* next;
};
struct threadqueue{
	struct queuenode* head;
	struct queuenode* tail;
};
struct threadqueue* init_queue(int);
int enqueue(struct threadqueue*,int);
int dequeue(struct threadqueue*);
