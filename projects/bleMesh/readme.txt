BLE Mesh Server 配网完整流程

  ┌──────────────────────────────────────────────────────────────────────────────────┐
  │                        BLE Mesh Network                                 │
  │                                                                      │
  │  ┌────────────────────────────────────────────────────────────────────┐      │
  │  │                     Provisioner (手机 App/网关)            │      │
  │  │                                                              │      │
  │  │  ┌─────────────────────────────────────────────────────────┐      │
  │  │  │              Server 设备 (待配网)                │      │
  │  │  │  - 初始状态: 地址=0x0000 (未分配)           │      │
  │  │  └─────────────────────────────────────────────────────────┘      │
  └──────────────────────────────────────────────────────────────────────────────┘

  ---
  一、设备启动阶段

  1. 系统初始化 (main.c:main())

  main()
    └─ sysInit()                 // 时钟、外设时钟
        └─ devInit()             // 调试、BLE、Mesh、GPIO
             ├─ dbgInit()         // UART 初始化 (921600 波特率)
             ├─ app_init(rsn)     // BLE 应用初始化
             ├─ mesh_init(0)       // Mesh 协议栈初始化
             └─ GPIO 初始化 (LED/KEY)
             ├─ LED0 (PA08) - 显示本地状态
             └─ LED1 (PA09) - 显示远程状态
             └─ KEY0 (PA15) - 重启 Proxy 广告

  2. Mesh 应用初始化 (app_mesh_server.c:app_mesh_create())

  app_mesh_create()
    ├─ genie_triple_init()         // 初始化 Genie 三元组
    │   └─ const 数据: pid/key/mac (Demo 使用固定值)
    │
    ├─ ms_set_ssid()             // 设置子网 ID
    │   └─ 参数: GENIE_MESH_CID, pid=0, vid=0, loc=0
    │
    ├─ 注册 4 种 Server 模型:
    │   ├─ mm_gens_oo_register(1, true)      // Generic OnOff Server
    │   ├─ mm_lights_ln_register(0, true)      // Lighting Lightness Server
    │   ├─ mm_lights_ctl_register(0, true)      // Lighting CTL Server
    │   └─ mm_lights_hsl_register(0, true)      // Lighting HSL Server
    │
    └─ mesh_enable()              // 启动 Mesh Bearer (发送广告)

  3. 设备状态

  ┌───────────────────────────────────────────────────────────────────────────┐
  │  设备状态: 未入网 (Unprovisioned)                             │
  │  ─────────────────────────────────────────────────────────────────────│
  │  • Mesh 单播地址: 0x0000 (MESH_UNASSIGNED_ADDR)                │
  │  • 网络密钥: 未设置                                               │
  │  • 角色: 未分配角色 (Relay/Friend/LPN 未使能)                   │
  │  • 状态: 等待 Provisioner 扫描和配网                          │
  │                                                                  │
  │  广告状态:                                                        │
  │  • Network Beacon: 关闭                                          │
  │  • Proxy 广告: 开启 (60 秒超时)                                │
  │  • 广告间隔: 20ms                                               │
  │  • 设备名称: "MeshServer" (cfg.h)                                │
  │  • BLE MAC: 0xFC42654220 (cfg.h)                                │
  └──────────────────────────────────────────────────────────────────────────┘

  ---
  二、Provisioner 扫描阶段

  1. Provisioner 发现设备

  手机 App / 网关
    └─ BLE 扫描
        └─ 发现设备广播
             ├─ 设备名称: "MeshServer"
             ├─ 设备 MAC: 0xFC42654220
             └─ 设备 UUID: 基于 Genie 三元组生成
                └─ genie_gen_uuid(uuid, &genie_triple)
                    ├─ pid: 0x006653de
                    ├─ key: 16 字节产品密钥
                    └─ mac: 0xFC4265491E2E

  2. Provisioner 连接设备

  Provisioner
    └─ GATT 连接
        ├─ 发现 Provisioning Service
        └─ 连接设备 (BLE 外设角色)

  ---
  三、Provisioning 认证阶段

  1. Provisioning 参数请求 (app_mesh_server.c:ms_prov_data_req_ind)

  MS_PROV_PARAM_DATA_REQ_IND
    └─ genie_prov_param_conf()  // 配置 Provisioning 参数
        ├─ 生成设备 UUID (基于三元组)
        ├─ static_oob = 0x01        // 支持 Static OOB 认证
        ├─ uri_hash = 0x0000
        └─ oob_info = 0x00          // 无 OOB 信息
        └─ 返回: ms_prov_param_rsp(&param)

  2. OOB 认证数据请求

  MS_PROV_AUTH_DATA_REQ_IND
    └─ genie_oob_auth_conf(oob_auth)  // 计算 OOB 认证数据
        └─ genie_calc_auth(oob_auth, &genie_triple)
        └─ 返回: ms_prov_auth_rsp(true, 16, oob_auth)

  3. 认证流程

  ┌───────────────────────────────────────────────────────────────────────────┐
  │                  Provisioning 认证流程                           │
  │                                                                │
  │  ┌─────────────────────────────────────────────────────────────────┐      │
  │  │              Provisioner (手机)                 │      │
  │  │                                                              │
  │  │ 1. 获取设备 UUID (16 字节)                                   │
  │  │                                                              │
  │  │ 2. 计算认证值                                                   │
  │  │   - 使用产品密钥加密                                          │
  │  │   - 或使用 Static OOB 值                                   │
  │  │                                                              │
  │  │ 3. 验证设备 (App 输入或扫描二维码)                              │
  │  │   - 输入 6 位数或扫描设备标签                              │
  │  │   - 与设备计算值比较                                         │
  │  │                                                              │
  │  │ 4. 认证成功 → 开始分配网络资源                               │
  │  └─────────────────────────────────────────────────────────────────┘      │
  └──────────────────────────────────────────────────────────────────────────┘

  ---
  四、网络资源分配阶段

  1. 分配 Mesh 单播地址

  Provisioner
    └─ 网络管理
        ├─ 从地址池分配单播地址
        │   └─ 分配给 Server 设备: 0x0001
        │       (用户指定或自动分配)
        │
        ├─ 生成网络密钥 (128-bit)
        │
        └─ 生成网络 IV (64-bit)

  2. 设备组成数据配置

  ┌───────────────────────────────────────────────────────────────────────────┐
  │              设备组成数据 (Composition Data)                    │
  │                                                                │
  │  Element 0 (主元素):                                            │
  │  ├─ Location: 0x0000                                            │
  │  ├─ Sig Models:                                                   │
  │  │   ├─ Generic OnOff Server (0x1000)                           │
  │  │   ├─ Generic Power OnOff Server (0x1200)                      │
  │  │   ├─ Generic Power Level Server (0x1203)                      │
  │  │   ├─ Lighting Lightness Server (0x1300)                      │
  │  │   ├─ Generic Level Server (自动关联)                           │
  │  │   ├─ Generic Default Transition Time Server (0x1204)             │
  │  │   ├─ Lighting Lightness Setup Server (0x1301)                  │
  │  │   ├─ Generic OnOff Setup Server (0x1202)                      │
  │  │   ├─ Lighting CTL Server (0x1303)                             │
  │  │   │   ├─ Light Lightness Server (自动关联)                  │
  │  │   │   └─ CTL Temperature Server (0x1306)                   │
  │  │   ├─ Generic Power Level Server (自动关联)                  │
  │  │   ├─ Generic Default Transition Time Server (自动关联)             │
  │  │   ├─ Generic OnOff Setup Server (自动关联)                  │
  │  │   └─ Lighting CTL Setup Server (0x1304)                     │
  │  │       ├─ CTL Temperature Setup Server (0x130B)            │
  │  │       └─ Generic Power OnOff Setup Server (自动关联)      │
  │  │   ├─ Lighting HSL Server (0x1307)                             │
  │  │   │   ├─ Light Lightness Server (自动关联)                  │
  │  │   │   ├─ Light Hue Server (0x130A)                     │
  │  │   │   ├─ Light Saturation Server (0x130B)              │
  │  │   │   ├─ Generic Level Server (自动关联)                   │
  │  │   │   ├─ Generic Default Transition Time Server (自动关联)     │
  │  │   │   ├─ Generic OnOff Setup Server (自动关联)                  │
  │  │   │   └─ Lighting HSL Setup Server (0x1308)              │
  │  │       ├─ Light Lightness Setup Server (自动关联)          │
  │  │       ├─ Light Hue Setup Server (0x130C)                │
  │  │       └─ Light Saturation Setup Server (0x130D)          │
  │  │           ├─ Generic Level Server (自动关联)               │
  │  │           ├─ Generic Default Transition Time Server (自动关联)   │
  │  │           └─ Generic OnOff Setup Server (自动关联)          │
  │  └─ Features: Relay/Friend/LPN 根据配置启用                 │
  └──────────────────────────────────────────────────────────────────────────┘

  3. 生成网络密钥

  ┌───────────────────────────────────────────────────────────────────────────┐
  │                网络密钥生成                                │
  │                                                                │
  │  Network Key (128-bit)                                          │
  │  - 用于加密 Mesh 网络消息                                         │
  │  - 所有网络节点共享                                              │
  │  - 存储在设备 Flash 中                                          │
  │                                                                │
  │ Network IV (64-bit)                                              │
  │  - 网络初始化向量                                                 │
  │  - 每个节点独立                                                 │
  │  - 用于消息加密                                                     │
  └──────────────────────────────────────────────────────────────────────────┘

  ---
  五、配置数据下发阶段

  1. AppKey 下发

  Provisioner
    └─ 发送 AppKey (128-bit)
        └─ ms_prov_app_key_set(APPKEY_DATA)
            └─ 128-bit 网络密钥

  2. 网络密钥下发

  Provisioner
    └─ 发送 Network Key (128-bit)
        └─ ms_prov_net_key_set(NETKEY_DATA)
            └─ 128-bit 网络密钥

  3. 单播地址下发

  Provisioner
    └─ 发送 Unicast Address (16-bit)
        └─ ms_prov_unicast_addr_set(UNICAST_DATA)
            └─ 0x0001 (分配给设备的地址)

  4. 设备组成数据下发

  Provisioner
    └─ 发送 Composition Data
        └─ 包含所有已注册的模型和配置

  ---
  六、数据存储阶段

  1. Server 设备存储配置

  Server (app_mesh_server.c)
    └─ 接收并保存到 Flash:
        ├─ Mesh 单播地址: 0x0001
        ├─ 网络密钥 (Network Key)
        ├─ 网络密钥 (AppKey)
        ├─ 网络 IV (Network IV)
        ├─ 设备组成数据
        ├─ IV Index
        └─ 网络状态

  ---
  七、配网完成阶段

  1. 状态通知 (app_mesh_server.c:ms_state_ind)

  MS_PROV_STATE_IND
    └─ MS_PROV_SUCCEED (配网成功)
        └─ debug 输出:
            └─ "PROV_STATE: <成功状态值>"

  2. 设备状态更新

  ┌───────────────────────────────────────────────────────────────────────────┐
  │              设备状态: 已入网 (Provisioned)                    │
  │  ─────────────────────────────────────────────────────────────────────│
  │  • Mesh 单播地址: 0x0001                                      │
  │  • 网络密钥: 已设置                                             │
  │  • 角色: 已分配                                                 │
  │  • 状态: 正常工作，可接收控制命令                               │
  │                                                                  │
  │  广告状态:                                                        │
  │  • Network Beacon: 开启 (广播网络信息)                            │
  │  • Proxy 广告: 开启/可控                                         │
  │  • 广告间隔: 20ms                                               │
  └──────────────────────────────────────────────────────────────────────────┘

  ---
  八、正常工作阶段

  1. 设备在 Mesh 网络中

  ┌───────────────────────────────────────────────────────────────────────────┐
  │                    BLE Mesh Network                                 │
  │                                                                │
  │  ┌─────────────────────────────────────────────────────────────┐        │
  │  │         Server (0x0001)         │      │
  │  │         ┌──────────┐               │        │
  │  │         │ 4 Server 模型             │        │
  │  │         │          │               │        │
  │  │         │          │               │        │
  │  └──────────┘               │        │
  └──────────────────────────────────────────────────────────────────┘
             ↓ Network Layer 路由
  ┌─────────────────────────────────────────────────────────────┐
  │         Client (0x0002)          │
  │         ┌──────────┐               │
  │  │         │ 4 Client 模型             │        │
  │  │         │          │               │        │
  │  └──────────┘               │
  └──────────────────────────────────────────────────────────┘
             ↓ Mesh Model Layer
           其他节点 (0x0003, 0x0004...)

  2. 接收控制命令 (app_mesh_server.c:mm_ind)

  MM_SRV_STATE_UPD_IND
    ├─ MM_STATE_GEN_ONOFF          // Generic OnOff 状态
    │   ├─ value = 0 → GPIO08 高电平 → LED0 关
    │   └─ value = 1 → GPIO08 低电平 → LED0 开
    │
    ├─ MM_STATE_LIGHT_LN          // Light Lightness 亮度
    │   ├─ value = 0x0000 → LED0 关
    │   ├─ value = 0xFFFF → LED0 开
    │   └─ value = 0x7FFF → LED0 半亮 (可扩展 PWM)
    │
    ├─ MM_STATE_LIGHT_CTL_LN      // CTL 亮度
    │   └─ 同 Light Lightness 处理
    │
    ├─ MM_STATE_LIGHT_CTL_TEMP    // CTL 色温
    │   └─ value = 0x0320 (800K) ~ 0x4E20 (20000K)
    │
    └─ MM_STATE_LIGHT_HSL_LN/HUE/SAT  // HSL 亮度/色调/饱和度
        └─ 处理颜色参数 (需要 RGB LED)

  3. 发送状态通知

  MM_SRV_STATE_UPD_IND
    └─ 收到控制命令后
        └─ 向网络广播状态更新消息
            └─ 其他节点可订阅收到

  4. 按键功能 (proc.c)

  KEY0 按键点击
    └─ ms_proxy_ctrl(MS_PROXY_ADV_NODE_START)
        └─ 重启 Proxy 广告 (60 秒)
            └─ 允许手机通过 GATT 连接配置设备

  ---
  九、完整时序图

  ┌───────────────────────────────────────────────────────────────────────────────────┐
  │                    Mesh Network Provisioning 时序                     │
  │                                                                │
  │  时间轴 ↓                                                         │
  │                                                                │
  │  [启动]    Server 上电，发送 Unprovisioned 广告                   │
  │    │           • 地址: 0x0000                                      │
  │    │           • 包含设备 UUID                                    │
  │    │           • 包含设备组成数据                                  │
  │    │                                                              │
  │  [T+100ms]  Provisioner 扫描发现设备                         │
  │    │                                                              │
  │  [T+200ms]  Provisioner GATT 连接                               │
  │    │                                                              │
  │  [T+500ms]  Provisioner 获取 UUID                               │
  │    │                                                              │
  │  [T+800ms]  OOB 认证完成                                     │
  │    │                                                              │
  │  [T+1s]    Provisioner 分配地址 0x0001                           │
  │    │                                                              │
  │  [T+1.5s]  发送 Network Key (128-bit)                         │
  │    │                                                              │
  │  [T+2s]    发送 Unicast Addr (16-bit: 0x0001)                 │
  │    │                                                              │
  │  [T+2.5s]  发送 AppKey (128-bit)                                │
  │    │                                                              │
  │  [T+3s]    发送 Composition Data                                 │
  │    │                                                              │
  │  [T+4s]    Server 存储配置到 Flash                             │
  │    │                                                              │
  │  [T+5s]    Provisioning 成功通知                            │
  │    │           • MS_PROV_SUCCEED                                   │
  │    │                                                              │
  │  [T+6s]    Server 停止 Unprovisioned 广告                        │
  │    │                                                              │
  │  [T+7s]    Server 开始发送 Network Beacon                       │
  │    │           • 携带网络信息                                         │
  │    │           • 广播 Mesh 单播地址: 0x0001                           │
  │    │                                                              │
  │  [T+8s]    Server 正式加入 Mesh 网络                              │
  │    │           • 开始路由和转发消息                                   │
  │    │           • 可接收控制命令                                       │
  │    │           • 可发送状态更新                                       │
  │    │                                                              │
  │           ↓ 正常工作状态                                         │
  │                                                              │
  │    • Client 发送 OnOff Set 命令 → Server 接收 → LED0 状态变化      │
  │    • Client 发送 Lightness Set 命令 → Server 接收 → 亮度调节        │
  │    • Client 发送 CTL Set 命令 → Server 接收 → 亮度+色温调节        │
  │    • Client 发送 HSL Set 命令 → Server 接收 → 颜色调节            │
  └──────────────────────────────────────────────────────────────────────────┘