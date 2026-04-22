import sys
import wave
import struct
import re
import os

# --- 配置参数 ---
SAMPLE_RATE = 16000  # 采样率
CHANNELS = 1         # 单声道
BIT_DEPTH = 16       # 16位深度 (2字节)

def main():
    print("--- 十六进制文本转 WAV 工具 ---")

    # 1. 获取输入文件名
    if len(sys.argv) > 1:
        input_filename = sys.argv[1]
        if len(sys.argv) > 2:
            output_filename = sys.argv[2]
        else:
            # 默认输出文件名：原文件名 + .wav
            base_name = os.path.splitext(input_filename)[0]
            output_filename = f"{base_name}.wav"
    else:
        # 如果没有命令行参数，则交互式询问
        input_filename = input("请输入包含十六进制数据的文本文件名 (例如 data.txt): ").strip()
        # 去除可能包含的引号
        input_filename = input_filename.replace('"', '').replace("'", "")
        
        if not input_filename:
            print("错误：必须输入文件名。")
            return
            
        base_name = os.path.splitext(input_filename)[0]
        output_filename = f"{base_name}.wav"

    # 2. 检查文件是否存在
    if not os.path.exists(input_filename):
        print(f"\n错误：找不到文件 '{input_filename}'")
        return

    print(f"\n正在读取文件: {input_filename} ...")

    # 3. 读取并解析数据
    try:
        with open(input_filename, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"读取文件出错: {e}")
        return

    # 正则表达式匹配 0x 开头的十六进制数 (忽略大小写)
    hex_list = re.findall(r'0x[0-9A-Fa-f]+', content)
    
    total_samples = len(hex_list)
    if total_samples == 0:
        print("错误：在文件中未找到格式为 '0x...' 的十六进制数据。")
        return

    print(f"找到 {total_samples} 个采样点。正在转换...")

    # 4. 转换为二进制 PCM 数据
    pcm_data = bytearray()
    
    # 预计算偏移量，用于处理负数 (Two's Complement)
    MAX_INT16 = 32767
    OFFSET = 65536

    for hex_str in hex_list:
        val = int(hex_str, 16)
        
        # 核心逻辑：将无符号的 hex 转为有符号的 16-bit int
        # 如果大于 0x7FFF (32767)，说明是负数
        if val > MAX_INT16:
            val -= OFFSET
        
        # 打包为小端序 (Little Endian) 的 16位整数
        # < : 小端序
        # h : short (2 bytes)
        try:
            pcm_data.extend(struct.pack('<h', val))
        except struct.error:
            # 如果数据异常（超过FFFF），做截断处理或跳过
            pass

    # 5. 写入 WAV 文件
    try:
        with wave.open(output_filename, 'wb') as wav_file:
            wav_file.setnchannels(CHANNELS)
            wav_file.setsampwidth(2) # 16位 = 2字节
            wav_file.setframerate(SAMPLE_RATE)
            wav_file.writeframes(pcm_data)
            
        duration = total_samples / SAMPLE_RATE
        print("\n" + "="*30)
        print(f"转换成功！")
        print(f"输出文件: {output_filename}")
        print(f"音频参数: {SAMPLE_RATE}Hz, 16-bit, Mono")
        print(f"音频时长: {duration:.4f} 秒")
        print("="*30)
        
    except Exception as e:
        print(f"写入 WAV 失败: {e}")

if __name__ == "__main__":
    main()