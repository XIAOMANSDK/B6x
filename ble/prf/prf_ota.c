/**
 ****************************************************************************************
 *
 * @file prf_ota.c
 *
 * @brief OTA Service - Server Role Implementation.
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (PRF_OTA)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "regs.h"
#include "drvs.h"
#include "bledef.h"
#include "app.h"
#include "prf.h"
#include "prf_ota.h"

#if (DBG_OTA)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#define NEW_LINE           32
#define debugDump(info, dat, len)                                                                  \
    do {                                                                                           \
        debug("<%s,%d>%s(%d):", __MODULE__, __LINE__, info, (len));                                \
        if (((len) > NEW_LINE) || ((len) == 0))                                                    \
            debug("\r\n");                                                                         \
        for (int _i = 0; _i < (len); _i++)                                                         \
        {                                                                                          \
            debug("%02X", *((dat) + _i));                                                          \
            if (((_i + 1) % NEW_LINE == 0) && (_i > 0))                                            \
                debug("\r\n");                                                                     \
        }                                                                                          \
        if ((len % NEW_LINE) != 0)                                                                 \
            debug("\r\n");                                                                         \
    } while (0)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#define debugDump(info, dat, len)
#endif

/*
 * DEFINITIONS
 ****************************************************************************************
 */
/// examples\loader
/// 使用loader模式时bankB只存储, 只需要更新bankA, 复位后通过loader搬到bankA.
/// 不使用loader模式时, 当前运行在bankA时需要更新bankB, 反之当前运行bankB时需要使用bankA.
#ifndef CFG_USE_LOAD
#define CFG_USE_LOAD (1)
#endif

/// Max length of received once
#ifndef OTA_RECV_MAX_LEN
#define OTA_RECV_MAX_LEN (BLE_MTU - 3)
#endif

#define OTA_SRAMFN(label) __attribute__((section("ram_func.prf_ota." #label)))

#ifndef FLASH_BASE
#define FLASH_BASE (0x18000000)
#endif

#define ALIGN(sz, align) (((sz) + (align) - 1) & ~((align) - 1))

#define FLASH_SIZE           (0x40000UL) // 256KB
#define FLASH_B64K_SIZE      (0x10000UL) // 64KB
#define FLASH_PAGE_SIZE      (0x100UL)   // 256B
#define FLASH_SECTOR_SIZE    (0x1000UL)  // 4KB
#define FLASH_PAGE_SIZE_WLEN (FLASH_PAGE_SIZE >> 2)

#define FLASH_INFO_PAGE      (0x00)
#define FLASH_INFO_MAGIC     (FLASH_BASE + 0x00)
#define FLASH_INFO_CODE_LEN  (FLASH_BASE + 0x04)
#define FLASH_INFO_CODE_ADDR (FLASH_BASE + 0x08)

#define OTA_BANK_A_BASE (0x18004000)
#define OTA_BANK_B_BASE (0x18020000)
#if (CFG_USE_LOAD)
// same with examples\loader "LDR_INFO_ADDR"
#define LOADER_INFO_BASE (0x18001F00)
#endif

#define OTA_BANK_A         (OTA_BANK_A_BASE - FLASH_BASE)
#define OTA_BANK_B         (OTA_BANK_B_BASE - FLASH_BASE)

// 1B(Head) + 1B(CMD) + 2B(IDX) + 2B(Len) + nB(data)
#define OTA_PKT_LEN (OTA_RECV_MAX_LEN)

/// 最大允许接收多少数据后写入flash
/// 更大的接收缓存, 速度会更快
#define OTA_BUFF_LEN (8 * FLASH_PAGE_SIZE)
#define OTA_DATA_POS (6)

#if ((OTA_BUFF_LEN == 0) || (OTA_BUFF_LEN % FLASH_PAGE_SIZE))
#error "ERROR, Receive Buff NOT PAGE ALIGN."
#endif

enum ota_flag
{
    FLAG_OTA_NTF  = 0x01,
    FLAG_OTA_END  = 0x02,
    FLAG_OTA_PROC = 0x04,
};

#define OTA_FLAG_SET(flag) (ota_env.ota_flag |= (flag))
#define OTA_FLAG_CLR(flag) (ota_env.ota_flag &= ~(flag))
#define OTA_FLAG_GET(flag) (ota_env.ota_flag & (flag))
#define OTA_FLAG_GET_ALL() (ota_env.ota_flag)
#define OTA_FLAG_CLR_ALL() (ota_env.ota_flag = 0)

