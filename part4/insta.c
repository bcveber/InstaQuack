#include "insta.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "TopicQueue.h"

//InstaQuack storage
Proxy_t cleanupProxy;
TopicQueue_t topicQueue[MAXTOPICS];
Proxy_t publisherProxy[NUMPROXIES];
Proxy_t subscriberProxy[NUMPROXIES];

//create topic 1 "xxx yyy zzz" 10
int CreateTopic(const char* cmd){

    char name[80];
    int size, topic;

    //read command and check for any syntax errors
    if (sscanf(cmd, "%*s %*s %d \"%80[^\"]\" %d", &topic, name, &size) != 3){
        printf("Syntax error: %s", cmd);
        return -1;
    }

    //error check
    if (topic < 0 || topic >= MAXTOPICS){
        printf("Topic number exceed the MAXTOPICS value\n");
        return -1;
    }

    //get referenced topic queue
    TopicQueueRef_t q = &topicQueue[topic];

    //initialize mutex
    if (pthread_mutex_init(&q->lock, NULL))
        return -1;

    //initialize our queue parameters
    q->head = 0;
    q->tail = 0;
    q->size = size;
    q->entrycnt = 0;
    q->delta = 0;
    q->name = strdup(name);

    //debug print
    printf("Topic created with name '%s' and size: %d\n", q->name, q->size);

    //return success
    return 0;
}

//query topics
int QueryTopics(const char* cmd){
    //iterate over all the topic queues in memory
    for (int i = 0; i < MAXTOPICS; i++){
        //check if this topic queue is valid
        if (topicQueue[i].name){
            //if yes, then we print the topic queue size and number
            printf("topic %d %d\n", i, topicQueue[i].size);
        }
    }

    //return success
    return 0;
}

//create publisher pub.txt
int CreatePublisher(const char *cmd){
    char name[80];
    //keep count of number of publishers allocated
    static int count = 0;

    //read command and check for any syntax errors
    if (sscanf(cmd, "%*s %*s %s", name) != 1){
        printf("Syntax error: '%s'\n", cmd);
        return -1;
    }

    //error check
    if (count == NUMPROXIES){
        printf("Maximum number of publishers allocated\n");
        return -1;
    }

    //get our publisher proxy
    ProxyRef_t p = &publisherProxy[count];

    //initialize parameters for this proxy
    p->id = count;
    p->status = STATUS_RUNNING;
    p->cmdfile = strdup(name);

    //increment our publisher count and debug print
    count++;
    printf("Publisher added with cmdfile '%s'\n", p->cmdfile);

    //return success
    return 0;
}

//create subscriber pub.txt
int CreateSubscriber(const char *cmd){
    char name[80];
    //keep a count of number of subscribers we've allocated
    static int count = 0;

    //read command and check for any syntax errors
    if (sscanf(cmd, "%*s %*s %s", name) != 1){
        printf("Syntax error: '%s'\n", cmd);
        return -1;
    }

    //error check
    if (count == NUMPROXIES){
        printf("Maximum number of subscribers allocated\n");
        return -1;
    }

    //get our subscriber proxy
    ProxyRef_t s = &subscriberProxy[count];

    //initialize parameters for this proxy
    s->id = count;
    s->status = STATUS_RUNNING;
    s->cmdfile = strdup(name);

    //increment subscriber count and debug print
    count++;
    printf("Subscriber added with cmdfile '%s'\n", s->cmdfile);

    //return success
    return 0;
}

//query publisher
int QueryPublisher(const char *cmd){
    //iterate over all publishers in memory
    for (int i = 0; i < NUMPROXIES; i++){
        //check if we have a valid publisher
        if (publisherProxy[i].cmdfile){
            //if yes, then print publisher id and command file name
            printf("publisher thread %d %s\n", i + 1, publisherProxy[i].cmdfile);
        }
    }

    //return success
    return 0;
}

