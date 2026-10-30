# T306 GPU M0/M1 时间线冷出口

2026-10-30；developer；不使用真机。消费 T302 工程，保留 T228 物理、手势与 owner 生命周期。

M0/M1 实际页面均提供显式诊断导出，分别调用本模块 timeline(cursor)。共用 GpuBridgeTimelineCold 每次最多4行，逐页让出 UI、30秒超时、取消、1MiB预算，保留前C行和原始页。严格校 cursor/next、serial连续、stage字典、count、U64十进制和单调ns。首末原header/status、跨epoch/run/surface/TID分组、页数/丢失/重复/溢出、截取边界一起写清单。新增native stageRevision和精确serial/dropped字符串，row.serial也改十进制字符串，固定字典 T295_STAGE23_V1 与 enum23项对应；不更改采样/环/Clock/Pump/源。

DiagnosticColdFiles 将段文件与清单写入 context.filesDir，短写循环、fsync后SHA256；短日志含清单文件名、长度、摘要和模块/包SHA。段文件约48KiB切分，单原页≤16KiB；清单自身SHA从外部短回执校验，文件段SHA/bytes在清单。导出期间追加、native dropped、坏页、取消/超时、身份变化均 INCOMPLETE。COMPLETE只指稳定未丢失的本次捕获，不代表设备物理/性能PASS。

签包SHA来源是显式输入的外部签包清单，并在清单固定标记 OPERATOR_SIGNED_ARTIFACT_MANIFEST_EXTERNAL_VERIFY_REQUIRED；应用不能把自己尚未生成的签名HAP SHA嵌回本包。入口必须有合法64hex才采集，tester需把输入SHA与实际安装包/交付清单核对，未提供时明确INCOMPLETE，不伪称运行时自验签包。PID为真实process.pid，route/module对应实际导入。M1入口在光学/诊断区，M0直接诊断控件；隐藏取消导出。无自动每帧分页、无改变active/newRun/offer，也不重置pause或唤醒GL。

非机证据在 test/reports/t306：20项实际collector+实际文件writer测试（OHOS平台API明确Node适配，真实磁盘fsync及独立Node SHA核），含0/1/1024行、U64越2^53、跨epoch、多页、坏cursor/next/serial/U64/stage、增长/溢出/取消/中断、SHA/fsync错误和双页可达。原生产bridge loop回归16/0。hvigor PackageHap构建及签包验签见delivery.json。未使用真机，OHOS实际私有目录写入/设备调度另交tester；开发交付不等设备PASS。原物理与XComponent+shader均未变，无Canvas回退。
