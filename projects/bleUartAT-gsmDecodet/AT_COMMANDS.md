# AT Command Reference

## BLE UART AT Command Set

**Device**: B6x BLE with GSM Audio Decoder
**Firmware**: bleUartAT-gsmDecoder
**Version**: 1.0.1
**Default Baudrate**: 115200

---

## Table of Contents

- [Basic Commands](#basic-commands)
- [System Commands](#system-commands)
- [BLE Configuration](#ble-configuration)
- [Connection Commands](#connection-commands)
- [Data Transfer](#data-transfer)
- [GSM Audio Commands](#gsm-audio-commands)

---

## Basic Commands

### AT

Test command.

**Format**: `AT`
**Response**: `[AT]OK`

**Example**:
```
AT
```
**Response**:
```
[AT]OK
```

---

### AT+ALL

Display all configuration parameters.

**Format**: `AT+ALL`
**Response**: `[AT]OK` followed by all configuration data

**Example**:
```
AT+ALL
```
**Response**:
```
[AT]OK
[DA]+NAME=gsmBle-AT
[DA]+MAC=AA:BB:CC:DD:EE:FF
[DA]+UUIDS=00F0FF
...
[DA]+BAUD=115200
```

---

## System Commands

### AT+VER?

Get firmware version.

**Format**: `AT+VER?`
**Response**: `[AT]OK` followed by version string

**Example**:
```
AT+VER?
```
**Response**:
```
[AT]OK
[DA]1.0.1
```

---

### AT+RESET

Reset device (reboot).

**Format**: `AT+RESET`
**Response**: `[AT]OK`
**Effect**: Device will reboot after storing configuration to Flash

**Example**:
```
AT+RESET
```
**Response**:
```
[AT]OK
```

---

### AT+RENEW

Restore factory settings and reset.

**Format**: `AT+RENEW`
**Response**: `[AT]OK`
**Effect**: Clears Flash configuration and resets device

**Example**:
```
AT+RENEW
```
**Response**:
```
[AT]OK
```

---

### AT+HELP

Display all available AT commands.

**Format**: `AT+HELP`
**Response**: List of all commands with their formats

**Example**:
```
AT+HELP
```
**Response**:
```
[AT]OK
0:AT+\r\n
1:AT+ALL\r\n
2:AT+MAC?\r\n
...
```

---

## BLE Configuration

### AT+DEV_NAME?

Get device name.

**Format**: `AT+DEV_NAME?`
**Max Name Length**: 11 characters
**Response**: `[AT]OK` followed by device name

**Example**:
```
AT+DEV_NAME?
```
**Response**:
```
[AT]OK
[DA]+DEV_NAME=gsmBle-AT
```

---

### AT+DEV_NAME=

Set device name (saved to Flash).

**Format**: `AT+DEV_NAME=<name>`
**Parameters**:
- `<name>`: Device name string (1-11 characters)

**Example**:
```
AT+DEV_NAME=MyDevice
```
**Response**:
```
[AT]OK [AT]OK MyDevice
```

---

### AT+MAC?

Get device MAC address.

**Format**: `AT+MAC?`
**Response**: `[AT]OK` followed by MAC address

**Example**:
```
AT+MAC?
```
**Response**:
```
[AT]OK
[DA]+MAC=AA:BB:CC:DD:EE:FF
```

---

### AT+BAUD?

Get current UART baud rate.

**Format**: `AT+BAUD?`
**Response**: `[AT]OK` followed by baud rate

**Supported Baud Rates**:
- 4800
- 9600
- 19200
- 38400
- 57600
- 115200 (default)
- 230400
- 460800
- 921600

**Example**:
```
AT+BAUD?
```
**Response**:
```
[AT]OK
[DA]+BAUD=115200
```

---

### AT+BAUD=

Set UART baud rate (saved to Flash, requires reset).

**Format**: `AT+BAUD=<baud>`
**Parameters**:
- `<baud>`: Baud rate value (4800-921600)

**Example**:
```
AT+BAUD=9600
```
**Response**:
```
[AT]OK [AT]OK 9600
```

**Note**: Device will not apply new baud rate until after reset (AT+RESET or power cycle).

---

## BLE Configuration

### AT+UUIDS?

Get Service UUID (16-bit).

**Format**: `AT+UUIDS?`
**Response**: `[AT]OK` followed by UUID in hex format

**Example**:
```
AT+UUIDS?
```
**Response**:
```
[AT]OK
[DA]+UUIDS=00F0FF
```

---

### AT+UUIDS=

Set Service UUID (saved to Flash).

**Format**: `AT+UUIDS=<uuid>`
**Parameters**:
- `<uuid>`: 16-bit UUID in hex format (4 hex digits)

**Example**:
```
AT+UUIDS=00F0FF
```
**Response**:
```
[AT]OK [AT]OK
```

---

### AT+UUIDN?

Get Notify UUID (16-bit).

**Format**: `AT+UUIDN?`
**Response**: `[AT]OK` followed by UUID in hex format

**Example**:
```
AT+UUIDN?
```
**Response**:
```
[AT]OK
[DA]+UUIDN=00F1FF
```

---

### AT+UUIDN=

Set Notify UUID (saved to Flash).

**Format**: `AT+UUIDN=<uuid>`
**Parameters**:
- `<uuid>`: 16-bit UUID in hex format (4 hex digits)

**Example**:
```
AT+UUIDN=00F1FF
```
**Response**:
```
[AT]OK [AT]OK
```

---

### AT+UUIDW?

Get Write UUID (16-bit).

**Format**: `AT+UUIDW?`
**Response**: `[AT]OK` followed by UUID in hex format

**Example**:
```
AT+UUIDW?
```
**Response**:
```
[AT]OK
[DA]+UUIDW=00F2FF
```

---

### AT+UUIDW=

Set Write UUID (saved to Flash).

**Format**: `AT+UUIDW=<uuid>`
**Parameters**:
- `<uuid>`: 16-bit UUID in hex format (4 hex digits)

**Example**:
```
AT+UUIDW=00F2FF
```
**Response**:
```
[AT]OK [AT]OK
```

---

### AT+AINTVL?

Get advertising interval.

**Format**: `AT+AINTVL?`
**Response**: `[AT]OK` followed by interval value
**Unit**: 0.625ms per count

**Example**:
```
AT+AINTVL?
```
**Response**:
```
[AT]OK
[DA]+AINTVL=160
```

---

### AT+AINTVL=

Set advertising interval (saved to Flash).

**Format**: `AT+AINTVL=<interval>`
**Parameters**:
- `<interval>`: Advertising interval (0x20-0x5000, default 0x20)

**Calculation**: Actual interval (ms) = (value * 5 / 8)

**Example**:
```
AT+AINTVL=160
```
**Response**:
```
[AT]OK [AT]OK
```

---

### AT+AMDATA?

Get advertising data.

**Format**: `AT+AMDATA?`
**Response**: `[AT]OK` followed by advertising data in hex format

**Example**:
```
AT+AMDATA?
```
**Response**:
```
[AT]OK
[DA]+AMDATA=0201060303F0FF...
```

---

### AT+AMDATA=

Set advertising data (saved to Flash).

**Format**: `AT+AMDATA=<data>`
**Parameters**:
- `<data>`: Advertising data in hex string (max 30 bytes = 60 hex chars)

**Example**:
```
AT+AMDATA=0201060303180309
```
**Response**:
```
[AT]OK [AT]OK
```

---

## Connection Commands

### AT+DISCON=1

Disconnect from connected device.

**Format**: `AT+DISCON=1`
**Response**: `[AT]OK` if command executed, `[AT]ERR[<code>]` if not connected

**Error Codes**:
- `ERR[3]`: Operation failed (not connected)

**Example**:
```
AT+DISCON=1
```
**Response**:
```
[AT]OK
```

---

### AT+CON_MAC?

Get MAC address of connected device.

**Format**: `AT+CON_MAC?`
**Response**: `[AT]OK` followed by connected device MAC

**Example**:
```
AT+CON_MAC?
```
**Response**:
```
[AT]OK
[DA]+MAC=AA:BB:CC:DD:EE:FF
```

---

## Data Transfer

### AT+CON_MAC=

Connect to device with specified MAC address (master mode).

**Format**: `AT+CON_MAC=<mac>`
**Parameters**:
- `<mac>`: MAC address in hex format (AA:BB:CC:DD:EE:FF)

**Example**:
```
AT+CON_MAC=AA:BB:CC:DD:EE:FF
```
**Response**:
```
[AT]OK
[DA]Connecting
```

---

## GSM Audio Commands

> **Note**: GSM audio commands are only available when `GSM_DECODE_EN=1` in cfg.h.

### AT+GSMPLAY

Play GSM audio from default Flash address.

**Format**: `AT+GSMPLAY`
**Flash Address**: `0x18020000` (default, defined by GSM_FLASH_ADDR in cfg.h)
**Behavior**: Plays once (does not loop)
**Response**: `[AT]OK` on success, `[AT]ERR[<code>]` on error

**Example**:
```
AT+GSMPLAY
```
**Response**:
```
[AT]OK
[DA]Playing 112 frames from 0x18020000
```

---

### AT+GSMPLAY=<addr>

Play GSM audio from specified Flash address.

**Format**: `AT+GSMPLAY=<addr>`
**Parameters**:
- `<addr>`: Flash address in hex format (e.g., 18020000)

**Behavior**: Plays once (does not loop)
**Response**: `[AT]OK` on success, `[AT]ERR[<code>]` on error

**Example**:
```
AT+GSMPLAY=18010000
```
**Response**:
```
[AT]OK
[DA]Playing 56 frames from 0x18010000
```

---

### AT+GSMSTOP

Stop current GSM audio playback.

**Format**: `AT+GSMSTOP`
**Response**: `[AT]OK`

**Effect**: Immediately stops playback and resets audio state machine

**Example**:
```
AT+GSMSTOP
```
**Response**:
```
[AT]OK
[DA]Playback stopped
```

---

### AT+GSMSTATUS?

Query GSM audio playback status.

**Format**: `AT+GSMSTATUS?`
**Response**: `[AT]OK` followed by playback status

**Status Values**:
- `GSM Status=Playing`: Audio is currently playing
- `GSM Status=Stopped`: Audio is stopped

**Example**:
```
AT+GSMSTATUS?
```
**Response**:
```
[AT]OK
[DA]GSM Status=Playing
```

---

## Error Codes

| Code | Description |
|-------|-------------|
| ERR[0] | No error |
| ERR[3] | Operation failed |
| ERR[4] | Invalid parameter |
| ERR[5] | Protocol error |

---

## Notes

### Flash Configuration Storage

Commands that modify device configuration (name, baud rate, UUIDs, etc.) are stored in non-volatile Flash memory and persist across power cycles.

### Reset Required

Some settings (like baud rate changes) require a device reset to take effect:
- Power cycle
- `AT+RESET` command

### Baud Rate

**Default**: 115200
**Configuration persists**: Yes (saved to Flash)

### MAC Address Format

All MAC addresses use hex format with colons:
`AA:BB:CC:DD:EE:FF`

### UUID Format

- 16-bit UUIDs: 4 hex digits (e.g., `00F0FF`)
- 128-bit UUIDs: 32 hex digits

### Advertising Interval

**Range**: 0x20 to 0x5000
**Default**: 0x20
**Unit**: 0.625ms
**Example**: Value 160 = 160 * 0.625 = 100ms

---

## Work Modes

### Overview

The device operates in two distinct modes depending on BLE connection status:

| Mode | Connection Status | LED Indicator | Data Behavior |
|-------|-----------------|--------------|---------------|
| **AT Command Mode** | Disconnected / Idle | Slow Blink (0.5s on/off) | Parses AT commands from UART |
| **Transparent Pass-through Mode** | Connected to BLE peer | Solid ON | Forwards all data between UART and BLE bidirectionally |

---

### Mode Switching

**From AT Command Mode → Transparent Mode:**

1. **Automatic**: Device successfully connects to BLE peer (central/peripheral)
   - Trigger: BLE connection established event
   - Action: `app_state_set(APP_CONNECTED)`
   - Effect: All subsequent UART data is forwarded to BLE without AT command parsing
   - LED: Changes from slow blink to solid ON

2. **Manual**: Send `AT+CON_MAC=<mac>` command (master mode)
   - Action: Initiates BLE connection to specified MAC address
   - Effect: On connection success, enters transparent mode
   - Note: Only works in master mode

**From Transparent Mode → AT Command Mode:**

1. **Automatic**: BLE connection is lost or closed
   - Trigger: BLE disconnect event
   - Action: `app_state_set(APP_IDLE)` or `app_state_set(APP_READY)`
   - Effect: Returns to AT command parsing mode
   - LED: Changes from solid ON to slow/fast blink

2. **Manual**: Send `AT+DISCON=1` command
   - Action: Actively disconnects current BLE connection
   - Effect: Immediately returns to AT command mode
   - Response: `[AT]OK` followed by disconnection

### AT Command Mode Details

**Characteristics**:
- Only AT commands are accepted
- All commands MUST start with `AT` or `AT+`
- Commands MUST end with `\r\n` (carriage return + linefeed)
- Invalid commands return: `[AT]ERR[5]` (Protocol error)
- Unrecognized commands return: `[AT]ERR[4]` (Invalid parameter)

**Available Operations**:
- Query device configuration (`AT+ALL`, `AT+MAC?`, `AT+VER?`)
- Modify device settings (`AT+DEV_NAME=`, `AT+BAUD=`)
- Control GSM audio (`AT+GSMPLAY`, `AT+GSMSTOP`, `AT+GSMSTATUS?`)
- Initiate BLE connection (`AT+CON_MAC=`)
- Disconnect BLE connection (`AT+DISCON=1`)
- Get command help (`AT+HELP`)

**Limitations**:
- Cannot send/receive raw data through BLE
- GSM audio playback runs in background but commands still work
- Some settings changes require reset (`AT+RESET` or power cycle)

### Transparent Pass-through Mode Details

**Characteristics**:
- All UART data is directly forwarded to BLE connection
- No AT command parsing or processing
- Data received from BLE is forwarded to UART
- Full-duplex bidirectional data transfer

**Data Flow**:
```
UART TX → BLE → Remote Device
Remote Device → BLE → UART RX
```

**Available Operations**:
- Send any data to remote BLE device
- Receive any data from remote BLE device
- Control GSM audio with AT commands (still processed in this mode)
- Query GSM status with `AT+GSMSTATUS?`
- Stop audio playback with `AT+GSMSTOP`
- Disconnect with `AT+DISCON=1`

**Special Features**:
- GSM audio playback continues in background during transparent data transfer
- AT commands like `AT+GSMSTATUS?` still work to query playback state
- High throughput with minimal latency (direct forwarding)
- MTU size: 247 bytes (configurable via BLE_MTU_SIZE in cfg.h)

### LED Indicators

| LED Pattern | State | Mode |
|-----------|-------|-------|
| **Slow Blink** (0.5s on/off) | APP_IDLE | AT Command Mode (idle) |
| **Fast Blink** (0.1s on/off) | APP_READY | AT Command Mode (advertising/scanning) |
| **Solid ON** | APP_CONNECTED | Transparent Mode (connected) |

### Usage Examples

**Example 1: AT Command Mode - Configure and Connect**

```
1. Power on device (LED slow blink - AT command mode)
2. AT+ALL                    (Read all configuration)
3. AT+DEV_NAME=MySensor     (Set device name)
4. AT+CON_MAC=AA:BB:CC:DD:EE:FF   (Connect to sensor)
   → Device connects, LED turns solid ON
   → Enter transparent mode
5. Send data: "Hello, Sensor!"  (Forwarded to BLE device)
6. Receive data: "Status: OK"       (Received from BLE device)
```

**Example 2: Transparent Mode - Data Transfer with GSM Audio**

```
1. Device connected to BLE peer (LED solid ON)
2. Send sensor data: {"temp": 25.4}  (Directly forwarded)
3. AT+GSMPLAY                     (Play audio announcement)
4. AT+GSMSTATUS?                  (Check: Still Playing)
5. Send more data: {"relay": "ON"}    (Works during playback)
6. AT+GSMSTOP                      (Stop audio)
7. AT+DISCON=1                     (Disconnect)
   → Returns to AT command mode (LED slow blink)
```

**Example 3: Auto-Reconnect with Transparent Mode**

```
1. AT+CON_MAC=AA:BB:CC:DD:EE:FF    (Connect to peer)
   → LED: Solid ON, Mode: Transparent
2. [Data transfer happens...]
3. Peer disconnects (connection lost)
   → LED: Slow Blink, Mode: AT Command
4. AT+CON_MAC=AA:BB:CC:DD:EE:FF    (Reconnect automatically)
   → LED: Solid ON, Mode: Transparent
```

### Transition Timing

| Event | Mode Before | Mode After | LED Change |
|--------|------------|-----------|------------|
| Power-on | - | AT Command | Slow Blink |
| BLE connection established | AT Command | Transparent | Slow Blink → Solid ON |
| BLE connection lost | Transparent | AT Command | Solid ON → Slow Blink |
| AT+DISCON=1 | Transparent | AT Command | Solid ON → Slow Blink |
| AT+RESET | Any | AT Command (reboots) | Depends on boot state |

---

## Quick Reference

| Command | Function | Example |
|----------|-------------|----------|
| `AT` | Test/echo | `AT` |
| `AT+ALL` | Show all config | `AT+ALL` |
| `AT+VER?` | Get version | `AT+VER?` |
| `AT+DEV_NAME=<name>` | Set device name | `AT+DEV_NAME=MyDevice` |
| `AT+MAC?` | Get MAC address | `AT+MAC?` |
| `AT+BAUD=<rate>` | Set baud rate | `AT+BAUD=115200` |
| `AT+RESET` | Reboot device | `AT+RESET` |
| `AT+HELP` | Show all commands | `AT+HELP` |
| `AT+GSMPLAY` | Play GSM audio (default addr) | `AT+GSMPLAY` |
| `AT+GSMPLAY=<addr>` | Play GSM audio (specified) | `AT+GSMPLAY=18010000` |
| `AT+GSMSTOP` | Stop GSM audio | `AT+GSMSTOP` |
| `AT+GSMSTATUS?` | Query GSM status | `AT+GSMSTATUS?` |
| `AT+DISCON=1` | Disconnect | `AT+DISCON=1` |

---

*Document Version: 1.0*
*Last Updated: 2026-02-12*
