#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

static int running = 0;
static pthread_mutex_t lock;

//common thread handler
void* handler(void* args){
    //wait for signal from main
    while(!running)
        sched_yield();

    //print thread id and unique message
    printf("thread # is %lu and = to %s\n", pthread_self(), (char*)args);

    //apply lock
    pthread_mutex_lock(&lock);
    //decrement count
    --running;
    //unlock
    pthread_mutex_unlock(&lock);

    //print exit message
    printf("thread %lu exit\n", pthread_self());
    return NULL;
}

int main(int argc, char *argv[]){
    pthread_t publisher;
    pthread_t subscriber;
    pthread_t cleanup;
    const char *msg1 = "publisher";
    const char *msg2 = "subscriber";
    const char *msg3 = "cleanup";

    //init mutex
    if (pthread_mutex_init(&lock, NULL)){
        //failed, print error and exit
        printf("failed to init mutex\n");
        return -1;
    }

    //create threads
    pthread_create(&publisher, NULL, &handler, strdup(msg1));
    pthread_create(&subscriber, NULL, &handler, strdup(msg2));
    pthread_create(&cleanup, NULL, &handler, strdup(msg3));

    //sleep for some time
    sleep(5);

    //give away to signal
    running = 3;

    //wait until all threads terminate
    while(1){
        //apply lock and check exit status
        pthread_mutex_lock(&lock);
        //are all threads terminated?
        if (running == 0){
            //if yes, break from the loop
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        //wait for other threads
        sched_yield();
    }

    //print exit message and terminate
    printf("program exit\n");
    return 0;
}
