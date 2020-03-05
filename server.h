#include <sys/stat.h>
#include <stdlib.h>
#include <cstring>
#include <csignal>
#include <fstream>
#include <memory>
#include <iostream>
#include <string>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_compat.h>
#include <event2/http_struct.h>
#include <hiredis/hiredis.h>
#include "log.h"
#include "config.h"

#define DEBUG LOG_TYPE::LOG_DEBUG
#define INFO LOG_TYPE::LOG_INFO
#define WARN LOG_TYPE::LOG_WARN
#define ERROR LOG_TYPE::LOG_ERROR

using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

// redis 连接
class RedisServer;
static unique_ptr<RedisServer> redis_server = nullptr;

class RedisServer
{
public:
    RedisServer(string addr, int port)
        : redis_addr(addr), redis_port(port) { connect_redis(); }

    ~RedisServer() { redisFree(conn.release()); }

    /**
     * add visited web page into Redis cache
     */
    void insert_cache(shared_ptr<const string> page_link, shared_ptr<string> page);

    /**
     * query if the web page of the page link in Redis cache 
     */
    bool query_cache(shared_ptr<const string> page_link, shared_ptr<string> page);

    /**
     *  add a record into Redis cache if someone visit any page
     */
    void add_visit_record(shared_ptr<const string> page_link);

    /**
     *  get the page link and visit time of the most popular (end-start) pages
     */
    bool get_visit_record(shared_ptr<string> page, int start, int end);

    /**
     * anyone visit any page, total visit count +1
     */
    void add_total_visit_count();

    /**
     *  get the total visit count of all pages
     */
    long long get_total_visit_count();

    /**
     *  make a connection to the redis server
     */
    bool connect_redis();

    /**
     *  checke if we are connecting the Redis server
     */
    bool check_connect() { return (nullptr == conn) ? false : true; }

    // void free_redis();

private:
    string redis_addr;
    int redis_port;
    int retry_count = 0;
    bool redis_is_health = true;
    unique_ptr<redisContext> conn = nullptr;
};

void RedisServer::insert_cache(shared_ptr<const string> page_link,
                               shared_ptr<string> page)
{
    if (nullptr == page_link || nullptr == page || !redis_is_health)
        return;

    if (nullptr == conn && !connect_redis())
        return;

    string key{"pageCache"};

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "HSET %s %s %s", key.c_str(), page_link->c_str(), page->c_str()));
    if (reply == nullptr)
    {
        LOG(WARN, "<%s> page_link:%s reply_type:%d", __func__, page_link->c_str(), -1);
        return;
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOG(WARN, "<%s> page_link:%s reply:%s", __func__, page_link->c_str(), reply->str);
    }

    freeReplyObject(reply.release());
}

void RedisServer::add_total_visit_count()
{
    if (!redis_is_health || (nullptr == conn && !connect_redis()))
        return;

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "INCR totalVisitCount"));
    if (nullptr == reply)
    {
        LOG(WARN, "<%s> INCR totalVisitCount failed reply_type:%d", __func__, -1);
        return;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOG(WARN, "<%s> reply:%s", __func__, reply->str);
    }

    freeReplyObject(reply.release());
}

long long RedisServer::get_total_visit_count()
{
    if (!redis_is_health || (nullptr == conn && !connect_redis()))
        return -1;

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "GET totalVisitCount"));

    if (nullptr == reply || reply->type == REDIS_REPLY_ERROR)
    {
        LOG(WARN, "<%s> get totalVisitCount failed %d", __func__, (reply != nullptr) ? reply->type : -1);
        return -1;
    }

    return atoi(reply->str);
}

void RedisServer::add_visit_record(shared_ptr<const string> page_link)
{
    if (nullptr == page_link || !redis_is_health)
        return;

    if (nullptr == conn && !connect_redis())
        return;

    add_total_visit_count();

    string key{"visitRecordMap"};

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "ZINCRBY %s 1 %s", key.c_str(), page_link->c_str()));
    if (nullptr == reply)
    {
        LOG(WARN, "<%s> page_link:%s reply_type:%d", __func__, page_link->c_str(), -1);
        return;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOG(WARN, "<%s> page_link:%s reply:%s", __func__, page_link->c_str(), reply->str);
    }

    freeReplyObject(reply.release());
    return;
}

