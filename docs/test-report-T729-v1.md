# T729 G13 编译对照离线判定与签包设施

离线设施已交付，T728 实物签包待到件。设备与产品首帧均为 NOT_RUN。任务启动时 T728 仍在开发，检查到 BOOT 和五份 OP 探索源码，未找到正式同父两臂 HAP 或完整角色及 BuildId 收据。已有 T720 历史单程序包只用于实物验证器回归，真实包内 BuildId `428dea61036dfa0fb9297eabe258aeabe3d63495`，三份着色器的 SHA 与 ELF 内编译源字节相符，四项错件负控通过。[历史工件联核](../test/reports/t729/t720-reference.json)

## T730 离线入口

| 输入 | 命令与判定 |
| --- | --- |
| 单轮 T727 测量 | `python test/t729/measure.py run MEASUREMENT.json`。检查原件路径/SHA、十进制宽整数、同调用序与作用域。原调用优先按 RETURNED 计时，开放调用保截尾下界。 |
| 冷暖四臂 | `python test/t729/measure.py pair DEPLOYMENT.json A.json B.json C.json D.json`。配对独立进程、设备、驱动、配置/输入/种子、包内角色与实际映射 ELF。查冷项目缓存清单和同角色暖缓存 key、原持久化、真实载入和反射。记录四轮全分母、首 Swap 成功数和截尾下界。 |
| 四页与 oracle | `python test/t729/measure.py pages DIR ROUND` 复用 T672/T721，锚相同以 ANCHORS_EQUAL_NOT_ATOMIC 登记。T631 判据沿用原合同，残差侧车绑定根 token、scope、config/input/seed SHA。完整物理判定需要相同身份的 T631 用例结果、冷档原件及经现有 reader 核验的档案收据。 |
| 首有效 Swap | 使用实际调用原返回、同 owner、同 run/context/session/许可、同已接受 root token。撤权、旧 token、跨代、仅 READY、截尾都无法形成首 Swap 成功。 |

每条原件相对本测量文件所在目录存放。缓存清单的 `entries` 列出对应项目缓存内容，冷轮为空。配对要求 `loaded-module` 原件中有 pid、processStartId、loadedModuleBuildId 与 loadedModuleSha，另保原始设备映射来源。可选 `oracle` 记录 T631 标识和原始数值字段，`field-oracle` 还引用 `rawArchiveOriginalId` 与 `archiveReceiptOriginalId`。

## 两臂到件后签包

`python test/t729/sign_pair.py MONOLITH.json SPLIT.json DEPLOYMENT.json --verify-only` 可独立生成预检回执。用另一输出路径并去掉 `--verify-only` 执行真实签包。每份输入是对 T728 原收据的只读适配，包含 variant、隔离项目目录 project、parentReceipt/parentSHA256、项目相对 unsigned 路径及 unsignedSHA256、nativeSHA256、buildId 和 roles。角色登记项目相对 path、kind（compute/graphics）、真实编译字节的 sha256/bytes、opcodes 和 reworked。由 C++ 字面量编译前统一 CRLF 到 LF 时，显式登记 `sourceNewlines=CPP_TRANSLATION_LF` 和原始 `sourceSHA256`。预检核同父、不同臂身份、真实 HAP/ELF 内源码、相同图形源和 opcode 覆盖、Boot 64/65/66 独立闭包及拆分重构单元 48 KiB 工程门。实际签包复用 T257，核签前后 ELF，输出 T730 所需签包路径、SHA、BuildId 和各角色。

`python test/t729/units.py` 与 `python test/t729/sign_units.py` 均通过。前者覆盖十项测量拒绝、暖二进制错配、四页宽票据与锚错配，后者覆盖合成包正例和五项错件拒绝。Python 编译检查通过。[测量用例](../test/reports/t729/measurement-units.json) [签包入口用例](../test/reports/t729/signature-units.json)

续办前置为 T728 正式 MONOLITH/SPLIT 两份构建目录、unsigned HAP、各自角色源和实际 BuildId、同父原回执。tester 到件后预检并签包，effector 再消费 T730 设备输入。当前暂无签名或部署收据。
