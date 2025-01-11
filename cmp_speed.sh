#!/bin/bash

# 检查是否传入了两个程序路径
if [ $# -ne 2 ]; then
  echo "请指定两个程序路径进行比较."
  exit 1
fi

program1=$1
program2=$2

# 运行第一个程序并记录时间和内存
output1=$(./measure "$program1")

# 从输出中提取时间和内存
time1=$(echo "$output1" | grep -oP '程序运行时间: \K[0-9]+\.[0-9]+')
mem1=$(echo "$output1" | grep -oP '最大内存占用: \K[0-9]+')

# 运行第二个程序并记录时间和内存
output2=$(./measure "$program2")

# 从输出中提取时间和内存
time2=$(echo "$output2" | grep -oP '程序运行时间: \K[0-9]+\.[0-9]+')
mem2=$(echo "$output2" | grep -oP '最大内存占用: \K[0-9]+')

# 确保时间和内存变量不是空的
if [ -z "$time1" ] || [ -z "$mem1" ] || [ -z "$time2" ] || [ -z "$mem2" ]; then
  echo "错误: 无效的时间或内存值"
  exit 1
fi

# 内存单位自动转换函数
convert_mem_unit() {
  mem=$1
  if [ "$mem" -ge 1048576 ]; then
    # 大于等于1GB
    echo "$(echo "scale=2; $mem / 1048576" | bc) GB"
  elif [ "$mem" -ge 1024 ]; then
    # 大于等于1MB但小于1GB
    echo "$(echo "scale=2; $mem / 1024" | bc) MB"
  else
    # 小于1MB
    echo "${mem} KB"
  fi
}

# 比较运行时间
echo "==================== 比较结果 ===================="

echo "$program1 总运行时间: $time1 秒, 最大内存占用: $(convert_mem_unit $mem1)"
echo "$program2 总运行时间: $time2 秒, 最大内存占用: $(convert_mem_unit $mem2)"

# 使用 bc 进行时间比较，确保时间变量是数字
if [ $(echo "$time1 < $time2" | bc) -eq 1 ]; then
    echo "$program1 更快, 运行时间: $time1 秒"
else
    echo "$program2 更快, 运行时间: $time2 秒"
fi

# 使用 bc 进行内存比较
if [ $(echo "$mem1 < $mem2" | bc) -eq 1 ]; then
    echo "$program1 使用的内存更少, 最大内存占用: $(convert_mem_unit $mem1)"
else
    echo "$program2 使用的内存更少, 最大内存占用: $(convert_mem_unit $mem2)"
fi

echo "==================== 所有程序已完成 ===================="