/// Macro for Client Config value operation
#define OTA_NTF_CFG_GET(conidx) OTA_FLAG_GET(FLAG_OTA_NTF)
#define OTA_NTF_CFG_CLR(conidx) OTA_FLAG_CLR(FLAG_OTA_NTF)
#define OTA_NTF_CFG_SET(conidx) OTA_FLAG_SET(FLAG_OTA_NTF)

/**
 ****************************************************************************************
 * @section ENVIRONMENT DEFINITION
 ****************************************************************************************
 */

/// Server Environment Variable
struct ota_env_tag
{
    uint32_t bank;
    uint32_t bin_len;
    uint32_t data_sum;
    uint16_t ota_flag;
    uint16_t start_hdl;
    uint16_t pkt_idx;
    uint16_t data_pos;
    uint8_t  recv_data[OTA_BUFF_LEN];
} __DATA_ALIGNED(4);

/// Global Variable Declarations
__ATTR_SRAM struct ota_env_tag ota_env;

/**
 ****************************************************************************************
 * @section ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Attributes Index
enum ota_att_idx
{
    // Service Declaration, *MUST* Start at 0
    OTA_IDX_SVC,

    // OTA NTF Char.
    OTA_IDX_NTF_CHAR,
    OTA_IDX_NTF_VAL,
    OTA_IDX_NTF_CFG,

    // OTA RECV Char.
    OTA_IDX_RECV_CHAR,
    OTA_IDX_RECV_VAL, // 5

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    OTA_IDX_NB,
};

// 128bits-uuid:584db6b8-ce89-72cd-9edf-2162ce8fcabf
// 128bits-uuid:584d09c5-ce89-72cd-9edf-2162ce8fcabf
// 128bits-uuid:584d0f42-ce89-72cd-9edf-2162ce8fcabf
/// Characteristic Base UUID128
#define OTA_BASE_UUID128(uuid)                                                                     \
    { 0xBF, 0xCA, 0x8F, 0xCE, 0x62, 0x21, 0xDF, 0x9E, 0xCD, 0x72, 0x89, 0xCE,                      \
        ((uuid >> 0) & 0xFF), ((uuid >> 8) & 0xFF), 0x4D, 0x58 }

const uint8_t ota_svc_uuid[] = OTA_BASE_UUID128(0xB6B8);
/// OTA Notify UUID128(Slave -> Master)
const uint8_t ota_char_ntf[] = OTA_BASE_UUID128(0x09C5);
/// OTA Receive Write Command UUID128(Master -> Slave)
const uint8_t ota_char_recv[] = OTA_BASE_UUID128(0x0F42);

/// Attributes Description
const att_decl_t ota_atts[] = {
    // OTA Notify Char. Declaration and Value and Client Char. Configuration Descriptor
    ATT_ELMT_DECL_CHAR(OTA_IDX_NTF_CHAR),
    ATT_ELMT128(OTA_IDX_NTF_VAL, ota_char_ntf, PROP_NTF, 0),
    ATT_ELMT_DESC_CLI_CHAR_CFG(OTA_IDX_NTF_CFG),

    // OTA Write Command Char. Declaration and Value
    ATT_ELMT_DECL_CHAR(OTA_IDX_RECV_CHAR),
    ATT_ELMT128(OTA_IDX_RECV_VAL, ota_char_recv, PROP_WC, OTA_RECV_MAX_LEN),
};

/// Service Description
const struct svc_decl ota_svc_db = {
    .uuid128 = ota_svc_uuid,
    .info    = SVC_UUID(128),
    .atts    = ota_atts,
    .nb_att  = OTA_IDX_NB - 1,
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void ota_reboot(uint8_t status);

/**
 ****************************************************************************************
 * @section SVC FUNCTIONS
 ****************************************************************************************
 */

/// Retrieve attribute handle from index (@see ota_att_idx)
static uint16_t ota_get_att_handle(uint8_t att_idx)
{
    ASSERT_ERR(att_idx < OTA_IDX_NB);

    return att_idx + ota_env.start_hdl;
}

/// Retrieve attribute index form handle
static uint8_t ota_get_att_idx(uint16_t handle)
{
    ASSERT_ERR((handle >= ota_env.start_hdl) && (handle < ota_env.start_hdl + OTA_IDX_NB));

    return handle - ota_env.start_hdl;
}

