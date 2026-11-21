# T783 · G13 同签包新进程缓存命中取证

2026-11-21 · G20 · P5 · architect · 离线复核和执行合同。

**用 T760 的签名产品包先做一对 G13 BOOT 冷暖证据。** 若同设备上原落盘文件仍在，可直接采暖轮。否则用同签包再取一个冷轮，等目标文件落盘，再保数据重启。该实验现在可做。BOOT 原 FP32 资格、产品首根和呈现门继续按 T741/T730 判定。 [T760 原报告](test-report-T760-v1.md) · [T783 离线复核](architecture-T783-evidence-v1.json)

## 已有原件及精确 key

| 原件 | 已证边界 |
| --- | --- |
| **T760 / T756 DEPENDENCY_SPLIT** | 签名 HAP `868106e2…`，映射 BuildId `e5ec50ff304bc372189b8ad210a24f0b82e24771`。PID 46924 / start 8613992 / owner TID 47218、attempt 191、run/context 1/1。T475 统一标签 `G13_COMPUTE`，以 manifest、实际 source/对象和 `G13BootQualificationV2` 联核为 **BOOT index 0、OP64/65/66、23,861 B**。编译 **65.237058593 s**，链接 **110.555203906 s**，反射及导出有原件。payload 301,961 B、format 38769、`PERSISTED`。现有设备归档缺实体 `.bin` 和同 key 新进程 `BINARY_LOAD`。 |
| **旧 T730 暖记录** | `856a2416…`、shader `a93aecf8…`、ABI hash `fb02ba34…`、binary SHA `0aea76cd…`。真实 `BINARY_LOAD_ENTER→LOAD_CHECKED→LOAD_LINK_OK_REFLECTION_PENDING→REFLECT_OK→REFLECT_PREPARE_OK`，load 窗 **5.645 ms**，build 起至反射准备收据 **10.601094 ms**。该原件冷暖 EGL handle 和 generation 相同，program 从 2 到 6，缺 PID/start 配对。只计旧 BOOT 加载和时长。相同的 payload 大小不能联据。旧 `G13_SCENE` 冷落盘单列。 |
| **RX T755** | 已见 `CACHE_PRELOAD_READY`，该次 SOURCE 实际编译、链接后 APPLIED，缺 `BINARY_LOAD`。预载完成只说明文件线程返回。RX 的 roleSet、shader、ABI、持久化时机独立于 G13。 |

T760 的 `EXPORT_WRITE_SUBMITTED` 行的 `persistence=PERSISTED` 来自文件 worker 处理结果，`endNs` 为收据提交时点，不能当写盘完成时戳。G13 的 BOOT 导出在资源初始化与 FP32 资格前，暖加载不需要越过数值门。 [T760 T475 原行](../test/reports/t491/t760/t756-compute-persisted.jsonl) · [T760 FP32 资格](../test/reports/t491/t760/t760-diag-full.json) · [旧 T730 暖原件](../test/reports/t491/t730b/split-cache/856a-warm.jsonl) · [T755 原件](../test/reports/t491/t755/h755.txt)

| T760 BOOT 身份 | 原值 |
| --- | --- |
| T475 key | `bacbc9118da2e5c1e8c459602f8d78c0a05b20b95aca1afdf4a4f244435c8cb6` |
| shaderSHA | `e18933691fc21846ae976557f18521de6450924e9095364b5b74d94d4367777a` |
| 工厂记录的 ABI SHA，即 `layoutSHA` | `34a397363d73a97df72d58c0795a5bec8cbb22b1480eb452e398b0177a5470d0` |
| linkConfigSHA | `6841f249566ce08e8ec085102ee73abcbc1f97190e468656f4599d07661791ae` |
| environmentSHA | `1338b161964299696eaa4a6c9c2c7b0f0bb5144bef61afded796983d1a613d72` |
| binary SHA、格式、字节 | `3368e1911fffcebfa8c95863406e3b0ffebee9801925475224fe5d96397b1b42`，38769，301,961 B |

