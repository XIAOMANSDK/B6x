#ifndef _REG_FSHC_H_
#define _REG_FSHC_H_

#include "reg_base.h" 

//================================
//BLOCK FSHC define 

#define FSHC_BASE                               ((uint32_t)0x40006000) 
#define FSHC_SPDR_RD_ADDR_OFFSET                0x000 
#define FSHC_SPDR_WR_ADDR_OFFSET                0x004 
#define FSHC_SPCR_ADDR_OFFSET                   0x008 
#define FSHC_RXTX_DAT_LEN_ADDR_OFFSET           0x00c 
#define FSHC_CMD_REG_ADDR_OFFSET                0x010 
#define FSHC_ADDR_REG_ADDR_OFFSET               0x014 
#define FSHC_MCU_ADRCMD_BIT_LEN_ADDR_OFFSET     0x018 
#define FSHC_SEND_CTRL_ADDR_OFFSET              0x01c 
#define FSHC_SEND_EN_ADDR_OFFSET                0x020 
#define FSHC_FIFO_STATUS_ADDR_OFFSET            0x024 
#define FSHC_RXPTR_INC_EN_ADDR_OFFSET           0x028 
#define FSHC_ST_ADDR_OFFSET                     0x02c 
#define FSHC_CACHE_CMD1_ADDR_OFFSET             0x030 
#define FSHC_CACHE_CMD2_ADDR_OFFSET             0x034 
#define FSHC_CACHE_DELAY_SET_ADDR_OFFSET        0x038 
#define FSHC_CACHE_RESUME_DELAY_MAX_ADDR_OFFSET 0x03c 
#define FSHC_DELAY_SET_ADDR_OFFSET              0x040 
#define FSHC_BYPASS_HPM_ADDR_OFFSET             0x044 
#define FSHC_ATOM_OP_EN_ADDR_OFFSET             0x048 
#define FSHC_CLR_HPM_ADDR_OFFSET                0x04c 
#define FSHC_CONTINUE_RD_MOD_INDEX_ADDR_OFFSET  0x050 
#define FSHC_CONTINUE_STATUS_ADDR_OFFSET        0x054 
#define FSHC_DLY_CFG_ADDR_OFFSET                0x058 
#define FSHC_CACHE_ACCESS_STATUS_ADDR_OFFSET    0x05c 

//================================
//BLOCK FSHC reg struct define 
typedef union //0x008 
{
    struct
    {
        uint32_t CPOL:                               1; // bit0 ---
                                                        // cpol = 1,spi_clk sample data_in at negedge, 
                                                        // cpol = 0,spi_clk sample data_in at posedge, 
        uint32_t AUTO_CHECK_ST:                      1; // bit1 --- auto_check_st is 1:
                                                        // auto read status before send suspend cmd
                                                        // note:set must before flash_busy set as 1
        uint32_t FLASH_BUSY_SET:                     1; // bit2 --- When this bit is set 1, cache will read flash with suspend and resume mode
                                                        // For example: flash in erase status, cache want to read, you should set this bit first;
        uint32_t FLASH_BUSY_CLR:                     1; // bit3 --- when auto_check_st is 0,
                                                        // use to clear flash_busy
                                                        // when auto_check_st is 1,
                                                        // use to clear flash_st_done
        uint32_t FLASH_INT_EN:                       1; // bit4 --- when is 1
                                                        // flash int is set when flash_st_done is 1
        uint32_t RSV_END:                           27; // bit[31:5]
    };
    uint32_t Word;
} FSHC_SPCR_TypeDef; //0x008 


