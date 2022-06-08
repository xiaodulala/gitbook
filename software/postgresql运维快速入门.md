# 资料

* [中文站](http://www.postgres.cn/)
* [官方站点](https://www.postgresql.org/)
* [官方docker镜像](https://hub.docker.com/_/postgres)
* [pgadmin4管理工具](https://www.pgadmin.org/)

# 安装

## yum&apt安装(测试环境使用)

根据官方文档，选择对应的操作系统和版本后可查看安装命令。

## 二进制安装(生产环境推荐)

官网下载源码包，以12.9为例。`https://www.postgresql.org/ftp/source/v12.9/`

后续补充。


## docker启动

### 环境变量

*  `POSTGRES_PASSWORD`  这个环境变量设置PostgreSQL的超级用户密码,不能为空。
*  `POSTGRES_USER` 指定超级用户和`POSTGRES_PASSWORD`配合使用。如果不设置，默认的超级用户为`postgres `
*  `POSTGRES_DB`  用来第一次启动容器创建默认的数据库。如果不指定,`POSTGRES_USER`的值会被使用。
*  `POSTGRES_INITDB_ARGS`   这个可选的环境变量可以用来将参数发送到postgres initdb。该值是一个由空格分隔的参数字符串，这是postgres initdb所期望的。-e POSTGRES_INITDB_ARGS="——data-checksum "。
*  `POSTGRES_INITDB_WALDIR ` 这个可选的环境变量可以用来定义Postgres事务日志的另一个位置。默认情况下，事务日志存储在主Postgres数据文件夹(PGDATA)的子目录中。有时，可以将事务日志存储在不同的目录中，该目录可能由具有不同性能或可靠性特征的存储支持。
*  `POSTGRES_HOST_AUTH_METHOD`  这个可选变量可用于控制所有数据库、所有用户和所有地址的主机连接的auth-方法。如果未指定，则使用md5密码验证.  
	注意: 
	1. 不建议使用trust，因为它允许任何人在没有密码的情况下连接，即使设置了密码(比如通过POSTGRES_PASSWORD)
	2. 如果将POSTGRES_HOST_AUTH_METHOD设置为trust，则不需要设置POSTGRES_PASSWORD
	3. 如果你将它设置为一个可选的值(例如scram_sha -256)，你可能需要额外的POSTGRES_INITDB_ARGS来正确初始化数据库(例如POSTGRES_INITDB_ARGS=——auth-host= scram_sha -256)

*  `PGDATA`  可以用来定义数据库文件的另一个位置.默认为/var/lib/postgresql/data

### compose文件

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
# 连接管理


## 本地socket连接的方式

```bash
# on socket "/var/run/postgresql/.s.PGSQL.5432"
psql -d duyong # duyong是数据库名称
```
## 远程连接

```bash
# 安装客户端
sudo apt install postgresql-client-12
# 登录
psql -h 10.0.0.100 -p 5432 -U root -d duyong
```

* 如果是二进制安装应该修改 `pg_hba.conf`配置文件

```bash
# TYPE  DATABASE        USER            ADDRESS                 METHOD

# "local" is for Unix domain socket connections only
local   all             all                                     trust
# IPv4 local connections:
host    all             all             127.0.0.1/32            trust
host    all             all             0.0.0.0/0               md5  # 新增的
```
注意：匹配从上到下，如果匹配到了就直接返回结构，下面的条件不再执行。

* 主配置文件 `postgresql.conf` 修改 `listen_addresses = '*'`

# 用户管理

```bash
create user; # 默认带连接
create role; # 不带连接
\help create user; #帮助
create user admin with superuser password '123'; # 创建一个管理员账户
drop user admin;  # 删除用户
alert user admin with password '1234' # 修改密码
\du  查看用户
```
	
# 权限管理

## 权限级别
pg资源管理逻辑图：

![逻辑图](../img/software/pg1.png)

权限管理就是针对以上级别做限制。

* cluster权限: 实例权限通过pg_hba.conf配置
* database权限： 数据库权限通过grant和revoke操作schema配置
* TBS权限： 表空间权限通过grant和revoke操作表，物化视图，索引，临时表配置。
* schema权限： 模式权限通过grant和revoke操作模式下的对象配置。
* object权限: 对象权限通过grant和revoke配置。

## 权限定义

* database 权限设置

```sql
GRANT create ON DATABASE  库名 TO 用户名； # 库级别权限。可以建库
```
* schema 权限

```sql
ALERT SCHEMA abc OWNER to abc; # 修改schema拥有者
GRANT select,insert,update,delete ON ALL TABLES IN SCHEMA 库 to abc;
```
* object权限

```sql
grant select,insert,update,delete on a.b to u;
```

* 案例 创建一个业务用户

```sql
create database test;
\c test;
create schema  abc;
create user abc with password 'abc';
alter schema abc owner to abc;
grant select,instet,update,delete on all tables in schema abc to abc;
```

# 常用命令

```bash
\?
\h 
\l 库列表
\c 
```

