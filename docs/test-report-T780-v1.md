# T780 DM 与 RX 工件和生成 GLSL 校核

离线核验通过。重跑 10 份实际包内 compute GLSL 静态编译全部通过。覆盖 T718 DM、T720 RX 历史版本，T748/T771 DM 与 T763 RX 当前手头研究臂，以及 T771 DM、T763 RX 默认臂。各版本独立登记 HAP/ELF 的 SHA-256、BuildId、各源码 SHA 和命令。T670/T718/T748/T771 DM 的 ARM64 独立 `g9_dm_differential` 确认依赖相应 `libgrs_dm_t647.so`，同时保全系统 DT_NEEDED 原件。RX 页面加载历史以 T766 BOOT 原件为依据，PID、BuildId 和 BOOT 源哈希与 T763 原件交叉核对。本轮无设备加载。[原件与检查](../test/reports/t780/offline.json)

| 工件 | BuildId |
| --- | --- |
| T771 DM SOURCE 驱动 | `71b03f560e78806ca43c29eea161ab50a6ec73c1` |
| T771 DM SOURCE 包内 ELF | `86e14c1a342d257ab3ddbaa87662332be4d782ea` |
| T771 DM DEFAULT 包内 ELF | `afe8968055bd57cd3271677b6fb40a729f53893d` |
| T763 RX SOURCE 包内 ELF | `4bdd76ddf02618f9579836d437aeca99d9039f63` |
| T763 RX DEFAULT 包内 ELF | `0db444d0a424cdafa9f08125c104b3db36b18396` |

同作用域 slot 重声明由 glslang 复现 `ERROR: 0:4: 'slot' : redefinition`，分离作用域正控通过。DM 与 RX 生成函数的 clearSlot、scanSlot、selectedSlot 及 45 个 qualifier 实参的 canonical 行均核对。恢复 readonly 或 coherent 的原直接实参时，专项扫描器均检出。桌面 glslang 对直接实参与值拷贝两种最小 shader 都返回 0，旧设备 S0001 仅保原诊断，设备行为在本轮未验证。

发现两个历史工作路径已变更，均从 SHA 一致的原件恢复了校核。T720-RX 未签包路径已变，用原签包与原签名收据锁定旧源。T670 的旧 .cxx 入口路径已变，用同次构建的 intermediates 原 ELF 匹配收据。两个差异均已留存。

复现使用 `python test/t780/check.py`。T771 使用工作树 `D:/hmprojects/HelloWorld-T771` 的最终收据。此轮只读包和原件，无签包或部署。
