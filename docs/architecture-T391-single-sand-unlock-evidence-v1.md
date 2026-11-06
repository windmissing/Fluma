# T391 · 单沙解锁后画面停更首次归因

2026-11-06，architect，P4。不使用真机。核 T373 续测原卡、正式报告、T385 签包和实际生成源码。本文补充 T370 窗口取证及 T383 预约合同。

## 1. 原档裁定

T385 本地签包 SHA256 实核 `db9e3ae46798642526703c567a18c138f92e8c66925f68317f81b53e3d607f68`，构建目录 manifest 的五个 T385 源 SHA、T373 续档登记的 17 个 SHA 前缀均匹配。

| 证据 | 可确认的范围 |
|---|---|
| `def-manifest.json` 与两份 RGBA | run `p011-run-1793961680062-3`、state epoch 2、surface 2、context 1、commit 1258、frame 629、count 0。默认 ROI 4 元素、JS/原生各 16B、`COMPLETE/CAPTURED`、同 token CURRENT、swap/unbind 成功。两份各 1024B，SHA 和 manifest sidecar 实核。该角落包含背景和边框两种像素，支持空场读回冒烟。 |
| `receipt_t385.json` | 同 run/epoch/surface/context，commit 4762、frame 6841、count 203、CURRENT、swap 成功、GL/EGL 原码 0。上传账为 `1024+203×16=4272`，readbackBytes=0。相对空场收据，物理提交与打包粒子内容已有变化。两点之间成功步数、实际 dt、角度与轨迹未按锁定阶段归档。 |
| 晚期床面 | `bedSupportCpu=EMPTY_BED`。displayProfileHash 和 pbHash 均等于 512 个零字节的 SHA256 `076a27c7…6560`，而 rawSourceHash 与空场样本不同。该回执关联的 128 个显示床高度与 128 个坡度是全零输入。故该帧在 GL 之前就没有 BED 支持。原始权威床、LAND 事件和预填点击时的 token 未归档，不能据此裁定为什么未生成床或将两次预填都归同一个失败分支。 |
| s1–s5 截图 | 按四份 layout 共同记录的 XComponent `[51,1632][1229,2650]` 从 1280×2832 的原 JPEG 解码后重裁。s1→s2 的 RGB 差异 bbox 相对组件为 `(492,399,894,641)`。s2、s3、s4、s5 的区域 RGB 逐字相同，SHA256 `ecfba0ad6f23de90d49ced5fd630295e5ef5e27833677a72c1d344816bafa25b`，全图互异。s4 显示「锁定当前样本」，s5 显示「恢复仿真」，支持按钮状态有切换。布局坐标一致和组件像素重复成立，JPEG 无帧 token，归档的两张派生 crop 也不提供呈现身份。 |

**首次可证明的缺口是三区夹具输入缺床**。原档晚期样本在入 GL 前已是全零显示床，继续等截图变化无法给该帧生成合法 BED ROI。**解锁后呈现停更另列待定位**。s2–s5 的组件图确实相同，但旧档缺同一窗口、同一 token 的有效粒子区域 candidate/back 与系统显示配对；203 个实例也不能直接与截图上的亮斑个数比较，实例可能相叠。保留疑似停更，现证据不足以指认 compositor、GL 或物理沉降为唯一根因。

## 2. 代码边界与最小工程修订

- T385 生成页的锁定按钮调用 `model.stop()` 清当前手势、取消 pending probe，再翻转 `fixtureLocked`。锁定只禁止手势、重力输入应用和 `model.tick`。每个正常回调仍复制已发布状态、递增 frame、pack 并走 native Draw/Swap。解锁后下一回调恢复 tick，新 DOWN 才重新开源；按钮不直接创建/销毁 XComponent，也不切换 FBO。失败 tick/copy 会早退，截图时段缺逐帧回执。
- `P013SingleSandPreparation.pack` 先验 V2 seal，再从权威快照生成显示床、坡度与实例数组。native 校验 token 和收到的 profile/pb/particle 字节哈希。Draw 对每个有效包上传 profile/实例，绘制背景、床、粒子、前景到 candidate，再绘制到 framebuffer 0。EGL swap 成功后交换 target 槽并提交 token。故晚期回执可以定位到带新内容的上传与交换调用，`CURRENT` 的语义止于 native 提交。T385 只有显式 arm 才记录 pre-swap 两个读点。
- 现有 `windowFixture()` 读取可复用的 `preparation` 和 `lastReceipt`。它只比两个 token 并检查成功位。copy 或 prepare 早退时两者可能仍保留同一旧 token，prepare 还可能先改几何再失败。这个接口也将「未取得有效当前帧」「空床」「无孤立 AIR」「无空背景/尺寸不合」折叠成空字符串。属于可直接修订的隔离诊断缺口，尚无原证表明本次触发了旧帧误取。

