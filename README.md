# WebServer

基于Linux下，C++实现的高性能WEB服务器

## 功能

- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型
- 基于小根堆实现的定时器，关闭超时的非活动连接
- 使用模板技术实现可处理任意类型、可变长参数任务的线程池，并且支持动态扩容减容
- 利用正则与状态机解析HTTP请求报文

## 环境要求

- Linux
- C++11

## 项目启动

```
make
./bin/server
```

## 压力测试

```
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```

- 测试环境: Ubuntu:18 两核4G

## 性能表现

与[markparticle](https://github.com/markparticle/WebServer/)的C++服务器做一个比较(表格中的为QPS的值)：

|   并发量   |  10   |  100  | 1000  | 10000 |
| :--: | :---: | :---: | :---: | :---: |
| QPS  |  |  |  |    |


## 参考

《Linux高性能服务器编程》（游双著）


