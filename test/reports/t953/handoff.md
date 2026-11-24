# T953 · 裁槽顺序、流场交接与 Demo 粒子离线复核

2026-11-24 · tester · Windows PC 离线。执行 `python test/t953/check.py selftest`，见 [机器收据](offline.json)。

- **目标9。** 独立读 T949 双态归档，确认 SOURCE ACTUAL 等于后续 Q BASE/CURRENT，诊断时间戳与同一业务 fence/代匹配，业务 vk=2、tailRead=false、无接受根。从 WORK byte 1166976+64 读取真正的 control[16]，晚尾值为 2404。BASE/ACTUAL 的 word 16 数值 1→2 对应 state header 的 lastNonce，不能用来证明逐槽计数。迟到尾 J1、EVENT 260/PATH 1 已核。已交计数保序、同代 latch、安全空 extent、拒绝前缀和独立 replay 证书。J0 全拒绝正例保持主动状态，伪提升负控拒绝。
- **目标13/14。** Split F02 原件有黏性/压力两个明确成功 EXIT，本次 stdout 未见 remainder 成功 EXIT。F02 因 GRANT_DEADLINE 拒绝，同 fence 晚读作诊断。GridView 判据核当前 typed 接受及 250 ms、坐标/单位、MAC 范围、动态压力与势及参考压、从壁面速度和近壁流速复算切应力。错代、迟到、错单位与加载错误均拒。此处为固定规则 MAC 交接夹具，实际可信流场和 T611 接入仍待相应工件。
- **Demo。** 从完整归档 SOURCE 前缀按 1/1/1 和 nonce 拆出四条 BIRTH，合计 504470 Q20，命令出生点为 (64.14501159, 32)。ordinal 5 是旧粒 id5/gen1/origin2 的 Q MOVE。现有同根 ROI 在床附近，未覆盖出生位置。离线 motion 门按粒子 id/代/origin，核同根 lease/image/fence、RGBA 和边框 mask、接受角、原 MOVE 的 0→65536 干沙步、速度增量、定点位置积分与屏幕重力。正例为合成，同粒实测轨迹和出生像素仍需补采。

独立入口为 `schedule|gridview|motion <JSON>`，字段示例见 `fixtures()`。Python 编译、15 项负控、T948 与 T866 回归通过。本轮未执行产品或设备测试。
