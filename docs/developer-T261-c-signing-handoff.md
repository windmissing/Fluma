# T261 明水 C 修复签包交接

2026-10-29，developer；不使用真机。T258 原共享输出包已被后续构建覆盖，扫描现有 unsigned 候选未找到 delivery.json 固定 SHA `2d00c792eaa9579ea4f6d3a342f435dfba5d01e5a5bc64950ea2701d70b3f204`。因此从 T258 已提交的固定 Git 状态 `408c457` 在 `.codex/local-test/t261-t258-source` 建立隔离源码副本，不使用当前共享输出。`napi_bridge.cpp` 与 `clear_water_identity.h` 的隔离原字节 SHA 分别等于 T258 delivery.json 中 `db4cd822b97d7c12a0b14b87a00a615dc7bca13f93bd927a7f5f0956e2fbed52`、`61b2f1ac5427c5d9e307c92f5853ba7532364dfe54595ef763da85fcf74ba777`。

隔离 hvigor native/ArkTS/PackageHap **BUILD SUCCESSFUL**，零错误。新 unsigned HAP SHA `794d740fb3f379b6f61152ff20544094720aeac78861e1778571f77096987118`，与 T258 当时包 SHA 不同；交接身份明确为**固定 T258 提交源码重建包**，不把它误称原 unsigned 的逐字节副本。按 T257 已授权脚本签名，输出 [T261-signed.hap](D:/hmprojects/HelloWorld/.codex/local-test/t257-sign/T261/T261-signed.hap)，SHA `419f36459af356276e3485b712d0b1611a57b72b738c1c7b29d414a52040eaf9`。`verify-profile`、`verify-app`、同 bundle/profile 公钥链、签名前后 ZIP payload entry hashes 保持、原签名配置不变均通过。逐项退出码/摘要见 `test/reports/t257/T261.json`，构建日志见 `test/reports/t261/package.log.gz`；凭据/设备资料未入日志或仓库。

此卡签名工程 **结束待验收**，供 T262 独立真机复测；签名不证明 C 六层绘制或设备通过。渲染仍只允许 XComponent+shader，无 Canvas 回退。
