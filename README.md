# lnixdb
基于libevent网络库，Linux dbm函数库（UNIX环境高级编程第三版），实现一个简单的分布式KV数据库。

程序分为四个组件：
 (1)client : 访问数据库的客户端，连接到route服务。
 (2)route  : 接受客户端的CRUD请求，将请求转发给master存储，接收master的回复并返回结果给客户端。