用 T756 冻结产物重算。T475 依次对实际 GLSL 原字节、ABI 字符串、environment SHA 的 ASCII 字符串、linkConfig 字符串加 LE64 长度前缀，合入 `T475_PROGRAM_KEY_V1` 求 SHA-256。ABI 含生成 layout hash、`BINDING=T753-v1`、MATH/GATES/ALGORITHM 和 `ROLE=BOOT`，收据里的 `layoutSHA` 是完整 ABI 字符串的摘要。环境来自实际 GL vendor/renderer/version/GLSL、major/minor、EGL config ID/client version 和排序后的 GL extensions。现有摘要能复算 key，完整环境前像仍未归档。format 由驱动支持列表另验。签包 SHA、PID、EGL handle、run 或 generation 属取证身份，均不参加 key。两轮仍固定同签产品包、设备、后端、OS/驱动、REST Config/seed 和真实窗口。图形 `G13_SCENE` 走独立 key。 [T756 构建](developer-T756-v1.md) · [T475 key 接线](../entry/src/main/cpp/gpu_resident/program_cache/program_factory.h#L77) · [T475 文件层](developer-T475-program-cache.md)

## 最短真机执行和证据

1. **先保存原文件。** 在 T770 替换安装前，或恢复原产品之后，由 effector 核 T760 签包、实际 G13 页面和 DSO。经授权沙箱确认本轮 `context.filesDir/gpu-program-cache/v1`，列目录并只读导出匹配文件。历史 `<key>.bin` 如仍在，完整长 **302,105 B**。头为 `T475B001`，LE32 format、payload 长度、stem/header key 和 payload SHA 全部匹配。分别保存原字、全文件 SHA、mtime 和对应冷轮 JSONL。若原文件不在或不匹配，隔离项目程序缓存，用**原签包**补一轮同条件的源码编译到真实 `PERSISTED`，并导出完整文件。新冷轮用实际新 payload SHA 配暖轮，独立记录系统隐式缓存状态，避免清应用数据。
2. **终止冷进程，保数据重进程。** 确认原 PID/start 已退出，保上一轮原 `.bin`，记录新进程启动前后的文件 SHA/长度以及同 key JSONL 新增边界。新开原 G13 页，从触发前连续采 `G13Init/G13Boot`，只点一次「配置 REST」。原页先 `g13BeginSession({filesDir: context.filesDir})`，再 `g13Configure`。文件线程执行 `Store.Configure` 的读盘校验，owner 等其完成。保 `cachePreload=READY/FAILED`，再取同 attempt 实际 Build。Store 每进程全局，同路径二次 Configure 可直接返 READY，单纯重入页面或切 context 不证明新读取。
3. **核实际驱动加载和原数值终态。** 保新 PID/start、owner TID、boot/attempt/queue/run/context/surface、签包与实际映射 ELF、角色 SHA。对 BOOT 取 `CACHE_BINARY_LOAD` ENTER/EXIT，T475 `BINARY_LOAD_ENTER`、`LOAD_CHECKED` 的原 GL/EGL、format、`LOAD_LINK_OK_REFLECTION_PENDING`、`REFLECT_OK`、`REFLECT_PREPARE_OK`，核本轮实际 payload SHA、非零 load/reflect 时点与新 GL program，及**该角色**源码 compile/link 次数皆为 0。前两种 load 事件可能仍显示 route 初值 SOURCE_COMPILE，成功后才为 BINARY_LOAD。之后照原 SSBO 与 uniform 设置、INIT 64/65/66、执行 fence、CTRL/WORK map 和 `BOOT_EFFECTIVE_TERMINAL` 原字取证。联据 `bootQualification.program.route/cacheKey/binarySHA/environmentSHA`。加载与反射成立而仍为 `INIT_ERROR_27` 时，只记 BOOT 暖窄证和另列数值失败。回落、截尾或错身份按实际阶段留原件。 [页面配置](../entry/src/main/ets/pages/GpuResidentG13.ets#L45) · [session 读盘](../entry/src/main/cpp/gpu_resident/g13flow/bridge.cpp#L226) · [BOOT 读回](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L637) · [Khronos OpenGL ES 3.1 §7.5](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)

**可执行采集。** 沿 T760 已用的状态采集取 `g13Status().initialization`。有已接入分页读口则归档 `g13InitDiagnostics(attempt,0..3)` 同 attempt/sampleSeq 的原页。初始 FP32 失败时无真实根冷档，页面「冷档」不能导出缓存。经授权可用 `hdc shell -b <bundle> ls -l <sandbox-dir>` 和 `hdc file recv -b <bundle> <sandbox-file> <local-dir>`，分别取 `.bin`、`.jsonl` 和独立 SHA。最小回传两轮身份、事件范围、实际文件头和原字、对应数值终态。分记预载、加载/状态检查、反射、init fence/map 和最终截止，不能只用 T475 聚合时间推 GPU 完成。暖窗内若回落冷编译且未返回，保本轮源码 ENTER 和真实观察截止。 [HDC 授权沙箱传输](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/ide-device-file-explorer/) · [T727 计时合同](architecture-T727-v1.md)

## 错 key、探针安装与后续放行

| 对照 | 预期 |
| --- | --- |
| **错身份但文件自身完整** | 独立缓存副本中只备旧 `856a…` 或其它合法 shader/layout/link/environment key，缺预期 `bacbc911…`。预载可 READY，目标 BOOT 不应有 `BINARY_LOAD_ENTER`，真实源码路径一次。按期望与实存 key 的差异核 miss，该情形可没有 `DISK_REJECTED`。 |
| **同名条目头不符或完整性坏** | tester 先在现有 T475 临时目录用例跑错 header key、坏 hash/截断。真机负控只在可恢复隔离副本且有正常授权时做。看到 `DISK_KEY_LENGTH_HASH_REJECTED` 或对应误差阶段，原坏文件不交 GL，源码最多一次。未知 format、driver 拒载和反射拒绝分开归类。反射拒绝按原工厂拒绝，不能预计已有源码重试。 |
| **T770/T778 同主 bundle 探针** | T767 有独立离屏 owner/context、自建 probe programs，`filesDir` 用于诊断报告。即使覆盖安装保留同一物理沙箱，也禁止 T475 读写、借用产品 cache 或预热产品。在 T777 窗口前后备份并核恢复。数据连续性未证时恢复原产品再建新的冷暖对照。 |

**执行触发。** effector 可先用 T760 同签包取一例 BOOT 新进程样本，真机串行并避开 T770 覆盖窗口，无需等待 T730 完整数值放行。tester 可先用 T475 临时目录夹具验证 key/header/miss 与正例收据。现有路径确实无法取得文件或联到 PID/start 时，再交 developer 补有界的 preload 实际读文件和逐 role 摘要；若产生新包，其暖轮用该包自己的冷轮。

**完整角色和首 Swap。** `G13_SCENE` 可在数值决定前准备，单独计图形程序命中。余下五个 steady compute（OP_00、OP_18、OP_37、OP_48、OP_61）须原 BOOT 通过才 Build，并通过逐角色 OP66 和 Prepared。T730 取得合格设备或按 T741/T744 证明的可执行等价路线后，以同产品 variant 的冷轮取实际完整角色 `PERSISTED`，新进程逐个记命中/回落/未达。首个实际有效 Swap 仍需原根、数值/oracle、同 generation XComponent+shader 的成功返回和有效画面，连续性能另按 **30 FPS** 核。原 FTZ、期限、GPU 驻留和禁止 Canvas 回退继续适用。Vulkan 缓存用独立管线/设备证据。 [T741 数值及计分](architecture-T741-v1.md) · [T777 恢复合同](architecture-T777-v1.md)

离线复核从仓库根目录执行 `python tools/architecture/analyze_t783.py`。若已取得历史 T760 原缓存，可加 `--t760-bin <原件>` 只读验证其头和 SHA。**G20 约 33%（4/12）**，本卡交取证合同，实机新进程暖命中仍待测。
