## CallMonitor

是用于识别通话过程中, 对方是否是语音信箱. 

之所以将 CallMonitor 是因为, 将来它还可能监控其它的通话信息.

此工程是跨平台的, 即: 在 windows 和 linux 平台都可以运行 (只在 centos:7 上运行过). 


### 基础环境

```text
system==centos:v7

python==3.8.10
cmake==3.25.0
gcc==11.1.0

```


### 安装步骤 - linux 平台

```shell
sh install.sh --stage -1 --stop_stage 3 --system_version centos
```
这个过程依次执行: 
1. 下载模型文件. 
2. 安装 python
3. 创建 python 虚拟环境 CallMonitor. 
4. 安装 cmake
5. 安装 gcc

```shell
source /data/local/bin/CallMonitor/bin/activate
pip3 install -r requirements.txt
```
1. 激活虚拟环境. 
2. 安装 python 依赖包. 

```shell
cmake -B build
```
1. 构建 cmake.


```shell
sh build.sh --system_version centos
```
1. 编译. 


```shell
sh start.sh --environment dev --http_port 4071 --build_dir build
```
1. 运行服务
environment 可选值: dev, gz, hk, mx, vi, id. 


```text
sh for_restart.sh --environment id --http_port 4071 --build_dir build
```
1. for_restart.sh 脚本是在服务挂掉之后自动拉起.


```shell
cd unittest
python3 unittest_cnn_voicemail.py --port 4071

```
1. 单元测试


```shell
cd python_script
python3 access_test_call_monitor.py
```
1. 调用测试


### 安装步骤 - windows 平台


安装 git
```text
我们需要在 git bash 中执行 cmake 构建, cmake 编译, 服务运行等工作, 而不是开发工作中. 

git 下载链接: 
https://git-scm.com/download/win

```



安装 MSVC (Visual Studio)
```text
下载 Visual Studio 社区版.
https://visualstudio.microsoft.com/zh-hans/thank-you-downloading-visual-studio

安装 Visual Studio Community 2022

工作负荷里面, 安装 `使用C++的桌面开发`. 

```


配置 cmake
```text
Visual Studio 安装后, 
添加 Visual Studio 中的 CMake 到环境变量 Path 中. 如下:  
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
如果原来已添加有 cmake 的路径, 则删掉它. 如下: 
C:\Program Files\cmake-3.25.0-rc2-windows-x86_64\bin


在 git bash 中查看 cmake 版本
>>> cmake --version
cmake version 3.25.1-msvc1

CMake suite maintained and supported by Kitware (kitware.com/cmake).


在 git bash 中查看 cmake 命令所在的路径
>>> which cmake
/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake


```

另外你还需要安装好 python 和依赖. 

之后在 git bash 中执行 cmake 构建和编译. 


```shell
cmake -B build

cmake --build ./build --target CallMonitor -j "$(grep -c ^processor /proc/cpuinfo)"

cp ./build/Debug/CallMonitor.exe  ./build/CallMonitor.exe

./build/CallMonitor.exe

```
1. 构建 cmake.
2. 编译
3. 复制可执行文件. 
4. 执行


之后: 
1. 通过 python_script 目录下的 python 脚本测试服务调用. 主要用于代码编写时的测试
2. 你可以通过 unittest 目录下的 python 文件做单元测试. 这里的主要目的是测试服务的识别是否准确, 以判断写代码逻辑是否写错了. 


### 从 Docker 容器启动


由于服务运行时会把请求中发来的音频文件保存, 因此, 这里采用 -v 将宿主机的路径和容器中的路径挂载起来. 

```shell

docker run -itd \
--name CallMonitor \
-p 4070:4070 \
-v /data/tianxing/update_stream_wav:/data/tianxing/update_stream_wav \
daocloud.io/centos:7 \
/bin/bash


```


### 文件上传 OSS

```text
参考链接: 
https://nxtele.coding.net/p/aibot/d/fs_callbot/git/tree/master/fs_wav_process.py

```


### 备注


上传和下载镜像
```text
bash /data/tianxing/images/transfer_nx.sh push cmake_gcc_py38:v1
bash /data/tianxing/images/transfer_img.sh pull nxtele-docker.pkg.coding.net/ops/callbot-generic/cmake_gcc_py38:v1

sh /data/tianxing/images/transfer_nx.sh push callmonitor:v20230515_1002
bash /data/tianxing/images/transfer_img.sh pull nxtele-docker.pkg.coding.net/ops/callbot-generic/callmonitor:v20230515_1002

```

查看 nginx 配置
```text
cd /etc/nginx/conf.d

cat mrcp.nginx

service nginx reload

nginx -t
```

启动容器
从 cmake_gcc_py38:v1 镜像布署服务, 这样可以避免 cmake, gcc, python 几个工具的下载和编译耗时太长. 

```shell
docker run -itd \
--name CallMonitor \
-p 4071:4071 \
-v /data/tianxing/update_stream_wav:/data/tianxing/update_stream_wav \
nxtele-docker.pkg.coding.net/ops/callbot-generic/cmake_gcc_py38:v1 \
/bin/bash

```



git 每次拉代码时都需要输入账号密码. 
先执行以下命令, 之后再输入一次账号密码, 以后就不需要再输入了. 
```text
git config --global credential.helper store

```

core 调试
```text

安装 gdb
yum install -y gdb

查看 core 文件
gdb -c core.4947 ./build/CallMonitor
where

```

查看进程的线程数
```text
cat /proc/770/status | grep Threads
```

从容器中复制文件
```text
docker cp \
CallMonitor:/data/tianxing/CLionProjects/CallMonitor/build/CallMonitor \
/tmp

docker cp \
/tmp/CallMonitor \
CallMonitor:/data/tianxing/CLionProjects/CallMonitor/build/

docker cp \
CallMonitor:/data/tianxing/CLionProjects/CallMonitor/build \
/tmp



```

nginx 重启
```text
cd /etc/nginx/conf.d

service nginx reload

nginx -t
```
