# T777 · G13 隔离探针的设备拒装与授权签名

2026-11-21 · G20 · P5 · architect · 离线归因和实施合同。

**T778 优先交付使用原授权 `com.example.helloworld` 的独立诊断构建，沿已有授权签包与覆盖安装链验证。** 该诊断包会和同名应用共享安装身份，设备只能在可恢复的串行窗口切换。诊断入口、GL owner 和资源仍按 T764 隔离。需要两应用并存或无法安全恢复时，使用为 `com.example.helloworld.g13visibility` 正式签发的 profile。 [T770 决策原件](test-report-T770-v1.md) · [T764 隔离合同](architecture-T764-v1.md)

## 首次归因

| 阶段 | 证据及可证范围 |
| --- | --- |
| T770 v1，本地签包前 | T767 探针真实包内 bundle 为 `com.example.helloworld.g13visibility`，unsigned SHA `bae482dc…`，`libgrs_g13_visibility.so` SHA `e6def2a7…`、BuildId `88b370c7c01d423a98c4a335f024b658e41592cf`。现有 profile 授权主 bundle，`sign.py` 的 bundle 断言拒绝符合预期，未发生安装。离线 fixture 836 行属于绑定表验证。 |
| T770 v2，本地签包和设备安装 | 本地签名命令返回 0，设备实际报告 **9568404 / CODE_SIGNATURE_DELIVERY_FILE_FAILED**。错误所在域为 BMS 交付代码签名 profile，发生在启动探针前。归档包含 T770 报告和转录摘要，缺同次 BMS/installd 原始日志、系统返回及设备证书细分记录。官方说明该码还覆盖文件路径和权限等交付错误。自签 profile 的设备授权未得到证实，现有证据不能唯一确定为某个 CA 被拒，也不能推断所有新 bundle 均被设备拒绝。 |
| 现有授权对照 | T770 报告记录 T738-rx 携原 profile 安装成功，本地签包回执为 `com.example.helloworld`、`verifyApp=PASS`，签包 SHA `73027d13…`。对照支持这台设备存在可用的正常主 bundle 签装链。它不预先证明 T778 的 probe 能覆盖安装、启动或通过可见性。 |

T770 当前停在安装域，固定 ordinal、WORK、map、PACK 等设备证据继续未执行。FTZ 与 T730 的完整根资格独立保持。原 9568404 正式错误、相关 BMS 行与字段按权限可用范围归档，请勿将本地 RC=0 当设备信任。 [T770 v1](../test/reports/t491/t760/v1-t770-sign-blocked.json) · [T770 v2](../test/reports/t491/t760/v2-t770-selfsign-verified.json) · [T738 授权签包](../test/reports/t257/T738-rx.json) · [华为 bm 错误码和安装规则](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/bm-tool/)

## T778 重打包和签名接口

