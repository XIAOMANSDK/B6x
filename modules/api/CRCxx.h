/**
 ****************************************************************************************
 *
 * @file CRCxx.h
 *
 * @brief Head File of Common CRC bitwise operation functions.
 *
 ****************************************************************************************
 */
#ifndef _CRCxx_H_
#define _CRCxx_H_

#include <stdint.h>

#ifndef ulen_t
#define ulen_t              uint16_t
#endif


/******************************************************************************
* Name:    CRC-4/ITU
* Poly:    0x03  ( x4+x+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc4_itu(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-5/EPC
* Poly:    0x09  ( x5+x3+1 )
* Init:    0x09
* Refin:   False
* Refout:  False
* Xorout:  0x00
*****************************************************************************/
uint8_t crc5_epc(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-5/ITU
* Poly:    0x15  ( x5+x4+x2+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc5_itu(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-5/USB
* Poly:    0x05  ( x5+x2+1 )
* Init:    0x1F
* Refin:   True
* Refout:  True
* Xorout:  0x1F
*****************************************************************************/
uint8_t crc5_usb(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-6/ITU
* Poly:    0x03  ( x6+x+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc6_itu(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-7/MMC
* Poly:    0x09  ( x7+x3+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x00
* Use:     MultiMediaCard,SD,ect.
*****************************************************************************/
uint8_t crc7_mmc(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-8
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x00
*****************************************************************************/
uint8_t crc8(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-8/ITU
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x55
* Alias:   CRC-8/ATM
*****************************************************************************/
uint8_t crc8_itu(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-8/ROHC
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0xFF
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc8_rohc(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-8/MAXIM
* Poly:    0x31  ( x8+x5+x4+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
* Alias:   DOW-CRC,CRC-8/IBUTTON
* Use:     Maxim(Dallas)'s some devices,e.g. DS18B20
*****************************************************************************/
uint8_t crc8_maxim(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0x0000
* Alias:   CRC-16/IBM,CRC-16/ARC,CRC-16/LHA
*****************************************************************************/
uint16_t crc16(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/MAXIM
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
*****************************************************************************/
uint16_t crc16_maxim(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/USB
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
*****************************************************************************/
uint16_t crc16_usb(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/MODBUS
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0x0000
*****************************************************************************/
uint16_t crc16_modbus(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/CCITT
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0x0000
* Alias:   CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT
*****************************************************************************/
uint16_t crc16_ccitt(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/CCITT-FALSE
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0xFFFF
* Refin:   False
* Refout:  False
* Xorout:  0x0000
*****************************************************************************/
uint16_t crc16_ccitt_false(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/X25
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0XFFFF
*****************************************************************************/
uint16_t crc16_x25(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/XMODEM
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0x0000
* Refin:   False
* Refout:  False
* Xorout:  0x0000
* Alias:   CRC-16/ZMODEM,CRC-16/ACORN
*****************************************************************************/
uint16_t crc16_xmodem(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-16/DNP
* Poly:    0x3D65  ( x16+x13+x12+x11+x10+x8+x6+x5+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
* Use:     M-Bus,ect.
*****************************************************************************/
uint16_t crc16_dnp(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24
* Poly:    0x864CFB  ( x24+x23+x18+x17+x14+x11+x10+x7+x6+x5+x4+x3+x1+1 )
* Init:    0xB704CE
* Refin:   True
* Refout:  True
* Xorout:  0x000000
* Alias:   CRC-24/OPENPGP 
*****************************************************************************/
uint32_t crc24(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/BLE
* Poly:    0x00065B  ( x24+x10+x9+x6+x4+x3+x1+1 )
* Init:    0x555555
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_ble(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/FLEXRAY-A
* Poly:    0x5D6DCB  ( x24+x22+x20+x19+x18+x16+x14+x13+x11+x10+x8+x7+x6+x3+x1+1 )
* Init:    0xFEDCBA
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_flexraya(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/FLEXRAY-B
* Poly:    0x5D6DCB  ( x24+x22+x20+x19+x18+x16+x14+x13+x11+x10+x8+x7+x6+x3+x1+1 )
* Init:    0xABCDEF
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_flexrayb(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/LTE-A
* Poly:    0x864CFB  ( x24+x23+x18+x17+x14+x11+x10+x7+x6+x5+x4+x3+x1+1 )
* Init:    0x000000
* Refin:   False
* Refout:  False
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_lte_a(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/LTE-B
* Poly:    0x800063  ( x24+x23+x6+x5+x1+1 )
* Init:    0x000000
* Refin:   False
* Refout:  False
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_lte_b(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/OS-9
* Poly:    0x800063  ( x24+x23+x6+x5+x1+1 )
* Init:    0xFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0xFFFFFF
*****************************************************************************/
uint32_t crc24_os9(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-24/INTERLAKEN
* Poly:    0x328B63  ( x24+x21+x20+x17+x15+x11+x9+x8+x6+x5+x1+1 )
* Init:    0xFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0xFFFFFF
*****************************************************************************/
uint32_t crc24_interlaken(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-32
* Poly:    0x4C11DB7  ( x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 )
* Init:    0xFFFFFFF
* Refin:   True
* Refout:  True
* Xorout:  0xFFFFFFF
* Alias:   CRC_32/ADCCP
* Use:     WinRAR,ect.
*****************************************************************************/
uint32_t crc32(uint8_t *data, ulen_t length);

/******************************************************************************
* Name:    CRC-32/MPEG-2
* Poly:    0x4C11DB7  ( x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 )
* Init:    0xFFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0x0000000
*****************************************************************************/
uint32_t crc32_mpeg2(uint8_t *data, ulen_t length);

#endif  // _CRCxx_H_
