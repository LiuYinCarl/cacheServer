#include <unistd.h>
#include "server.h"

int main(int argc, char *argv[])
{
    // set signal handler
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    int option_daemon = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0)
    {
        option_daemon = 1;
    }

    if (option_daemon)
    {
        pid_t pid;
        pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            daemon(1, 0);
        }
    }

    redis_server.reset(new RedisServer(REDIS_ADDR, REDIS_PORT));

    event_init();

    struct evhttp *server(evhttp_start(SERVER_ADDR, SERVER_PORT));
    if (nullptr == server)
    {
        LOG(LOG_TYPE::LOG_ERROR, "<%s> create evhttp failed", __func__);
        return -1;
    }

    evhttp_set_timeout(server, 5);
    evhttp_set_cb(server, "/visit_info", visit_info_cb, nullptr);
    evhttp_set_gencb(server, get_cb, nullptr);
    // start event loop
    event_dispatch();
    evhttp_free(server);
    return 0;
}
