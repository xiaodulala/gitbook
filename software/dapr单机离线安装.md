[TOC]

# Dapr Cli 官方在线安装方式

官方安装文档: `https://docs.dapr.io/getting-started/install-dapr-cli`

安装命令: 

`wget -q https://raw.githubusercontent.com/dapr/cli/master/install/install.sh -O - | DAPR_INSTALL_DIR="$HOME/dapr" /bin/bash
`

通过指定上面的命令,我们知道需要下载一个安装脚本，并执行脚本。但是在这个过程中，都有可能遇到网络受限问题。

```bash
duyong@duyong-ubuntu:~/dev-tools/dapr$ wget -q https://raw.githubusercontent.com/dapr/cli/master/install/install.sh -O - | DAPR_INSTALL_DIR="$HOME/dapr" /bin/bash

Getting the latest Dapr CLI...
Your system is linux_amd64
Installing Dapr CLI...

Installing v1.8.0 Dapr CLI...
Downloading https://github.com/dapr/cli/releases/download/v1.8.0/dapr_linux_amd64.tar.gz ...
```


# Dapr 离线安装

文档：  [https://docs.dapr.io/operations/hosting/self-hosted/self-hosted-airgap/](https://docs.dapr.io/operations/hosting/self-hosted/self-hosted-airgap/)


1. 下载安装包 此安装包包含 DaprCli  Dapr 用这种离线方式安装，不需要在单独安葬daprCli

	下载地址 [https://github.com/dapr/installer-bundle/releases](https://github.com/dapr/installer-bundle/releases)
	根据系统选择版本
	
2. 解压安装包

	```tar
	tar xzvf daprbundle_linux_amd64.tar.gz
	```
3. 将dapr cli 移动到目录

	```bash
	cd daprbundle/
	sudo cp dapr /usr/local/bin/
	```
4. 通过dapr cli 启动dapr

	```bash
	# 我自己指定了网络，可以不指定
	dapr init --from-dir .
	```
	```bash
	 # 输出结果
	⌛  Making the jump to hyperspace...
	⚠  Local bundle installation using --from-dir flag is currently a preview feature and is subject to change. It is only available from CLI version 1.7 onwards.
	ℹ️  Installing runtime version 1.8.0
	↑  Extracting binaries and setting up components... Loaded image: daprio/dapr:1.8.0
	↑  Extracting binaries and setting up components...
	Dapr runtime installed to /home/duyong/.dapr/bin, you may run the following to add it to your path if you want to run daprd directly:
	    export PATH=$PATH:/home/duyong/.dapr/bin
	✅  Extracting binaries and setting up components...
	✅  Extracted binaries and completed components set up.
	ℹ️  daprd binary has been installed to /home/duyong/.dapr/bin.
	ℹ️  dapr_placement_mb container is running.
	ℹ️  Use `docker ps` to check running containers.
	✅  Success! Dapr is up and running. To get started, go here: https://aka.ms/dapr-getting-started
	```
	
	这里只启动了`dapr_placement` 容器,自己启动redis和zipkin
	
	```bash
	docker run --name "dapr_zipkin"  --restart always -d -p 9411:9411 openzipkin/zipkin
	docker run --name "dapr_redis"   --restart always -d -p 6379:6379 redislabs/rejson
	```