#include "TopicQueue.h"

#include <string.h>
#include <assert.h>


//topic queue enqueue function
int enqueue(TopicQueueRef_t queue, TopicEntryRef_t entry){
    assert(queue != NULL);
    assert(entry != NULL);

    //apply queue lock
    pthread_mutex_lock(&queue->lock);

    //check if our queue is full
    if ((queue->head + 1) % queue->size == queue->tail){
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    //get an empty slot in the queue
    TopicEntryRef_t qe = &queue->entries[queue->head];

    //update the head pointer to point to next empty slot
    queue->head = (queue->head + 1) % queue->size;

    //update entry data
    qe->pubID = entry->pubID;
    qe->entrynum = ++queue->entrycnt;

    //update timestamp
    gettimeofday(&qe->timestamp, NULL);

    //copy photo url
    strncpy(qe->photoUrl, entry->photoUrl, QUACKSIZE);
    qe->photoUrl[QUACKSIZE - 1] = 0;

    //copy photo caption
    strncpy(qe->photoCaption, entry->photoCaption, CAPTIONSIZE);
    qe->photoCaption[CAPTIONSIZE - 1] = 0;

    //relase queue lock
    pthread_mutex_unlock(&queue->lock);

    //return success
    return 0;
}

//topic queue getentry function
int getentry(TopicQueueRef_t queue, TopicEntryRef_t entry, int last_entry){
    assert(queue != NULL);
    assert(entry != NULL);
    assert(last_entry >= 0);

    //apply queue lock
    pthread_mutex_lock(&queue->lock);

    //check if our queue is empty
    if (queue->head == queue->tail){
        pthread_mutex_unlock(&queue->lock);
        return 0;
    }

    //iterate over our queue
    int latest_entry = 0, j;
    for (int i = 0; i < queue->size; i++){
        //from oldest to latest
        j = (queue->tail + i) % queue->size;

        //check if entry is valid
        if (queue->entries[j].pubID == -1)
            break;

        //check entry number
        if (queue->entries[j].entrynum > last_entry){
            //if latest, fetch it
            latest_entry = queue->entries[j].entrynum;
            break;
        }
    }

    //check if latest entry is fetched
    if (latest_entry > last_entry){
        //if yes, copy the entry into our entry structure
        memcpy(entry, &queue->entries[j], sizeof(TopicEntry_t));
    }

    //relase queue lock
    pthread_mutex_unlock(&queue->lock);

    //return latest entry #
    return latest_entry;
}

//topic queue dequeue function
int dequeue(TopicQueueRef_t queue){
    //error check
    assert(queue != NULL);

    //apply queue lock
    pthread_mutex_lock(&queue->lock);

    //check queue empty case
    if (queue->head == queue->tail){
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    //get current time
    struct timeval now;
    gettimeofday(&now, NULL);

    //calculate how much time has elapsed
    int time_diff = difftime(now.tv_sec, queue->entries[queue->tail].timestamp.tv_sec);

    //check if the amt of time elapsed is greater than our delta
    if (time_diff >= queue->delta){
        //if yes, remove that entry from the queue
        memset(&queue->entries[queue->tail], 0, sizeof(TopicEntry_t));
        queue->entries[queue->tail].pubID = -1;
        queue->tail = (queue->tail + 1) % queue->size;
    }

    //relase queue lock
    pthread_mutex_unlock(&queue->lock);
    return 0;
}
