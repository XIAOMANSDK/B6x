RF FCC测试

uart指令说明
A0    : FCC初始化配置
A1    : FCC Stop

B0 xx : RF Tx单载波模式(xx: 00 ~ 27)(00 ~ 27对应tx频率2402 ~ 2480)
B1 xx : RF Rx单载波模式(xx: 00 ~ 27)

C0 xx : RF Tx调制数据模式(xx: 00 ~ 27)(00 ~ 27对应tx频率2402 ~ 2480)
C1 xx : RF Rx调制数据模式(xx: 00 ~ 27)

D0    : RF Tx单载波跳频模式, 跳频间隔200ms, 从2402MHz~2480MHz

E0 xx : 在RF单载波模式下调晶振频偏(xx: 00 ~ 3F)
E1    : 获取当前晶振频偏配置值

F0 xx：设置Tx功率等级（xx:00~0f）

F1xx：设置RF->PLL_DAC_TAB0.PLL_DAC_ADJ00 （2402）用于发射载数据需要控制1M带宽 （xx：00~3F）
F2xx ：设置RF->PLL_DAC_TAB1.PLL_DAC_ADJ12 （2440）用于发射载数据需要控制1M带宽 （xx：00~3F）
F3xx ：设置RF->PLL_DAC_TAB3.PLL_DAC_ADJ05 （2480）用于发射载数据需要控制1M带宽（xx：00~3F）

F4xx：设置vco_adj，用于接收杂散过高进行调试，一般要求是-54dB以下（xx：00~07）