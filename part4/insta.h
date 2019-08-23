#ifndef __INSTA_H__
#define __INSTA_H__

#include <pthread.h>

#define STATUS_INVALID   0
#define STATUS_RUNNING   1
#define STATUS_EXITED    2

typedef struct{
    int id;
    int status;
    char *cmdfile;
    pthread_t thread;
} Proxy_t, *ProxyRef_t;

#endif
