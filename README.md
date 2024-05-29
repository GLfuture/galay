
# Galay

![Static Badge](https://img.shields.io/badge/License-MIT-yellow)
![Static Badge](https://img.shields.io/badge/Language-C%2B%2B-red%20) 
![Static Badge](https://img.shields.io/badge/Platfrom-Linux-red)

## 简介

> galay网络框架是一个轻量级的网络框架，拥有较少的依赖

## 安装

1. 下载必要依赖openssl，g++(version > 9)，cmake spdlog
    > linux
    >```shell
    > sudo apt-get install libssl-dev g++ cmake
    > git clone https://github.com/gabime/spdlog.git
    > cd spdlog
    > mkdir build
    > cd build && sudo make -j4 && make install
    >```

2. 编译安装
    > ```shell
    > mkdir build
    > cd build
    > cmake ..
    > sudo make -j4 && make install
    >```
    > 头文件安装在/user/local/include,库文件安装在/user/local/lib

3. 使用
    > ```shell
    > g++ example.cc -o example -lgalay-static
    > ```
    > 具体使用请查看tutorial目录下的内容
