# drone安装部署

## 官网

(drone)[https://docs.drone.io/server/overview/]

## 关联gogs的安装方式

1. 拉取镜像

```bash
docker pull drone/drone:2
```

2. 运行容器

```bash
docker run \
  --volume=/var/lib/drone:/data \
  --env=DRONE_AGENTS_ENABLED=true \
  --env=DRONE_GOGS_SERVER=https://gogs.tyduyong.com \
  --env=DRONE_RPC_SECRET=super-duper-secret \
  --env=DRONE_SERVER_HOST=drone.tyduyong.com \
  --env=DRONE_SERVER_PROTO=https \
  --publish=10030:80 \
  --publish=10033:443 \
  --restart=always \
  --detach=true \
  --name=drone \
  drone/drone:2
```

3. 配置nginx代理

```yaml
server {
    listen 443 ssl;
    server_name drone.tyduyong.com;

    ssl_certificate /etc/nginx/conf.d/cert/drone/drone.tyduyong.com_cert_chain.pem;
    ssl_certificate_key /etc/nginx/conf.d/cert/drone/drone.tyduyong.com_key.key;


    location / {
	proxy_pass http://172.22.187.175:10030;  # 将请求转发到后端服务器
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
	    client_max_body_size 1024m;
    }
}

```

4. Install Runners 选择docker


```bash
docker pull drone/drone-runner-docker:1
```


```bash

docker run --detach \
  --volume=/var/run/docker.sock:/var/run/docker.sock \
  --env=DRONE_RPC_PROTO=https \
  --env=DRONE_RPC_HOST=drone.tyduyong.com \
  --env=DRONE_RPC_SECRET=super-duper-secret \
  --env=DRONE_RUNNER_CAPACITY=2 \
  --env=DRONE_RUNNER_NAME=my-first-runner \
  --publish=10035:3000 \
  --restart=always \
  --name=runner \
  drone/drone-runner-docker:1

```


```bash
# 验证
$ docker logs runner

INFO[0000] starting the server
INFO[0000] successfully pinged the remote server 

```