//query subscriber
int QuerySubscriber(const char *cmd){
    //iterate over all subscribers in memory
    for (int i = 0; i < NUMPROXIES; i++){
        //check if we have a valid subscriber
        if (subscriberProxy[i].cmdfile){
            //if yes, then print subscriber id and command file name
            printf("subscriber thread %d %s\n", i + 1, subscriberProxy[i].cmdfile);
        }
    }

    //return success
    return 0;
}

//delta
int UpdateDelta(const char *cmd){
    int delta;

    //read command and check for any syntax errors
    if (sscanf(cmd, "%*s %d", &delta) != 1){
        printf("Syntax error: '%s'\n", cmd);
        return -1;
    }

    //error check
    if (delta <= 0){
        printf("Invalid delta value\n");
        return -1;
    }

    //iterate over all topics queues in memory
    for (int i = 0; i < MAXTOPICS; i++){
        //check if this queue is valid
        if (topicQueue[i].name){
            //if yes, update our delta
            topicQueue[i].delta = delta;
        }

    }

    //debug print
    printf("Delta updated to %d\n", delta);

    //return success
    return 0;
}

//publisher thread handler
void* Publisher(void *args){
    FILE *fp;
    ProxyRef_t info;
    int topic, seconds;
    TopicEntry_t entry;

    //get our thread parameter which contains publisher details
    info = (ProxyRef_t)args;

    //error check
    assert(info != NULL);

    //debug print
    printf("publisher thread %d %s\n", info->id, info->cmdfile);

    //open our command file
    fp = fopen(info->cmdfile, "r");

    //error check
    if (fp == NULL){
        //mark thread as exited and exit
        info->status = STATUS_EXITED;
        printf("failed to open '%s'\n", info->cmdfile);
        return NULL;
    }

    //scan each line of our command file and process accordingly
    entry.pubID = info->id;
    while(fscanf(fp, " %d %[^\n] %[^\n] %d", &topic, entry.photoUrl, entry.photoCaption, &seconds) > 0){
        //error check for the topic #
        if (topic < 0 || topic >= MAXTOPICS){
            printf("Invalid topic number %d\n", topic);
            break;
        }

        //we're going to try a max of 5 times to enqueue an item to our queue
        int t = 0;
        while(++t < 5){
            //try to enqueue
            if (!enqueue(&topicQueue[topic], &entry))
                break; //if we were successful then break loop

            //yield so other threads can dequeue from the queue
            sched_yield();
        }

        //check if successfully added
        if (t != 5){
            //debug print
            printf("Added %s by %d\n", entry.photoUrl, info->id);
        }

        //sleep
        sleep(seconds);
    }

    //debug print
    printf("publisher thread %d Exited\n", info->id);

    //mark thread as exited and exit
    info->status = STATUS_EXITED;
    return NULL;
}

//subscriber thread handler
void* Subscriber(void *args){
    char name[80];
    int last_entry;
    ProxyRef_t info;
    int topic, seconds;
    TopicEntry_t entry;
    FILE *fp;

    //get our thread parameter which contains subscriber info
    info = (ProxyRef_t)args;

    //error check
    assert(info != NULL);

    //debug print
    printf("subscriber thread %d %s\n", info->id, info->cmdfile);

    //open command file
    fp = fopen(info->cmdfile, "r");

    //error check
    if (fp == NULL){
        //mark thread as exited and exit
        info->status = STATUS_EXITED;
        printf("failed to open '%s'\n", info->cmdfile);
        return NULL;
    }

    //scan each line of our command file and process accordingly
    last_entry = 0;
    while(fscanf(fp, "%d %d", &topic, &seconds) > 0){
        //error check for topic #
        if (topic < 0 || topic >= MAXTOPICS){
            printf("Invalid topic number %d\n", topic);
            break;
        }

        //now try 5 times max to getentry from the queue
        int t = 0;
        while(++t < 5){
            //try getentry
            int x = getentry(&topicQueue[topic], &entry, last_entry);
            if (x > last_entry){
                //if success, break loop
                last_entry = x;
                break;
            }

            //yield so other threads can enqueue to the queue
            sched_yield();
        }

        //check if successfully fetched
        if (t != 5){
            //debug print and write to our html file
            printf("Recieved %s by %d\n", entry.photoUrl, info->id);
        }

        //sleep
        sleep(seconds);
    }

    //debug print
    printf("subscriber thread %d Exited\n", info->id);

    //mark thread as exited and exit
    info->status = STATUS_EXITED;
    return NULL;
}

