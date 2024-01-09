# kubeadm和二进制安装k8s适用场景分析

## kubeadmin

kubeadm是官方提供的开源工具。kubeadm可以快速搭建k8s集群，是目前官方推荐的一个方式。kubeadm init 和 kubeadm join 这两个命令就可以快速搭建一个8ks,并且可以实现k8s集群的扩容。

kubeadm初始化k8s的时候，所有的组件都是以pod形式运行的，具备故障自恢复的能力。

kubeadm是工具，属于自动化部署，简化部署操作，自动部署屏蔽了很多细节。使得对各个模块感知很少，如果对k8s架构组件理解不深的话，遇到问题比较难排查。

## 二进制安装

二进制安装是在官网下载相关组件的二进制包，如果手动安装，对k8s理解也会更全面。


## 总结

kubeadm和二进制都适合生产环境，在生产环境运行都很稳定。具体如何选择，可以根据实际项目进行评估。


# 环境规划


|集群角色|IP|主机名
|----|----|----|
|控制节点|192.168.56.3|node1
|控制节点|192.168.56.4|node2
|工作节点|192.168.56.5|node3
|VIP|192.168.56.199|


## 1. 环境准备

## 1.1. 配置集群机器可以通过主机名访问

```bash
# 每个机器
sudo vim /etc/hosts

192.168.56.3 node1
192.168.56.4 node2
192.168.56.5 node3
```

## 1.2 配置主机免密登录

```bash
# 每个机器
ssh-keygen
ssh-copy-id node1
ssh-copy-id node2
ssh-copy-id node3

```

## 1.3 关闭交换分区，提升性能

```bash
# 临时关闭
swapoff -a
# 验证 
free -m

# 永久关闭，一般选择永久关闭
sudo vim /etc/fstab
# 注释掉swap的一行
```

## 1.4 修改机器内核参数

```bash
# 查看 br_netfilter 是否开启
lsmod | grep br_netfilter
# 如果没有开启，执行下面的
modprobe br_netfilter
# 编写k8s.conf
sudo cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF

# 加载指定文件的内核参数
sudo sysctl -p /etc/sysctl.d/k8s.conf
```

## 1.5 关闭防火墙

```bash
# ubuntu
sudo systemctl stop ufw &&  sudo systemctl disable ufw 
```

## 1.6 关闭selinux

```bash
sudo vim /etc/selinux/config
SELINUX=disabled
# 重启虚拟机
# 查看
sudo apt install selinux-utils
getenforce
```

## 1.7 配置时间同步

https://cn.linux-console.net/?p=10121

```bash
# 设置时区
sudo timedatectl set-timezone Asia/Shanghai
# 安装ntp客户端工具 ntpdate
sudo apt update
sudo apt install ntpdate
# 同步时间
sudo  ntpdate cn.pool.ntp.org
```

## 1.8 开启ipvs

ipvs: IP virtual Service  4层负载均衡
iptables: 

kube-proxy支持iptables和ipvs对比分析
1. 都是基于netfilter实现的
2. ipvs采用hash表存规则，查询效率较高
3. iptables是一条条存的，查询效率较低
4. 如果不开启ipvs，k8s默认采用iptables
5. ipvs是为大型集群提供了更好的可扩展性和性能
6. ipvs支持比iptables更复杂的负载均衡算法。
7. ipvs支持服务器健康检查和连接重试等功能。

k8s1.8以后出来的ipvs 1.11稳定版

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

## 1.9 安装iptables


# 2. 安装docker

## 2.1 修改驱动


```bash
sudo vim /etc/docker/daemon.json
{
  "registry-mirrors": ["https://docker.mirrors.ustc.edu.cn"],
  "exec-opts": ["native.cgroupdriver=systemd"]
}
sudo systemctl restart docker
```

# 安装k8s软件包


