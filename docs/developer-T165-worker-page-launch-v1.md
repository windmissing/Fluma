# T165 页面运行期 Worker 创建候选交付

日期：2026-10-27。任务：6ae02ee1e4b06ee35751bd8b。不使用真机。

## 修改与证据边界

仅修改 `test/p013_compute` 隔离模板及生成器；主工程现役 Worker 不变。
原 EntryAbility 未等待 loadContent，直接解包配置/写 shader/创建 Worker。
现 EntryAbility `await loadContent('pages/Index')` 后配置独立的 T165PageLauncher，
Index `onPageShow` 通知页面就绪。两事件无论先后，只有同时到达才启动一次；
loadContent 拒绝、配置无效、资源读取失败均明确记录 FAULT，不静默重试或回退。
Worker 路径、配置身份、路由三开关、selector/native 行为和物理生成白名单不变。
生成器只新增 launcher 模板复制，仍输出 productionTouched:false。

这是依照 T163 排除项实施的**时机候选修复**。T161 原包和本包均有
T159PhysicsWorker record 与 func_main_0；不存在“本次补齐缺失 record”的证据。
页面时机是否解释设备旧式 abc 查找仍未实证，设备结论交 T164。

## 离线验证

- `node test/run_t165_worker_launch_units.cjs`：7/7。覆盖两个生命周期顺序、延迟/失败 loadContent、重复回调、冷配置拒绝、资源故障、三路配置与生成白名单。
- `node test/p013_compute/tools/run_t159_s2.cjs`：三路各 12/12；GPU 是明确标注的 CPU interface stand-in。
- `node test/p013_compute/tools/run_t159_transactions.cjs`：原完整事务离线测试通过；不是 GPU 设备执行/收益证据。
- 生成目录 `.codex/local-test/t159-project` 的 hvigor assembleHap：BuildNativeWithCmake/Ninja、CompileArkTS、PackageHap 全通过；SignHap 仍因既有 storePassword/keyPassword 长度配置失败，未修改密码。
- SDK ark_disasm 对新 HAP 的 ets/modules.abc 反汇编：Worker 与新增 launcher 都含 record、func_main_0。详见 after.json。

## 归档与 T164 交接

`test/reports/t165/` 保存 before/after SHA、source/post-build manifests、build.txt、selftest.json、s2.txt、transactions.txt。
原 unsigned HAP SHA：f7b1baf33bc7911e83954597bf30d1c7d8d9496d3680ae1ed0bf82b415d57fb5。
新 unsigned HAP SHA：49b45c024ea5935e96a306e16faf4572a654542cf72fcada95312e028c0c8db0。
新 sourceManifestSha256：ed26af35c31751b057b5dcc9bb5128809ecd83801c9791b011bf4e3f2b2e174c。
新 so SHA：b53b415a3b9f84ce3801f976f41650f54d33b19abe04b67ac6f2c38797536524。

T164 使用生成器重建或消费以上 unsigned 包，沿已有隔离签包流程处理；冷启动参数维持 T159 协议，buildManifestSha256 使用其最终归档身份，不将 source SHA 冒充最终签包 SHA。
设备签包 SHA 与本次 unsigned SHA 分列。先检查 WORKER_FAULT/FAULT 及实际 JSONL 返回，再执行三路完整收益和四类注错；设备成功尚未证明。
