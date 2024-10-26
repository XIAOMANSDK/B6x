/**
 ****************************************************************************************
 *
 * @file gap.h
 *
 * @brief Generic Access Profile Header file.
 *
 ****************************************************************************************
 */

#ifndef GAP_H_
#define GAP_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Minimal authorized MTU value (defined by Bluetooth SIG)
#define GAP_MIN_LE_MTU          (23)
/// Minimal authorized MTU value if secure connection is supported
#define GAP_MIN_LE_MTU_SEC_CON  (65)
/// Maximum MTU (defined in BLE Lib)
#define GAP_MAX_LE_MTU          (512)

/// BD address length
#define GAP_BD_ADDR_LEN       (6)
/// LE Channel map length
#define GAP_LE_CHNL_MAP_LEN   (0x05)
/// LE Feature Flags Length
#define GAP_LE_FEATS_LEN      (0x08)
/// ADV Data and Scan Response length
#define GAP_ADV_DATA_LEN      (0x1F)
#define GAP_SCAN_RSP_DATA_LEN (0x1F)
/// Random number length
#define GAP_RAND_NB_LEN       (0x08)
/// Key length
#define GAP_KEY_LEN           (16)
/// P256 Key Len
#define GAP_P256_KEY_LEN      (0x20)


///***** AD Type Flag - Bit set *******/
/// Limited discovery flag - AD Flag
#define GAP_LE_LIM_DISCOVERABLE_FLG             0x01
/// General discovery flag - AD Flag
#define GAP_LE_GEN_DISCOVERABLE_FLG             0x02
/// Legacy BT not supported - AD Flag
#define GAP_BR_EDR_NOT_SUPPORTED                0x04
/// Dual mode for controller supported (BR/EDR/LE) - AD Flag
#define GAP_SIMUL_BR_EDR_LE_CONTROLLER          0x08
/// Dual mode for host supported (BR/EDR/LE) - AD Flag
#define GAP_SIMUL_BR_EDR_LE_HOST                0x10

/*********** GAP Miscellaneous Defines *************/
/// Invalid connection index
#define GAP_INVALID_CONIDX                      0xFF

/// Invalid connection handle
#define GAP_INVALID_CONHDL                      0xFFFF

/// Connection interval min (N*1.250ms)
#define GAP_CONN_INTERVAL_MIN            6       //(7.5ms)
/// Connection interval Max (N*1.250ms)
#define GAP_CONN_INTERVAL_MAX            3200    //(4 sec)
/// Connection latency min (N*cnx evt)
#define GAP_CONN_LATENCY_MIN             0       //(0x00)
/// Connection latency Max (N*cnx evt
#define GAP_CONN_LATENCY_MAX             499     //(0x1F3)
/// Supervision TO min (N*10ms)
#define GAP_CONN_SUP_TO_MIN              10      //(100ms)
/// Supervision TO Max (N*10ms)
#define GAP_CONN_SUP_TO_MAX              3200    //(32 sec)


/// Length of resolvable random address prand part
#define GAP_ADDR_PRAND_LEN            (3)
/// Length of resolvable random address hash part
#define GAP_ADDR_HASH_LEN             (3)


/// HCI 7.8.33 LE Set Data Length Command
/// Preferred minimum number of payload octets
#define LE_MIN_OCTETS       (27)
/// Preferred minimum number of microseconds
#define LE_MIN_TIME         (328)
/// Preferred minimum number of microseconds LL:4.5.10
#define LE_MIN_TIME_CODED   (2704)
/// Preferred maximum number of payload octets
#define LE_MAX_OCTETS       (124)
/// Preferred maximum number of microseconds
#define LE_MAX_TIME         (1104)
/// Preferred maximum number of microseconds LL:4.5.10
#define LE_MAX_TIME_CODED   (17040)

/*
 * DEFINES - Optional for BLE application usage
 ****************************************************************************************
 */

/// Central idle timer
/// TGAP(conn_pause_central)
/// recommended value: 1 s: (100 for ke timer)
#define GAP_TMR_CONN_PAUSE_CT                               0x0064

/// Minimum time upon connection establishment before the peripheral
/// starts a connection update procedure: TGAP(conn_pause_peripheral)
/// recommended value: 5 s: (500 for ke timer)
#define GAP_TMR_CONN_PAUSE_PH                               0x01F4