/// Handles reception of the atts request from peer device
static void ota_svc_func(uint8_t conidx, uint8_t opcode, uint16_t handle, const void *param)
{
    uint8_t att_idx = ota_get_att_idx(handle);

    DEBUG("svc_func(cid:%d,op:0x%x,hdl:0x%x,att:%d)", conidx, opcode, handle, att_idx);

    switch (opcode)
    {
        case ATTS_READ_REQ:
        {
            if (att_idx == OTA_IDX_NTF_CFG)
            {
                // retrieve notification config
                uint16_t cli_cfg = OTA_NTF_CFG_GET(conidx);

                DEBUG("  read_cfm(txd_ntf:%d)", cli_cfg);
                gatt_read_cfm(conidx, LE_SUCCESS, handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                break;
            }

            // Send error response
            gatt_read_cfm(conidx, PRF_ERR_APP_ERROR, handle, 0, NULL);
        }
        break;

        case ATTS_WRITE_REQ:
        {
            const struct atts_write_ind *ind = param;

            DEBUG("  write_req(hdl:0x%x,att:%d,more:%d, wr:0x%x,len:%d)", handle, att_idx,
                ind->more, ind->wrcode, ind->length);

            if (att_idx == OTA_IDX_RECV_VAL)
            {
                // Send write conform first!
                if (!ind->more)
                {
                    gatt_write_cfm(conidx, LE_SUCCESS, handle);
                }

                // Next to process data received
                ota_recv(conidx, ind->length, ind->value);
                break;
            }
            else if (att_idx == OTA_IDX_NTF_CFG)
            {
                if ((!ind->more) && (ind->length == sizeof(uint16_t)))
                {
                    uint16_t cli_cfg = read16p(ind->value);

                    // update configuration if value for stop or NTF/IND start
                    if (cli_cfg <= PRF_CLI_START_IND)
                    {
                        DEBUG("  set txd_ntf(cid:%d,cfg:%d)", conidx, cli_cfg);

                        OTA_NTF_CFG_SET(conidx);
                        // Send write conform quickly!
                        gatt_write_cfm(conidx, LE_SUCCESS, handle);
                        break;
                    }
                }
            }
            else
            {
                // Send write conform with error!
                gatt_write_cfm(conidx, PRF_ERR_APP_ERROR, handle);
            }
        }
        break;

        case ATTS_INFO_REQ:
        {
            uint8_t  status = LE_SUCCESS;
            uint16_t length = 0;

            if (att_idx == OTA_IDX_RECV_VAL)
            {
                length = OTA_RECV_MAX_LEN; // accepted length
            }
            else if (att_idx == OTA_IDX_NTF_CFG)
            {
                length = sizeof(uint16_t); // CCC attribute
            }
            else
            {
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            }

            // Send length-info confirm for prepWR judging.
            DEBUG("  info_cfm(hdl:0x%x,att:%d,sta:0x%X,len:%d)", handle, att_idx, status, length);
            gatt_info_cfm(conidx, status, handle, length);
        }
        break;

        case ATTS_CMP_EVT:
        {
#if (DBG_OTA)
            const struct atts_cmp_evt *evt = param;

            DEBUG("  cmp_evt(op:0x%x,sta:0x%x)", evt->operation, evt->status);
            // add 'if' to avoid warning #117-D: "evt" never referenced
            if (evt->operation == GATT_NOTIFY)
            {
                // Notify result
            }
#endif

            ota_reboot(((struct atts_cmp_evt *)param)->status);
        }
        break;

        default:
        {
            // nothing to do
        }
        break;
    }
}

/**
 ****************************************************************************************
 * @section API FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Serial Service Profile in the DB.
 *        Customize via pre-define @see OTA_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t ota_svc_init(void)
{
    uint8_t status = LE_SUCCESS;

    memset((uint8_t *)&ota_env, 0x00, sizeof(ota_env) - OTA_BUFF_LEN);

    // Init Environment
    ota_env.start_hdl = OTA_START_HDL;

    // Create Service in database
    status = attmdb_svc_create(&ota_env.start_hdl, NULL, &ota_svc_db, ota_svc_func);

    DEBUG("svc_init(sta:0x%X,shdl:%d)", status, ota_env.start_hdl);

    return status;
}

/**
 ****************************************************************************************
 * @brief Transmit data to peer device via NTF or IND
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t ota_ntf_send(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if (len > 0)
    {
        if (OTA_NTF_CFG_GET(conidx))
        {
            status = LE_SUCCESS;
            gatt_ntf_send(conidx, ota_get_att_handle(OTA_IDX_NTF_VAL), len, data);
            debugDump("send", data, len);
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Callback on received data from peer device via WC or WQ (__WEAK func)
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 ****************************************************************************************
 */

#if (CFG_USE_LOAD)
OTA_SRAMFN(block_er) static void flash_b64k_erase(uint32_t offset)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY);

    CACHE->CCR.Word    = 0; // disable cache
    CACHE->CIR.INV_ALL = 1; // flush cache
    fshc_erase(offset, 0xD8 /*FSH_CMD_ER_B64K*/);
    CACHE->CCR.Word = 1;    // enable cache

    GLOBAL_INT_RESTORE();
}
#else
OTA_SRAMFN(block_er) static void flash_sector_erase(uint32_t offset)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY);

    CACHE->CCR.Word    = 0; // disable cache
    CACHE->CIR.INV_ALL = 1; // flush cache
    fshc_erase(offset, FSH_CMD_ER_SECTOR);
    CACHE->CCR.Word = 1;    // enable cache

    GLOBAL_INT_RESTORE();
}

