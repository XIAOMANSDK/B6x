# MP3 to GSM Converter Tool

## 功能说明
将任意MP3文件转换为GSM 6.10格式，并在文件头添加uint32长度信息。

## 文件格式
生成的GSM文件格式：
```
+-------------------+
| File Size (4B)    |  uint32, little-endian, GSM数据大小（不含头）
+-------------------+
| GSM Data (variable)|  GSM 6.10 encoded audio data
+-------------------+
```

## 使用方法

### 1. 查看帮助信息
```bash
python mp3_to_gsm.py
```

显示：
```
============================================================
MP3 to GSM Converter with File Size Header
============================================================

Usage: python mp3_to_gsm.py <mp3_file>

Example:
  python mp3_to_gsm.py audio.mp3
  Output: audio.gsm

Requirements:
  - ffmpeg must be installed and in PATH
  - Python 3.x
============================================================
```

### 2. 转换单个MP3文件
```bash
# Windows
python mp3_to_gsm.py audio.mp3

# Linux/Mac
python3 mp3_to_gsm.py audio.mp3
```
生成 `audio.gsm`

### 3. 批量转换多个MP3文件
```bash
# Windows批处理
for %f in (*.mp3) do python mp3_to_gsm.py "%f"
```

```bash
# Linux/Mac
for file in *.mp3; do
    python mp3_to_gsm.py "$file"
done
```

### 4. 转换指定路径的文件
```bash
# 相对路径
python mp3_gsm.py ../audio/test.mp3

# 绝对路径
python mp3_gsm.py "C:\Users\Username\Music\voice.mp3"

# 带空格的文件名（使用引号）
python mp3_gsm.py "my audio.mp3"
```

### 5. 在当前工作目录转换所有MP3
```bash
# 创建批处理脚本
echo off
for %%f in (*.mp3) do python mp3_to_gsm.py "%%f"
```

保存为 `convert_all.bat` 然后运行。

### 6. 在代码中使用转换后的GSM文件

转换完成后，可以使用 `gsm_to_c_array.py` 将GSM文件转换为C数组：

```bash
# 1. 转换MP3为GSM
python mp3_to_gsm.py my_audio.mp3

# 2. 转换GSM为C数组
python gsm_to_c_array.py my_audio.gsm

# 3. 将生成的gsm_audio_data.h复制到src/目录
# 4. 在Keil MDK中重新编译项目
```

## 编码参数
- **输入格式**: MP3 (任意采样率/声道)
- **输出采样率**: 8000 Hz
- **输出声道**: 单声道 (Mono)
- **编码格式**: GSM 6.10 (libgsm)
- **帧大小**: 33字节/帧
- **帧率**: 约100帧/秒
- **比特率**: 约13 kbps

## 依赖要求
- **Python**: 3.x 或更高版本
- **ffmpeg**: 必须已安装并在系统PATH环境变量中

### 安装ffmpeg

#### Windows系统
1. 访问 https://ffmpeg.org/download.html
2. 下载 "essentials build" 版本
3. 解压到目录，例如 `C:\ffmpeg`
4. 添加到系统PATH：
   - 右键"此电脑" → 属性 → 高级系统设置 → 环境变量
   - 编辑系统变量 `Path`，添加 `C:\ffmpeg\bin`
   - 确认添加

验证安装：
```cmd
ffmpeg -version
```
应该显示版本信息，例如 `ffmpeg version 8.0.1`

#### Linux系统
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install ffmpeg

# CentOS/RHEL
sudo yum install ffmpeg

# Arch Linux
sudo pacman -S ffmpeg
```

#### macOS
```bash
# 使用Homebrew
brew install ffmpeg
```

## 实际使用示例

### 示例1: 转换单个语音文件
```bash
cd D:\svn\bxx_DragonC1\sdk6\examples\GSMDoceder
python mp3_to_gsm.py greeting.mp3
```

输出：
```
Converting: greeting.mp3
        -> greeting.gsm

Step 1: Encoding to GSM format...
Step 2: Adding file size header...
  GSM data size: 1234 bytes
  Frames: 37 (33 bytes per frame)
  Final file size: 1238 bytes (including 4-byte header)

Success! Created: greeting.gsm
Cleaned up temporary file: greeting_temp.gsm
```

### 示例2: 批量处理项目音频
```bash
cd D:\svn\bxx_DragonC1\sdk6\examples\GSMDoceder

# 批量转换
for file in voice1.mp3 voice2.mp3 voice3.mp3; do
    python mp3_to_gsm.py "$file"
done

# 查看生成的文件
dir *.gsm
```

### 示例3: 完整工作流程（转换 → 数组 → 编译）
```bash
# 1. 转换MP3为GSM
python mp3_to_gsm.py new_audio.mp3

# 2. 转换GSM为C数组（使用之前创建的脚本）
python gsm_to_c_array.py new_audio.gsm

# 3. 备份原文件（可选）
copy src\gsm_audio_data.h src\gsm_audio_data.h.old

# 4. 复制新的数据文件
copy gsm_audio_data_new.c src\gsm_audio_data.h

