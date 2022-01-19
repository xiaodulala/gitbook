[TOC]

### docker 安装

根据操作系统安装(需提供外网)  
官方文档: [docker安装](https://docs.docker.com/engine/install/)

* centos

```bash
# 1.卸载
sudo yum remove docker \
                  docker-client \
                  docker-client-latest \
                  docker-common \
                  docker-latest \
                  docker-latest-logrotate \
                  docker-logrotate \
                  docker-engine
# 2. Enable the nightly or test repositories.
sudo yum install -y yum-utils
sudo yum-config-manager \
    --add-repo \
    https://download.docker.com/linux/centos/docker-ce.repo
sudo yum-config-manager --enable docker-ce-nightly
sudo yum-config-manager --enable docker-ce-test

# 3. 查看版本 安装
yum list docker-ce --showduplicates | sort -r
sudo yum install docker-ce-<VERSION_STRING> docker-ce-cli-<VERSION_STRING> containerd.io
# sudo yum install docker-ce docker-ce-cli containerd.io 安装最新

# 4. 启动
sudo systemctl start docker

sudo systemctl enable docker

# 5. 验证
sudo docker run hello-world

```

* ubuntu

```bash
# 1. 卸载
sudo apt-get remove docker docker-engine docker.io containerd runc
# 2. Set up the repository
sudo apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg \
    lsb-release
 
 # Add Docker’s official GPG key:  
 curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
 
 # To add the nightly or test repository
 echo \
  "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
  
  # 3. 版本列表 安装
  sudo apt-get update
  apt-cache madison docker-ce
  sudo apt-get install docker-ce=<VERSION_STRING> docker-ce-cli=<VERSION_STRING> containerd.io
  # sudo apt-get install docker-ce docker-ce-cli containerd.io 最新版本
 
 # 4. 验证
 sudo docker run hello-world
```

* Manage Docker as a non-root user

```bash
# 1. Create the docker group.
sudo groupadd docker
# 2.Add your user to the docker group.
sudo usermod -aG docker $USER
# 3. Log out and log back in so that your group membership is re-evaluated.
newgrp docker
```

* Configure Docker to start on boot

```bash
sudo systemctl enable docker.service
sudo systemctl enable containerd.service
```

### 加速

```bash
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json <<-'EOF'
{
  "registry-mirrors": ["https://docker.mirrors.ustc.edu.cn"]
}
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker
```

### docker-compose 安装

1. [二进制下载](https://github.com/docker/compose/releases)
2. 脚本安装

```bash
sudo curl -L "https://github.com/docker/compose/releases/download/1.29.1/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
```

### 卸载

```bash
sudo yum remove docker-ce docker-ce-cli containerd.io

sudo rm -rf /var/lib/docker
sudo rm -rf /var/lib/containerd
```

### 问题

centos 3.10.0-229.7.2.el7.x86_64 这个版本

只要安装最新的docker,端口映射无法访问。 安装旧版本


```bash
sudo yum -y --downloadonly update
sudo yum install -y yum-utils device-mapper-persistent-data lvm2
sudo yum-config-manager --add-repo http://mirrors.aliyun.com/docker-ce/linux/centos/docker-ce.repo

sudo yum -y install docker-ce-18.03.1.ce

# 剩下都一样
```