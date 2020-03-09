#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BUF_SIZE 1024 * 1024

#define REDIS_ADDR "127.0.0.1"
#define REDIS_PORT 6379
#define REDIS_PASSWORD "hello world"
/* if define it, you need a password to connect redis */
#define NEED_AUTH
#define MAX_RETRY_COUNT 5
#define WANT_VISIT_COUNT 100

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080

/* forbid the client request website.log */
#define HIDE_LOG_FILE

#endif // __CONFIG_H__