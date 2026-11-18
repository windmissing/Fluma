# T607 纯流场工程落地复盘

2026-11-18，architect，工程交付而非产品验证。主稿 [T607工程闭包](architecture-T607-g13-engineering-v1.md)，配套layout/dispatch/math/engineering四表及有限生成检查脚本。

选择独立G13模块和最小构建依赖，复用缓存、资源账本、cold shared broker及新shader整帧机制，避免把旧M1/G12数值假设、目录或NotImplemented writer带入纯流场。T605已明定MAC/FP32速度/Q32压力/黏性与MG及原门，T607不另换算法，不让实现猜字段和实际opcode；完整820行提交、3bank/2trace、四binding阶段重绑和冷场真实原件都显式落表。

关键接缝是persistent只能单workgroup同步；Coriolis pair唯一owner；压力证书用真实整数残差和RHS扰动，actual FP32散度独立；物理压力误差不由线性残差代替。首次成功trial就是权威，重放重现失败前缀且全场比较，取消/晚归来/未收敛不提交时间和动画。SOURCE零模拟时间，UI30fps与native配额/freshness分开。

数学函数不能仅凭highp或未量化builtin冒精度资格。本轮固定GPU软件数学/常数归约路线及有理生成证据，不将常数截断界冒实际shader精度；该backend的实现、单dispatch最坏耗时、820次提交和真实推进比仍需独立GPU证据。F05闭盒压力分裂误差保为明确风险，失败回architect分设计，不临场放宽门。

此次只做布局/范围/计数/常数的有限静态核对，不运行流体演化、产品构建/测试或真机。T608仅开发编译，已有签名部署和独立GPU/观测验证归tester/effector；目标14须等实际13可信门，不以报告交付当产品PASS。CPU停投、仅XComponent+shader、保1–11及10暂停，旧水线暂缓不回退源码。