bool RedisServer::get_visit_record(shared_ptr<string> page, int start, int end)
{
    if (!redis_is_health || (nullptr == conn && !connect_redis()))
        return false;

    string key{"visitRecordMap"};

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "ZREVRANGE %s %d %d WITHSCORES", key.c_str(), start, end));

    if (nullptr == reply)
    {
        LOG(WARN, "<%s> reply_type:%d", __func__, -1);
        return false;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOG(WARN, "<%s> reply:%s", __func__, reply->str);

        freeReplyObject(reply.release());
        return false;
    }

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        int n = reply->elements;
        string tmp;
        for (int i = 0; i < n; ++i)
        {
            if (i % 2)
            {
                /*TODO I know it's ugly, because I am not sure the reply type of ZREVRANGE. in the future I will modify the code */
                (reply->element[i]->type == REDIS_REPLY_STRING) ? tmp.append(reply->element[i]->str) : tmp.append(std::to_string(reply->element[i]->integer));

                tmp.append("</br>");
                page->append(tmp);
                tmp = "";
            }
            else
            {
                (reply->element[i]->type == REDIS_REPLY_STRING) ? tmp.append(reply->element[i]->str) : tmp.append(std::to_string(reply->element[i]->integer));

                tmp.append("<br>[VISIT COUNT]:");
            }
        }
    }

    freeReplyObject(reply.release());
    return true;
}

bool RedisServer::query_cache(shared_ptr<const string> page_link, shared_ptr<string> page)
{
    if (nullptr == page_link || !redis_is_health)
        return false;

    if (nullptr == conn && !connect_redis())
        return false;

    
    add_visit_record(page_link);

    string key{"pageCache"};

    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "HGET %s %s", key.c_str(), page_link->c_str()));
    if (nullptr == reply)
    {
        LOG(WARN, "<%s> page_link:%s reply_type:%d", __func__, page_link->c_str(), -1);
        return false;
    }

    // query result is null means the page isn't in Redis cache
    if (reply->type == REDIS_REPLY_NIL)
    {
        freeReplyObject(reply.release());
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        LOG(WARN, "<%s> page_link:%s reply_type:%d", __func__, page_link->c_str(), reply->type);

        freeReplyObject(reply.release());
        return false;
    }

    page->append(reply->str);
    freeReplyObject(reply.release());
    return true;
}

bool RedisServer::connect_redis()
{
    if (retry_count > MAX_RETRY_COUNT)
    {
        redis_is_health = false;
        return false;
    }

    conn.reset(redisConnect(redis_addr.c_str(), redis_port));
    if (conn->err)
    {
        LOG(ERROR, "<%s> error_flag:%d, addr:%s port: %d", __func__, conn->err, redis_addr.c_str(), redis_port);

        retry_count += 1;
        return false;
    }

#ifdef NEED_AUTH
    unique_ptr<redisReply> reply((redisReply *)redisCommand(conn.get(), "AUTH %s", REDIS_PASSWORD));

    if (nullptr == reply)
    {
        LOG(INFO, "<%s> reply is null", __func__);
        return false;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOG(ERROR, "<%s> redis AUTH error addr:%s port:%d, password:%s", __func__, redis_addr.c_str(), redis_port, REDIS_PASSWORD);
        freeReplyObject(reply.release());
        return false;
    }
#endif

    LOG(INFO, "<%s> redis connect success! addr:%s port:%d", __func__, redis_addr.c_str(), redis_port);
    retry_count = 0;
    return true;
}

inline unique_ptr<const string> parse_request_path(struct evhttp_request *req)
{
    if (nullptr == req)
        return nullptr;

    string uri{evhttp_request_get_uri(req)};
    if (uri == "")
    {
        LOG(INFO, "<%s> URI is null", __func__);
        return nullptr;
    }

    struct evhttp_uri *decode_uri = evhttp_uri_parse(uri.c_str());
    if (nullptr == decode_uri)
    {
        LOG(INFO, "<%s> decode_uri is null", __func__);
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return nullptr;
    }

    unique_ptr<string> path(new string(evhttp_uri_get_path(decode_uri)));
    evhttp_uri_free(decode_uri);
    return path;
}