# 5. 在Keil MDK中重新编译项目
```

## 输出示例

**输入**: `noAudio.mp3` (8928 bytes)
```
Duration: 00:00:02.23, bitrate: 32 kb/s
Audio: mp3float, 16000 Hz, mono
```

**输出**: `noAudio.gsm` (3700 bytes)
```
File size from header: 3696 bytes
First 16 bytes: 0xD8 0x20 0xA2 0xE1 0x5A 0x50 0x00 ...
Total file size: 3700 bytes (including 4-byte header)
Frames: 112 (33 bytes per frame)
Audio: GSM, 8000 Hz, mono, 13 kbps
```

## 文件结构对比

**旧GSM文件（无文件头）**:
```
[GSM数据开始... 33字节/帧]
```

**新GSM文件（带头信息）**:
```
[文件大小: 4字节 uint32 LE] [GSM数据开始... 33字节/帧]
```

## 技术细节

### 转换流程
1. **解码**: ffmpeg自动检测并解码MP3
2. **重采样**: 转换为8000 Hz（如需要）
3. **降混音**: 转换为单声道（如需要）
4. **编码**: 使用libgsm编码为GSM 6.10格式
5. **添加头**: 在文件开头写入4字节文件大小信息

### 文件头格式
```c
// C语言读取示例
#include <stdio.h>
#include <stdint.h>

uint32_t read_gsm_size(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return 0;

    uint32_t size;
    fread(&size, sizeof(uint32_t), 1, fp);
    fclose(fp);

    return size;  // 小端序
}

// 使用示例
uint32_t gsm_size = read_gsm_size("audio.gsm");
uint8_t* gsm_data = (uint8_t*)malloc(gsm_size);
// 读取GSM数据...
```

## 注意事项

### 音质考虑
- **GSM限制**:
  - 采样率仅8kHz，适合语音通信
  - 比特率约13 kbps，音频质量较低
  - 有损压缩，不适合音乐存储
- **适用场景**:
  - 嵌入式系统语音播放
  - 通信系统音频传输
  - 存储空间受限的场景

### 文件大小
- MP3 → GSM转换后的文件大小约为原文件的40-50%（取决于原MP3比特率）
- GSM格式：约13 kbps
- 计算示例：2.23秒音频 → 约3.7KB GSM文件

### 编码延迟
- GSM编码延迟：约20ms（一帧）
- 适合实时通信场景

## 故障排除

### 错误1: ffmpeg not found
**症状**:
```
Error: ffmpeg not found! Please install ffmpeg first.
```

**解决方案**:
1. 确认ffmpeg已安装
2. 检查ffmpeg是否在PATH中：
   ```bash
   ffmpeg -version
   ```
3. 如果未安装，按照上面的"安装ffmpeg"章节进行安装

### 错误2: File 'xxx.mp3' not found
**症状**:
```
Error: File 'xxx.mp3' not found!
```

**解决方案**:
1. 检查文件路径是否正确
2. 使用绝对路径或确认当前工作目录
3. 确认文件扩展名是否为.mp3（区分大小写）

### 错误3: 音频质量问题
**症状**:
- 转换后音频质量差
- 有杂音或失真

**原因**:
- GSM采样率只有8kHz
- GSM是有损压缩

**解决方案**:
- 提高原始MP3的比特率（推荐192kbps或更高）
- 确保原音频质量良好
- 考虑使用其他音频格式用于音乐存储

### 错误4: 编译后无声音
**症状**:
- 程序编译正常但播放无声音

**检查清单**:
1. 确认GSM数据已正确替换
2. 检查文件大小是否正确（main.c会自动计算帧数）
3. 检查GSM文件头是否正确（4字节头+数据）
4. 验证音频数据是否损坏：
   ```bash
   # 使用调试输出检查解码结果
   # 检查帧数和解码输出
   ```

## 高级用法

### 自定义转换参数
如需修改编码参数，编辑脚本中的ffmpeg命令：

```python
cmd = [
    'ffmpeg',
    '-y',
    '-i', mp3_file,
    '-ar', '8000',      # 采样率 (可修改)
    '-ac', '1',         # 声道数
    '-b:a', '13k',      # 比特率 (可调整)
    '-f', 'gsm',
    '-acodec', 'libgsm',
    temp_gsm
]
```

### 集成到构建系统

**Makefile示例**:
```makefile
# MP3 to GSM conversion rule
%.gsm: %.mp3
	python mp3_to_gsm.py $<

.PHONY: convert-all
convert-all:
	for file in *.mp3; do \
		python mp3_to_gsm.py "$$file"; \
	done
```

**CMake示例**:
```cmake
# 添加自定义目标
add_custom_target(convert_mp3 ALL
    COMMAND python ${CMAKE_SOURCE_DIR}/mp3_to_gsm.py ${AUDIO_FILE}
    COMMENT "Convert MP3 to GSM format"
)
```

## 版本历史
- **v1.0** (2026-02-04): 初始版本
  - MP3 to GSM转换
  - 自动添加文件头
  - 支持批量处理
  - 完整使用文档

## 相关工具
- `gsm_to_c_array.py`: GSM转C数组工具
- `compare_gsm_decode.py`: GSM解码器对比工具
- `replace_gsm_data.py`: GSM数据替换工具

## 技术支持
如有问题，请检查：
1. Python和ffmpeg版本
2. 文件路径和权限
3. 原始MP3文件的完整性

更多信息，参考：
- GSM 6.10标准文档
- ffmpeg官方文档
- 项目主README.md
