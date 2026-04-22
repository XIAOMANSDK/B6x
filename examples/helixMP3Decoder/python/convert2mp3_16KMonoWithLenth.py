import sys
import os
import subprocess
import struct

def convert_with_length_header(input_file):
    if not os.path.exists(input_file):
        print(f"错误: 找不到文件 {input_file}")
        return

    base_dir = os.path.dirname(input_file)
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    
    # 临时文件路径
    tmp_wav = os.path.join(base_dir, f"tmp_{base_name}.wav")
    tmp_mp3 = os.path.join(base_dir, f"tmp_{base_name}.mp3")
    
    # 最终输出路径
    final_wav = os.path.join(base_dir, f"{base_name}_16k_header.wav")
    final_mp3 = os.path.join(base_dir, f"{base_name}_16k_16kbps_header.mp3")

    print(f"--- 正在处理: {base_name} ---")

    try:
        # 1. 生成中间 WAV (16kHz, Mono, S16LE)
        print("1. 正在导出标准 PCM 数据...")
        subprocess.run([
            "ffmpeg", "-y", "-i", input_file,
            "-ac", "1", "-ar", "16000", "-acodec", "pcm_s16le",
            tmp_wav
        ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

        # 2. 强制编码 MP3 (Strict CBR 16kbps, 16kHz, No Xing Header)
        print("2. 正在编码固定比特率 MP3 (16kbps)...")
        subprocess.run([
            "ffmpeg", "-y", "-i", tmp_wav,
            "-c:a", "libmp3lame", "-b:a", "16k",
            "-minrate", "16k", "-maxrate", "16k", "-bufsize", "16k",
            "-write_xing", "0", 
            tmp_mp3
        ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

        # 3. 为文件添加 uint32_t 长度报头
        def add_length_header(src_path, dst_path):
            file_size = os.path.getsize(src_path)
            # '<I' 代表小端序 uint32 (4 字节)
            # 如果你的硬件需要大端序，请改为 '>I'
            header = struct.pack('<I', file_size)
            
            with open(src_path, 'rb') as f_src, open(dst_path, 'wb') as f_dst:
                f_dst.write(header)       # 写入 4 字节长度
                f_dst.write(f_src.read()) # 写入原始音频数据
            
            os.remove(src_path) # 删除不带 Header 的临时文件
            return file_size

        wav_size = add_length_header(tmp_wav, final_wav)
        mp3_size = add_length_header(tmp_mp3, final_mp3)

        print("\n" + "="*40)
        print("【转换成功】")
        print(f"WAV 文件: {os.path.basename(final_wav)}")
        print(f"   -> 原始大小: {wav_size} bytes")
        print(f"   -> 最终大小: {wav_size + 4} bytes (含4字节Header)")
        
        print(f"MP3 文件: {os.path.basename(final_mp3)}")
        print(f"   -> 原始大小: {mp3_size} bytes")
        print(f"   -> 最终大小: {mp3_size + 4} bytes (含4字节Header)")
        print("="*40)
        
        # 验证 MP3 前四个字节
        with open(final_mp3, 'rb') as f:
            head_bytes = f.read(4)
            val = struct.unpack('<I', head_bytes)[0]
            print(f"验证：文件头前4字节解析值为 {val}，读取正确。")

    except subprocess.CalledProcessError as e:
        print(f"FFmpeg 运行出错: {e.stderr.decode()}")
    except Exception as e:
        print(f"程序出错: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        path = input("请输入音频文件路径: ").strip().replace('"', '')
    else:
        path = sys.argv[1]
    convert_with_length_header(path)