#!/bin/bash

# 找到所有 .cc, .h, .hpp, .tcc 文件并统计每个文件的行数
find . -type f \( -name "*.cc" -o -name "*.h" -o -name "*.hpp" -o -name "*.tcc" \) -exec wc -l {} +