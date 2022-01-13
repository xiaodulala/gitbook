# Mysql

* 创建目录

```bash
# 创建目录
mkdir -p mysql/datadir mysql/conf mysql/mydir
```

* Mysql配置文件

vim ./mysql/conf/my.cnf

```ini
[mysqld]
user=mysql
default-storage-engine=INNODB
#character-set-server=utf8
character-set-client-handshake=FALSE
character-set-server=utf8mb4
collation-server=utf8mb4_unicode_ci
init_connect='SET NAMES utf8mb4'
# 主从时使用
server_id=1
# 开启binlog 按需开启。影响性能
log-bin=mysql-bin
# 行模式(5.7默认)
binlog_format=row
# binlog同步事务数(5.7默认)
sync_binlog=1
[client]
#utf8mb4字符集可以存储emoji表情字符
#default-character-set=utf8
default-character-set=utf8mb4
[mysql]
#default-character-set=utf8
default-character-set=utf8mb4
```

* docker-compose.yaml

```yaml
version: '3'
services:
  dy_mysql:
    restart: always
    image: mysql:5.7
    container_name: dy_mysql
    volumes:
      - ./mydir:/mydir
      - ./datadir:/var/lib/mysql
      - ./conf/my.cnf:/etc/my.cnf
    environment:
      - TZ=Asia/Shanghai
    ports:
      - 3306:3306
```

* 修改密码

```bash
# 进入容器
docker exec -it dy_mysql bash

# 进入mysql
mysql

# 修改密码
mysql>update  mysql.user  set  authentication_string=password('duyong')  where user='root';

# 允许root远程连接(看需要设置)
grant all privileges on *.* to 'root'@'%' identified by 'duyong' with grant option;
flush privileges;
# 退出容器
```
* 删除配置中的skip-grant-tables 重启容器

# Postgres + pgadmin4

* 创建目录

```bash
mkdir -p postgres/data
```

* docker-compose.yaml

```yaml
version: '3.1'

services:
  postgres:
    container_name: duyong_postgres
    image: postgres:12.9
    restart: always
    environment:
      POSTGRES_USER: root
      POSTGRES_PASSWORD: root
      POSTGRES_DB: duyong
    ports:
      - 5432:5432
    volumes:
      - ./data:/var/lib/postgresql/data
  pgadmin4:
    container_name: duyong_pgadmin4
    image: dpage/pgadmin4
    restart: always
    environment:
      PGADMIN_DEFAULT_EMAIL: admin@fskj.com
      PGADMIN_DEFAULT_PASSWORD: 123
    ports:
      - 18080:80
```


# zookeeper单机集群

* 创建目录

```
mkdir -p zk
```

* docker-compose.yaml

```yaml
version: '3.1'

services:
  zoo1:
    image: zookeeper
    restart: always
    hostname: zoo1
    ports:
      - 2181:2181
    environment:
      ZOO_MY_ID: 1
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181

  zoo2:
    image: zookeeper
    restart: always
    hostname: zoo2
    ports:
      - 2182:2181
    environment:
      ZOO_MY_ID: 2
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181

  zoo3:
    image: zookeeper
    restart: always
    hostname: zoo3
    ports:
      - 2183:2181
    environment:
      ZOO_MY_ID: 3
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181
```

* 查看集群状态

```bash
# 进入容器
docker exec -it zk-single_zoo1_1 bash
# 
./bin/zkServer.sh status

Using config: /conf/zoo.cfg
Client port found: 2181. Client address: localhost. Client SSL: false.
Mode: follower
```

# zookeeper多机集群


* 机器

| 节点      | ip           | zk | 
| ------------- |:-------------:|:-------------:|
|   node1   | 10.0.0.99 | zk1
|   node2   | 10.0.0.100 | zk2
|   node3  | 10.0.0.101 | zk3


* 分别创建目录

```bash
# 99
mkdir -p zk1
#100
mkdir -p zk2
#101
mkdir -p zk3
```

* docker-compose.yaml

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

# ZK+Kafka单机集群
* 创建资源目录

```bash
mkdir -p kafka-single
```

* docker-compose.yaml

```yaml
version: '3.1'
services:
  zoo1:
    image: zookeeper
    restart: always
    hostname: zoo1
    ports:
      - 2181:2181
    environment:
      ZOO_MY_ID: 1
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181

  zoo2:
    image: zookeeper
    restart: always
    hostname: zoo2
    ports:
      - 2182:2181
    environment:
      ZOO_MY_ID: 2
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181

  zoo3:
    image: zookeeper
    restart: always
    hostname: zoo3
    ports:
      - 2183:2181
    environment:
      ZOO_MY_ID: 3
      ZOO_SERVERS: server.1=zoo1:2888:3888;2181 server.2=zoo2:2888:3888;2181 server.3=zoo3:2888:3888;2181

  kafka1:
    image: wurstmeister/kafka
    restart: always
    hostname: kafka1
    container_name: kafka1
    ports:
      - 9092:9092
    environment:
      KAFKA_ADVERTISED_HOST_NAME: kafka1
      KAFKA_ADVERTISED_PORT: 9092
      KAFKA_ZOOKEEPER_CONNECT: zoo1:2181,zoo2:2182,zoo3:2183
      KAFKA_ADVERTISED_LISTENERS: PLAINTEXT://kafka1:9092
      KAFKA_LISTENERS: PLAINTEXT://kafka1:9092
    volumes:
      - ./kafka1/logs:/kafka
  kafka2:
    image: wurstmeister/kafka
    restart: always
    hostname: kafka2
    container_name: kafka2
    ports:
      - 9093:9092
    environment:
      KAFKA_ADVERTISED_HOST_NAME: kafka2
      KAFKA_ADVERTISED_PORT: 9092
      KAFKA_ZOOKEEPER_CONNECT: zoo1:2181,zoo2:2182,zoo3:2183
      KAFKA_ADVERTISED_LISTENERS: PLAINTEXT://kafka2:9092
      KAFKA_LISTENERS: PLAINTEXT://kafka2:9092
    volumes:
      - ./kafka2/logs:/kafka
  kafka3:
    image: wurstmeister/kafka
    restart: always
    hostname: kafka3
    container_name: kafka3
    ports:
      - 9094:9092 
    environment:
      KAFKA_ADVERTISED_HOST_NAME: kafka3
      KAFKA_ADVERTISED_PORT: 9092
      KAFKA_ZOOKEEPER_CONNECT: zoo1:2181,zoo2:2182,zoo3:2183
      KAFKA_ADVERTISED_LISTENERS: PLAINTEXT://kafka3:9092
      KAFKA_LISTENERS: PLAINTEXT://kafka3:9092
    volumes:
      - ./kafka3/logs:/kafka
```

* 测试集群

```bash
# 生产者
docker exec -it  kafka1 bash

# 创建topic

kafka-topics.sh --create --zookeeper 10.0.0.99:2181 --replication-factor 1 --partitions 1 --topic test1

# 容器1发送消息

kafka-console-producer.sh --broker-list 10.0.0.99:9092 --topic test1

# 进入kakfa2容器查看消息
docker exec -it  kafka2 bash

kafka-console-consumer.sh --bootstrap-server 10.0.0.99:9092 --topic test1 --from-beginning

```

# Kafka多机集群