developer 修隔离包，不动默认生产页。给锁定/解锁/重锁递增独立阶段序号，锁定只使用该阶段一次成功的 publication copy、pack 和 native CURRENT 回执。成功前显示等待，任何新阶段、生命周期、几何变化、copy/pack/draw 失败撤销对应 fixture 就绪；无需为此重建 EGL。在同一同步回调存放带身份的成功快照或经证明仍属该帧的只读副本，点击预填不得直接回看可变工作槽。ROI 消费继续核 fixture 内容与几何，允许静态光学两臂各用自己的有效 token。拆出 `WAITING_LOCKED_FRAME`、`STALE_INPUT`、`EMPTY_BED`、`BED_ROI_UNSUPPORTED`、`AIR_ROI_UNSUPPORTED`、`BG_ROI_UNSUPPORTED` 和几何失败，沿原预约、冷导出和 SHA。锁定保持原物理暂停与重新开源语义。

## 3. 两臂取证，先让结果可区分

1. **独立三区颜色臂**。用现有权威写入或经同版本 owner 恢复、重新验封的冷夹具，预先构造可容纳 16×16 且远离边框的厚床、孤立 AIR、空背景，按实际 viewport/像素尺度校验支持集。禁止用渲染缓存冒权威输入或放宽 palette/封签。夹具来源、权威库存、display profile、实例、配置及 ROI 固定。锁定后分别中性/纹理预约，取同次真实 candidate、真实 default back 与 Swap/Unbind 回执；按 T370 独立判各层色彩。该臂可验证颜色而无需自然沉降先形成床，归档明确标记夹具。
2. **解锁动态臂**。同一 surface/viewport 保持光学不变，分别取不锁定对照和一次锁定→解锁→重锁。通过正常 owner 写入一个可定位、与旧亮斑分离的新粒子群，或选择预先验证可辨识的输入变化。锁定 A 与解锁后首个有内容变化的 B 分别预约相同固定 ROI，至少覆盖旧/新支持区，另取背景负控。每臂保同次 copy→pack→候选 FBO→default back→Swap/Unbind 的完整 token、epoch、阶段序号和单调时刻。少量选定 ID 的实际 x/y/state、可见掩膜、接收手势/实际入量、dt、权威床与显示床量和正 LAND 事件冷档化。若需要判自然床缺口，先看权威 LAND 和床量，其次才看 pack/薄床支持。
3. **系统显示配对**。优先取得可与 swap 相联的 buffer/present 序号或同域时钟。接口不可用时，用相同渲染管线生成可辨识且不同的稳定 A/B 内容，把目标样本保持到截图完成，保存无损全图、真实组件 bounds、屏幕映射及时间；记录期间无下一内容替换。只凭按钮标签、commit 计数或 UI 单独显示的 token 不作配对。不能建立关联时记 `DISPLAY_NOT_JUDGABLE`。

| 最先出现的差异 | 分流 |
|---|---|
| 解锁未应用、无有效 tick/入量，或权威床持续零 | 隔离页输入/物理证据。权威床为零且粒子未 LAND 时先查真实物理前置，不能由截图反推床。 |
| seal/权威坐标有变，pack/ROI 输入未跟随 | publication/pack 或隔离 fixture 归工程，依据首错和位型修改。 |
| pack 的独立 oracle 预期 ROI 变化，candidate 未变 | 查上传、实例/剔除和绘制目标，核完整 GL 状态及像素。 |
| candidate 变化，default back 未跟随 | 查 present pass、framebuffer/viewport 和 EGL 目标。 |
| 两读点变化、CURRENT/swap/unbind 成功，已配对的显示仍停在 A | 进入 XComponent/NativeWindow buffer 到系统呈现链定位。未配对的重复截图只保现象。 |

开发离线覆盖旧 CURRENT 遇失败/跨代不可预填、阶段切换、空床与足床支持、单次预约、常态零读回和旧 T385 冒烟。交隔离签包后 tester 定点真机，分别判三区颜色与动态停更。沿 T385 每次两目标合计 ≤98,304B 的有界读回，保 GL 状态与失败原码。背景、前景及所有对照只走 XComponent+shader；故障为 FAULT 或标明 STALE 的完整旧 token。T391 完成离线归因与补证合同，设备层根因待配对证据。
