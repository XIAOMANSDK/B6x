#!/usr/bin/env python3
"""
Replace gsm_audio_data.h with new converted data
"""

import os
import sys

def create_gsm_header_file(gsm_bin_file, output_h_file):
    """Create new gsm_audio_data.h from binary GSM file"""

    # Read binary GSM data
    with open(gsm_bin_file, 'rb') as f:
        data = f.read()

    file_size = len(data)
    num_frames = file_size // 33

    print(f'Converting {gsm_bin_file} to {output_h_file}')
    print(f'  Size: {file_size} bytes')
    print(f'  Frames: {num_frames}')

    # Write new header file
    with open(output_h_file, 'w') as f:
        f.write('/* GSM audio data - converted from noAudio.mp3 */\n')
        f.write(f'/* Total size: {file_size} bytes */\n')
        f.write(f'/* Frames: {num_frames} (33 bytes per frame) */\n')
        f.write(f'/* Format: GSM 6.10, 8000 Hz, Mono */\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'const uint8_t gsm_audio_data[{file_size}] = {{\n')

        # Write data in hex format (16 bytes per line)
        for i in range(0, file_size, 16):
            line_bytes = data[i:i+16]
            hex_values = [f'0x{b:02X}' for b in line_bytes]
            f.write('    ' + ', '.join(hex_values))

            # Add comma if not last line
            if i + 16 < file_size:
                f.write(',')

            f.write('\n')

        f.write('};\n')

    print(f'Created: {output_h_file}')
    return file_size

def main():
    gsm_bin = 'D:\\svn\\bxx_DragonC1\\sdk6\\examples\\GSMDoceder\\noAudio.gsm'
    output_h = 'D:\\svn\\bxx_DragonC1\\sdk6\\examples\\GSMDoceder\\src\\gsm_audio_data.h'
    backup_h = 'D:\\svn\\bxx_DragonC1\\sdk6\\examples\\GSMDoceder\\src\\gsm_audio_data.h.backup'

    # Backup original file
    if os.path.exists(output_h):
        import shutil
        shutil.copy2(output_h, backup_h)
        print(f'Backed up original file to: {backup_h}')

    # Create new header file
    file_size = create_gsm_header_file(gsm_bin, output_h)
    num_frames = file_size // 33

    print(f'\n========================================')
    print(f'Success! GSM audio data replaced.')
    print(f'Old size: 53229 bytes (1613 frames)')
    print(f'New size: {file_size} bytes ({num_frames} frames)')
    print(f'========================================')
    print(f'\nNext steps:')
    print(f'1. Rebuild project in Keil MDK')
    print(f'2. Flash and test the new audio')

if __name__ == '__main__':
    main()