//================================
#define FSHC_CPOL_POS                       0
#define FSHC_AUTO_CHECK_ST_POS              1
#define FSHC_FLASH_BUSY_SET_POS             2
#define FSHC_FLASH_BUSY_CLR_POS             3
#define FSHC_FLASH_INT_EN_POS               4
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t MCU_ADR_MAX_LEN:                    6; // bit[5:0] --- MCU adr cycle length,include dummy cycle, max is 32 bit, only 24 bit is valid, others is zero;
                                                        // we use largest flash is 16MByte, so max address bit number is 24 bit;
                                                        // transmit address with spi mode, mcu_adr_max_len = (address bit number - 1) + dummy cycle
                                                        // transmit address with qpi mode, mcu_adr_max_len = ((address bit number)/4 - 1) + dummy cycle
        uint32_t MCU_CMD_MAX_LEN:                    6; // bit[11:6] --- MCU cmd cycle length,include dummy cycle, max is 32 bit, only 8 bit is vaild, others is zero;
                                                        // transmit command with spi mode, mcu_cmd_max_len = 7 + dummy cycle
                                                        // transmit command with qpi mode, mcu_cmd_max_len = 1 + dummy cycle
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} FSHC_MCU_ADRCMD_BIT_LEN_TypeDef; //0x018 


//================================
#define FSHC_MCU_ADR_MAX_LEN_LSB            0
#define FSHC_MCU_ADR_MAX_LEN_WIDTH          6
#define FSHC_MCU_CMD_MAX_LEN_LSB            6
#define FSHC_MCU_CMD_MAX_LEN_WIDTH          6
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t DATA_BIT_NUM:                       2; // bit[1:0] --- MCU transmit data bit number
                                                        // 0/1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mode
        uint32_t ADDR_BIT_NUM:                       2; // bit[3:2] --- MCU transmit address bit number
                                                        // 0/1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mode
        uint32_t CMD_BIT_NUM:                        2; // bit[5:4] --- MCU transmit command bit number
                                                        // For GD, only 1bit command is used. For EON, 4bits #command can be used
                                                        // 0/1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mode
        uint32_t ADR_WITH_ARG:                       2; // bit[7:6] --- MCU address argument
                                                        // 2'b00/2'b11: address followed by idle
                                                        // 2'b01:       address followed by wdata
                                                        // 2'b10:       address followed by rdata
        uint32_t CMD_WITH_ARG:                       3; // bit[10:8]--- MCU command argument
                                                        // 3'b011: command followed by address
                                                        // 3'b101: command followed by wdata
                                                        // 3'b110: command followed by rdata
                                                        // others: command followed by idle
        uint32_t RSV_END:                           21; // bit[31:11]
    };
    uint32_t Word;
} FSHC_SEND_CTRL_TypeDef; //0x01c 


//================================
#define FSHC_DATA_BIT_NUM_LSB               0
#define FSHC_DATA_BIT_NUM_WIDTH             2
#define FSHC_ADDR_BIT_NUM_LSB               2
#define FSHC_ADDR_BIT_NUM_WIDTH             2
#define FSHC_CMD_BIT_NUM_LSB                4
#define FSHC_CMD_BIT_NUM_WIDTH              2
#define FSHC_ADR_WITH_ARG_LSB               6
#define FSHC_ADR_WITH_ARG_WIDTH             2
#define FSHC_CMD_WITH_ARG_LSB               8
#define FSHC_CMD_WITH_ARG_WIDTH             3
//================================

typedef union //0x024 
{
    struct
    {
        uint32_t RXFIFO_EMPTY:                       1; // bit0 --- MCU rxfifo empty flag,when '1',fifo is empty,can't read;
                                                        // Additional one AHB cycle are needed to read this register bit versus a standard AHB read
        uint32_t RXFIFO_FULL:                        1; // bit1 --- MCU rxfifo full flag,when '1',fifo is full,can continue read data from rxfifo 
        uint32_t TXFIFO_EMPTY:                       1; // bit2 --- MCU txfifo empty flag,when '1', fifo is empty,can continue write data to txfifo;
        uint32_t TXFIFO_FULL:                        1; // bit3 --- MCU txfifo full flag,when '1',fifo is full,can't write data to the fifo;
                                                        // Additional one AHB cycle are needed to read this register bit versus a standard AHB read
        uint32_t RSV_END:                           28; // bit[31:4]
    };
    uint32_t Word;
} FSHC_FIFO_STATUS_TypeDef; //0x024 


