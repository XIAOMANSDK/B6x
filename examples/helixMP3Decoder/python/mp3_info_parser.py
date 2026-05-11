#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MP3 文件信息分析工具

功能：
- 解析 MP3 比特流帧头
- 获取 MPEG 版本、Layer、比特率、采样率
- 声道模式、声道数量
- 帧长度、帧数量、时长估算
- 是否为 CBR / VBR（基于帧比特率变化判断）
- 支持跳过 ID3v2 标签

不依赖第三方库，适合嵌入式/底层音频分析参考

使用方法：
    python mp3_info_parser.py test.mp3
"""

import sys
import struct

# ==============================
# MP3 标准查表
# ==============================
BITRATE_TABLE = {
    # version, layer : [bitrate index 0~15]
    ('MPEG1', 'LayerI'):   [None,32,64,96,128,160,192,224,256,288,320,352,384,416,448,None],
    ('MPEG1', 'LayerII'):  [None,32,48,56,64,80,96,112,128,160,192,224,256,320,384,None],
    ('MPEG1', 'LayerIII'): [None,32,40,48,56,64,80,96,112,128,160,192,224,256,320,None],

    ('MPEG2', 'LayerI'):   [None,32,48,56,64,80,96,112,128,144,160,176,192,224,256,None],
    ('MPEG2', 'LayerII'):  [None,8,16,24,32,40,48,56,64,80,96,112,128,144,160,None],
    ('MPEG2', 'LayerIII'): [None,8,16,24,32,40,48,56,64,80,96,112,128,144,160,None],
}

SAMPLE_RATE_TABLE = {
    'MPEG1': [44100, 48000, 32000, None],
    'MPEG2': [22050, 24000, 16000, None],
    'MPEG2.5': [11025, 12000, 8000, None]
}

CHANNEL_MODE = {
    0: 'Stereo',
    1: 'Joint Stereo',
    2: 'Dual Channel',
    3: 'Mono'
}


# Layer I 为 384, Layer II 为 1152
# Layer III: MPEG1 为 1152, MPEG2/2.5 为 576
SAMPLES_PER_FRAME_TABLE = {
    'LayerI': 384,
    'LayerII': 1152,
    ('MPEG1', 'LayerIII'): 1152,
    ('MPEG2', 'LayerIII'): 576,
    ('MPEG2.5', 'LayerIII'): 576,
}
# ==============================
# 工具函数
# ==============================

def skip_id3v2(f):
    header = f.read(10)
    if header[0:3] == b'ID3':
        size = ((header[6] & 0x7F) << 21) | ((header[7] & 0x7F) << 14) | ((header[8] & 0x7F) << 7) | (header[9] & 0x7F)
        f.seek(size, 1)
        return 10 + size
    f.seek(0)
    return 0


def parse_frame_header(hdr):
    if hdr & 0xFFE00000 != 0xFFE00000:
        return None

    version_id = (hdr >> 19) & 0x3
    layer_id   = (hdr >> 17) & 0x3
    padding    = (hdr >> 9) & 0x1
    bitrate_id = (hdr >> 12) & 0xF
    sample_id  = (hdr >> 10) & 0x3
    channel_id = (hdr >> 6) & 0x3

    version_map = {0:'MPEG2.5', 2:'MPEG2', 3:'MPEG1'}
    layer_map   = {1:'LayerIII', 2:'LayerII', 3:'LayerI'}

    if version_id not in version_map or layer_id not in layer_map:
        return None

    version = version_map[version_id]
    layer = layer_map[layer_id]
    
    if layer == 'LayerI':
        samples = 384
    elif layer == 'LayerII':
        samples = 1152
    else: # Layer III
        samples = 1152 if version == 'MPEG1' else 576
        
    bitrate = BITRATE_TABLE.get((version.replace('.5',''), layer), [None]*16)[bitrate_id]
    samplerate = SAMPLE_RATE_TABLE[version][sample_id]

    if bitrate is None or samplerate is None:
        return None

    return {
        'version': version,
        'layer': layer,
        'bitrate_kbps': bitrate,
        'sample_rate': samplerate,
        'padding': padding,
        'samples_per_frame': samples,
        'channels': 1 if channel_id == 3 else 2,
        'channel_mode': CHANNEL_MODE[channel_id]
    }


def frame_length(info):
    """
    计算精确的帧长度（字节）
    Layer I: L = (12 * Bitrate / SampleRate + Padding) * 4
    Layer II/III: L = (SamplesPerFrame / 8 * Bitrate / SampleRate) + Padding
    """
    bitrate_bps = info['bitrate_kbps'] * 1000
    if info['layer'] == 'LayerI':
        return int((12 * bitrate_bps / info['sample_rate'] + info['padding']) * 4)
    else:
        # 这里的系数由 SamplesPerFrame / 8 决定
        # MPEG1 L3: 1152/8 = 144
        # MPEG2 L3: 576/8 = 72
        coefficient = info['samples_per_frame'] // 8
        return int(coefficient * bitrate_bps / info['sample_rate'] + info['padding'])

# ==============================
# 主流程
# ==============================

def analyze_mp3(filename):
    with open(filename, 'rb') as f:
        id3_size = skip_id3v2(f)
        frame_count = 0
        bitrates = set()
        first_info = None

        while True:
            hdr_bytes = f.read(4)
            if len(hdr_bytes) < 4:
                break
            hdr = struct.unpack('>I', hdr_bytes)[0]
            info = parse_frame_header(hdr)
            if not info:
                f.seek(-3, 1)
                continue
            if not first_info:
                first_info = info
            bitrates.add(info['bitrate_kbps'])
            flen = frame_length(info)
            f.seek(flen - 4, 1)
            frame_count += 1

        if not first_info:
            raise RuntimeError('不是有效的 MP3 文件')

        # 使用动态采样点计算时长
        total_samples = frame_count * first_info['samples_per_frame']
        duration = total_samples / first_info['sample_rate']

        return {
            'id3v2_tag': f"{id3_size} Bytes" if id3_size else "NONE",
            'version': first_info['version'],
            'layer': first_info['layer'],
            'sample_rate': f"{first_info['sample_rate']} Hz",
            'bitrates': f"{sorted(bitrates)} kbps",
            'samples_per_frame': f"{first_info['samples_per_frame']} Samples/Frame",
            'channels': first_info['channels'],
            'channel_mode': first_info['channel_mode'],
            'frame_count': f"{frame_count} Frames",
            'duration': f"{round(duration, 2)} sec",
            'is_vbr': len(bitrates) > 1
        }


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: python mp3_info_parser.py xxx.mp3')
        sys.exit(1)

    info = analyze_mp3(sys.argv[1])
    for k, v in info.items():
        print(f'{k}: {v}')
