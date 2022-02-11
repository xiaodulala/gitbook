# 介绍

数据实时同步有很多种方法,其中一种叫CDC。

CDC(Change Data Capture)是变更数据捕获简称。这种方法可以基于增量日志，以极低的侵入性完成增量数据捕获的工作。核心思想是: **监测并捕获数据库的变动(包括数据或者数据表的插入、更新、以及删除等)**。将这些变更按照发生的顺序完成记录下来,写入到消息中间件以供其他服务进行订阅和消费。

简单来讲：CDC是指从源数据库捕获到数据和数据结构(也称为模式)的增量变更，近乎实时地将这些变更，传播到其他数据库或应用程序之处。 通过这种方式，CDC能够向数据仓库提供高效、低延迟的数据传输，以便信息被及时转换并交付给专供分析的应用程序。


与批量复制相比，变更数据的捕获通常具有如下优势:

* CDC通过仅发送增量的变更，来降低通过网络传输数据的成本。
* CDC可以帮助用户根据最新的数据做出更快、更准确的决策。例如，CDC会将事务直接传输到专供分析的应用上。
* CDC最大限度地减少了对于生产环境网络流量的干扰。


# CDC中间件工具对比

目前常用的工具有如下几种:

| 特色      | Canal           | Maxwell  |  mysql_staram | go-mysql-transfer 
| ------------- |:-------------:| -----:|-----:|-----:|
|   开发语言    | java(阿里) | java(外国) | python | golang | 
|   活跃度    | 活跃 | 活跃 | - | - | 
|  高可用     | 支持 | 支持(断点还原功能) | 支持 | 支持 | 
|  数据落地 | 定制 | Kafka，Kinesis、RabbitMQ、Redis、Go	ogle Cloud Pub/Sub、文件等) | Kafka等(MQ) | Redis、MongoDB、Elasticsearch、RabbitMQ、Kafka、RocketMQ、HTTP API 等 | 
|  分区| 支持 | 支持 | - | - | 
|  引导| 不支持 | 支持 | - | - | 
|  数据格式| 定制 | json | json | json | 
|  文档| 详细 | 详细 | - | - | 


现在用的最多的工具为canal和maxwell. 其他两种工具需要继续调研。

[canal](https://github.com/alibaba/canal)  
[maxwell](https://github.com/zendesk/maxwell)


* canal属于比较重。服务端需要一个客户端来配合使用。需要用户自己定制的数据落地方式和数据格式方式。自由的同时也增加了开发量。

* maxwell 属于轻量级的服务。服务端+客户端为一体。高可用的方式使用的是断点还原的方法。并且支持数据引导。

可以根据不同的应用场景进行选择。如果不是非常大型的并且定制化要求很高的服务。推荐使用maxwell

# Maxwell的使用

## 配置Mysql

* 修改配置文件

```cnf
[mysqld]
server_id=1
log-bin=master 
binlog_format=row 
sync_binlog=1 
```

* 验证

```mysql
mysql> show variables like "%binlog%";
mysql>  show variables like '%server_id%'; 
mysql>  show variables like '%log_bin%'; 
```

* 创建maxwell用户并赋予权限。主要用来记录binlog同步点。

```mysql
mysql> CREATE USER 'maxwell'@'%' IDENTIFIED BY 'duyong';
mysql> GRANT ALL ON maxwell.* TO 'maxwell'@'%';
mysql> GRANT SELECT, REPLICATION CLIENT, REPLICATION SLAVE ON *.* TO 'maxwell'@'%';
mysql> flush privileges;
```

## 启动测试的maxwell

* 测试

```bash
docker run -it --rm zendesk/maxwell bin/maxwell --user=maxwell\
    --password=duyong --host=10.0.0.99 --producer=stdout
    
docker run -it --rm zendesk/maxwell bin/maxwell --user=maxwell\
    --password=duyong --host=10.0.0.100 --producer=stdout
```


## 安装kafka集群

### docker 安装zookeeper集群

```yaml
# 10.0.0.99
version: '3.1'

services:
  zoo1:
    image: zookeeper
    restart: always
    hostname: zoo1
    ports:
      - 2181:2181
      - 2888:2888
      - 3888:3888
    environment:
      ZOO_MY_ID: 1
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=10.0.0.100:2888:3888;2181 server.3=10.0.0.101:2888:3888;2181
```

```yaml
# 10.0.0.100
version: '3.1'

services:
  zoo1:
    image: zookeeper
    restart: always
    hostname: zoo2
    ports:
      - 2181:2181
      - 2888:2888
      - 3888:3888
    environment:
      ZOO_MY_ID: 2
      ZOO_SERVERS: server.1=10.0.0.99:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=10.0.0.101:2888:3888;2181
```

```yaml
# 10.0.0.101
version: '3.1'

services:
  zoo1:
    image: zookeeper
    restart: always
    hostname: zoo3
    ports:
      - 2181:2181
      - 2888:2888
      - 3888:3888
    environment:
      ZOO_MY_ID: 3
      ZOO_SERVERS: server.1=10.0.0.99:2888:3888;2181 server.2=10.0.0.100:2888:3888;2181 server.3=zoo3:2888:3888;2181
```

### docker 安装kafka集群

[docker-compose启动应用](./docker-compose安装软件.md)

## docker启动maxwell并使用kafka

```bash
docker run -it --rm zendesk/maxwell bin/maxwell --user=maxwell\
    --password=duyong --host=10.0.0.99 --producer=kafka \
    --kafka.bootstrap.servers=10.0.0.99:9092,10.0.0.100:9092,10.0.0.101:9092 --kafka_topic=maxwell
    
 # 后台启动   
docker run -it --name=maxwell -d   zendesk/maxwell bin/maxwell --user=maxwell --password=duyong --host=10.0.0.246 --port=13306  --producer=kafka --kafka.bootstrap.servers=10.0.0.247:9092,10.0.0.248:9092,10.0.0.230:9092 --kafka_topic=maxwell
    
    
docker run -it --rm zendesk/maxwell bin/maxwell --user=maxwell\
    --password=duyong --host=10.0.0.100 --producer=kafka \
    --kafka.bootstrap.servers=10.0.0.99:9092,10.0.0.100:9092,10.0.0.101:9092 --kafka_topic=maxwell
 
 # 后台启动   
docker run -it --name=maxwell -d   zendesk/maxwell bin/maxwell --user=maxwell --password=duyong --host=10.0.0.246 --port=13306  --producer=kafka --kafka.bootstrap.servers=10.0.0.247:9092,10.0.0.248:9092,10.0.0.230:9092 --kafka_topic=maxwell
```




