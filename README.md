# 密码学大作业上手指南

## 1. 目标

我们将用 strongswan 搭建一个 IPsec-VPN，以保证点对点（Site-to-Site）场景的安全通讯：
```
172.18.204.66 -- | 120.79.52.57 | === | 120.79.19.223 | -- 172.18.204.67
    VPC-A          Cloud-Server-A       Cloud-Server-B         VPC-B
```

在实验环境中，我们拥有两个云服务器，配置如下：

+ 云服务器A：公网 IPV4 地址为 120.79.52.57，私网 IPV4 地址为 172.18.204.66
+ 云服务器B：公网 IPV4 地址为 120.79.19.223，私网 IPV4 地址为 172.18.204.67

我们希望搭建一个 VPN，使得这两个云服务器能在途径不可信公网的情况下仍然能安全通讯，互相访问对方的内网。

## 2. 前置条件

### 2.1 知识储备

为了理解上述模型，可以先复习一下这些计网知识：

+ 公网、私网地址
+ 子网、子网掩码与网段

### 2.2 macOS 客户端环境准备

服务器的环境我已经搭建好了，因此我们只要准备客户端的环境即可。以 **macOS** 为例，大家应该都配过 `homebrew` 了，直接输入以下指令：
```
brew install strongswan
```

在 **Ubuntu** 下：
```
sudo apt install strongswan strongswan-pki libcharon-extra-plugins libcharon-extauth-plugins charon-cmd
```

安装结束后，请执行以下指令：
```
charon-cmd
```

应该能看到类似结果：
```
00[KNL] kernel-pfkey plugin requires CAP_NET_ADMIN capability
00[LIB] plugin 'kernel-pfkey': failed to load - kernel_pfkey_plugin_create returned NULL
00[NET] installing IKE bypass policy failed
00[NET] installing IKE bypass policy failed
00[NET] enabling UDP decapsulation for IPv4 on port 56915 failed
00[NET] installing IKE bypass policy failed
00[NET] installing IKE bypass policy failed
00[NET] enabling UDP decapsulation for IPv6 on port 56915 failed
00[LIB] feature CUSTOM:libcharon in critical plugin 'charon-cmd' has unmet dependency: CUSTOM:kernel-ipsec
00[LIB] failed to load 1 critical plugin feature
```

这说明客户端环境已经**配置完毕**，上述输出只是因为没有指定证书和连接地址产生的报错。

## 3. 测试 VPN

