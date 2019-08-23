#include "TopicQueue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

//init queue function
void InitQueue(TopicQueueRef_t q){
    q->head = 0;
    q->tail = 0;
    q->size = 10;
    q->delta = 5;
    q->entrycnt = 0;
    q->name = strdup("MyQueue");
    assert(pthread_mutex_init(&q->lock, NULL) == 0);
}

int main(int argc, char *argv[]){
    TopicQueue_t q;
    TopicEntry_t e;

    //init queue
    InitQueue(&q);

    //fill the queue
    for (int i = 1; i < q.size; i++){
        //random pub id
        e.pubID = rand() % 3;

        //fill required fields
        sprintf(e.photoUrl, "photo_%3d.jpg", i);
        sprintf(e.photoCaption, "caption %3d", i);

        //check insertion
        assert(enqueue(&q, &e) == 0);
    }

    //check insertion
    assert(enqueue(&q, &e) == -1);
    printf("Passed 1\n");

    //sleep for sometime
    sleep(5);

    //check deletion
    for (int i = 1; i < q.size; i++)
        assert(dequeue(&q) == 0);

    //check deletion
    assert(dequeue(&q) == -1);
    printf("Passed 2\n");

    //fill the queue
    InitQueue(&q);
    for (int i = 1; i < q.size; i++){
        e.pubID = rand() % 3;
        sprintf(e.photoUrl, "photo_%3d.jpg", i);
        sprintf(e.photoCaption, "caption %3d", i);
        assert(enqueue(&q, &e) == 0);
    }

    //sleep for sometime
    sleep(5);

    //delete 2 entries
    dequeue(&q);
    dequeue(&q);

    //get entry
    int lastentry = getentry(&q, &e, 0);
    assert(lastentry == 3);
    printf("Passed 3\n");

    lastentry = getentry(&q, &e, lastentry);
    assert(lastentry == 4);
    printf("Passed 4\n");

    //return success
    return 0;
}
