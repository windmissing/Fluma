# T694 · M2 与正式 M1 同页采集的工件身份

2026-11-20，architect。A/B 可以分别推进。现有同页补充包分别改变了 native，保为独立诊断包。原 T663 / T597 native 字节是正式复核锚。

| 域 | 原生与页面身份 | 证据资格 |
|---|---|---|
| A 原 T590-cursor2 | HAP 6b1faa1d…，libgrsm1 SHA 7a0bcbaa… / BuildId c23e231c…，与 T663 相符，ABC397766f8… | 真实 M2 页面可读已有 cursor，T590-v3 保单API开放采样。当前包有独立 T590Cursor 路由，脱离真实 XComponent 的操作无法证明创建；手动导出也不证明 pre-arm。 |
| A T676-same-a | HAP87a6703b…，lib f25e37a9… / b31cf133…，ABC c63d9448… | 实际 GpuSandWaterExperience 同页按钮，从当前 serial读三对象 log 后复查。无创建前arm、无持续发现。 |
| B 正式 T597-fixed / T593-probe | HAP20bad499…/133f1257…，ABC511bc6af…/fee60633…，lib均527c863f… / dc54802f… | 正式 native 对照，T593-probe 设施页另计，只有实际 GpuResidentM1 的真 XComponent 路径可做同 owner 输入证明。 |
| B T676-same-b | HAPaf1bad16…，lib f57e450b… / 59e6c47e…，ABC aab03e81… | 原同页改造逻辑可复用，实际重编 native 的设备结果另立 scope。 |

两组 A/B 原包均可使用自身已有出口。优先用已归档的同进程资料；采集补丁走隔离 ArkTS 加工及正常签包，将原基线未签 HAP 的 ABI 依赖、所有已打包 native ELF 精确固定。可由 tester 在受支持的 staging 步骤选择真实原 native 输入，复核每条包内 native SHA/BuildId、依赖、XComponent 模块、Ability/页面入口、签名及新的 ABC/overlay manifest。不能只比同名库或只回填 BuildId。若标准链确实无法保字节，登记失败命令/BuildId差，继续原包现有出口；新构建只作新的诊断实验，其设备数据不修补历史或原正式 ELF 的缺失链。效用声明区分“原native + 新页面夹具”与“原整包 + 外部采集”。拒绝跨 PID/启动时间、包/loaded ELF、run/context/epoch/window/owner lifecycle 拼接。

## A · 先 arm，再挂真实 M2 XComponent

保 T663 实际 GpuSandWaterExperience、原 Ability/入口、native owner、XComponent 和冷创建合同。tester 在同一页面的 XComponent mount 前建立一次受控采集窗口。独立冷启动中先保存未装载组件的 `m2CreateDiagnostics()` 原回应（NO_CREATE_ATTEMPT 或真实已有 serial）和 host/native 时刻，登记 preArm。完成之后通过同页 UI 状态只装载一次**原** XComponent，先 arm 后 mount 的时序保原记录。初始记录不能假定旧 slot 为空；发现后必须确认 serial 超过/区别于已证初始槽，并核当前五字段 bootSerial/epoch/window/context/ownerTid、进程启动和实际 loaded ELF。无可判定的新 serial 明记 CLOSED_WITHOUT_ATTEMPT。除指定一次真实 mount，期间不重置/换页/代发 pulse。

真实读只用 m2CreateDiagnostics()/带 serial 版本及 m2CreateLog(serial,object)。探测后锁 serial，每次保完整 raw，选阶段/cursor变化、最后活动边与可得终态；只在 log 元数据变化或真实终态取匹配的三对象 log，随后复核同 serial anchor（独立读取并非原子）。游标区分 ENTER/EXIT/未知条件，终态需 sealed 且 nativeEndNs>0；非零失败保原值，旧指针/窗口变化只作另一 scope。手动原导出与有限采集各登记实际窗口，T590的124s手动快照不计入60s pre-arm窗口。另时刻的同进程原件如果确实可绑定可保留为追加证据，不能拼作原预算内完成。