/// Minimum time to perform scanning when user initiated
/// TGAP(scan_fast_period)
/// recommended value: 30.72 s: (3072 for ke timer)
#define GAP_TMR_SCAN_FAST_PERIOD                            0x0C00

/// Minimum time to perform advertising when user initiated
/// TGAP(adv_fast_period)
/// recommended value: 30 s: (3000 for ke timer)
#define GAP_TMR_ADV_FAST_PERIOD                             0x0BB8

/// Scan interval used during Link Layer Scanning State when
/// performing the Limited Discovery procedure
/// TGAP(lim_disc_scan_int)
/// recommended value: 11.25ms; (18 decimal)
#define GAP_LIM_DISC_SCAN_INT                               0x0012

/// Scan interval in any discovery or connection establishment
/// procedure when user initiated: TGAP(scan_fast_interval)
/// recommended value: 30 to 60 ms; N * 0.625
#define GAP_SCAN_FAST_INTV                                  0x0030

/// Scan window in any discovery or connection establishment
/// procedure when user initiated: TGAP(scan_fast_window)
/// recommended value: 30 ms; N * 0.625
#define GAP_SCAN_FAST_WIND                                  0x0030

/// Scan interval in any discovery or connection establishment
/// procedure when background scanning: TGAP(scan_slow_interval1)
/// recommended value: 1.28 s : 0x00CD (205); N * 0.625
#define GAP_SCAN_SLOW_INTV1                                 0x00CD

/// Scan interval in any discovery or connection establishment
/// procedure when background scanning: TGAP(scan_slow_interval2)
/// recommended value: 2.56 s : 0x019A (410); N * 0.625
#define GAP_SCAN_SLOW_INTV2                                 0x019A

/// Scan window in any discovery or connection establishment
/// procedure when background scanning: TGAP(scan_slow_window1)
/// recommended value: 11.25 ms : 0x0012 (18); N * 0.625
#define GAP_SCAN_SLOW_WIND1                                 0x0012

/// Scan window in any discovery or connection establishment
/// procedure when background scanning: TGAP(scan_slow_window2)
/// recommended value: 22.5 ms : 0x0024 (36); N * 0.625
#define GAP_SCAN_SLOW_WIND2                                 0x0024

/// Minimum to maximum advertisement interval in any discoverable
/// or connectable mode when user initiated: TGAP(adv_fast_interval1)
/// recommended value: 30 to 60 ms; N * 0.625
#define GAP_ADV_FAST_INTV1                                  0x0030

/// Minimum to maximum advertisement interval in any discoverable
/// or connectable mode when user initiated: TGAP(adv_fast_interval2)
/// recommended value: 100 to 150 ms; N * 0.625
#define GAP_ADV_FAST_INTV2                                  0x0064

/// Minimum to maximum advertisement interval in any discoverable or
/// connectable mode when background advertising: TGAP(adv_slow_interval)
/// recommended value: 1 to 1.2 s : 0x00B0 (176); N * 0.625
#define GAP_ADV_SLOW_INTV                                   0x00B0

/// Minimum to maximum connection interval upon any connection
/// establishment: TGAP(initial_conn_interval)
/// recommended value: 30 to 50 ms ; N * 1.25 ms
#define GAP_INIT_CONN_MIN_INTV                              0x0018
#define GAP_INIT_CONN_MAX_INTV                              0x0028

/// Scan Defines
#define GAP_INQ_SCAN_INTV                                   0x0012
#define GAP_INQ_SCAN_WIND                                   0x0012

/// Connection supervision timeout
/// recommended value: 20s
#define GAP_CONN_SUPERV_TIMEOUT                             0x07D0

/// Minimum connection event
/// default value: 0x0000
#define GAP_CONN_MIN_CE                                     0x0000

/// Maximum connection event
/// default value: 0xFFFF
#define GAP_CONN_MAX_CE                                     0xFFFF

/// Connection latency
/// default value: 0x0000
#define GAP_CONN_LATENCY                                    0x0000

/// Maximum time to remain advertising when in the Limited
/// Discover able mode: TGAP(lim_adv_timeout)
/// required value: 180s: (18000 for ke timer)
#define GAP_TMR_LIM_ADV_TIMEOUT                             0x4650

/// Minimum time to perform scanning when performing
/// the General Discovery procedure: TGAP(gen_disc_scan_min)
/// recommended value: 10.24s: (1024 for ke timer)
#define GAP_TMR_GEN_DISC_SCAN                               0x0400

