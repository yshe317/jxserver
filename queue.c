#include "queue.h"
#include <stdlib.h>

int enqueue(struct threadqueue* q,int val) {
    struct queuenode* newnode = calloc(1,sizeof(struct queuenode));
    newnode->val = val;
    if(q -> head == NULL) {
        q->head = newnode;
        q->tail = newnode;
    }else{
        q->tail->next = newnode;
        q->tail = newnode;
    }
    return 0;
}
int dequeue(struct threadqueue* q) {
    if(q->head == NULL) {
        return -1;
    }
    int result = q->head->val;
    struct queuenode* temp = q->head;

    q->head = q->head->next;
    if(q->head == NULL) {
        q->tail = NULL;
    }
    free(temp);
    return result;

}

struct threadqueue* init_queue(int num) {
    struct threadqueue* q = calloc(1,sizeof(struct threadqueue));
    for(int i = 0;i<num;i++) {
        enqueue(q,i);
    }
    return q;
}