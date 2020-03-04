#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <string>

#define LOG_FILE "website.log"

using std::string;


typedef enum class LOG_TYPE
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} LOG_TYPE;

LOG_TYPE log_level = LOG_TYPE::LOG_INFO;

static string get_current_time();
void LOG(LOG_TYPE t, const char *format, ...);

void SET_LOG_LEVEL(LOG_TYPE t)
{
    log_level = t;
}

static string get_current_time()
{
    time_t t = time(NULL);
    char ch[64] = {0};
    strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));
    return ch;
}

void LOG(LOG_TYPE t, const char *format, ...)
{
    if (t < log_level)
        return;

    FILE *pf = fopen(LOG_FILE, "a+");
    if (nullptr == pf)
        return;

    string current_time = get_current_time();
    fprintf(pf, "%s", current_time.c_str());

    switch (t)
    {
    case LOG_TYPE::LOG_DEBUG:
        fprintf(pf, "[DEBUG]");
        break;
    case LOG_TYPE::LOG_INFO:
        fprintf(pf, "[INFO]");
        break;
    case LOG_TYPE::LOG_WARN:
        fprintf(pf, "[WARN]");
        break;
    case LOG_TYPE::LOG_ERROR:
        fprintf(pf, "[ERROR]");
        break;
    }

    va_list arg;
    va_start(arg, format);
    vfprintf(pf, format, arg);
    fprintf(pf, "\n");
    fflush(pf);
    va_end(arg);

    fclose(pf);
}

#endif // __LOG_H__