/// Minimum time to perform scanning when performing the
/// Limited Discovery procedure: TGAP(lim_disc_scan_min)
/// recommended value: 10.24s: (1024 for ke timer)
#define GAP_TMR_LIM_DISC_SCAN                               0x0400

/// Minimum time interval between private address change
/// TGAP(private_addr_int)
/// recommended value: 15 minutes
/// Minimum value 150s
#define GAP_TMR_PRIV_ADDR_MIN                                (150)
/// Maximum time interval between private address change
/// 0xA1B8 (approximately 11.5 hours)
#define GAP_TMR_PRIV_ADDR_MAX                             (0xA1B8)


/// Timer used in connection parameter update procedure
/// TGAP(conn_param_timeout)
/// recommended value: 30 s: (3000 for ke timer)
#define GAP_TMR_CONN_PARAM_TIMEOUT                          0x0BB8

/// Timer used in LE credit based connection procedure
/// TGAP(lecb_conn_timeout)
/// recommended value: 30 s: (3000 for ke timer)
#define GAP_TMR_LECB_CONN_TIMEOUT                           0x0BB8

/// Timer used in LE credit based disconnection procedure
/// TGAP(lecb_disconn_timeout)
/// recommended value: 30 s: (3000 for ke timer)
#define GAP_TMR_LECB_DISCONN_TIMEOUT                        0x0BB8

/// GAP Appearance or Icon Characteristic - 2 octets
/// - @url https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf
#define GAP_APPEARANCE_UNKNOWN                              0x0000 //!< Unknown
#define GAP_APPEARANCE_GENERIC_PHONE                        0x0040 //!< Generic Phone
#define GAP_APPEARANCE_GENERIC_COMPUTER                     0x0080 //!< Generic Computer
#define GAP_APPEARANCE_GENERIC_WATCH                        0x00C0 //!< Generic Watch
#define GAP_APPEARANCE_WATCH_SPORTS                         0x00C1 //!< Watch: Sports Watch
#define GAP_APPEARANCE_GENERIC_CLOCK                        0x0100 //!< Generic Clock
#define GAP_APPEARANCE_GENERIC_DISPLAY                      0x0140 //!< Generic Display
#define GAP_APPEARANCE_GENERIC_RC                           0x0180 //!< Generic Remote Control
#define GAP_APPEARANCE_GENERIC_EYE_GALSSES                  0x01C0 //!< Generic Eye-glasses
#define GAP_APPEARANCE_GENERIC_TAG                          0x0200 //!< Generic Tag
#define GAP_APPEARANCE_GENERIC_KEYRING                      0x0240 //!< Generic Keyring
#define GAP_APPEARANCE_GENERIC_MEDIA_PLAYER                 0x0280 //!< Generic Media Player
#define GAP_APPEARANCE_GENERIC_BARCODE_SCANNER              0x02C0 //!< Generic Barcode Scanner
#define GAP_APPEARANCE_GENERIC_THERMOMETER                  0x0300 //!< Generic Thermometer
#define GAP_APPEARANCE_GENERIC_THERMO_EAR                   0x0301 //!< Thermometer: Ear
#define GAP_APPEARANCE_GENERIC_HR_SENSOR                    0x0340 //!< Generic Heart rate Sensor
#define GAP_APPEARANCE_GENERIC_HRS_BELT                     0x0341 //!< Heart Rate Sensor: Heart Rate Belt
#define GAP_APPEARANCE_GENERIC_BLOOD_PRESSURE               0x0380 //!< Generic Blood Pressure
#define GAP_APPEARANCE_GENERIC_BP_ARM                       0x0381 //!< Blood Pressure: Arm
#define GAP_APPEARANCE_GENERIC_BP_WRIST                     0x0382 //!< Blood Pressure: Wrist
#define GAP_APPEARANCE_GENERIC_HID                          0x03C0 //!< Generic Human Interface Device (HID)
#define GAP_APPEARANCE_HID_KEYBOARD                         0x03C1 //!< HID Keyboard
#define GAP_APPEARANCE_HID_MOUSE                            0x03C2 //!< HID Mouse
#define GAP_APPEARANCE_HID_JOYSTIC                          0x03C3 //!< HID Joystick
#define GAP_APPEARANCE_HID_GAMEPAD                          0x03C4 //!< HID Gamepad
#define GAP_APPEARANCE_HID_DIGITIZER_TYABLET                0x03C5 //!< HID Digitizer Tablet
#define GAP_APPEARANCE_HID_DIGITAL_CARDREADER               0x03C6 //!< HID Card Reader
#define GAP_APPEARANCE_HID_DIGITAL_PEN                      0x03C7 //!< HID Digital Pen
#define GAP_APPEARANCE_HID_BARCODE_SCANNER                  0x03C8 //!< HID Barcode Scanner