//================================
#define FSHC_RXFIFO_EMPTY_POS               0
#define FSHC_RXFIFO_FULL_POS                1
#define FSHC_TXFIFO_EMPTY_POS               2
#define FSHC_TXFIFO_FULL_POS                3
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t CMD_SEND_STATUS:                    1; // bit0 --- MCU
                                                        // 1: mcu request is not ack, can't configurate new command
                                                        // 0: allow configure new command
                                                        // Additional 3 AHB cycles are needed to read this register bit versus a standard AHB read
        uint32_t FLASH_BUSY:                         1; // bit1 --- set by flash_busy_set
                                                        // when auto_check_st is 0:
                                                        // cleared by flash_busy_clr
                                                        // when auto_check_st is 0:
                                                        // cleared by hardware flash_st_done
        uint32_t FLASH_ST_DONE:                      1; // bit2 --- only effect when auto_check_st is 1,
                                                        // cleared by flash_busy_clr 
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} FSHC_ST_TypeDef; //0x02c 


//================================
#define FSHC_CMD_SEND_STATUS_POS            0
#define FSHC_FLASH_BUSY_POS                 1
#define FSHC_FLASH_ST_DONE_POS              2
//================================

typedef union //0x030 
{
    struct
    {
        uint32_t READ_CMD:                           8; // bit[7:0]  --- Read command index,EBH,0BH etc
        uint32_t SUS_CMD:                            8; // bit[15:8] --- suspend command index, 75H
        uint32_t RESUME_CMD:                         8; // bit[23:16]--- resume command index,7AH
        uint32_t HPM_CMD:                            8; // bit[31:24]--- high performance mode command index,A3H
    };
    uint32_t Word;
} FSHC_CACHE_CMD1_TypeDef; //0x030 


//================================
#define FSHC_READ_CMD_LSB                   0
#define FSHC_READ_CMD_WIDTH                 8
#define FSHC_SUS_CMD_LSB                    8
#define FSHC_SUS_CMD_WIDTH                  8
#define FSHC_RESUME_CMD_LSB                 16
#define FSHC_RESUME_CMD_WIDTH               8
#define FSHC_HPM_CMD_LSB                    24
#define FSHC_HPM_CMD_WIDTH                  8
//================================

typedef union //0x034 
{
    struct
    {
        uint32_t EXITHPM_CMD:                        8; // bit[7:0]  --- exit high performance mode command index, 06H
        uint32_t CONTINU_RD_MOD:                     8; // bit[15:8] --- continuous read mode command, continu_rd_mod will be sent follow by address;
                                                        // Notice: continu_rd_mod and continue_rd_mod_index control flash in continuous read mode.
                                                        // Look at the continue_rd_mod_index for details.
        uint32_t RD_STATUS_CMD:                      8; // bit[23:16]--- read status cmd
                                                        // only used when both auto_check_st and flash_busy is 1
        uint32_t RD_STATUS_MASK:                     8; // bit[31:24]
    };
    uint32_t Word;
} FSHC_CACHE_CMD2_TypeDef; //0x034 


//================================
#define FSHC_EXITHPM_CMD_LSB                0
#define FSHC_EXITHPM_CMD_WIDTH              8
#define FSHC_CONTINU_RD_MOD_LSB             8
#define FSHC_CONTINU_RD_MOD_WIDTH           8
#define FSHC_RD_STATUS_CMD_LSB              16
#define FSHC_RD_STATUS_CMD_WIDTH            8
#define FSHC_RD_STATUS_MASK_LSB             24
#define FSHC_RD_STATUS_MASK_WIDTH           8
//================================

