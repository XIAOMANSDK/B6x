import sys
import os
import subprocess

def convert_to_strict_cbr(input_file):
    # 1. 检查输入
    if not os.path.exists(input_file):
        print(f"错误: 找不到文件 {input_file}")
        return

    # 2. 准备路径
    base_dir = os.path.dirname(input_file)
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    
    # 输出文件名
    wav_output = os.path.join(base_dir, f"{base_name}_16k.wav")
    mp3_output = os.path.join(base_dir, f"{base_name}_16k_16kbps_CBR.mp3")

    print(f"--- 正在处理: {base_name} ---")

    # ==========================================
    # 第一步：生成标准的 PCM WAV (作为中间文件)
    # ==========================================
    # 这步确保源数据一定是 16k 单声道，为压缩做准备
    print("1. 生成中间 WAV 文件 (PCM 16kHz Mono)...")
    cmd_wav = [
        "ffmpeg", "-y",              # -y: 覆盖不询问
        "-i", input_file,            # 输入
        "-vn",                       # 去掉视频
        "-ac", "1",                  # 单声道
        "-ar", "16000",              # 采样率 16000
        "-acodec", "pcm_s16le",      # 编码: PCM 16-bit
        wav_output
    ]
    
    try:
        subprocess.run(cmd_wav, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        print(f"WAV 转换失败: {e}")
        return

    # ==========================================
    # 第二步：强力压缩为 MP3 CBR 16kbps
    # ==========================================
    print("2. 强制编码为 MP3 (Strict CBR 16kbps)...")
    
    # 核心参数解析：
    # -b:a 16k          : 目标比特率
    # -minrate 16k      : 最小门槛
    # -maxrate 16k      : 最大门槛
    # -bufsize 16k      : 缓冲区设得极小，迫使编码器不能"攒"数据
    # -write_xing 0     : 【关键】不写入 VBR 索引头(Xing Header)，防止被误认为 VBR
    # -c:a libmp3lame   : 指定编码器
    
    cmd_mp3 = [
        "ffmpeg", "-y",
        "-i", wav_output,
        "-ac", "1",
        "-ar", "16000",
        "-c:a", "libmp3lame",   # 使用 LAME 编码器
        "-b:a", "16k",          # 强制 16k
        "-minrate", "16k",      # 下限锁定
        "-maxrate", "16k",      # 上限锁定
        "-bufsize", "16k",      # 缓冲区锁定
        "-write_xing", "0",     # 禁用 VBR 头，这对 CBR 识别至关重要！
        mp3_output
    ]

    try:
        # 运行命令
        subprocess.run(cmd_mp3, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        
        # ==========================================
        # 第三步：验证结果
        # ==========================================
        file_size = os.path.getsize(mp3_output)
        
        # 计算时长（通过 WAV 文件计算最准）
        import wave
        with wave.open(wav_output, 'rb') as w:
            frames = w.getnframes()
            rate = w.getframerate()
            duration = frames / rate
            
        print("\n[结果验证]")
        print(f"MP3 文件: {mp3_output}")
        print(f"WAV 文件: {wav_output}")
        print(f"音频时长: {duration:.2f} 秒")
        print(f"MP3 大小: {file_size} 字节")
        
        # CBR 理论大小计算: 16000 bps / 8 = 2000 字节/秒
        expected_size = int(duration * 2000)
        diff = abs(file_size - expected_size)
        
        print(f"理论 CBR 大小: {expected_size} 字节")
        print(f"差异: {diff} 字节 (通常应 < 500 字节，仅包含极少帧头开销)")
        
        if diff < 1000:
            print("✅ 验证通过：文件大小严格符合 CBR 16Kbps 标准。")
        else:
            print("⚠️ 警告：文件大小与理论值有偏差，请检查。")

    except subprocess.CalledProcessError as e:
        print(f"MP3 编码失败。请确保安装了 ffmpeg。\n错误信息: {e.stderr.decode()}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        path = input("请输入音频文件路径: ").strip().replace('"', '')
    else:
        path = sys.argv[1]
    
    convert_to_strict_cbr(path)