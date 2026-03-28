# 代码规范

MISRA C:2023 适配 ARM Cortex-M0+。

## 命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 文件/变量 | 小写+下划线 | `uart.c`, `g_rx_buffer` |
| 函数 | 模块_功能 | `uart_init()` |
| 宏 | 大写+下划线 | `UART_BUFFER_SIZE` |
| 类型 | 名称_t | `uart_config_t` |

## 声明

- 使用 `<stdint.h>` 固定宽度类型
- 变量必须初始化
- 只读参数 `const`，内部函数 `static`

## 函数

- 单一职责，<150 行，≤4 参数（多用结构体）
- 必须检查返回值

## MISRA 安全

| 禁止 | 必须 |
|------|------|
| malloc/free, 递归, goto | 边界检查, 显式转换, 宏替代魔法数 |

## 格式

- 缩进 4 空格，行宽 100，K&R 大括号

## 注释 (Doxygen)

`@file` `@brief` `@param[in/out]` `@return` `@note`

## 嵌入式

- ISR: 简短非阻塞，`volatile`
- 位操作: `BIT_SET`/`BIT_CLEAR`
- 低功耗: `__WFI()`