typedef union //0x038 
{
    struct
    {
        uint32_t ADR_BIT_MAX:                        5; // bit[4:0]  --- Only use in cache read mode, other cache cmd don't have address.
                                                        // Cache address cycle length. adr_bit_max = address length cycle - 1 + dummy cycle.
                                                        // For example: In GD flash Quad I/O Fast Read, want address 8 cycles + dummy 4 cycles, so adr_bit_max = (8-1+4) = 11 (Decimal)
                                                        // For example: In GD flash SPI Fast Read (0BH), want address 24 cycles + dummy 8 cycles, so adr_bit_max = (24-1+8) = 31 (Decimal)
        uint32_t SUS_DELAY_MAX:                     16; // bit[20:5] --- Set the delay time after send hpm command,default is GigaDevice flash value(0.2us) 
        uint32_t HPM_DELAY_MAX:                      6; // bit[26:21]--- The value of hpm_delay_max can not be 2+5n,(n=0,1,2,...) like 22,27,32,37.
                                                        // Best valus is 4+5n. like 24,29,34,39. Here 22,27,32,37 is decimal number.
                                                        // Set the delay time after sumpend command, default is GigaDevice flash value(2us)  
        uint32_t RSV_END:                            5; // bit[31:27]
    };
    uint32_t Word;
} FSHC_CACHE_DELAY_SET_TypeDef; //0x038 


//================================
#define FSHC_ADR_BIT_MAX_LSB                0
#define FSHC_ADR_BIT_MAX_WIDTH              5
#define FSHC_SUS_DELAY_MAX_LSB              5
#define FSHC_SUS_DELAY_MAX_WIDTH            16
#define FSHC_HPM_DELAY_MAX_LSB              21
#define FSHC_HPM_DELAY_MAX_WIDTH            6
//================================

typedef union //0x040 
{
    struct
    {
        uint32_t HPM_LEN:                            6; // bit[5:0]  --- hpm command length, default is 6'h1f, 
                                                        // on quad command mode      -------- hpm_len = (length of hpm_command)/4 - 1 + dummy cycle,
                                                        // on spi command mode       -------- hpm_len = (length of hpm_command) + dummy cycle - 1
                                                        // cache_no_dummy_cmd_len/cache_cmd_len/hpm_len distinguish
                                                        // -------------------------------------------------------#
                                                        // cmd               cmd length register
                                                        // hpm               hpm_len
                                                        // qpi read          cache_cmd_len
                                                        // suspend           cache_no_dummy_cmd_len
                                                        // resume            cache_no_dummy_cmd_len
                                                        // exit_hpm          cache_no_dummy_cmd_len
                                                        // rst_continue      cache_no_dummy_cmd_len
                                                        // set continue      include in adr_bit_max
        uint32_t CACHE_CMD_LEN:                      4; // bit[9:6]  --- only use in qpi read cmd, default is 4'h7,
                                                        // on quad command mode      ----- cache_cmd_len = 1 + dummy cycle,
                                                        // on spi command mode       ----- cache_cmd_len = 7 + dummy cycle
        uint32_t CACHE_NO_DUMMY_CMD_LEN:             4; // bit[13:10]--- use in suspend/resume/exit_hpm cmd, default is 4'h7,
                                                        // on quad command mode      -------- cache_no_dummy_cmd_len = (length of command)/4 - 1,
                                                        // on spi command mode       -------- cache_no_dummy_cmd_len = (length of command) - 1.
        uint32_t CACHE_ADDR_BIT_NUM:                 2; // bit[15:14]--- CACHE transmit address bit number
                                                        // 0/1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mode
        uint32_t CACHE_CMD_BIT_NUM:                  2; // bit[17:16]--- CACHE transmit command bit number
                                                        // /1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mode
        uint32_t CACHE_DATA_BIT_NUM:                 2; // bit[19:18]--- CACHE transmit data bit number
                                                        // 0/1: 1 bit spi mode, 2: 2bits dual mode, 3: 4bits quad mod
        uint32_t RSV_END:                           12; // bit[31:20] 
    };
    uint32_t Word;
} FSHC_DELAY_SET_TypeDef; //0x040 