//clean up thread handler
void* Cleanup(void *args){
    int flag = 1;
    ProxyRef_t info;

    //get thread parameter which contains cleanup thread details
    info = (ProxyRef_t)args;

    //error check
    assert(info != NULL);

    //debug print
    printf("dequeue thread created\n");

    //while atleast one thread is running other than main and cleanup
    while(flag){
        //reset flag
        flag = 0;

        //check for subscriber thread
        for (int i = 0; i < NUMPROXIES && !flag; i++){
            if (subscriberProxy[i].status == STATUS_RUNNING)
                flag++;
        }


        //check for publisher thread
        for (int i = 0; i < NUMPROXIES && !flag; i++){
            if (publisherProxy[i].status == STATUS_RUNNING)
                flag++;
        }

        //now iterate over all valid topic queues and dequeue from that
        for (int i = 0; i < MAXTOPICS; i++){
            if (topicQueue[i].name)
                dequeue(&topicQueue[i]);
        }
    }

    //debug print
    printf("dequeue thread exited\n");

    //mark the thread as exited and exit
    info->status = STATUS_EXITED;
    return NULL;
}

//start
int Start(const char *cmd){
    cleanupProxy.status = STATUS_RUNNING;

    //start publisher thread
    printf("Starting publisher threads\n");

    //iterate over all our proxies
    for (int i = 0; i < NUMPROXIES; i++){
        //now check if we have a valid publisher
        if (publisherProxy[i].cmdfile){
            //error check
            assert(publisherProxy[i].status == STATUS_RUNNING);

            //start thread
            pthread_create(&publisherProxy[i].thread, NULL, &Publisher, &publisherProxy[i]);
        }
    }

    //iterate over all proxies
    printf("Starting subscriber threads\n");
    for (int i = 0; i < NUMPROXIES; i++){
        //check if we have a valid subscriber
        if (subscriberProxy[i].cmdfile){
            //error check
            assert(subscriberProxy[i].status == STATUS_RUNNING);

            //start thread
            pthread_create(&subscriberProxy[i].thread, NULL, &Subscriber, &subscriberProxy[i]);
        }
    }

    //start cleanup thread
    printf("Starting cleanup thread\n");
    pthread_create(&cleanupProxy.thread, NULL, &Cleanup, &cleanupProxy);

    //now wait until our cleanup thread has exited
    while(cleanupProxy.status != STATUS_EXITED){
        //sleep to get rid of 100% usage incase only cleanup thread is running
        sleep(1);
        //yield
        sched_yield();
    }

    //return success
    return 0;
}

int main(int argc, char *argv[]){
    char line[200], error = 0;

    //all the commands for the server
    char *cmds[8] = {"create topic", "query topics", "create publisher", "create subscriber", "query publishers", "query subscriber", "delta ", "start"};

    //all the command handlers for the server
    int (*handlers[8])(const char*) = {CreateTopic, QueryTopics, CreatePublisher, CreateSubscriber, QueryPublisher, QuerySubscriber, UpdateDelta, Start};

    //scan each line of command file until an error or we reach end
    while(!error && scanf(" %[^\n]", line) > 0){
        //error check
        if (strlen(line) == 0)
            continue;

        //mark error as true
        error = 1;

        //scan for each of the commands
        for (int i = 0; i < 8; i++){
            //check if this is that command
            if (strncmp(line, cmds[i], strlen(cmds[i])) == 0){
                //if yes, then call our handler and set error flag
                if (!(*handlers[i])(line)) error = 0;
                break; //we have handled it, so break out of the loop
            }
        }
    }

    //if our loop terminated and error flag is set then we prob have a syntax error
    if (error) printf("Invalid command.\n");

    //return exit code
    return error;
}