OTA_SRAMFN(wr_protect) void flash_wr_protect(uint8_t val)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY);

    uint8_t sta_reg0 = fshc_rd_sta(FSH_CMD_RD_STA0, 1);

    if (val != sta_reg0)
    {
        CACHE->CCR.Word = 0;                      // disable cache
        CACHE->CIR.Word = BIT(CACHE_INV_ALL_POS); // flush cache
        // 写保护0x000-0xFFF (4KB)
        // write en singal
        fshc_en_cmd(FSH_CMD_WR_EN);

        // send write sta cmd
        fshc_wr_sta(FSH_CMD_WR_STA, 1, val);

        CACHE->CCR.Word = 1; // enable cache
        // 状态标志：bit0-WEL(写使能锁存), bit1-WIP(写进行中)
        do {
            sta_reg0 = fshc_rd_sta(FSH_CMD_RD_STA0, 1);
        } while (sta_reg0 & 0x03);
    }

    GLOBAL_INT_RESTORE();
}
#endif // CFG_USE_LOAD

// 避免长跳转生成veneer
typedef void (*flash_er)(uint32_t);
typedef void (*flash_wr_reg)(uint8_t);
typedef void (*flash_wr_rd)(uint32_t, uint32_t *, uint32_t);

struct ota_flash_op_tag
{
    flash_er volatile block_er;
#if (!CFG_USE_LOAD)
    flash_er volatile page_er;
    flash_wr_reg volatile wr_reg;
#endif
    flash_wr_rd volatile word_wr;
    flash_wr_rd volatile word_rd;
};

const struct ota_flash_op_tag ota_flash_op = {
#if (CFG_USE_LOAD)
    .block_er = flash_b64k_erase,
#else
    .block_er = flash_sector_erase,
    .page_er  = flash_page_erase,
    .wr_reg   = flash_wr_protect,
#endif
    .word_wr = flash_write,
    .word_rd = flash_read,
};

// State/CMD/Head
enum ota_info
{
    /************************************/
    // state
    OTA_OK  = 0x00,
    OTA_ERR = 0x01,

    /************************************/
    // cmd
    OTA_CMD_VER = 0x10,
    OTA_CMD_START,
    OTA_CMD_DATA_WC,
    OTA_CMD_DATA_WR,
    OTA_CMD_END,

    /************************************/
    // head
    OTA_RSP_HEAD = 0xAB,
    OTA_CMD_HEAD = 0xBA,
};

static bool is_no_ota_cmd(uint8_t ota_cmd)
{
    return ((ota_cmd < OTA_CMD_VER) || (ota_cmd > OTA_CMD_END));
}

