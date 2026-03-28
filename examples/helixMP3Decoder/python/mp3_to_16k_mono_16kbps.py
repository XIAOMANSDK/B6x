#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MP3 转换工具

功能：
- 输入任意 MP3 文件
- 转换为：
    * 单音道 (Mono)
    * 16 kHz 采样率
    * 16 kbps 比特率
    * MP3 输出格式

实现方式：
- 使用 ffmpeg 作为底层音频引擎（工业标准，质量与兼容性最好）
- Python 仅负责参数组织与调用

依赖：
- 已安装 ffmpeg（>= 4.x）
- Python 3.7+

安装 ffmpeg（示例）：
- macOS:   brew install ffmpeg
- Ubuntu:  sudo apt install ffmpeg
- Windows: https://ffmpeg.org

使用方法：
    python mp3_to_16k_mono_16kbps.py input.mp3 output.mp3
"""

import subprocess
import sys
import os


def convert_mp3(input_mp3, output_mp3):
    if not os.path.exists(input_mp3):
        raise FileNotFoundError(input_mp3)

    cmd = [
        'ffmpeg', '-y',
        '-i', input_mp3,

        '-ac', '1',
        '-ar', '16000',
        '-codec:a', 'libmp3lame',

        '-b:a', '16k',
        '-minrate', '16k',
        '-maxrate', '16k',
        '-bufsize', '16k',

        '-write_xing', '0',   # 禁用 Xing Header
        '-vn',

        output_mp3
    ]



    print('Running:', ' '.join(cmd))
    subprocess.check_call(cmd)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python mp3_to_16k_mono_16kbps.py input.mp3 output.mp3')
        sys.exit(1)

    convert_mp3(sys.argv[1], sys.argv[2])
    print('Done.')
