/**
 ****************************************************************************************
 *
 * @file CRCxx.c
 *
 * @brief Common CRC bitwise operation functions.
 *        could verify at http://www.ip33.com/crc.html
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include "CRCxx.h"


/******************************************************************************
* Name:    CRC-4/ITU
* Poly:    0x03  ( x4+x+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc4_itu(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x0C; // 0x0C = (reverse 0x03)>>(8-4)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-5/EPC
* Poly:    0x09  ( x5+x3+1 )
* Init:    0x09
* Refin:   False
* Refout:  False
* Xorout:  0x00
*****************************************************************************/
uint8_t crc5_epc(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0x48; // Initial value: 0x48 = 0x09<<(8-5)
    while (length--)
    {
        crc ^= *data++;
        for ( i = 0; i < 8; i++ )
        {
            if ( crc & 0x80 )
                crc = (crc << 1) ^ 0x48; // 0x48 = 0x09<<(8-5)
            else
                crc <<= 1;
        }
    }
    return crc >> 3;
}

/******************************************************************************
* Name:    CRC-5/ITU
* Poly:    0x15  ( x5+x4+x2+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc5_itu(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++; 
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x15; // 0x15 = (reverse 0x15)>>(8-5)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-5/USB
* Poly:    0x05  ( x5+x2+1 )
* Init:    0x1F
* Refin:   True
* Refout:  True
* Xorout:  0x1F
*****************************************************************************/
uint8_t crc5_usb(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0x1F; // Initial value
    while (length--)
    {
        crc ^= *data++; 
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x14; // 0x14 = (reverse 0x05)>>(8-5)
            else
                crc = (crc >> 1);
        }
    }
    return crc ^ 0x1F;
}

/******************************************************************************
* Name:    CRC-6/ITU
* Poly:    0x03  ( x6+x+1 )
* Init:    0x00
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc6_itu(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x30; // 0x30 = (reverse 0x03)>>(8-6)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-7/MMC
* Poly:    0x09  ( x7+x3+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x00
* Use:     MultiMediaCard,SD,ect.
*****************************************************************************/
uint8_t crc7_mmc(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for ( i = 0; i < 8; i++ )
        {
            if ( crc & 0x80 )
                crc = (crc << 1) ^ 0x12; // 0x12 = 0x09<<(8-7)
            else
                crc <<= 1;
        }
    }
    return crc >> 1;
}


/******************************************************************************
* Name:    CRC-8
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x00
*****************************************************************************/
uint8_t crc8(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for ( i = 0; i < 8; i++ )
        {
            if ( crc & 0x80 )
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-8/ITU
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0x00
* Refin:   False
* Refout:  False
* Xorout:  0x55
* Alias:   CRC-8/ATM
*****************************************************************************/
uint8_t crc8_itu(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for ( i = 0; i < 8; i++ )
        {
            if ( crc & 0x80 )
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc ^ 0x55;
}

/******************************************************************************
* Name:    CRC-8/ROHC
* Poly:    0x07  ( x8+x2+x+1 )
* Init:    0xFF
* Refin:   True
* Refout:  True
* Xorout:  0x00
*****************************************************************************/
uint8_t crc8_rohc(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0xFF; // Initial value
    while (length--)
    {
        crc ^= *data++; 
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xE0; // 0xE0 = reverse 0x07
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

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
uint8_t crc8_maxim(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8C; // 0x8C = reverse 0x31
            else
                crc >>= 1;
        }
    }
    return crc;
}


/******************************************************************************
* Name:    CRC-16
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0x0000
* Alias:   CRC-16/IBM,CRC-16/ARC,CRC-16/LHA
*****************************************************************************/
uint16_t crc16(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse 0x8005
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-16/MAXIM
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
*****************************************************************************/
uint16_t crc16_maxim(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++; 
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse 0x8005
            else
                crc = (crc >> 1);
        }
    }
    return ~crc; // crc^0xffff
}

/******************************************************************************
* Name:    CRC-16/USB
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
*****************************************************************************/
uint16_t crc16_usb(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0xffff; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse 0x8005
            else
                crc = (crc >> 1);
        }
    }
    return ~crc; // crc^0xffff
}

/******************************************************************************
* Name:    CRC-16/MODBUS
* Poly:    0x8005  ( x16+x15+x2+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0x0000
*****************************************************************************/
uint16_t crc16_modbus(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0xffff; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse 0x8005
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-16/CCITT
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0x0000
* Alias:   CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT
*****************************************************************************/
uint16_t crc16_ccitt(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408; // 0x8408 = reverse 0x1021
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-16/CCITT-FALSE
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0xFFFF
* Refin:   False
* Refout:  False
* Xorout:  0x0000
*****************************************************************************/
uint16_t crc16_ccitt_false(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0xffff; // Initial value
    while (length--)
    {
        crc ^= (uint16_t)(*data++) << 8;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x8000 )
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-16/X25
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0xFFFF
* Refin:   True
* Refout:  True
* Xorout:  0XFFFF
*****************************************************************************/
uint16_t crc16_x25(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0xffff; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408; // 0x8408 = reverse 0x1021
            else
                crc = (crc >> 1);
        }
    }
    return ~crc; // crc^Xorout
}

/******************************************************************************
* Name:    CRC-16/XMODEM
* Poly:    0x1021  ( x16+x12+x5+1 )
* Init:    0x0000
* Refin:   False
* Refout:  False
* Xorout:  0x0000
* Alias:   CRC-16/ZMODEM,CRC-16/ACORN
*****************************************************************************/
uint16_t crc16_xmodem(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= (uint16_t)(*data++) << 8;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x8000 )
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-16/DNP
* Poly:    0x3D65  ( x16+x13+x12+x11+x10+x8+x6+x5+x2+1 )
* Init:    0x0000
* Refin:   True
* Refout:  True
* Xorout:  0xFFFF
* Use:     M-Bus,ect.
*****************************************************************************/
uint16_t crc16_dnp(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint16_t crc = 0; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA6BC; // 0xA6BC = reverse 0x3D65
            else
                crc = (crc >> 1);
        }
    }
    return ~crc; // crc^Xorout
}


