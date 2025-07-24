#!/bin/bash

# 使用 readlink -f 获取脚本的绝对路径（兼容符号链接）
SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# 切换到脚本所在目录
cd "$SCRIPT_DIR" || exit

# 优化编译
g++ -shared -fPIC -O3 -march=native -flto -fno-semantic-interposition \
    -funroll-loops -ftree-vectorize \
    -fipa-pta -o line_checker.so line_checker.cpp 