/// GAP Peripheral Preferred Connection Parameter - 8 octets
#define GAP_PPCP_CONN_INTV_MAX                              0x0064
#define GAP_PPCP_CONN_INTV_MIN                              0x00C8
#define GAP_PPCP_SLAVE_LATENCY                              0x0000
#define GAP_PPCP_STO_MULT                                   0x07D0


/*
 * Enumerations
 ****************************************************************************************
 */

/// GAP Advertising Flags
enum gap_ad_type
{
    /// Flag
    GAP_AD_TYPE_FLAGS                      = 0x01,
    /// Use of more than 16 bits UUID
    GAP_AD_TYPE_MORE_16_BIT_UUID           = 0x02,
    /// Complete list of 16 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID  = 0x03,
    /// Use of more than 32 bit UUD
    GAP_AD_TYPE_MORE_32_BIT_UUID           = 0x04,
    /// Complete list of 32 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_32_BIT_UUID  = 0x05,
    /// Use of more than 128 bit UUID
    GAP_AD_TYPE_MORE_128_BIT_UUID          = 0x06,
    /// Complete list of 128 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_128_BIT_UUID = 0x07,
    /// Shortened device name
    GAP_AD_TYPE_SHORTENED_NAME             = 0x08,
    /// Complete device name
    GAP_AD_TYPE_COMPLETE_NAME              = 0x09,
    /// Transmit power
    GAP_AD_TYPE_TRANSMIT_POWER             = 0x0A,
    /// Class of device
    GAP_AD_TYPE_CLASS_OF_DEVICE            = 0x0D,
    /// Simple Pairing Hash C
    GAP_AD_TYPE_SP_HASH_C                  = 0x0E,
    /// Simple Pairing Randomizer
    GAP_AD_TYPE_SP_RANDOMIZER_R            = 0x0F,
    /// Temporary key value
    GAP_AD_TYPE_TK_VALUE                   = 0x10,
    /// Out of Band Flag
    GAP_AD_TYPE_OOB_FLAGS                  = 0x11,
    /// Slave connection interval range
    GAP_AD_TYPE_SLAVE_CONN_INT_RANGE       = 0x12,
    /// Require 16 bit service UUID
    GAP_AD_TYPE_RQRD_16_BIT_SVC_UUID       = 0x14,
    /// Require 32 bit service UUID
    GAP_AD_TYPE_RQRD_32_BIT_SVC_UUID       = 0x1F,
    /// Require 128 bit service UUID
    GAP_AD_TYPE_RQRD_128_BIT_SVC_UUID      = 0x15,
    /// Service data 16-bit UUID
    GAP_AD_TYPE_SERVICE_16_BIT_DATA        = 0x16,
    /// Service data 32-bit UUID
    GAP_AD_TYPE_SERVICE_32_BIT_DATA        = 0x20,
    /// Service data 128-bit UUID
    GAP_AD_TYPE_SERVICE_128_BIT_DATA       = 0x21,
    /// Public Target Address
    GAP_AD_TYPE_PUB_TGT_ADDR               = 0x17,
    /// Random Target Address
    GAP_AD_TYPE_RAND_TGT_ADDR              = 0x18,
    /// Appearance
    GAP_AD_TYPE_APPEARANCE                 = 0x19,
    /// Advertising Interval
    GAP_AD_TYPE_ADV_INTV                   = 0x1A,
    /// LE Bluetooth Device Address
    GAP_AD_TYPE_LE_BT_ADDR                 = 0x1B,
    /// LE Role
    GAP_AD_TYPE_LE_ROLE                    = 0x1C,
    /// Simple Pairing Hash C-256
    GAP_AD_TYPE_SPAIR_HASH                 = 0x1D,
    /// Simple Pairing Randomizer R-256
    GAP_AD_TYPE_SPAIR_RAND                 = 0x1E,
    /// 3D Information Data
    GAP_AD_TYPE_3D_INFO                    = 0x3D,

