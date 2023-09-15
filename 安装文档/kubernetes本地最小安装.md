[TOC]

# 官方文档

1. [kubernetes](https://kubernetes.io/zh-cn/docs/tasks/tools/#kubectl)
2. [minikube](https://minikube.sigs.k8s.io/docs/start/)
3. [kubectl linux](https://kubernetes.io/zh-cn/docs/tasks/tools/install-kubectl-macos/)


# docker 

[安装docker](https://note.tyduyong.com/%E5%AE%89%E8%A3%85%E6%96%87%E6%A1%A3/docker&docker-compose%E5%AE%89%E8%A3%85.html), kebemini启动时需要安装兼容的容器引擎或者虚拟机管理器

# kubectl

* 安装最新版本

```bash
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"

# 安装指定版本
curl -LO https://dl.k8s.io/release/v1.27.0/bin/linux/amd64/kubectl
```



* 验证可执行文件(可选)

```bash
curl -LO "https://dl.k8s.io/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl.sha256"

#基于校验和文件，验证 kubectl 的可执行文件
echo "$(cat kubectl.sha256)  kubectl" | sha256sum --check
```

* 安装 kubectl

```bash
sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

# 即使你没有目标系统的 root 权限，仍然可以将 kubectl 安装到目录 ~/.local/bin 中：
chmod +x kubectl
mkdir -p ~/.local/bin
mv ./kubectl ~/.local/bin/kubectl
# 之后将 ~/.local/bin 附加（或前置）到 $PATH
```

* 执行测试，以保障你安装的版本是最新的：

```bash
kubectl version --client

# 或者使用如下命令来查看版本的详细信息：
kubectl version --client --output=yaml

```


# minikube

* 安装

```bash
# linux
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube
```

* 启动

```bash
 minikube start
```

如果启动失败，需要安装兼容的容器引擎或者虚拟机管理器。查看[drivers page](https://minikube.sigs.k8s.io/docs/drivers/)。


* 和集群交互

可以直接使用`kubectl`。 如果没有安装，可以直接使用 ` minikube kubectl -- get pods -A`