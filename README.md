# cacheServer
使用 Redis 和 Libevent 构建的网页缓存服务器


## 介绍
此项目开发的原因是访问诸如 cppreference 等站点的话太慢，自己又恰好有空闲
服务器，所以想利用服务器做一个简单的镜像（资料）网站，供平时查查资料使用。
cppreference 网站也有提供相关的离线文件下载，于是将离线文件下载到服务器，利用
Libevent 实现了一个简爱的 HTTP 服务器，考虑到服务器内存比较多，平时使用不到，
所以又在原有代码上做了改造，加上了 Redis 对热点网页进行缓存。

主要数据流程如下：

```

                                           找到
浏览器 --> HTTP服务器 --> 查询 Redis 缓存 ----> 返回给浏览器
                               | 
                               | 没找到          找到
                               -------> 查询磁盘-----> 返回给浏览器/添加到Redis
                                          |
                                          | 没找到
                                          ------>  返回 404.html 给浏览器

```

## 结构介绍

```
log.h		日志相关
config.h	配置文件
server.h	服务器主体代码
server.cpp      主函数位置
website.log 	日志文件
www/            提供的测试网页
```

## 依赖
- Redis
- hiredis



## 试用

[cppreference 备份（无查询功能）](http://47.93.196.173:7878/reference/zh/index.html)

[libevent-book](http://47.93.196.173:7878/libevent-book/TOC.html)

[boost_1.72_0](http://www.bearcarl.top:7878/boost_1_72_0/doc/html/index.html)

[查看访问信息](http://47.93.196.173:7878/visit_info)



## TODO

- [x] 完善文件未找到的处理逻辑，减少客户端等待时间
- [ ] 修改 HTTP 服务器相关代码，停止使用 Libevent 废弃函数
- [x] 修改访问计数缓存错误问题
- [ ] 将记录日志的相关操作放到一个单独的线程中进行处理，避免日记降低系统性能
- [ ] 读取文本文件的方式和读取二进制文件的方式应该是不相同的，需要分别抽取出来作为两个接口
