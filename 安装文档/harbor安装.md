# Harbor

[harbor github](https://github.com/goharbor/harbor)

[下载地址](https://github.com/goharbor/harbor/releases)

[安装文档](https://goharbor.io/docs/2.10.0/install-config/)

# 安装步骤

1.  [确认目标服务器满足harbor安装的先决条件](https://goharbor.io/docs/2.10.0/install-config/installation-prereqs/)
2. [下载安装包](https://goharbor.io/docs/2.10.0/install-config/download-installer/)
3. [配置访问harbor的http证书](https://goharbor.io/docs/2.10.0/install-config/configure-https/)
4. [配置文件](https://goharbor.io/docs/2.10.0/install-config/configure-yml-file/)
5. [配置内部组件https通信](https://goharbor.io/docs/2.10.0/install-config/configure-internal-tls/)
6. [脚本启动](https://goharbor.io/docs/2.10.0/install-config/run-installer-script/)


# 离线包方式安装

*  下载离线安装包

```bash
# 下载安装包
wget https://github.com/goharbor/harbor/releases/download/v2.10.0/harbor-offline-installer-v2.10.0.tgz

# 解压
tar xzvf harbor-offline-installer-v2.10.0.tgz
```

* 配置自签名证书

```bash
# 创建证书目录
cd /home/duyong/env/harbor
mkdir ssl

# 生成根证书私钥
cd ssl
mkdir root
openssl genrsa -out ./root/ca.key 4096 
# 生成根证书证书  

/C= Country 国家
/ST= State or Province 省
/L= Location or City 城市
/O= Organization 组织或企业
/OU= Organization Unit 部门
/CN= Common Name 域名或IP

openssl req -x509 -new -nodes -sha512 -days 3650 \
 -subj "/C=CN/ST=Beijing/L=Beijing/O=duyong/OU=Personal/CN=tyduyong.com" \
 -key ./root/ca.key \
 -out ./root/ca.crt

# 生成服务器证书目录
mkdir harbor.tyduyong.com

# 生成服务器私钥
openssl genrsa -out ./harbor.tyduyong.com/harbor.tyduyong.com.key 4096

# 生成服务器证书请求文件
openssl req -sha512 -new \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=duyong/OU=Personal/CN=harbor.tyduyong.com" \
    -key ./harbor.tyduyong.com/harbor.tyduyong.com.key \
    -out  ./harbor.tyduyong.com/harbor.tyduyong.com.csr

# 生成x509 v3扩展文件。
cat > ./harbor.tyduyong.com/v3.ext <<-EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1=harbor.tyduyong.com
DNS.2=harbor.tyduyong
DNS.3=duyong
EOF

# 根据证书请求文件生成证书
openssl x509 -req -sha512 -days 3650 \
    -extfile ./harbor.tyduyong.com/v3.ext \
    -CA ./root/ca.crt -CAkey ./root/ca.key -CAcreateserial \
    -in ./harbor.tyduyong.com/harbor.tyduyong.com.csr \
    -out ./harbor.tyduyong.com/harbor.tyduyong.com.crt

# 将服务器证书和密钥复制到Harbor主机上的证书文件夹中
cd ..
mkdir -p ./data/cert
cp ssl/harbor.tyduyong.com/harbor.tyduyong.com.crt ./data/cert
cp ssl/harbor.tyduyong.com/harbor.tyduyong.com.key ./data/cert

#将yourdomain.com.crt转换为yourdomain.com.cert，以供Docker使用。Docker守护进程将.crt文件解释为CA证书，将.cert文件解释为客户端证书。
openssl x509 -inform PEM -in ./data/cert/harbor.tyduyong.com.crt -out ./data/cert/harbor.tyduyong.com.cert

# 将服务器证书、密钥和CA文件复制到Harbor主机的Docker证书文件夹中。您必须首先创建适当的文件夹。
sudo mkdir -p /etc/docker/certs.d/harbor.tyduyong.com/
sudo cp data/cert/harbor.tyduyong.com.cert /etc/docker/certs.d/harbor.tyduyong.com/
sudo cp data/cert/harbor.tyduyong.com.key   /etc/docker/certs.d/harbor.tyduyong.com/
sudo cp ssl/root/ca.crt /etc/docker/certs.d/harbor.tyduyong.com/

# 重启docker
sudo systemctl restart docker

```

* 配置harbor

```bash
 cp harbor.yml.tmpl harbor.yml
 
```

```yaml
hostname: harbor.tyduyong.com

# http related config
http:
  # port for http, default is 80. If https enabled, this port will redirect to https port
  port: 80

# https related config
https:
  # https port for harbor, default is 443
  port: 443
  # The path of cert and key files for nginx
  certificate: /home/duyong/env/harbor/data/cert/harbor.tyduyong.com.crt
  private_key: /home/duyong/env/harbor/data/cert/harbor.tyduyong.com.key
```

* 脚本启动

```bash
# 如果是离线形式安装，先导入镜像包
docker load -i harbor.v2.10.0.tar.gz
sudo ./prepare
sudo ./install.sh
```


# 上传镜像到私有仓库

```bash

# 在运行Docker守护进程的机器上，检查/etc/docker/daemon.确保没有为https://yourdomain.com设置 insecure-registry选项。
docker login harbor.tyduyong.com

# harbor新建一个项目test

# 随便拉取一个镜像
docker pull busybox

# 给这个镜像更换标签
docker tag busybox:latest harbor.tyduyong.com/test/busybox:latest
# 推送
docker push harbor.tyduyong.com/test/busybox


```

