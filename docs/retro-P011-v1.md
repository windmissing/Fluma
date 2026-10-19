# P-011 复盘：bottom-creep-v2.7（2026-10-16）

## 经验

1. **角色归因顺序不能跳**
   - 测试失败时，先由 designer 判断算法语义是否正确。
   - 若算法正确，再由 architect 判断工程方案/架构口径是否清楚。
   - 只有两者都确认无问题后，才允许 developer 修实现。
   - 不允许 developer 直接改测试用例或跳过架构确认，避免实现偏离设计。

2. **测试失败要先定位责任边界**
   - P-011 的 3 个总预算失败并非算法问题，也不是测试口径问题，而是实现把 bottom creep 排除在第一轮总预算请求之外。
   - 正确口径是：`totalRequested = bottomGrant + otherRequested`，总预算耗尽时按同一组原始 `allRequests` 统一缩放。
   - 不能用状态变化后的重放结果替代原始请求集合。

3. **Developer 自验边界要严格**
   - developer 在转测前只做最小化自验，用于确认代码能构建、目标改动能触发预期日志或基础行为。
   - developer 不做正式测试判定、不分析正式失败归因、不修改测试用例。
   - 正式执行、失败归类与验收判定由 tester 负责；失败后按 designer → architect → developer 的顺序归因。

4. **Windows/沙箱环境构建经验**
   - hvigor 可通过以下方式直接调用：
     ```powershell
     node .hvigor\project_caches\044a440af38c6196c1e94e33e1a65550\workspace\node_modules\@ohos\hvigor\bin\hvigor.js test -p module=entry@default --no-daemon
     ```
   - 本机 SDK 位置：
     `C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0`
   - 本机 `hdc` 位置：
     `C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe`
   - `hdc` 不在 PATH，调用真机时要使用绝对路径。
   - 真机当前目标：`62T0225B10010020`。

5. **权限问题要尽早上报**
   - 遇到 `EPERM: operation not permitted, lstat 'C:\Users\admin'` 时，不要长期绕路。
   - 应该尽早向用户确认是否有权限，由用户批准或调整 Sandbox 权限，而不是反复尝试本地替代方案。

## 当前状态

- developer 正在按 architect 修复实现。
- tester 已完成客观脚本自检，但设备日志仍需补齐。
- effector 已准备 v2.7 主观测试 runbook 与报告入口，之前因 `hdc` 缺失标记 `BLOCKED(no_device_access)`。
- 现在确认有真机和 `hdc`，后续可继续真机验证。
