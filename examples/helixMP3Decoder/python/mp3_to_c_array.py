#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MP3 -> C 数组转换工具

功能：
- 读取 MP3 二进制文件
- 生成一个 .c 文件
- 以 unsigned char 数组形式保存 MP3 原始比特流
- 可直接用于 MCU / DSP / 嵌入式解码测试

典型用途：
- 在 C 代码中内嵌 MP3 测试向量
- 无文件系统环境下测试 MP3 解码器
- BLE / Flash / ROM 中的音频资源

使用方法：
    python mp3_to_c_array.py input.mp3 output.c mp3_data

参数说明：
    input.mp3   : 输入 MP3 文件
    output.c    : 输出 C 文件
    mp3_data    : C 数组变量名（可选，默认 mp3_data）
"""

import sys
import os


def mp3_to_c_array(input_mp3, output_c, var_name="mp3_data"):
    if not os.path.exists(input_mp3):
        raise FileNotFoundError(input_mp3)

    with open(input_mp3, 'rb') as f:
        data = f.read()

    with open(output_c, 'w', encoding='utf-8') as out:
        out.write("/* Auto-generated MP3 C array */\n")
        out.write("/* Source file: {} */\n".format(os.path.basename(input_mp3)))
        out.write("/* File size  : {} bytes */\n\n".format(len(data)))

        out.write("#include <stdint.h>\n\n")
        out.write("const uint8_t {}[] = {{\n".format(var_name))

        for i, b in enumerate(data):
            if i % 12 == 0:
                out.write("    ")
            out.write("0x{:02X}, ".format(b))
            if i % 12 == 11:
                out.write("\n")

        if len(data) % 12 != 0:
            out.write("\n")

        out.write("};\n\n")
        out.write("const unsigned int {}_len = {};\n".format(var_name, len(data)))


if __name__ == '__main__':
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python mp3_to_c_array.py input.mp3 output.c [var_name]")
        sys.exit(1)

    input_mp3 = sys.argv[1]
    output_c = sys.argv[2]
    var_name = sys.argv[3] if len(sys.argv) == 4 else "mp3_data"

    mp3_to_c_array(input_mp3, output_c, var_name)
    print("Generated", output_c)