    /// Manufacturer specific data
    GAP_AD_TYPE_MANU_SPECIFIC_DATA         = 0xFF,
};

/// Constant defining the role
enum conn_role
{
    /// Master role
    ROLE_MASTER,
    /// Slave role
    ROLE_SLAVE,
};

/// BD address type
enum addr_type
{
    /// Public BD address
    ADDR_PUBLIC                   = 0x00,
    /// Random BD Address
    ADDR_RAND,
    /// Controller generates Resolvable Private Address based on the
    /// local IRK from resolving list. If resolving list contains no matching
    /// entry, use public address.
    ADDR_RPA_OR_PUBLIC,
    /// Controller generates Resolvable Private Address based on the
    /// local IRK from resolving list. If resolving list contains no matching
    /// entry, use random address.
    ADDR_RPA_OR_RAND,
    /// mask used to determine Address type in the air
    ADDR_MASK                     = 0x01,
    /// mask used to determine if an address is an RPA
    ADDR_RPA_MASK                 = 0x02,
    /// Random device address (controller unable to resolve)
    ADDR_RAND_UNRESOLVED          = 0xFE,
    /// No address provided (anonymous advertisement)
    ADDR_NONE                     = 0xFF,
};

/// Random Address type
enum gap_rnd_addr_type
{
    /// Static random address           - 11 (MSB->LSB)
    GAP_STATIC_ADDR     = 0xC0,
    /// Private non resolvable address  - 01 (MSB->LSB)
    GAP_NON_RSLV_ADDR   = 0x00,
    /// Private resolvable address      - 01 (MSB->LSB)
    GAP_RSLV_ADDR       = 0x40,
};

/// Boolean value set
enum
{
    /// Disable
    GAP_DISABLE = 0x00,
    /// Enable
    GAP_ENABLE
};


/// GAP Attribute database handles
/// Generic Access Profile Service
enum
{
    GAP_IDX_PRIM_SVC,
    GAP_IDX_CHAR_DEVNAME,
    GAP_IDX_DEVNAME,
    GAP_IDX_CHAR_ICON,
    GAP_IDX_ICON,
    GAP_IDX_CHAR_SLAVE_PREF_PARAM,
    GAP_IDX_SLAVE_PREF_PARAM,
    GAP_IDX_CHAR_CNT_ADDR_RESOL,
    GAP_IDX_CNT_ADDR_RESOL,
    GAP_IDX_CHAR_RSLV_PRIV_ADDR_ONLY,
    GAP_IDX_RSLV_PRIV_ADDR_ONLY,
    GAP_IDX_NUMBER
};


/****************** GAP Role **********************/
enum gap_role
{
    /// No role set yet
    GAP_ROLE_NONE        = 0x00,

    /// Observer role
    GAP_ROLE_OBSERVER    = 0x01,

    /// Broadcaster role
    GAP_ROLE_BROADCASTER = 0x02,

    /// Master/Central role
    GAP_ROLE_CENTRAL     = (0x04 | GAP_ROLE_OBSERVER),

    /// Peripheral/Slave role
    GAP_ROLE_PERIPHERAL  = (0x08 | GAP_ROLE_BROADCASTER),

    /// Device has all role, both peripheral and central
    GAP_ROLE_ALL         = (GAP_ROLE_CENTRAL | GAP_ROLE_PERIPHERAL),
};

/// IO Capability Values
enum gap_io_cap
{
    /// Display Only
    GAP_IO_CAP_DISPLAY_ONLY = 0x00,
    /// Display Yes No
    GAP_IO_CAP_DISPLAY_YES_NO,
    /// Keyboard Only
    GAP_IO_CAP_KB_ONLY,
    /// No Input No Output
    GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
    /// Keyboard Display
    GAP_IO_CAP_KB_DISPLAY,
    GAP_IO_CAP_LAST
};

/// TK Type
enum gap_tk_type
{
    ///  TK get from out of band method
    GAP_TK_OOB         = 0x00,
    /// TK generated and shall be displayed by local device
    GAP_TK_DISPLAY,
    /// TK shall be entered by user using device keyboard
    GAP_TK_KEY_ENTRY
};

