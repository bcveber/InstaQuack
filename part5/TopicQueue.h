#ifndef __TOPIC_QUEUE_H__
#define __TOPIC_QUEUE_H__

#include "Config.h"

#include <pthread.h>
#include <sys/time.h>

typedef struct
{
    int entrynum;
    struct timeval timestamp;
    int pubID;
    char photoUrl[QUACKSIZE];
    char photoCaption[CAPTIONSIZE];
} TopicEntry_t, *TopicEntryRef_t;

typedef struct
{
    int head;
    int tail;
    int size;
    int delta;
    int entrycnt;
    const char* name;
    pthread_mutex_t lock;
    TopicEntry_t entries[MAXENTRIES];
} TopicQueue_t, *TopicQueueRef_t;

int enqueue(TopicQueueRef_t queue, TopicEntryRef_t entry);
int getentry(TopicQueueRef_t queue, TopicEntryRef_t entry, int last_entry);
int dequeue(TopicQueueRef_t queue);

#endif