void visit_info_cb(struct evhttp_request *req, void *arg)
{
    if (nullptr == req)
    {
        LOG(ERROR, "<%s> visit_info request is null", __func__);
        return;
    }

    long long total_visit_cnt = redis_server->get_total_visit_count();
    shared_ptr<string> page(new string);
    redis_server->get_visit_record(page, 0, WANT_VISIT_COUNT);

    struct evbuffer *retbuff = evbuffer_new();
    if (nullptr == retbuff)
        return;

    string html1 = R"(<html> <head> <meta charset="UTF-8"></head><body> Total Visit Count:)";
    string html2 = R"(</body></html>)";

    evbuffer_add_printf(retbuff, "%s %lld</br> %s %s",
                        html1.c_str(), total_visit_cnt, page->c_str(), html2.c_str());

    // HTTP header
    evhttp_add_header(req->output_headers, "Server", "HTTP v1.1");
    evhttp_add_header(req->output_headers, "Content-Type", "text/html");
    evhttp_add_header(req->output_headers, "Connection", "close");

    evhttp_send_reply(req, HTTP_OK, "OK", retbuff);
}

void get_cb(struct evhttp_request *req, void *arg)
{
    if (nullptr == req)
    {
        LOG(INFO, "<%s> get request is null", __func__);
        return;
    }

    shared_ptr<const string> path_old(parse_request_path(req));
    if (nullptr == path_old)
    {
        LOG(INFO, "<%s> path is null", __func__);
        return;
    }
    /* TODO In the future, take out the common code below, 
    and delete path, just use path_old as the common page link */
    // shared_ptr<string> path(new string(*path_old.get()));
    /* remove the first character '/' */
    shared_ptr<string> path(new string(path_old->substr(1)));

    /* check if file is exist before open it */
    struct stat buf;
    if (stat(path->c_str(), &buf) == -1)
    {
        LOG(WARN, "<%s> file not exist file:%s", __func__, path->c_str());
        path->assign("404.html");
    }

#ifdef HIDE_LOG_FILE
    if (path->compare("website.log") == 0)
    {
        path->assign("404.html");
    }
#endif

    LOG(DEBUG, "<%s> path:%s", __func__, path.get());

    struct evbuffer *retbuff = evbuffer_new();
    if (nullptr == retbuff)
        return;

    shared_ptr<string> page(new string);

    string::size_type i = path->rfind('.');
    if (i == string::npos)
    {
        LOG(INFO, "<%s> error page link:%s:", __func__, path->c_str());
        path->assign("404.html");
        i = path->rfind('.');
    }

    const string ext = path->substr(i);

    string content_type;

    if (ext.compare(".html") == 0 || ext.compare(".htm") == 0)
        content_type = "text/html";
    else if (ext.compare(".ico") == 0)
        content_type = "image/x-icon";
    else if (ext.compare(".js") == 0)
        content_type = "application/x-javascript";
    else if (ext.compare(".css") == 0)
        content_type = "text/css";
    else if (ext.compare(".pdf") == 0)
        content_type = "application/pdf";
    else if (ext.compare(".png") == 0)
        content_type = "application/x-png";
    else
    {
        LOG(INFO, "<%s> unknown extension:%s", __func__, ext.c_str());
        /* take unknow files types as plain file*/
        content_type = "text/plain";
    }

    do
    {
        /* query page in Redis Cache */
        if (redis_server->query_cache(path, page))
        {
            evbuffer_add_printf(retbuff, "%s\n", page->c_str());
            LOG(DEBUG, "<%s> get page from cache path_link:%s", __func__, path->c_str());

            break;
        }

        std::ifstream file(*path);
        shared_ptr<string> buffer(new string);
        if (!file.is_open())
        {
            LOG(WARN, "<%s> open file error file:%s", __func__, path->c_str());
            path->assign("404.html");
        }
        string tmp;
        while (getline(file, tmp))
        {
            buffer->append(tmp);
        }
        file.close();

        evbuffer_add_printf(retbuff, "%s\n", buffer->c_str());
        redis_server->insert_cache(path, buffer);
    } while (0);

    // HTTP header
    evhttp_add_header(req->output_headers, "Server", "HTTP v1.1");
    evhttp_add_header(req->output_headers,
                      "Content-Type", content_type.c_str());
    evhttp_add_header(req->output_headers, "Connection", "close");

    evhttp_send_reply(req, HTTP_OK, "OK", retbuff);

    evbuffer_free(retbuff);
}

/* signal function */
void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGTERM:
    case SIGHUP:
    case SIGQUIT:
    case SIGINT:
        event_loopbreak();
        break;
    }
}