/// OOB Data Present Flag Values
enum gap_oob
{
    /// OOB Data not present
    GAP_OOB_AUTH_DATA_NOT_PRESENT = 0x00,
    /// OOB data present
    GAP_OOB_AUTH_DATA_PRESENT,
    GAP_OOB_AUTH_DATA_LAST
};

/// Authentication mask
enum gap_auth_mask
{
    /// No Flag set
    GAP_AUTH_NONE    = 0,
    /// Bond authentication
    GAP_AUTH_BOND    = (1 << 0),
    /// Man In the middle protection
    GAP_AUTH_MITM    = (1 << 2),
    /// Secure Connection
    GAP_AUTH_SEC_CON = (1 << 3),
    /// Key Notification
    GAP_AUTH_KEY_NOTIF = (1 << 4)
};

/// Security Link Level
enum gap_lk_sec_lvl
{
    /// No authentication
    GAP_LK_NO_AUTH             = 0,
    /// Unauthenticated link
    GAP_LK_UNAUTH,
    /// Authenticated link
    GAP_LK_AUTH,
    /// Secure Connection link
    GAP_LK_SEC_CON,
};

/// Authentication Requirements
enum gap_auth
{
    /// No MITM No Bonding
    GAP_AUTH_REQ_NO_MITM_NO_BOND  = (GAP_AUTH_NONE),
    /// No MITM Bonding
    GAP_AUTH_REQ_NO_MITM_BOND     = (GAP_AUTH_BOND),
    /// MITM No Bonding
    GAP_AUTH_REQ_MITM_NO_BOND     = (GAP_AUTH_MITM),
    /// MITM and Bonding
    GAP_AUTH_REQ_MITM_BOND        = (GAP_AUTH_MITM | GAP_AUTH_BOND),
    /// SEC_CON and No Bonding
    GAP_AUTH_REQ_SEC_CON_NO_BOND  = (GAP_AUTH_SEC_CON),
    /// SEC_CON and Bonding
    GAP_AUTH_REQ_SEC_CON_BOND     = (GAP_AUTH_SEC_CON | GAP_AUTH_BOND),

    GAP_AUTH_REQ_LAST,

    /// Mask of  authentication features without reserved flag
    GAP_AUTH_REQ_MASK             = 0x1F,
};

/// Key Distribution Flags
enum gap_kdist
{
    /// No Keys to distribute
    GAP_KDIST_NONE = 0x00,
    /// Encryption key in distribution
    GAP_KDIST_ENCKEY = (1 << 0),
    /// IRK (ID key)in distribution
    GAP_KDIST_IDKEY  = (1 << 1),
    /// CSRK(Signature key) in distribution
    GAP_KDIST_SIGNKEY= (1 << 2),
    /// LTK in distribution
    GAP_KDIST_LINKKEY=   (1 << 3),

    GAP_KDIST_LAST =   (1 << 4)
};

/// Security Defines
enum gap_sec_req
{
    /// No security (no authentication and encryption)
    GAP_NO_SEC = 0x00,
    /// Unauthenticated pairing with encryption
    GAP_SEC1_NOAUTH_PAIR_ENC,
    /// Authenticated pairing with encryption
    GAP_SEC1_AUTH_PAIR_ENC,
    /// Unauthenticated pairing with data signing
    GAP_SEC2_NOAUTH_DATA_SGN,
    /// Authentication pairing with data signing
    GAP_SEC2_AUTH_DATA_SGN,
    /// Secure Connection pairing with encryption
    GAP_SEC1_SEC_CON_PAIR_ENC,
};

/// Bit field use to select the preferred TX or RX LE PHY. 0 means no preferences
enum gap_phy
{
    /// No preferred PHY
    GAP_PHY_ANY               = 0x00,
    /// LE 1M PHY preferred for an active link
    GAP_PHY_LE_1MBPS          = (1 << 0),
    /// LE 2M PHY preferred for an active link
    GAP_PHY_LE_2MBPS          = (1 << 1),
    /// LE Coded PHY preferred for an active link
    GAP_PHY_LE_CODED          = (1 << 2),
};

/// Option for PHY configuration
enum gap_phy_option
{
    /// No preference for rate used when transmitting on the LE Coded PHY
    GAP_PHY_OPT_LE_CODED_ALL_RATES     = 0,
    /// S=2, 500kbps rate preferred when transmitting on the LE Coded PHY
    GAP_PHY_OPT_LE_CODED_500K_RATE     = 1,
    /// S=8, 125kbps  when transmitting on the LE Coded PHY
    GAP_PHY_OPT_LE_CODED_125K_RATE     = 2,
};