/******************************************************************************
* Name:    CRC-24
* Poly:    0x864CFB  ( x24+x23+x18+x17+x14+x11+x10+x7+x6+x5+x4+x3+x1+1 )
* Init:    0xB704CE
* Refin:   True
* Refout:  True
* Xorout:  0x000000
* Alias:   CRC-24/OPENPGP 
*****************************************************************************/
uint32_t crc24(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0x7320ED; // Initial value: 0x7320ED = revbit24(0xB704CE)
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xDF3261; // 0xDF3261 = revbit24(0x864CFB)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-24/BLE
* Poly:    0x00065B  ( x24+x10+x9+x6+x4+x3+x1+1 )
* Init:    0x555555
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_ble(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xAAAAAA; // Initial value: 0xAAAAAA = revbit24(0x555555)
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xDA6000; // 0xDA6000 = revbit24(0x00065b)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-24/FLEXRAY-A
* Poly:    0x5D6DCB  ( x24+x22+x20+x19+x18+x16+x14+x13+x11+x10+x8+x7+x6+x3+x1+1 )
* Init:    0xFEDCBA
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_flexraya(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0x5D3B7F; // Initial value: 0x5D3B7F = revbit24(0xFEDCBA)
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xD3B6BA; // 0xD3B6BA = revbit24(0x5D6DCB)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-24/FLEXRAY-B
* Poly:    0x5D6DCB  ( x24+x22+x20+x19+x18+x16+x14+x13+x11+x10+x8+x7+x6+x3+x1+1 )
* Init:    0xABCDEF
* Refin:   True
* Refout:  True
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_flexrayb(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xF7B3D5; // Initial value: 0xF7B3D5 = revbit24(0xABCDEF)
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xD3B6BA; // 0xD3B6BA = revbit24(0x5D6DCB)
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

/******************************************************************************
* Name:    CRC-24/LTE-A
* Poly:    0x864CFB  ( x24+x23+x18+x17+x14+x11+x10+x7+x6+x5+x4+x3+x1+1 )
* Init:    0x000000
* Refin:   False
* Refout:  False
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_lte_a(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0x000000; // Initial value
    while (length--)
    {
        crc ^= (uint32_t)(*data++) << 16;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x800000 )
                crc = (crc << 1) ^ 0x864CFB;
            else
                crc <<= 1;
        }
    }
    return (crc & 0xFFFFFF);
}

/******************************************************************************
* Name:    CRC-24/LTE-B
* Poly:    0x800063  ( x24+x23+x6+x5+x1+1 )
* Init:    0x000000
* Refin:   False
* Refout:  False
* Xorout:  0x000000
*****************************************************************************/
uint32_t crc24_lte_b(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0x000000; // Initial value
    while (length--)
    {
        crc ^= (uint32_t)(*data++) << 16;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x800000 )
                crc = (crc << 1) ^ 0x800063;
            else
                crc <<= 1;
        }
    }
    return (crc & 0xFFFFFF);
}

/******************************************************************************
* Name:    CRC-24/OS-9
* Poly:    0x800063  ( x24+x23+x6+x5+x1+1 )
* Init:    0xFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0xFFFFFF
*****************************************************************************/
uint32_t crc24_os9(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xFFFFFF; // Initial value
    while (length--)
    {
        crc ^= (uint32_t)(*data++) << 16;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x800000 )
                crc = (crc << 1) ^ 0x800063;
            else
                crc <<= 1;
        }
    }
    return (crc & 0xFFFFFF) ^ 0xFFFFFF;
}

/******************************************************************************
* Name:    CRC-24/INTERLAKEN
* Poly:    0x328B63  ( x24+x21+x20+x17+x15+x11+x9+x8+x6+x5+x1+1 )
* Init:    0xFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0xFFFFFF
*****************************************************************************/
uint32_t crc24_interlaken(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xFFFFFF; // Initial value
    while (length--)
    {
        crc ^= (uint32_t)(*data++) << 16;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x800000 )
                crc = (crc << 1) ^ 0x328B63;
            else
                crc <<= 1;
        }
    }
    return (crc & 0xFFFFFF) ^ 0xFFFFFF;
}


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
uint32_t crc32(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xffffffff; // Initial value
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320; // 0xEDB88320= reverse 0x04C11DB7
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}

/******************************************************************************
* Name:    CRC-32/MPEG-2
* Poly:    0x4C11DB7  ( x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 )
* Init:    0xFFFFFFF
* Refin:   False
* Refout:  False
* Xorout:  0x0000000
*****************************************************************************/
uint32_t crc32_mpeg2(uint8_t *data, ulen_t length)
{
    uint8_t i;
    uint32_t crc = 0xffffffff; // Initial value
    while(length--)
    {
        crc ^= (uint32_t)(*data++) << 24;
        for (i = 0; i < 8; ++i)
        {
            if ( crc & 0x80000000 )
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc <<= 1;
        }
    }
    return crc;
}
