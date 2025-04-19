#!/bin/bash

# 创建构建目录
mkdir -p build
cd build

# 运行 CMake 生成构建文件
cmake ..

# 检查 CMake 是否成功运行
if [ $? -ne 0 ]; then
    echo "CMake 配置失败"
    exit 1
fi

# 使用 make 进行编译
make

# 检查 make 是否成功运行
if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

# 返回到项目根目录
cd ..

mkdir -p lib

mkdir -p lib/include

# 复制 include 目录到 lib/include 目录下
cp -r include/ lib/

echo "项目构建完成，库文件生成成功"    