# tiny web server

实现了一个简单的web服务器

具体实现功能：
-------------------------------------
- GET方法请求解析
- POST方法请求解析
- 返回资源请求页面
- 利用GET方法实现加减法
- 利用POST方法实现加减法
- HTTP请求行具体解析
- 400、403、404错误码返回的处理


实现用到的相关技术
-------------------------------------
- 线程同步机制
- 线程池实现
- epoll多路复用
- http相关知识：http请求和http响应等

编译和执行分别如下
------------------------------------




> g++ webserver -o server -lpthread



> ./server




此项目参考：[minyweb_sever](https://github.com/jialuhu/2018/tree/master/minyweb_sever)
