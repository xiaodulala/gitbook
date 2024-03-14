# Gogs安装部署


## 官网

[官网](https://gogs.io/docs/installation/install_from_binary)

## 安装

```bash
./gogs web

# 访问
/install 进行配置
```

## systemd守护进程运行

1. 修改脚本文件

```bash
vim scripts/systemd/gogs.service

# 修改目录和用户
Type=simple
User=root
Group=root
WorkingDirectory=/root/env/gogs
ExecStart=/root/env/gogs/gogs web
Restart=always
Environment=USER=root HOME=/root

```

2. 启动

```bash

cp scripts/systemd/gogs.service /etc/systemd/system

systemctl start gogs

systemctl enable gogs

# 如果gogs.service有修改
systemctl daemon-reload

# 排错
journalctl -u gogs
```

## nginx代理

```conf

server {
    listen 443 ssl;
    server_name gogs.tyduyong.com;

    ssl_certificate /etc/nginx/conf.d/cert/gogs/gogs.tyduyong.com_cert_chain.pem;
    ssl_certificate_key /etc/nginx/conf.d/cert/gogs/gogs.tyduyong.com_key.key;

    location / {
	proxy_pass http://172.22.187.175:10020;  # 将请求转发到后端服务器
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
	    client_max_body_size 1024m;
    }

}
```