## B · 真实 Down 的分段证明

T593-v5 的原 UI CURRENT·NONE sample5981和物理649可登记。POST_TOUCH pid13252、run2/epoch4/session2/attempt4、resources=true、publishedRun2/readyEpoch4 是实际 READY。v4 POST_CREATE pid38336虽数字相同仍非同进程。触摸期间 #0 是UI摘要读数，busy不决定 `allowed`，无法证明六次 Down 均拒，亦不能证明某次已进入 `model.down`、enqueue 或Claim。v4无窗口 probe 的错误参数（m1,64）及 v5历史五条无效JSON保原状。

正式 GpuResidentM1 的 `touch()` 先 `recordPageTouch`，后 pixelMode/Cancel/无点分支，逐 changedTouches 调用 model.down/move/up。GpuM1DemoController.down真实 refresh、检查 allowed/坐标并记录 POINTER_DOWN_READY_COORDINATES、recordSourceAttempt；成功只选择 pointer 和 source 边界。是在后续实际 tick 按 current/Fresh 状态生成128B typed SOURCE 并调用 offer，不把 Down true 当提交。

1. 以原页既有 `recordInputStageV1`、`recordSourceAttempt`、`executionDiagnosticsV1`、timelineSnapshot 和必要 layout capture 作第一路径。有一条真实 Down/hold/Up 序列，在原允许时钟和 SOURCE 入口下观察。取 handler ENTER/event raw timestamp域、count/pointer/坐标/宽高/布局/原action序号，Down 内真实 allowed 各字段、返回/reason，以及同 scope 后续 `UPLOAD_32U32`、`OFFER_REPLY`、request bytes与真实 `ENQUEUED_NOT_ACK`，再联 claimWitness 的同 seq/root/nonce、真实 SOURCE 终态和同token DRAW/SWAP。重试不能把非空输出从不同手势拼起来。既有 page record 只保第一点，单 pointer 正控；要覆盖多点原件用真实 handler的有界附加只读记录，等实际缺口明确再评估。
2. 原始 `coldCapabilities()` 的完整 scope 是 EXEC_DIAG 参数1的 JSON 字符串。参数2是**总写入信用**，先核当前终端 Gate 最少16384B；按 API 自身上限及剩余目录信用传入，8192仅原始切片。冷输出按接口字典读 inputStages 与 claimWitness（空或截断要分开），其 scope 前后相等只支持相关性，保各 native 时刻。status 含 ObserveControlLocked 副作用，安排在窗口首尾与必要状态点，不能制造额外 accepted pulse或刷新期限。strict JSON解析失败只能作原始错误，不能补写未采数据。
3. 若原包实际页已有记录但没有可用导出按钮，tester 仅把缺的已有 API 导出或 handler-return 观察接到真实体验页，不替换原 handler、不在热路径同步写盘、不改 native。优先做原 ELF + 仅 ArkTS 的新夹具，冷时导出同scope；若现有端点在实际原 ELF 真缺/回包必然超界，先保准确 API/调用scope、原码/raw或截断、实际模块和阶段，再由architect裁定最小产品 seam。默认不把 CURRENT 重采当完成。

## 共同封口

每域独立 60s/64 次诊断 native query（含前后锚、三对象或所有页），单持久记录≤16KiB、总≤1MiB含32KiB索引，API 大原串一次读后切≤8192B且保 offset/总SHA/长度。记录预付 query、入口/返回时刻、初始真实未绑定的scope、落盘不足/超界的明确终态。采集调用在页 owner 可控冷边界；不通过后置 bind 把 pre-arm 空样本回填成当前 attempt，不重复读取同一大出参来构造分页。只有真实退出码、终态或明确窗口/资源失败才封口。设备串行由 effector，tester 离线设施、签包与解析。现有可用产物直接推进，不等待他域形式关闭。T671可复用解析模式，但其单沙工件和状态不能供本 M1/M2 使用。

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。
