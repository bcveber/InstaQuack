#ifndef __INSTA_H__
#define __INSTA_H__

#include <pthread.h>

#define STATUS_INVALID   0
#define STATUS_RUNNING   1
#define STATUS_EXITED    2

typedef struct
{
    int id;
    int status;
    char *cmdfile;
    pthread_t thread;
} Proxy_t, *ProxyRef_t;

const char* HTML_HEADER_FMT = "<!DOCTYPE html><head></head><body>Topic %d<table><tr><td align=\"left\">"
                              "<img SRC=\"puddles.gif\" WIDTH=\"140\" HEIGHT=\"140\">&nbsp&nbsp&nbsp&nbsp&nbsp</a>"
                              "</td><td align=\"center\"><h1>InstaQuack</h1><h1>Subscriber %d : Topic %s</h1></td>"
                              "<td align=\"right\">&nbsp&nbsp&nbsp&nbsp&nbsp<img SRC=\"puddles.gif\" WIDTH=\"140\""
                              "HEIGHT=\"140\"></a></td></tr></table>";

const char* HTML_BODY_FMT   = "<hr><img SRC=\"%s\"></a><br>%s<hr>";

const char* HTML_FOOTER_FMT = "</body></html>";

#endif