/// Enumeration of TX/RX PHY used for Test Mode
enum gap_test_phy
{
    /// LE 1M PHY (TX or RX)
    GAP_TEST_PHY_1MBPS        = 1,
    /// LE 2M PHY (TX or RX)
    GAP_TEST_PHY_2MBPS        = 2,
    /// LE Coded PHY (RX Only)
    GAP_TEST_PHY_CODED        = 3,
    /// LE Coded PHY with S=8 data coding (TX Only)
    GAP_TEST_PHY_125KBPS      = 3,
    /// LE Coded PHY with S=2 data coding (TX Only)
    GAP_TEST_PHY_500KBPS      = 4,
};

/// Modulation index
enum gap_modulation_idx
{
    /// Assume transmitter will have a standard modulation index
    GAP_MODULATION_STANDARD,
    /// Assume transmitter will have a stable modulation index
    GAP_MODULATION_STABLE,
};

/// Packet Payload type for test mode
enum gap_pkt_pld_type
{
    /// PRBS9 sequence "11111111100000111101..." (in transmission order)
    GAP_PKT_PLD_PRBS9,
    /// Repeated "11110000" (in transmission order)
    GAP_PKT_PLD_REPEATED_11110000,
    /// Repeated "10101010" (in transmission order)
    GAP_PKT_PLD_REPEATED_10101010,
    /// PRBS15 sequence
    GAP_PKT_PLD_PRBS15,
    /// Repeated "11111111" (in transmission order) sequence
    GAP_PKT_PLD_REPEATED_11111111,
    /// Repeated "00000000" (in transmission order) sequence
    GAP_PKT_PLD_REPEATED_00000000,
    /// Repeated "00001111" (in transmission order) sequence
    GAP_PKT_PLD_REPEATED_00001111,
    /// Repeated "01010101" (in transmission order) sequence
    GAP_PKT_PLD_REPEATED_01010101,
};


/*************** GAP Structures ********************/

/// Device name
/*@TRACE*/
struct gap_dev_name
{
    /// name length
    uint16_t length;
    /// name value
    uint8_t value[];
};

/// Slave preferred connection parameters
/*@TRACE*/
struct gap_slv_pref
{
    /// Connection interval minimum
    uint16_t con_intv_min;
    /// Connection interval maximum
    uint16_t con_intv_max;
    /// Slave latency
    uint16_t slave_latency;
    /// Connection supervision timeout multiplier
    uint16_t conn_timeout;
};

/// BD Address structure
/*@TRACE*/
typedef struct
{
    /// 6-byte array address value
    uint8_t  addr[GAP_BD_ADDR_LEN];
} bd_addr_t;

/// Channel map structure
/*@TRACE*/
typedef struct
{
    /// 5-byte channel map array
    uint8_t map[GAP_LE_CHNL_MAP_LEN];
} le_chnl_map_t;


/// Random number structure
/*@TRACE*/
typedef struct
{
    /// 8-byte array for random number
    uint8_t     nb[GAP_RAND_NB_LEN];
} rand_nb_t;

/// P256 Public key data format
typedef struct
{
    /// X Coordinate of the key
    uint8_t x[GAP_P256_KEY_LEN];
    /// X Coordinate of the key
    uint8_t y[GAP_P256_KEY_LEN];
} public_key_t;

/// Address information about a device address
/*@TRACE*/
struct gap_bdaddr
{
    /// BD Address of device
    bd_addr_t addr;
    /// Address type of the device 0=public/1=private random
    uint8_t addr_type;
};

/// Resolving list device information
/*@TRACE*/
struct gap_ral_dev_info
{
    /// Device identity
    struct gap_bdaddr addr;
    /// Privacy Mode
    uint8_t priv_mode;
    /// Peer IRK
    uint8_t peer_irk[GAP_KEY_LEN];
    /// Local IRK
    uint8_t local_irk[GAP_KEY_LEN];
};

/// Generic Security key structure
/*@TRACE*/
struct gap_sec_key
{
    /// Key value MSB -> LSB
    uint8_t key[GAP_KEY_LEN];
};


#endif // GAP_H_