1. **保构建隔离。** 从 T767 最终诊断实现单独构建，主 bundle 授权仅在显式诊断 variant 选择。在 fresh workspace 由 Hvigor 正常生成 unsigned HAP，独立入口 `EntryAbility → G13VisibilityAbility → pages/G13Visibility`、`libgrs_g13_visibility.so`、诊断 NAPI 和固定 case 允许表。默认产品构建继续使用自己的入口和 `libgrs_g13.so`。不在主代码中默认注册诊断 route、动态通用 op、产品 monitor/commit/publish/Swap；无产品 cache 导入和写回。两包的相同 bundle 是预期安装冲突，不能以 bundle 名区别诊断与产品。
2. **绑定真实产物。** 生成专属 T778 编译回执，记录 build variant、源码提交、实际 bundle、module/mainElement、`srcEntry`、versionCode、HAP SHA、打包 ELF SHA/BuildId、12 份生成 shader 的 SHA/bytes 和生产/default 负控。若原 native 字节完整相同可报同 BuildId，应用身份发生重打包仍需新 HAP SHA；有 native 差异须重新核原因和本次 BuildId。保持 T767 的 Q5/6、Q410/411 gate/copy、Q26/27、Q431/432、Q814、Q817/818 等实际 pass、原字/屏障、资源与退役。每个 shader 完整输入 ≤49,152 B。原 T767 manifest、HAP 和回执保作不可变历史。 [T767 构建回执](</D:/hmprojects/HelloWorld-T767/docs/receipt-T767-compile-v1.json>)
3. **按显式身份更新判定。** T767 构建验证器目前把后缀和不同 bundle 作为常量检查，T769 设备 reader 读取固定 T767 诊断回执。T778 采用显式选择的同主 bundle 诊断 variant，把“两个包是否同名”变为按此 variant 的精确预期，同时加强主包相同名下的 ability/page/ABC/DSO/export/marker 负控。T769 传本轮编译回执或等价显式参数，不覆盖 T767 回执或放宽签名、SHA、bundle、映射 BuildId、PID/start/attempt。若签名工具输出 T257 的嵌套 `input.sha256/signed.sha256`，tester 按原收据生成受核对的字段映射，T769 不靠手填哈希判真机。独立拒绝旧 probe 包、默认产品包和 fixtureOnly 合成文件。 [T767 构建验证](</D:/hmprojects/HelloWorld-T767/tools/g13/verify_visibility_build.py>) · [T769 设备判定](</D:/hmprojects/HelloWorld-T767/test/t757/verify_visibility.py>)
4. **授权签包。** tester 核实际包 manifest 和正式 profile 的 bundle、appIdentifier/签名连续性、配置类型、证书链、有效期和已授权目标设备。原主 bundle profile 以已有批准原件和配套签名链使用，按原工具正常签包并验签。记录 profile、签前后包和 payload SHA、实际 `verifyApp` 与签名身份关联，收据只含必要指纹。必须走设备原验证，使用不同 profile 的既往本地签名结果无安装信用。若必须用诊断后缀，请由授权签名负责人从正式渠道取得该 bundle/app identifier 与目标设备匹配的有效调试 profile 及其配套证书。当前缺少它不阻碍主 bundle 诊断变体的离线开发。 [华为调试签名与设备授权](https://developer.huawei.com/consumer/en/doc/harmonyos-faqs-V2/faq-debugging-and-running-0000001122066466-V2/)

## 设备窗口、备份与恢复

- **进入窗口前。** effector 先完成正在用同 bundle 的任务并保进程原件。记录当前设备、用户、安装 bundle/module/versionCode、产品映射 BuildId、可恢复的已签原包及 SHA。确认诊断与恢复包的签名一致性和双向版本安装条件。用已有授权接口备份本轮需要保留的应用配置、报告与缓存或数据范围，记清单、SHA、可恢复范围及无法备份项。探针证据写独立目录或唯一文件名，不打开产品物理状态和 T475 cache。目标窗口无法安全恢复就提交具体缺项，换可恢复测试设备或使用正式后缀方案。
- **覆盖安装及运行。** 仅在上述条件成立时用正常保留数据的覆盖安装流程，完整记录安装原返回。拒绝时归安装阶段，禁止通过卸载、清数据、修改设备信任或关闭校验获取成功。装后核实际 bundle/version、诊断页面和输出 schema、映射 DSO BuildId 与 HAP 对应，才做 T770 原定的离屏逐项采证。每轮留原 trace 和退役证据，T769 只按覆盖域判。产品数值门、首 Swap 和 G20 暖启另计。
- **恢复产品。** 结束诊断后先导出其原件；用预先核实的已签产品包走正常恢复安装，核 device 安装成功、默认页面入口、实际产品映射 BuildId、诊断入口不可达及保留数据的清单。恢复失败立即保留原错误与现场，交回 owner 和签名负责人，设备不交给下一条需使用主 bundle 的测试。整个窗口串行，仅 effector 上机，未经确认不执行有数据损失的处理。 [HDC 已授权应用沙箱备份](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/ide-device-file-explorer/)

本卡完成工程裁定和离线原件复核。T778 负责开发和编译，tester 负责实际授权签包并更新证据判定，effector 继续原 T770 串行。**G20 约 33%（4/12）**。
