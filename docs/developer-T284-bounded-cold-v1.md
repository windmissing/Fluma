# T284 V2 有界冷采交付

2026-10-30，developer，原卡 `6ae3e0eae4b0e9cb68dbe0ea`。按T274 §5.1，仅修改隔离生成流程；不使用真机。

`test/t276/generate.cjs` 在原T266/T276精确patch后调用 `test/t284/apply.cjs`。生产页面、物理、dt/clock、默认路由不改；生成隔离页仍XComponent+shader，背景/前景无Canvas上屏或回退。重日志开关仅开放冷采：不再每帧anchor、PACK或STEP/LAND大对象格式化，不再重SOURCE/failure全trace。普通帧每60个成功循环保留小 `T266_CLOCK_V3`、首生/床/8秒/失败原码小钟，源停止仍按成功step秒增长。

自动锚点每run FIRST_BIRTH后续一帧、FIRST_BED、SINCE_BIRTH_8S各最多一次；`--pi t284captureframe N --ps t266heavy true` 可手选页面frame一次，合最多4。首生小钟在源成功当场先输出，后续快照不倒造出生时刻。每run/epoch绑定，reset/new run取消待导旧档并标RUN_CHANGED，不从新canonical重建旧token。首床原生像素接点仍仅4帧，pixel link按frame/run/epoch/commit和真实receipt token输出；单次native导出仅最新像素行，超1800字符标PIXEL_LINE_BUDGET，不重复打印累积4行巨串。像素是否CURRENT/同token须由tester核native原token/raw，不能只凭link.current（呈现receipt）宣像素PASS。

锚点在同一帧封存不可变字符串，PACK→FRAME（clock、床四数组、粒子id/state/mass/x/y/vx/vy、权威y/vy极值、LAND writer、publication/receipt、原生token像素与成本）按同anchor追加；hash/parts计数锁定封存内容。主帧不做分片发送；50ms独立回调仅在触摸源已停时分批，页面hide停止物理但可继续排空，disappear做最后有界batch后停timer，未排完如实PENDING/缺片。每批最多4片、2ms；每anchor原始UTF16LE≤262144B/512片、封存4ms/累计导出200ms；每run≤1048576B/2048片；待发送≤524288B/16项。每ASCII物理片≤1814字符（含prefix），最多512片即≤928768个分片字符/anchor，索引/小钟单列。上限不意味着保证能在任意设备完整采集；触发BUDGET_ANCHOR/RUN/QUEUE/CAPTURE_MS/BATCH_MS/EXPORT_MS/LINE后索引INCOMPLETE，保物理成功前缀，不回写失败或补跑。

时间检查在有界操作间进行，ArkTS不能抢占一次JSON.stringify/console调用；单次超时会在返回后测得并标INCOMPLETE/BUDGET，不能声称实时硬抢占。普通帧0大导出与锚点限次是前置约束，避免再每帧1.87MB/7314片同步输出。detail trace改COUNTERS，冷trace最多64行，省略计数/inner incomplete显式，不宣完整run trace；该卡完整性只覆盖被选择的有界锚点与原始parts，产品/长时行为另测。

新增 `test/t284/reassemble.cjs` 在原T276 part/digest校验后消费最新 `T284_INDEX`、预算、pixel link。PENDING/INCOMPLETE索引、任一预算、缺片/格式损坏都强制全档INCOMPLETE，即使已有单项parts恰好完整也不宣整锚点成功。`MISSING_PART` 保留具体exportId；无T284索引的旧档不冒本版完整。使用本版命令 `node test/t284/reassemble.cjs capture.txt outputDirectory`。

非机检查 **59/0**：预算/限次/推迟token/跨run取消/实际模型开关等价15；重组完整/缺片/预算/pending4；既有钟/部分子步/权威receipt/commit/LAND等价24；LAND writer/分片与缺片16。实际模型对照核床bh/bl/mh/ml、粒子质量/位置/速度/id/state逐值相同及commit/首码/成功秒一致；不同独立run标识不冒相同identity。Node仅hilog替换为无操作输出sink，模型与物理模块真实执行。

最终项目 `.codex/local-test/t284-v4`，重新生成命令 `node test/t276/generate.cjs .codex/local-test/<新目录>`；测试 `test/t284/selftest.cjs`、`reassembly_test.cjs`、原 `test/t266/selftest.cjs`/`test/t276/selftest.cjs` 参数为生成目录。**hvigor PackageHap BUILD SUCCESSFUL**，ArkTS新增快照显式类型已编译验证。签包verify PASS：`.codex/local-test/t257-sign/T284/T284-signed.hap`，SHA256 `ed16f0660a544f98a5c850581b6aa554a23fb3eefbc9c2777efccf6c2eaba6ad`；unsigned `bd0c7614e1ae1013716d838d5388e3586b36f6f1157a8f65f26bac31bb692c11`。证据 `test/reports/t284/delivery.json`、generated-manifest及四份测试输出、package.log.gz，签名 `test/reports/t257/T284.json`。未安装、未启动真机、未宣首生后8秒/床/像素产品PASS；T285沿真实证据边界续证。
