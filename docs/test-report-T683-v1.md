# T683 单沙配对摘要离线交付

2026-11-20。独立重新核对 T681 的 600 个真实生成文件、成功构建日志、ABC 与 ARM64 ELF。复用 unsigned 176461c3…，T683 签包 verifyApp 通过。路径和 SHA 见 test/reports/t683/handoff.json 及 test/reports/t257/T683.json。

pairing_units.cjs 使用实际生成模板。早期 owner=null 合法。同次 authority 只读一次。验证 gate/raster/FRAME serial 分列、不同 serial 不误判同期、0/false/null 的区别、128 字节身份裁减、256 事件上限、authority 截断和读取前后状态。check_pairing.py 可检查实际 prepare-cold 原件。合成读者正反控共 11 项，含 UTF8 14336/16384 精确边界、截断身份、overflow 和 scope 不符拒绝精确 join。4096 为摘要目标，允许显式超目标但必须接受外层硬上限。

T592 effector 使用 T683 签包，同一次实际 prepare-cold 先运行 python test/t683/check_pairing.py <prepare-cold.json>。FULL 且 eligible 才进入原 test/t671/verify_capture.py 联核 reserve witness、同 scope HOLD、严格 PNG。SUMMARY_ONLY 原件可定位问题，禁止用来补 owner join。设备未运行。
