# GSM 6.10 Decoder Implementation Summary

## Overview
This project implements a GSM 6.10 audio decoder for the B6x chip (ARM Cortex-M0+). The decoder converts GSM 6.10 compressed audio data (33 bytes/frame) to PCM audio samples (160 samples/frame at 8kHz).

## Key Features
- **Static memory allocation** - No dynamic malloc/free, suitable for embedded systems
- **Decoder-only implementation** - Encoding functionality removed to save code space
- **Optimized stack/heap** - Stack: 2048 bytes, Heap: 2048 bytes (with ~30% margin)
- **Integrated test audio** - Complete GSM audio file converted to static array

## File Structure

### Core Decoder Files
- `src/gsm.h` - Main decoder interface and state structure definition
- `src/gsm.c` - Decoder implementation with `gsm_decode_frame()` function

### GSM Library (gsm-1.0-pl23/)
- `src/gsm_create.c` - Static initialization (modified from malloc)
- `src/gsm_destroy.c` - Cleanup function (modified from free)
- `src/gsm_decode.c` - Core GSM 6.10 decoding algorithm
- `inc/gsm.h` - Library header file
- `inc/private.h` - Internal definitions
- `inc/config.h` - Configuration (fixed nested comments)

### Audio Data
- `gsm_audio_data.h` - Static array containing 53,229 bytes of GSM audio data
  - Total frames: 1,613
  - Frame size: 33 bytes
  - Output: 258,080 PCM samples (1,613 × 160)

### Application
- `src/main.c` - Main application that tests the decoder with audio data
  - Initializes GSM decoder state
  - Decodes all frames sequentially
  - Prints first 10 PCM samples from each frame for verification

## Memory Configuration

### Stack Size
- **Calculated requirement**: ~1,500 bytes minimum for GSM decoder
- **Configured size**: 2,048 bytes (0x800)
- **Margin**: ~37% above minimum

### Heap Size
- **Configured size**: 2,048 bytes (0x800)
- **Note**: Heap reduced from 18KB since GSM uses static allocation

### Configuration Files
- `mdk/link_xip.sct:33` - RW_IRAM_STACK: 0x1000 (4096 bytes total for stack+heap)
- `src/startup.s:6` - Stack_Size: 0x800
- `src/startup.s:16` - Heap_Size: 0x800

## API Reference

### Main Decoder Function
```c
void gsm_decode_frame(gsm_state_t *S, uint8_t *c, int16_t *target)
```
**Parameters:**
- `S` - Pointer to GSM decoder state structure
- `c` - Pointer to 33-byte GSM compressed frame
- `target` - Output buffer for 160 PCM samples (int16_t)

### State Initialization
```c
// Method 1: Static initialization (recommended for embedded)
gsm_state_t decoder;
memset(&decoder, 0, sizeof(decoder));
decoder.nrp = 40;

// Method 2: Using library function
gsm decoder = gsm_create();  // Returns pointer to static state
```

## Usage Example

```c
#include "gsm.h"
#include "gsm_audio_data.h"

int main(void) {
    gsm_state_t decoder;
    int16_t pcm_output[160];

    // Initialize decoder
    memset(&decoder, 0, sizeof(decoder));
    decoder.nrp = 40;

    // Decode one frame
    gsm_decode_frame(&decoder, gsm_audio_data, pcm_output);

    // pcm_output now contains 160 PCM samples
    return 0;
}
```

## Technical Specifications

### GSM 6.10 Format
- **Compression**: 13:1 (from 16-bit PCM)
- **Frame size**: 33 bytes (264 bits)
- **Sample rate**: 8 kHz
- **Frame duration**: 20 ms
- **Bit rate**: 13 kbit/s

### Output Format
- **Samples per frame**: 160
- **Sample format**: 16-bit signed integer (int16_t)
- **Sample rate**: 8 kHz
- **Channels**: Mono

## Testing
The main application (`src/main.c`) automatically tests the decoder by:
1. Printing audio data statistics
2. Decoding all 1,613 frames
3. Printing first 10 PCM samples from each frame
4. Displaying completion statistics

## Compiler Notes
- **Compiler**: Keil MDK-ARM (ARMCC)
- **Target**: ARM Cortex-M0+
- **Optimization**: Recommended -O2 or -O3 for best performance

## Modifications from Original GSM Library
1. Removed all encoding functionality (code.c, preprocess.c, gsm_encode.c, etc.)
2. Replaced malloc/free with static allocation
3. Removed OS-dependent utilities (toast files)
4. Fixed nested comment warnings in config.h
5. Added embedded-friendly API with gsm_state_t type

## Memory Usage Summary
- **GSM state structure**: ~1,500 bytes on stack
- **PCM output buffer**: 320 bytes (160 × int16_t)
- **Code size**: ~20-30 KB (decoder only, no encoding)
- **Audio data**: 53 KB (stored in flash)

---
Generated: 2025-02-04
Target: B6x Chip (ARM Cortex-M0+)
Compiler: Keil MDK-ARM
