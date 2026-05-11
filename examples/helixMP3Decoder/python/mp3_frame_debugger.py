#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import struct
import binascii
import wave
import os

try:
    import miniaudio
except ImportError:
    print("【错误】未检测到 miniaudio 库。请运行: pip install miniaudio")
    sys.exit(1)

# MP3 标准参数表
BITRATE_TABLE = {
    ('MPEG2', 'LayerIII'): [0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0],
    ('MPEG1', 'LayerIII'): [0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0],
}
SAMPLE_RATE_TABLE = {
    'MPEG1': [44100, 48000, 32000, 0],
    'MPEG2': [22050, 24000, 16000, 0],
}

def parse_header(header_bytes):
    if len(header_bytes) != 4: return None
    hdr = struct.unpack('>I', header_bytes)[0]
    if (hdr & 0xFFE00000) != 0xFFE00000: return None
    ver_id = (hdr >> 19) & 0x3
    lay_id = (hdr >> 17) & 0x3
    pad_bit = (hdr >> 9) & 0x1
    br_id = (hdr >> 12) & 0xF
    sr_id = (hdr >> 10) & 0x3
    ver_map = {2: 'MPEG2', 3: 'MPEG1'}
    if ver_id not in ver_map or lay_id != 1: return None 
    version = ver_map[ver_id]
    bitrate = BITRATE_TABLE.get((version, 'LayerIII'), [0]*16)[br_id]
    samplerate = SAMPLE_RATE_TABLE.get(version, [0]*4)[sr_id]
    if bitrate == 0 or samplerate == 0: return None
    samples = 1152 if version == 'MPEG1' else 576
    coeff = 144 if version == 'MPEG1' else 72
    size = int((coeff * bitrate * 1000 / samplerate) + pad_bit)
    return {'samples': samples, 'size': size}

def save_as_wav(samples, output_filename, sample_rate=16000):
    """将采样数据保存为标准 WAV 文件"""
    print(f"\n[系统] 正在生成 WAV 文件: {output_filename}...")
    try:
        with wave.open(output_filename, 'wb') as wf:
            wf.setnchannels(1)          # 单声道
            wf.setsampwidth(2)         # 16-bit (2字节)
            wf.setframerate(sample_rate)
            # 将 samples (int16 数组) 转换为二进制字节流并写入
            wf.writeframes(samples.tobytes())
        print(f"【成功】WAV 文件已保存，大小: {os.path.getsize(output_filename)} 字节")
    except Exception as e:
        print(f"【错误】写入 WAV 失败: {e}")

def analyze_and_convert_mp3(filename):
    # 强制采样率为 16000Hz, 单声道以匹配嵌入式环境
    TARGET_SR = 16000
    
    try:
        decoded_data = miniaudio.decode_file(filename, sample_rate=TARGET_SR, nchannels=1)
        all_samples = decoded_data.samples # 这里是 array.array 类型
    except Exception as e:
        print(f"【错误】解码失败: {e}")
        return

    # --- 执行 WAV 保存 ---
    output_wav = os.path.splitext(filename)[0] + "_decoded.wav"
    save_as_wav(all_samples, output_wav, TARGET_SR)

    # --- 原有的帧解析逻辑 ---
    with open(filename, 'rb') as f:
        header = f.read(10)
        start_offset = 0
        if header.startswith(b'ID3'):
            size = ((header[6] & 0x7F) << 21) | ((header[7] & 0x7F) << 14) | \
                   ((header[8] & 0x7F) << 7) | (header[9] & 0x7F)
            start_offset = 10 + size
        f.seek(start_offset)
        
        frame_idx = 0
        sample_ptr = 0

        while True:
            offset = f.tell()
            hdr_bytes = f.read(4)
            if len(hdr_bytes) < 4: break
            
            info = parse_header(hdr_bytes)
            if not info:
                f.seek(offset + 1)
                continue
            
            body = f.read(info['size'] - 4)
            raw_data = hdr_bytes + body
            
            print(f"\n{'='*25} FRAME #{frame_idx} (Offset: 0x{offset:X}) {'='*25}")
            
            # 1. 打印原始压缩 Hex
            print(f"[原始 MP3 数据] ({len(raw_data)} 字节):")
            hex_raw = binascii.hexlify(raw_data).decode().upper()
            for i in range(0, len(hex_raw), 32):
                line = hex_raw[i:i+32]
                print("    " + " ".join(line[j:j+2] for j in range(0, len(line), 2)))
            
            # 2. 打印解码后 PCM Hex
            frame_len = info['samples']
            # 注意：此处切片后的数据仅用于打印展示
            pcm_frame = all_samples[sample_ptr : sample_ptr + frame_len]
            
            if len(pcm_frame) > 0:
                print(f"\n[解码后 PCM 数据] ({len(pcm_frame)} 采样点, S16LE Hex):")
                for i in range(0, len(pcm_frame)):
                    if i % 12 == 0:
                        print(f"\n  ", end="")
                    print(f"{pcm_frame[i] & 0xFFFF:04X}", end=" ")
                print()
            
            sample_ptr += frame_len
            frame_idx += 1
            
            # 限制打印帧数，防止控制台爆满（可选）
            if frame_idx >= 5: 
                print(f"\n... 后续帧已省略，完整音频已写入 {output_wav} ...")
                break

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("用法: python script.py test.mp3")
    else:
        analyze_and_convert_mp3(sys.argv[1])