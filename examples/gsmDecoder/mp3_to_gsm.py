#!/usr/bin/env python3
"""
Convert MP3 to GSM with file size header
Usage: mp3_to_gsm.py mp3_filename.mp3

Requirements:
- ffmpeg must be installed and in PATH
"""

import sys
import os
import subprocess
import struct

def convert_mp3_to_gsm(mp3_file):
    """
    Convert MP3 file to GSM format with file size header

    Args:
        mp3_file: Path to input MP3 file
    """

    # Check if file exists
    if not os.path.exists(mp3_file):
        print(f"Error: File '{mp3_file}' not found!")
        return 1

    # Check if ffmpeg is available
    try:
        subprocess.run(['ffmpeg', '-version'],
                       capture_output=True,
                       check=True,
                       creationflags=subprocess.CREATE_NO_WINDOW)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: ffmpeg not found! Please install ffmpeg first.")
        print("Download from: https://ffmpeg.org/download.html")
        return 1

    # Generate output filename
    base_name = os.path.splitext(mp3_file)[0]
    gsm_file = base_name + '.gsm'

    # Temporary GSM file (without header)
    temp_gsm = base_name + '_temp.gsm'

    print(f"Converting: {mp3_file}")
    print(f"        -> {gsm_file}")
    print()

    # Step 1: Convert MP3 to GSM using ffmpeg
    print("Step 1: Encoding to GSM format...")
    try:
        # Convert: mp3 -> 8kHz mono -> GSM
        cmd = [
            'ffmpeg',
            '-y',  # Overwrite output files
            '-i', mp3_file,  # Input file
            '-ar', '8000',  # Audio sample rate: 8kHz
            '-ac', '1',  # Audio channels: mono
            '-f', 'gsm',  # Output format: GSM
            '-acodec', 'libgsm',  # GSM codec
            temp_gsm  # Output file
        ]

        result = subprocess.run(cmd,
                              capture_output=True,
                              text=True,
                              creationflags=subprocess.CREATE_NO_WINDOW)

        if result.returncode != 0:
            print(f"Error during ffmpeg conversion:")
            print(result.stderr)
            return 1

    except Exception as e:
        print(f"Error running ffmpeg: {e}")
        return 1

    # Step 2: Read GSM file and get size
    print("Step 2: Adding file size header...")

    try:
        with open(temp_gsm, 'rb') as f:
            gsm_data = f.read()

        file_size = len(gsm_data)

        print(f"  GSM data size: {file_size} bytes")
        print(f"  Frames: {file_size // 33} (33 bytes per frame)")

        # Step 3: Write new GSM file with size header
        with open(gsm_file, 'wb') as f:
            # Write file size as uint32 little-endian at the beginning
            f.write(struct.pack('<I', file_size))
            # Write original GSM data
            f.write(gsm_data)

        print(f"  Final file size: {file_size + 4} bytes (including 4-byte header)")
        print(f"\nSuccess! Created: {gsm_file}")

    except Exception as e:
        print(f"Error processing GSM data: {e}")
        return 1
    finally:
        # Clean up temporary file
        if os.path.exists(temp_gsm):
            try:
                os.remove(temp_gsm)
                print(f"Cleaned up temporary file: {temp_gsm}")
            except:
                pass

    return 0

def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("=" * 60)
        print("MP3 to GSM Converter with File Size Header")
        print("=" * 60)
        print()
        print("Usage: python mp3_to_gsm.py <mp3_file>")
        print()
        print("Example:")
        print("  python mp3_to_gsm.py audio.mp3")
        print("  Output: audio.gsm")
        print()
        print("Requirements:")
        print("  - ffmpeg must be installed and in PATH")
        print("  - Python 3.x")
        print("=" * 60)
        return 1

    mp3_file = sys.argv[1]

    # Convert mp3 to gsm
    return convert_mp3_to_gsm(mp3_file)

if __name__ == "__main__":
    sys.exit(main())
