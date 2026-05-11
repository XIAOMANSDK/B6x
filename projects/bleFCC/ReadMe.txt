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