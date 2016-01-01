#ifndef _LOGMACROS_H_
#define _LOGMACROS_H_

#include <time.h>

enum
{
    INF,
    WAN,
    ERR
};

#define DEBUG_LOG(type, fmt, ...) \
    if(isWriteLog())\
    {\
        writeLog(type, "[%s:%d]>> "fmt, getName(__FILE__), __LINE__, ##__VA_ARGS__);\
    }

#define DEBUG_HEX(start, length) \
    if(isWriteLog())\
    {\
        printHex(start, length, getName(__FILE__), __LINE__);\
    }

int isWriteLog();

void initDebugLog(char* filename);

void writeLog(int type, const char *fmt,...);

const char* getName(const char* path);

void printHex(const unsigned char* start, int len, const char* filename, int linenumber);

const char* timeToStr(const time_t time);


#endif

