# lnixdb

基于libevent网络库，Linux dbm函数库（UNIX环境高级编程第三版数据库函数库章节），实现一个简单的分布式KV数据库。

1、程序分为四个组件：
 (1)client : 访问数据库的客户端，连接到route服务。
 (2)route  : 接受客户端的CRUD请求，将请求转发给master存储，接收master的回复并返回结果给客户端。
 (3)master : 接收route转发的CRUD请求，执行请求，发送回复，并通过“命令传播”将请求发送给slaves。
 (4)slave  : 接收master的传播命令请求，执行请求。

2、master-slave复制实现
 (1)初次复制，slave向master发送"slaveof"命令，master复制数据库全部KV发送给slaves。
 (2)初次复制完成后，master每接收一条请求，将请求发送给slaves，主从同步。
 
3、故障转移的实现
 (1)master每2秒发送master及对应的slaves节点信息（ip,port,节点主从flag）给route服务器。
 (2)route服务器记录下(1)中的节点信息，并每隔2秒向master、slaves发送"PING"，并接收消息回复"PONG"。
 (3)如果master没有及时（超过4秒）回复"PONG"消息，route服务器认为master没有正常工作，不能继续接收请求，route自动切换该master对应的slaves中的一台接收并执行客户端请求，切换策略很简单，随机选择一台。
 
4、使用线程池提高并发能力。