__WEAK void ota_recv(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    debugDump("recv", data, len);
    uint8_t ota_head = data[0];
    uint8_t ota_cmd  = data[1];

    // check head/cmd
    if ((ota_head != OTA_CMD_HEAD) || (is_no_ota_cmd(ota_cmd)))
    {
        return;
    }

    uint16_t peer_pkt_idx;
    uint16_t data_len;
    bool     ntf_send = true;

    uint8_t rsp_data[16];

    rsp_data[0] = OTA_RSP_HEAD;
    rsp_data[1] = OTA_OK;
    rsp_data[2] = ota_cmd;
    rsp_data[3] = 0x00;

    OTA_FLAG_SET(FLAG_OTA_PROC);

    switch (ota_cmd)
    {
        case OTA_CMD_VER:
        {
            ota_env.bin_len  = 0;
            ota_env.data_sum = 0;
            ota_env.pkt_idx  = 0;
            ota_env.data_pos = 0;

            write16p(rsp_data + 4, OTA_BUFF_LEN);
            write16p(rsp_data + 6, OTA_PKT_LEN);
            len = 8;

#if (CFG_USE_LOAD)
            ota_env.bank = (OTA_BANK_B_BASE - FLASH_BASE);
#else
            uint32_t curr_code_addr = RD_32(FLASH_INFO_CODE_ADDR);
            DEBUG("Curr Addr:0x%x", curr_code_addr);
            if (curr_code_addr == OTA_BANK_A_BASE)
            {
                rsp_data[3]  = 0x01;
                ota_env.bank = OTA_BANK_B;
            }
            else if (curr_code_addr == OTA_BANK_B_BASE)
            {
                ota_env.bank = OTA_BANK_A;
            }
            else
            {
                ota_env.bank = 0;
                rsp_data[1]  = OTA_ERR;
                OTA_FLAG_SET(FLAG_OTA_END);
                break;
            }

            if (ota_env.bank)
            {
                ota_flash_op.wr_reg(0x00);
            }
#endif
        }
        break;

        case OTA_CMD_START:
        {
            uint32_t size         = flash_size();
            uint32_t bin_size     = read32p(data + 2);
            DEBUG("bin_size:%"PRIX32", flash_size:%"PRIX32, bin_size, size);
            len = 3;

            // 判断bin_size合法性.
            if (bin_size > size / 2)
            {
                rsp_data[1] = OTA_ERR;
                OTA_FLAG_SET(FLAG_OTA_END);
                break;
            }

#if (CFG_USE_LOAD)
            // 0x20000 - 0x30000
            flash_b64k_erase(OTA_BANK_B_BASE - FLASH_BASE);
            // 0x30000 - 0x40000
            flash_b64k_erase(OTA_BANK_B_BASE - FLASH_BASE + FLASH_B64K_SIZE);
#else
            uint32_t curr_rst_hdl = RD_32(FLASH_INFO_CODE_ADDR) & 0xFFFFFF00UL;
            uint32_t peer_rst_hdl = read32p(data + 6) & 0xFFFFFF00UL;
            DEBUG("curr_hdl:%X, peer_hdl:%X", curr_rst_hdl, peer_rst_hdl);
            // 判断Reset_Handler地址合法性.
            if ((peer_rst_hdl == 0x00) || ((peer_rst_hdl >> 24) != 0x18) ||
                (peer_rst_hdl == curr_rst_hdl))
            {
                rsp_data[1] = OTA_ERR;
                OTA_FLAG_SET(FLAG_OTA_END);
                break;
            }
            // Erase
            uint32_t erase_offset = (ota_env.bank & 0xFFFFF000UL);
            uint32_t erase_end    = erase_offset + ALIGN(bin_size, FLASH_SECTOR_SIZE);

            for (; erase_offset < erase_end; erase_offset += FLASH_SECTOR_SIZE)
            {
                ota_flash_op.block_er(erase_offset);
            }
#endif
        }
        break;

        case OTA_CMD_DATA_WC:
        {
            peer_pkt_idx = read16p(data + 2);
            data_len     = read16p(data + 4);
            DEBUG("id:%d, %d", peer_pkt_idx, data_len);

            memcpy(ota_env.recv_data + ota_env.data_pos, data + OTA_DATA_POS, data_len);

            ota_env.bin_len  += data_len;
            ota_env.data_pos += data_len;
            ota_env.pkt_idx++;

            for (uint16_t i = 0; i < data_len; ++i)
            {
                ota_env.data_sum += data[OTA_DATA_POS + i];
            }

            ntf_send = false;
        }
        break;

        case OTA_CMD_DATA_WR:
        {
            len          = 3;
            peer_pkt_idx = read16p(data + 2);
            data_len     = read16p(data + 4);
            DEBUG("id:%d, %d", peer_pkt_idx, data_len);

            memcpy(ota_env.recv_data + ota_env.data_pos, data + OTA_DATA_POS, data_len);
            ota_env.data_pos += data_len;
            DEBUG("local_len:%"PRIu32", pos:%"PRIu16", dlen:%"PRIu16", blen:%"PRIu32, ota_env.bin_len, ota_env.data_pos,
                data_len, OTA_BUFF_LEN);

            if (peer_pkt_idx != ota_env.pkt_idx)
            {
                rsp_data[1] = OTA_ERR;
                ntf_send    = false;
                OTA_FLAG_SET(FLAG_OTA_END);
                ota_ntf_send(conidx, len, rsp_data);
                break;
            }

            // write flash
            uint32_t wr_offset = (ota_env.bin_len / OTA_BUFF_LEN) * OTA_BUFF_LEN + ota_env.bank;

#if (OTA_BUFF_LEN > FLASH_PAGE_SIZE)

            uint8_t page_num =
                (ota_env.data_pos / FLASH_PAGE_SIZE) + (!!(ota_env.data_pos % FLASH_PAGE_SIZE));

            for (uint8_t i = 0; i < page_num; ++i)
            {
                ota_flash_op.word_wr(wr_offset,
                    (uint32_t *)(ota_env.recv_data + i * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE_WLEN);
                wr_offset += FLASH_PAGE_SIZE;
            }
#else
            ota_flash_op.word_wr(wr_offset, (uint32_t *)ota_env.recv_data, FLASH_PAGE_SIZE_WLEN);
#endif

            ota_env.bin_len  += data_len;
            ota_env.data_pos  = 0;
            ota_env.pkt_idx++;

            for (uint16_t i = 0; i < data_len; ++i)
            {
                ota_env.data_sum += data[OTA_DATA_POS + i];
            }
        }
        break;

        case OTA_CMD_END:
        {
            uint32_t peer_bin_len  = read32p(data + 2);
            uint32_t peer_data_sum = read32p(data + 6);
            DEBUG("bank:%"PRIX32", sum:%"PRIX32", peer_len:%"PRIu32", lsum:%"PRIX32", local_len:%"PRIu32, ota_env.bank,
                peer_data_sum, peer_bin_len, ota_env.data_sum, ota_env.bin_len);

            write32p(rsp_data + 3, ota_env.bin_len);
            write32p(rsp_data + 7, ota_env.data_sum);
            len = 11;

            OTA_FLAG_SET(FLAG_OTA_END);

            if ((peer_bin_len != ota_env.bin_len) || (peer_data_sum != ota_env.data_sum))
            {
                ntf_send    = false;
                rsp_data[1] = OTA_ERR;
                ota_ntf_send(conidx, len, rsp_data);
                break;
            }

// 写boot info
#if (CFG_USE_LOAD)
            memset(ota_env.recv_data, 0xFF, FLASH_PAGE_SIZE);

            write32p(ota_env.recv_data + 0, 0x55AA5AA5);
            write32p(ota_env.recv_data + 4, ota_env.bin_len);
            write32p(ota_env.recv_data + 8, OTA_BANK_B_BASE);

            ota_flash_op.word_wr(LOADER_INFO_BASE - FLASH_BASE, (uint32_t *)ota_env.recv_data,
                FLASH_PAGE_SIZE_WLEN);
#else

            ota_flash_op.word_rd(
                FLASH_INFO_PAGE, (uint32_t *)ota_env.recv_data, FLASH_PAGE_SIZE_WLEN);

            write32p(ota_env.recv_data + 0, 0xAA55A004);
            write32p(ota_env.recv_data + 4, ota_env.bin_len);
            write32p(ota_env.recv_data + 8, ota_env.bank + FLASH_BASE);

            ota_flash_op.page_er(FLASH_INFO_PAGE);
            ota_flash_op.word_wr(
                FLASH_INFO_PAGE, (uint32_t *)ota_env.recv_data, FLASH_PAGE_SIZE_WLEN);
#endif
        }
        break;

        default:
        {
            len = 3;

            rsp_data[1] = OTA_ERR;
            OTA_FLAG_SET(FLAG_OTA_END);
        }
        break;
    };

    if (ntf_send)
    {
        ota_ntf_send(conidx, len, rsp_data);
    }
}

void ota_reboot(uint8_t status)
{
    if (!OTA_FLAG_GET(FLAG_OTA_END) || status)
    {
        return;
    }

    OTA_FLAG_CLR_ALL();

#if (0)
    uint8_t app_sta;
    do {
        ble_schedule();

        app_sta = app_state_get();

    } while (app_sta >= APP_CONNECTED);

    NVIC_SystemReset();
#else
    iwdt_conf(32000);
#endif
}

#endif // PRF_OTA
