
SAN(Subject Alternative Name)是 SSL 标准 x509 中定义的一个扩展。使用了 SAN 字段的 SSL 证书，可以扩

展此证书支持的域名，使得一个证书可以支持多个不同域名的解析。接下来我们重新利用配置文件生成CA证书，

再利用ca相关去生成服务端的证书。

1. 生成根证书

```conf
[ req ]
default_bits       = 2048
distinguished_name = req_distinguished_name

[ req_distinguished_name ]
countryName                 = Country Name (2 letter code)
countryName_default         = CN
stateOrProvinceName         = State or Province Name (full name)
stateOrProvinceName_default = ShanXi
localityName                = Locality Name (eg, city)
localityName_default        = TaiYuan
organizationName            = Organization Name (eg, company)
organizationName_default    = Step
commonName                  = CommonName (e.g. server FQDN or YOUR name)
commonName_max              = 64
commonName_default          = localhost
```


```bash
cd ssl
# 生成ca秘钥，得到ca.key
openssl genrsa -out ./ca/ca.key 4096
# 生成ca证书签发请求，得到ca.csr
openssl req -new -sha256 -out ./ca/ca.csr -key ./ca/ca.key -config ./conf/ca.conf
# 成ca根证书，得到ca.pem
openssl x509 -req -sha256 -days 3650 -in ./ca/ca.csr -signkey ./ca/ca.key -out ./ca/ca.pem
```

2. 生成服务端证书

```conf
#req 总配置
[ req ]
default_bits       = 2048
distinguished_name = req_distinguished_name  #使用 req_distinguished_name配置模块
req_extensions     = req_ext  #使用 req_ext配置模块

[ req_distinguished_name ]
countryName                 = Country Name (2 letter code)
countryName_default         = CN
stateOrProvinceName         = State or Province Name (full name)
stateOrProvinceName_default = ShanXi
localityName                = Locality Name (eg, city)
localityName_default        = TaiYuan
organizationName            = Organization Name (eg, company)
organizationName_default    = DuYong
commonName                  = Common Name (e.g. server FQDN or YOUR name)
commonName_max              = 64
commonName_default          = localhost    #这里的Common Name 写主要域名即可(注意：这个域名也要在alt_names的DNS.x里) 此处尤为重要，需要用该服务名字填写到客户端的代码中

[ req_ext ]
subjectAltName = @alt_names #使用 alt_names配置模块

[alt_names]
DNS.1   = localhost
DNS.2   = tyduyong.com
DNS.3   = www.tyduyong.com
IP      = 127.0.0.1
```


```bash
# 生成秘钥，得到server.key
openssl genrsa -out ./server/server.key 2048
# 生成证书签发请求，得到server.csr
openssl req -new -sha256 -out ./server/server.csr -key ./server/server.key -config ./conf/server.conf
# 用CA证书生成服务端证书，得到server.pem
openssl x509 -req -sha256 -days 3650 -CA ./ca/ca.pem -CAkey ./ca/ca.key -CAcreateserial -in ./server/server.csr -out ./server/server.pem -extensions req_ext -extfile ./conf/server.conf
```


3. 生成客户端证书

```conf
#req 总配置
[ req ]
default_bits       = 2048
distinguished_name = req_distinguished_name  #使用 req_distinguished_name配置模块
req_extensions     = req_ext  #使用 req_ext配置模块

[ req_distinguished_name ]
countryName                 = Country Name (2 letter code)
countryName_default         = CN
stateOrProvinceName         = State or Province Name (full name)
stateOrProvinceName_default = ShanXi
localityName                = Locality Name (eg, city)
localityName_default        = TaiYuan
organizationName            = Organization Name (eg, company)
organizationName_default    = DuYong
commonName                  = Common Name (e.g. server FQDN or YOUR name)
commonName_max              = 64
commonName_default          = localhost    #这里的Common Name 写主要域名即可(注意：这个域名也要在alt_names的DNS.x里) 此处尤为重要，需要用该服务名字填写到客户端的代码中

[ req_ext ]
subjectAltName = @alt_names #使用 alt_names配置模块

[alt_names]
DNS.1   = localhost
DNS.2   = tyduyong.com
DNS.3   = www.tyduyong.com
IP      = 127.0.0.1
```

```bash
# 生成秘钥，得到client.key
openssl ecparam -genkey -name secp384r1 -out ./client/client.key
# 生成证书签发请求，得到client.csr
openssl req -new -sha256 -out ./client/client.csr -key ./client/client.key -config ./conf/client.conf
# 用CA证书生成客户端证书，得到client.pem
openssl x509 -req -sha256 -days 3650 -CA ./ca/ca.pem -CAkey ./ca/ca.key -CAcreateserial -in ./client/client.csr -out ./client/client.pem -extensions req_ext -extfile ./conf/client.conf


```
