#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
/* #include <sys/time.h>  */
/* #include "GroupsockHelper.hh"  */
#include <errno.h>

#include "log.h"


FILE *g_fout = NULL;
const char* LOG_FILE_PREFIX = "debug";
const char* LOG_FILE_POSTFIX = "log";
const char* LOG_FILE_NAME = "debug.log";

void initDebugLog(char* filename)
{
      if(NULL == (g_fout = fopen(filename, "w+")))
      {
          fprintf(stderr, "Open log file error: %s", strerror(errno));
      }
}

int isWriteLog()
{
    return NULL != g_fout ? 1 : 0;
}

void writeLog(int type, const char *fmt,...)
{
    char szTimeString[128];
    struct tm *today;
    time_t tmp_time;
    int off;
    
    char buf[4096] = {0};
    va_list ap;


    tmp_time = time(NULL);
    today = localtime(&tmp_time);
    off = strftime( szTimeString, 128, "%y-%m-%d %H:%M:%S", today );

    
    va_start(ap, fmt);
    vsprintf( buf, fmt, ap );
    va_end( ap); 

    if(INF == type)
    {
        fprintf(g_fout, "[%s][%s]%s\n", szTimeString, "Inf", buf);
    }
    else
    {
        fprintf(g_fout, "[%s][%s]%s\n", szTimeString, "Err", buf);
    }
    
    fflush(g_fout);
}

const char* getName(const char* path)
{
  int i;
  if(NULL == path)
  {
      return "null";
  }
  for(i= strlen(path)-1; i >= 0; i--)
  {
      if(*(path+i) == '\\' || *(path+i) == '/')
      {
          return path+i+1;
      }
  }
  return path;
}

  #define BUF_SIZE  78
  #define OFFSET_HEAD  4
  #define OFFSET_HEX  10
  #define OFFSET_CHAR  62
  
void printHex(const unsigned char* start, int len, const char* filename, int linenumber)
{
  char szTimeString[128];
  struct tm *today;
  time_t tmp_time;
  int off;
  
  int iAllLine ;
  char szLineBuf[BUF_SIZE+1];
  int iCurLine;
  int iCurMaxCharNum;
  int i, j, left;

  
  tmp_time = time(NULL);
  today = localtime(&tmp_time);
  off = strftime( szTimeString, 128, "%y-%m-%d %H:%M:%S", today );
  
  fprintf(g_fout, "[%s][Hex][%s:%d]>> start = %p, lenght = %d\n", 
    szTimeString, filename, linenumber, start, len);

  len = (len == 0) ? 1 : len;
  iAllLine = (len-1) / 16;
  for(iCurLine = 0; iCurLine <= iAllLine; iCurLine++)
  {
    memset(szLineBuf, ' ', BUF_SIZE);
    szLineBuf[BUF_SIZE] = '\0';
    
    sprintf(szLineBuf+OFFSET_HEAD, "[%04d]", iCurLine+1);

    iCurMaxCharNum = (iCurLine == iAllLine ? (len%17) : 16);
    for(i = 0; i < iCurMaxCharNum; i++)
    {
      sprintf(szLineBuf+OFFSET_HEX+i*3, " %02X", start[iCurLine*16+i]);
    }
    for(left = iCurMaxCharNum; left < 16; left++)
    {
      sprintf(szLineBuf+OFFSET_HEX+left*3, "   ");
    }

    sprintf(szLineBuf+OFFSET_HEX+16*3, "    ");
    

    for(j = 0; j < iCurMaxCharNum; j++)
    {
      if(start[iCurLine*16+j] >=33 && start[iCurLine*16+j] <=128)
      {
        szLineBuf[OFFSET_CHAR+j] = start[iCurLine*16+j];
      }
      else
      {
        szLineBuf[OFFSET_CHAR+j] = '.';
      }
    }
    fprintf(g_fout, "%s\n", szLineBuf);
  }
  
  fflush(g_fout);
  
}

const char* timeToStr(const time_t time)
{
  //static char szTimeString[20] = {0};
  //tm* today = localtime(&time);
  //strftime( szTimeString, 20, "%y-%m-%d %H:%M:%S", today );
  //return szTimeString;
  return "incomplete";
}