本小节将介绍 strongswan 官方虚拟系统 **charon** 环境下的 VPN 连接测试方案。虽然[参考文档](https://www.digitalocean.com/community/tutorials/how-to-set-up-an-ikev2-vpn-server-with-strongswan-on-ubuntu-22-04)给出了一些其它可能的连接方案，但是我都失败了。

首先，你需要把项目的 ca-cert.pem 文件拉到本地，登录云服务器并执行：
```
cat /usr/local/etc/ipsec.d/cacerts/ca-cert.pem
```

复制命令行输出的全部内容，形如：
```
-----BEGIN CERTIFICATE-----
MIIE8DCCAtigAwIBAgIIUUepKT1hL6gwDQYJKoZIhvcNAQEMBQAwFjEUMBIGA1UE
...
...
...
kqp7oqRu6K8jFInfWCet8rysatg=
-----END CERTIFICATE-----
```

在本地新建 `ca-cert.pem` 文件，然后执行以下指令：
```
sudo charon-cmd --cert ca-cert.pem --host 120.79.52.57 --identity Mimaxue
```

如果一切正常（希望如此），终端会让你输入 VPN 的密码，请输入 **1226**，然后 VPN 就连接成功啦！你可以在本地\云服务器执行以下指令验证：
```
# 在本地
ping 172.18.204.66

# 在云服务器
ping 10.10.10.1
```

可以发现，两端都能 ping 通，这说明我们的 VPN 成功跑起来了啦！此外，你还可以在云服务器上执行：
```
ipsec status
```

你将看到：
```
172.18.204.66/32 === 10.10.10.1/32
```

这说明我们成功建立了云服务器私网和你的虚拟地址之间的通讯。

## 4. 源码代码编译

**注意：请同时看完 4. 5. 两个章节后再进行相关操作～**

在项目根目录下，我放了 strongswan-5.9.8 目录，是 strongswan 社区发布的最新版源码，接下来的工作应该是围绕源码改造进行。但是我试了很久，macOS 下一直无法编译源码。总是报这个错：
```
checking x86/x64 target... no
checking for EVP_CIPHER_CTX_new in -lcrypto... no
configure: error: OpenSSL libcrypto not found
```

似乎是找不到 OpenSSL 的路径？网上的解决方案只有 Linux 系统的，我确实是没有办法了。如果大家想尝试 macOS 的编译，可以试试下面的指令：
```
./configure --prefix=/usr --sysconfdir=/etc --disable-gmp --enable-openssl --disable-kernel-netlink --enable-kernel-pfroute --enable-kernel-pfkey --enable-osx-attr --disable-scripts
```

参考自：

1. [官网安装文档](https://docs.strongswan.org/docs/5.9/install/install.html)
2. [macOS 附加参数](https://docs.strongswan.org/docs/5.9/os/macos.html)

在 Ubuntu 下，先进入 `~/strongswan` 目录，执行：
```
./configure --enable-chapoly --enable-eap-identity --enable-eap-mschapv2 --enable-openssl \
    && make && make install
```

**注：** 这里的附加参数请参考[官方文档](https://docs.strongswan.org/docs/5.9/install/autoconf.html)，如果没有加载好指定模块，VPN 服务将无法运行。

源码编译完成后，请进行以下两个验证：

### 4.1 验证 pki
`pki` 指令用于生成 IPsec-VPN 使用的证书，是我们算法模块国密化替换的主要验证工具，执行：
```
pki
```

你将会看到：
```
strongSwan 5.9.8 PKI tool
loaded plugins: aes des rc2 sha2 sha1 md5 mgf1 random x509 revocation pubkey pkcs1 pkcs7 pkcs12 dnskey sshkey pem openssl pkcs8 gmp curve25519 hmac kdf drbg
```

**注：** `loaded plugins` 标识了你加载的所有模块

### 4.2 验证 strongswan-starter
`strongswan-starter` 是云服务器上的 VPN 后台服务，先重新加载并重启模型：
```
systemctl daemon-reload && \
systemctl restart strongswan-starter
```

然后查询它的状态：
```
systemctl status strongswan-starter
```

你将会看到：
```
Active: active (running)
```

**注：** 在使用 `status` 指令时，你也会在 `loaded plugins` 中看到你加载的所有模块

接下来应该就可以测试替换的算法模块啦！

## 5. 算法测试

现在的环境就可以完成本小节的操作流程了，大家可以先按照本小节试一遍，但是请注意，**试完在群里说一声，需要按照 3. 的流程更新客户端证书后，才能成功连接 VPN**。

以下用 rsa 算法为例，生成公钥、公共证书、服务器私钥和服务器证书：

1. 生成公钥：
```
pki --gen --type rsa --size 4096 --outform pem > ~/pki/private/ca-key.pem
```
2. 生成公共证书：
```
pki --self --ca --lifetime 3650 --in ~/pki/private/ca-key.pem \
    --type rsa --dn "CN=VPN root CA" --outform pem > ~/pki/cacerts/ca-cert.pem
```
3. 生成服务器私钥：
```
pki --gen --type rsa --size 4096 --outform pem > ~/pki/private/server-key.pem
```
4. 生成服务器证书：
```
pki --pub --in ~/pki/private/server-key.pem --type rsa \
    | pki --issue --lifetime 1825 \
        --cacert ~/pki/cacerts/ca-cert.pem \
        --cakey ~/pki/private/ca-key.pem \
        --dn "CN=120.79.52.57" --san @120.79.52.57 --san 120.79.52.57 \
        --flag serverAuth --flag ikeIntermediate --outform pem \
    >  ~/pki/certs/server-cert.pem
```

替换新生成的的证书：
```
sudo cp -r ~/pki/* /usr/local/etc/ipsec.d
```

重启服务：
```
systemctl restart strongswan-starter
```

此时，按照 3. 的教程，你应该还能连通 VPN～

[参考资料](https://www.digitalocean.com/community/tutorials/how-to-set-up-an-ikev2-vpn-server-with-strongswan-on-ubuntu-22-04)

## 6. 实际操作

生成公钥/私钥文件：
```
# For CA private key
pki --gen --type sm2 --outform pem > ~/pki/private/ca.key.pem

# For self-signed CA certificate
pki --self --in ~/pki/private/ca.key.pem --type sm2 --digest sm3 \
    --dn "C=China, O=Tsinghua, CN=VPN" --ca --lifetime 3650 --outform pem \
    > ~/pki/x509ca/ca.cert.pem

# For VPN server private key
pki --gen --type sm2 --outform pem > ~/pki/private/server.key.pem

# For VPN server public key
pki --pub --type sm2 --in ~/pki/private/server.key.pem --outform pem \
    > ~/pki/pubkey/server.pub.key.pem

# For VPN server certificate key
pki --req --in ~/pki/private/server.key.pem --type sm2 \
    --dn "C=China, O=Tsinghua, CN=VPN" --digest sm3 --outform pem \
    | pki --issue --type pkcs10 --digest sm3 --lifetime 1200 --cacert \
    ~/pki/x509ca/ca.cert.pem --cakey ~/pki/private/ca.key.pem --flag \
    serverAuth --flag ikeIntermediate --outform pem  --san="120.79.52.57" \
    > ~/pki/x509/server.cert.pem

# For client private key
pki --gen --type sm2 --outform pem > ~/pki/private/client.key.pem
pki --req --in ~/pki/private/client.key.pem --type sm2 \
    --dn "C=China, O=Tsinghua, CN=VPN" --digest sm3 --outform pem \
    | pki --issue --type pkcs10 --digest sm3 --lifetime 1200 --cacert \
    ~/pki/x509ca/ca.cert.pem --cakey ~/pki/private/ca.key.pem --outform pem \
    --san="120.79.19.223" > ~/pki/x509/client.cert.pem

cp -r ~/pki/* /usr/local/etc/swanctl
scp -r ~/pki root@120.79.19.223:~

# 登陆 120.79.19.223
cp -r ~/pki/* /usr/local/etc/swanctl
```

用两个客户端连接服务器 A（方便看日志）
```
# 关闭 charon-5.9.8
systemctl stop strongswan-starter

# 运行 charon 系统
/ipsec/libexec/ipsec/charon

# 加载 VPN 配置
swanctl --load-all --clear
```

VPN 服务端配置加载成功：
```
loaded certificate from '/usr/local/etc/swanctl/x509/server.cert.pem'
loaded certificate from '/usr/local/etc/swanctl/x509/client.cert.pem'
loaded certificate from '/usr/local/etc/swanctl/x509ca/ca.cert.pem'
loaded certificate from '/usr/local/etc/swanctl/pubkey/server.pub.key.pem'
loading '/usr/local/etc/swanctl/private/server.key.pem' failed: invalid key type: SM2
unsupported key type in '/usr/local/etc/swanctl/private/server.key.pem'
loaded private key from '/usr/local/etc/swanctl/private/server.key.pem'
loading '/usr/local/etc/swanctl/private/ca.key.pem' failed: invalid key type: SM2
unsupported key type in '/usr/local/etc/swanctl/private/ca.key.pem'
loaded private key from '/usr/local/etc/swanctl/private/ca.key.pem'
loading '/usr/local/etc/swanctl/private/client.key.pem' failed: invalid key type: SM2
unsupported key type in '/usr/local/etc/swanctl/private/client.key.pem'
loaded private key from '/usr/local/etc/swanctl/private/client.key.pem'
no authorities found, 0 unloaded
no pools found, 0 unloaded
loaded connection 'gw-ent1'
successfully loaded 1 connections, 0 unloaded
```

登陆服务器 B 以启动客户端：
```
# 加载 VPN 配置
swanctl --load-all --clear

# 建立 VPN 连接
swanctl -i --child gw-ent1
```

查看 VPN 连接：
```
ipsec status
```