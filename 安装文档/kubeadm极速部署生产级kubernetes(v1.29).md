# 1. Kubernetes集群节点准备

## 1.1 主机操作系统说明

|操作系统及版本
|----|
|ubuntu22.04|

## 1.2 节点配置

|节点IP|CPU|内存|硬盘|角色|主机名|
|----|----|----|----|----|----|
|192.168.56.3|2C|4G|50G|master|k8s-master01|
|192.168.56.4|2C|4G|50G|master|k8s-worker01|
|192.168.56.5|2C|4G|50G|master|k8s-worker02|

## 1.3 主机配置

### 1.3.1 主机名配置

```bash
# master
sudo hostnamectl set-hostname k8s-master01
# worker01
sudo hostnamectl set-hostname k8s-worker01
# worker02
sudo hostnamectl set-hostname k8s-worker02
```
### 1.3.2 主机IP地址配置

根据自己的系统进行配置

### 1.3.3 主机名与ip地址解析

```bash
sudo vim /etc/hosts
	
192.168.56.3 k8s-master01
192.168.56.4 k8s-worker01
192.168.56.5 k8s-worker02
	
# 每个机器
ssh-keygen
ssh-copy-id k8s-master01
ssh-copy-id k8s-worker01
ssh-copy-id k8s-worker02	
```


### 1.3.4 防火墙配置

	```bash
	# 查看防火墙状态
	sudo ufw status
	# 关闭
	sudo systemctl stop ufw &&  sudo systemctl disable ufw 
	```
### 1.3.5 SELINUX配置

	```bash
	# 查看状态。如果没安装就不用管了
	sudo sestatus
	# 关闭
	sudo vim /etc/selinux/config
	# 修改值为
	SELINUX=disabled 
	```
### 1.3.6 时间同步配置

	```bash
	# 设置时区
	sudo timedatectl set-timezone Asia/Shanghai
	# 安装ntp客户端工具 ntpdate
	sudo apt update
	sudo apt install ntpdate
	# 同步时间
	sudo  ntpdate cn.pool.ntp.org
	```

### 1.3.7 配置内核路由转发及网桥过滤

```bash
# 查看 br_netfilter 是否开启
lsmod | grep br_netfilter
# 如果没有开启，执行下面的
sudo modprobe br_netfilter
# 编写k8s.conf
sudo cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF

# 加载指定文件的内核参数
sudo sysctl -p /etc/sysctl.d/k8s.conf
```

### 1.3.8 安装ipset及ipvsadm

```bash
#安装ipset和ipvsadm：
sudo apt install -y ipset ipvsadm

### 开启ipvs 转发
sudo modprobe br_netfilter 

# 配置加载模块
sudo su - root
cat > /etc/modules-load.d/ipvs.conf << EOF
modprobe -- ip_vs
modprobe -- ip_vs_rr
modprobe -- ip_vs_wrr
modprobe -- ip_vs_sh
modprobe -- nf_conntrack
EOF

# 临时加载
modprobe -- ip_vs
modprobe -- ip_vs_rr
modprobe -- ip_vs_wrr
modprobe -- ip_vs_sh
modprobe -- nf_conntrack

# 开机加载配置，将ipvs相关模块加入配置文件中
cat >> /etc/modules <<EOF
ip_vs_sh
ip_vs_wrr
ip_vs_rr
ip_vs
nf_conntrack
EOF

# 查看 ip_vs 是否加载进入内核中
lsmod | grep -e ip_vs -e nf_conntrack
```

### 1.3.9 关闭交换分区

```bash
swapoff -a
sed -ri 's@(.*swap.*)@#\1@g' /etc/fstab
# 验证
free -h
```





# 2. 容器运行时安装(Dokcer-ce及cri-dockerd)

## 2.1 Docker-ce

### 2.1.1 docker安装

