# T414 开发交付

2026-11-09。依据 T413 §4 交付单沙截图 hold 隔离包和冷档工具。

- `test/t413/generate.cjs` 基于 T406 生成独立工程。预约先校验三组 ROI、预算、配置、当前 owner、几何和旧事务，再提交 phase 与 request。只有同帧同 token 的 native `CAPTURED/CURRENT`、双目标读回、swap/unbind 及 ROI 身份匹配才冻结。归档 ack 核 attempt/request/SHA 并保留冻结。封存在内存中固定第一次端点，落盘使用同一快照，存在同名不同内容时拒绝。释放核终态及归档结果。位置、缩放、可见区、owner 或前后台改变保存失效首因。`maxHoldMs` 到期只进入 EXPIRED，保留显式释放边界。
- 页面固定控制区高度，独立收焦点和软键盘。只有完整可见且几何代已与原生绘制匹配才接受预约。B 臂在预约成功时注入。采集工具分别读取预约回执和状态，以免把旧 OPEN 当成新成功。
- `capture_hold.py` 将截图启动、远端拉取、PNG 完整性及 SHA 统一放入保守 host 区间。工具取得同一 OPEN 的 app ACTIVE 原始时钟样本后才请求封存，保存原命令收据、PNG、manifest sidecar、RGBA、封存及 spec。`clock.py` 用有理数求所有可行正斜率仿射映射的保守包络。`screen.py` 校验新版时间和身份后复用 T406 映射/颜色判据。绘制仍走 XComponent + shader。

| 开发验证 | 结果 |
| --- | --- |
| 实际生成 controller，native 桩 | 11/0，含预约零副作用、冻结、旧 ack、首次 seal、真实 elapsed expiry 和几何/owner/hide 失效 |
| 版本化冷档工具 | 16/0，含标定正控、先 seal、映射包络越界、重复或错域时钟、缺 PNG/文件/SHA |
| 真实子进程 runner，合成设备命令 | 4/0，含完整成功、拉取失败、缺 PNG、拒绝预约且旧 OPEN 仍存在 |
| 旧 T394/T406 配对、控制器及 PC ANGLE/GLES | PASS。三 ROI 和 98304 B 预算、GL 恢复、普通绘制零读回、failed Swap LastGood 均覆盖。PC sRGB-decode 仍有原 1 码差，按原判据保留 |
| 最终 Hvigor `PackageHap` | 30/30 tasks，BUILD SUCCESSFUL，零编译错误 |
| HAP 签名与验签 | PASS，见 `test/reports/t257/T414-finalized.json` |

最终签包 `.codex/local-test/t257-sign/T414-finalized/T414-finalized-signed.hap`。

SHA256 `7850376c9ceec002e72008b79b77ddc785bf0ab1d4ccafc71a7e9b2f2e55f61f`。

真机未使用。T408 使用 `test/t413/README.md` 配置采集工具并确认实际 app 文件取回权限、无遮挡 bounds、系统 PNG 和独立原域时间链。默认 HDC 适配器假定测试设备支持读取指定 app files 路径，可通过 README 的 argv 接口替换传输。相邻命令的 host 边界允许同一计时刻度，样本时刻仍须严格递增，映射不确定性仍整体纳入时间门。