//================================
#define FSHC_HPM_LEN_LSB                    0
#define FSHC_HPM_LEN_WIDTH                  6
#define FSHC_CACHE_CMD_LEN_LSB              6
#define FSHC_CACHE_CMD_LEN_WIDTH            4
#define FSHC_CACHE_NO_DUMMY_CMD_LEN_LSB     10
#define FSHC_CACHE_NO_DUMMY_CMD_LEN_WIDTH   4
#define FSHC_CACHE_ADDR_BIT_NUM_LSB         14
#define FSHC_CACHE_ADDR_BIT_NUM_WIDTH       2
#define FSHC_CACHE_CMD_BIT_NUM_LSB          16
#define FSHC_CACHE_CMD_BIT_NUM_WIDTH        2
#define FSHC_CACHE_DATA_BIT_NUM_LSB         18
#define FSHC_CACHE_DATA_BIT_NUM_WIDTH       2
//================================

//================================
//BLOCK FSHC top struct define 
typedef struct
{
    __I   uint32_t                               SPDR_RD               ; // 0x000, 
                                                                         // mcu read rx fifo data via this register;
                                                                         // read this register, should set rxptr_inc_en first;
                                                                         // rxfifo_empty in fifo_status register, 
                                                                         // if rxfifo_empty is 1, means rxfifo is empty, mcu can't read 
    __O   uint32_t                               SPDR_WR               ; // 0x004, 
                                                                         // mcu write tx fifo data via this register;
                                                                         // txfifo_full in fifo_status register, 
                                                                         // if txfifo_full is 1, means txfifo is full, mcu can't write; 
    __IO  FSHC_SPCR_TypeDef                      SPCR                  ; // 0x008,  
    __IO  uint32_t                               RXTX_DAT_LEN          ; // 0x00c, 
                                                                         // set must before flash_busy set as 1 
    __IO  uint32_t                               CMD_REG               ; // 0x010, 
                                                                         // MCU access flash command. 
    __IO  uint32_t                               ADDR_REG              ; // 0x014, 
                                                                         // MCU access flash start address,byte address 
    __IO  FSHC_MCU_ADRCMD_BIT_LEN_TypeDef        MCU_ADRCMD_BIT_LEN    ; // 0x018,  
    __IO  FSHC_SEND_CTRL_TypeDef                 SEND_CTRL             ; // 0x01c,  
    __O   uint32_t                               SEND_EN               ; // 0x020, 
                                                                         // MCU start transmit,write 1 to start transmit command/address/data, 
                                                                         // hardware will check the posedge of the signal 
    __I   FSHC_FIFO_STATUS_TypeDef               FIFO_STATUS           ; // 0x024,  
    __IO  uint32_t                               RXPTR_INC_EN          ; // 0x028, 
                                                                         // MCU read rxfifo enable
                                                                         // 1 ------ rx fifo rxptr increment enable
                                                                         // 0 ------ rx fifo rxptr increment disable 
    __I   FSHC_ST_TypeDef                        ST                    ; // 0x02c,  
    __IO  FSHC_CACHE_CMD1_TypeDef                CACHE_CMD1            ; // 0x030, 
                                                                         // cache command, Gigadevice command is supported in default
                                                                         // cache access flash only use qpi_read/suspend/resume/hpm/exithpm/rst_continue/continu_rd_mod cmd;
                                                                         // in these cmd, rst_continue is fixed 8'hFF, others can configure. 
    __IO  FSHC_CACHE_CMD2_TypeDef                CACHE_CMD2            ; // 0x034, 
                                                                         // cache command, Gigadevice command is supported in default
                                                                         // cache access flash only use qpi_read/suspend/resume/hpm/exithpm/rst_continue/continu_rd_mod cmd;
                                                                         // in these cmd, rst_continue is fixed 8'hFF, others can configure. 
    __IO  FSHC_CACHE_DELAY_SET_TypeDef           CACHE_DELAY_SET       ; // 0x038,  
    __IO  uint32_t                               CACHE_RESUME_DELAY_MAX; // 0x03c, 
                                                                         // resume delay time.
                                                                         // This register control the interval time of two read when erase flash.
                                                                         // delay time: (1/F(flash))*resume_delay_max. 
    __IO  FSHC_DELAY_SET_TypeDef                 DELAY_SET             ; // 0x040,  
    __IO  uint32_t                               BYPASS_HPM            ; // 0x044, 
                                                                         // 0----enable cache hpm  
                                                                         // 1----disable cache hpm  
    __IO  uint32_t                               ATOM_OP_EN            ; // 0x048, 
                                                                         // Set this bit, fshc won't ack cache request and cache request will be pended.
                                                                         // Note: 1. if cache is reading in hpm mode or continue mode, MCU should exit hpm or rst continue first before accessing flash
                                                                         // 2. The system will die when set this bit to 1 and CPU fetch instruct or data, because fshc won't ack cache request 
    __O   uint32_t                               CLR_HPM               ; // 0x04c, 
                                                                         // Set this bit to 1 before MCU send command when cache is working in hpm, and flash clock will be switched.
                                                                         // For example: when cache reads flash in hpm, MCU want to access flash. In such case, MCU should set atom_op_en,
                                                                         // then set this bit, then send exit hpm cmd. 
    __IO  uint32_t                               CONTINUE_RD_MOD_INDEX ; // 0x050, 
                                                                         // continue_rd_mod valid indication
                                                                         // if you want to enter continuous read mode
                                                                         // 1.set continue_rd_mod with enter continue read mode value;
                                                                         // 2.set continue_rd_mod == continue_rd_mod_index.
                                                                         // both satisfy two condition, flash will entrance continuous read mode.
                                                                         // if you don't want to enter continuous read mode
                                                                         // 1.set continue_rd_mod with exit continue read mode value;
                                                                         // 2.set continue_rd_mod != continue_rd_mod_index.
                                                                         // both satisfy two condition, flash won't entrance continuous read mode.
                                                                         // gigedevice ----- enter continue read mode value is 8'hAX, ex:8'ha0;
                                                                         //                  exit continue read mode value is other value except 8'hAX, ex:8'h00,8'h55,8'hff;
                                                                         // MACRONIX   ----- enter performance enhance mode value is bit7 != bit3, bit6 != bit2, bit5 != bit1, bit4 != bit0, ex:8'ha5,8'h5a,8'h0f;
                                                                         //                  exit performance enhance value mode bit7 = bit3, bit6 = bit2, bit5 = bit1, bit4 = bit0, ex: 8'haa,8'h00,8'hff;
                                                                         // Winbond    ----- enter continue read mode index,{2'bxx,2'b10,4'bxxxx},the value of continu_rd_mod = {2'bxx,2'b10,4'bxxxx} if want to use continue read mode, 
                                                                         // SST        ----- don't have continuous read mode, so the value of continu_rd_mod != continue_rd_mod_index.
                                                                         // EoN        ----- enter performance enhance mode value bit7 != bit3, bit6 != bit2, bit5 != bit1, bit4 != bit0, ex:8'ha5,8'h5a,8'h0f;
                                                                         //                  exit performance enhance value bit7 = bit3, bit6 = bit2, bit5 = bit1, bit4 = bit0, ex: 8'haa,8'h00,8'hff. 
    __I   uint32_t                               CONTINUE_STATUS       ; // 0x054, 
                                                                         // 1 ------ flash be in continue read mode for cache path. 
    __IO  uint32_t                               DLY_CFG               ; // 0x058, 
                                                                         // used delay cell number
                                                                         // 0 - used 1 delay cell; 1 - used 2 delay cell; ... 7 - used 8 delay cell; 
    __I   uint32_t                               CACHE_ACCESS_STATUS   ; // 0x05c, 
                                                                         // This bit reflect cache access flash status. use for switching from cache to mcu access flash.
                                                                         // 1: cache bus is busy, mcu not to break.
                                                                         // 0: only when is 0, MCU can send cmd to access flash. 
} FSHC_TypeDef;


#define FSHC  (( FSHC_TypeDef  *)     FSHC_BASE)

#endif
