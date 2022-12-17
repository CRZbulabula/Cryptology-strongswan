# 密码学大作业上手指南

## 1. 目标

我们将用 strongswan 搭建一个 IPsec-VPN，以保证下述场景（Roadwarrior Case with Virtual IP）的安全通讯：
```
172.18.204.66 -- | 120.79.52.57 | === | x.x.x.x | -- 10.10.10.0/24
     VPC           Cloud Server           You       Your virtual IP
```

简单来说，我们拥有一个云服务器，它的公网 IPV4 地址为 120.79.52.57，私网 IPV4 地址为 172.18.204.66。我们希望能在一个不可信的网络环境下，安全地连接这个云服务器，并且能和它的私有地址通讯。同时，为了简化该云服务从专有网络（VPC，也就是私有地址）出发，连接到我们的路由，IPsec 服务还会给我们分配一个虚拟地址。我们的虚拟地址会从 10.10.10.0/24 网段中分配，虚拟地址的范围是 10.10.10.1\~10.10.10.254。

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

首先，你需要把项目的 ca-cert.pem 文件拉到本地。然后执行以下指令：
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

在 Ubuntu 下，先进入 `~/Cryptology-strongswan/strongswan-5.9.8` 目录（这也是我们的 git 项目），执行：
```
./configure --prefix=/usr --sysconfdir=/etc --disable-gmp --enable-openssl --enable-systemd --enable-swanctl --disable-charon --disable-stroke --disable-scepclient
```

然后：
```
make && make install
```

看到以下输出：
```
Making install in testing
make[2]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8/testing'
make[3]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8/testing'
make[3]: Nothing to be done for 'install-exec-am'.
make[3]: Nothing to be done for 'install-data-am'.
make[3]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8/testing'
make[2]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8/testing'
Making install in scripts
make[2]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8/scripts'
make[3]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8/scripts'
make[3]: Nothing to be done for 'install-exec-am'.
make[3]: Nothing to be done for 'install-data-am'.
make[3]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8/scripts'
make[2]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8/scripts'
make[2]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8'
make[3]: Entering directory '/root/Cryptology-strongswan/strongswan-5.9.8'
make[3]: Nothing to be done for 'install-exec-am'.
make[3]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8'
make[2]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8'
make[1]: Leaving directory '/root/Cryptology-strongswan/strongswan-5.9.8'
```

这里就编译成功了，`make && make install` 指令会还把相关工具直接安装进环境变量，可以用以下指令测试：
```
pki
```

**注：**`pki` 指令用于生成 IPsec-VPN 使用的证书，应该是我们算法模块国密化替换的主要验证工具。



接下来应该就可以测试替换的算法模块啦！

## 5. 算法测试

现在的环境就可以完成本小节的操作流程了，大家可以先按照本小节试一遍，但是请注意，**试完在群里说一声，需要更新客户端证书才能连接 VPN**。

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

[参考资料](https://www.digitalocean.com/community/tutorials/how-to-set-up-an-ikev2-vpn-server-with-strongswan-on-ubuntu-22-04)