[docker安装](https://note.tyduyong.com/%E5%AE%89%E8%A3%85%E6%96%87%E6%A1%A3/docker&docker-compose%E5%AE%89%E8%A3%85.html)

### 2.1.2 修改CGroup方式

```bash
sudo vim /etc/docker/daemon.json

{
  "registry-mirrors": ["https://docker.mirrors.ustc.edu.cn"],
  "exec-opts": ["native.cgroupdriver=systemd"]
}

sudo systemctl restart docker
```

## 2.2 cri-dockerd

```
# 官网
https://github.com/Mirantis/cri-dockerd

# 下载
https://github.com/Mirantis/cri-dockerd/releases/download/v0.3.10/cri-dockerd_0.3.10.3-0.ubuntu-focal_amd64.deb

# 安装
sudo dpkg -i cri-dockerd_0.3.10.3-0.ubuntu-focal_amd64.deb

# 查看cri-dockerd配置文件
systemctl status cri-docker
# 修改配置文件第十行
sudo vim /lib/systemd/system/cri-docker.service
ExecStart=/usr/bin/cri-dockerd --pod-infra-container-image=registry.k8s.io/pause:3.9 --container-runtime-endpoint fd://
# 重新启动
sudo systemctl daemon-reload
sudo systemctl restart cri-docker.service
# 验证 
ls /var/run/cri-dockerd.sock
```



# 3. Kubernetes 1.29集群部署

## 3.1 集群软件及版本说明

||kubeadm|kubelet|kubectl
|----|----|----|----|----|
|版本|1.29|1.29|1.29|
|安装集群|所有主机|集群所有主机|集群所有主机|
|作用|初始化集群，管理集群|用于接收api-server命令，对pod声明周期进行管理|集群应用命令行管理工具|


## 3.2 基于Debian发行版安装

[官网安装](# https://kubernetes.io/zh-cn/docs/setup/production-environment/tools/kubeadm/install-kubeadm/)

```bash
1. 更新 apt 包索引并安装使用 Kubernetes apt 仓库所需要的包：
sudo apt-get update
# apt-transport-https 可能是一个虚拟包（dummy package）；如果是的话，你可以跳过安装这个包
sudo apt-get install -y apt-transport-https ca-certificates curl gpg


2. 下载用于 Kubernetes 软件包仓库的公共签名密钥。所有仓库都使用相同的签名密钥，因此你可以忽略URL中的版本：
sudo mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.29/deb/Release.key | sudo gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg

3. 添加 Kubernetes apt 仓库。 请注意，此仓库仅包含适用于 Kubernetes 1.29 的软件包； 对于其他 Kubernetes 次要版本，则需要更改 URL 中的 Kubernetes 次要版本以匹配你所需的次要版本 （你还应该检查正在阅读的安装文档是否为你计划安装的 Kubernetes 版本的文档）。

# 此操作会覆盖 /etc/apt/sources.list.d/kubernetes.list 中现存的所有配置。
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.29/deb/ /' | sudo tee /etc/apt/sources.list.d/kubernetes.list

4. 更新 apt 包索引，安装 kubelet、kubeadm 和 kubectl，并锁定其版本：

sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl
sudo apt-mark hold kubelet kubeadm kubectl
```

## 3.3 配置kubelet

**为了实现docker使用的cgroupdriver与kubelet使用的cgroup一致，建议修改如下文件内容**

```bash

sudo vim /etc/sysconfig/kubelet

KUBELET_EXTRA_ARGS="--cgroup-driver=systemd"

sudo systemctl enable kubelet

```

## 3.4 镜像准备

```bash
# 查看kubeadm 版本
kubeadm version
# 查看镜像
kubeadm config images list

registry.k8s.io/kube-apiserver:v1.29.2
registry.k8s.io/kube-controller-manager:v1.29.2
registry.k8s.io/kube-scheduler:v1.29.2
registry.k8s.io/kube-proxy:v1.29.2
registry.k8s.io/coredns/coredns:v1.11.1
registry.k8s.io/pause:3.9
registry.k8s.io/etcd:3.5.10-0

# 下载镜像 
# kubeadm config images pull
kubeadm config images pull --cri-socket unix:///var/run/cri-dockerd.sock


```

```bash
#镜像打包脚本
#!/bin/bash
image_list='
registry.k8s.io/kube-apiserver:v1.29.2
registry.k8s.io/kube-controller-manager:v1.29.2
registry.k8s.io/kube-scheduler:v1.29.2
registry.k8s.io/kube-proxy:v1.29.2
registry.k8s.io/coredns/coredns:v1.11.1
registry.k8s.io/pause:3.9
registry.k8s.io/etcd:3.5.10-0
'

for i in $image_list

do
	docker pull $i
done

docker save -o k8s-1-29-2.tar $image_list

```

## 3.5 集群初始化

```bash
# 初始化集群
sudo kubeadm  init --kubernetes-version=v1.29.2  --pod-network-cidr=10.244.0.0/16 --apiserver-advertise-address=10.0.0.104 --cri-socket unix:///var/run/cri-dockerd.sock
# 设置配置
  mkdir -p $HOME/.kube
  sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

# 添加工作节点
sudo kubeadm join 10.0.0.104:6443 --token wo8xsg.i1b5pc65t1babl2f \
        --discovery-token-ca-cert-hash sha256:4366def480864026db4ef451e08370416cc69a115bfa57762e997a6bc60b59dd  --cri-socket unix:///var/run/cri-dockerd.sock

```

## 3.6 集群网络插件部署calio

[官方文档](https://docs.tigera.io/calico/latest/getting-started/kubernetes/quickstart)

```bash
kubectl create -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.2/manifests/tigera-operator.yaml

# 验证
kubectl get pods -n tigera-operator
```

```bash
# 下载配置文件
wget  https://raw.githubusercontent.com/projectcalico/calico/v3.27.2/manifests/custom-resources.yaml

sudo vim  custom-resources.yaml
修改为 cidr: 10.244.0.0/16

kubectl apply -f custom-resources.yaml
```

# Kubernetes 集群可用性验证



```bash

kubectl get nodes

kubectl  get cs

```



# 高可用

## 4. 通过keepalive+nginx实现k8s apiserver节点高可用


* 安装

```bash
  # 只在两个master节点安装 

  # 安装Nginx：
  sudo apt update
  sudo apt install nginx

  # 安装Keepalived：
  sudo apt install keepalived

  sudo systemctl enable nginx
  sudo systemctl enable keepalived

```

* 配置nginx四层负载均衡

```bash
stream {
    upstream k8s-apiserver {
        server 192.168.56.3:6443; #master1
        server 192.168.56.4:6443; #master2
    }

    server {
        listen 16443; # 由于nginx与master节点复用。这个监听端口不能是6443
        proxy_pass k8s-apiserver;
    }
}

```

* 配置主keepalived

1. 写一个检测nginx是否存活的脚本

```bash
#!/bin/bash
#计算nginx进程数量
#注意：千万不要使用ps -ef | grep nginx | grep -v grep | wc -l 进行统计。这样统计会有问题的
n=`ps -C nginx --no-heading|wc -l`

#当nginx停止后，直接干掉keepalived，这样备收不到心跳包就会接管vip了
if [ $n -eq "0" ]; then
  /usr/bin/systemctl stop keepalived
fi
```

2. 给权限 `chmod +x check_nginx.sh`

3. 接下来，配置 Keepalived 使用这个脚本来检测 Nginx 的存活状态。编辑 Keepalived 配置文件（通常是 /etc/keepalived/keepalived.conf）

```bash
# 定义脚本检测块
vrrp_script check_nginx {	#定义要检测的脚本，check_nginx是指定的检测脚本名称
    script  /etc/keepalived/check_nginx.sh	#指定哪个脚本，定义脚本路径
    interval 2				#脚本检测的时间间隔，表示每n秒就检查一次，默认1秒就检测一次
    timeout 2				#脚本检测超时时间，超过这个时间则认为检测失败
}

vrrp_instance VI_1 {
    state MASTER # 主用MASTER 备用BACKUP
    interface enp0s8 # 根据你的网络配置更改接口名称
    virtual_router_id 51
    priority 100 # 主服务器优先级高于备用服务器 备用90
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass yourpassword # 用你自己的密码替换
    }
    virtual_ipaddress {
        192.168.56.199/24 # 替换为你的虚拟IP地址和子网掩码
    }
    track_script {
        check_nginx
    }
}

```

4. 重启nginx 和 keepalived

```bash

sudo systemctl restart nginx
sudo systemctl restart keepalived
```
5. 测试

```bash
sudo systemctl stop nginx
```