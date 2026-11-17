# T521 M2 正确修复包零账与空日志首次归因 v1

> 2026-11-17 T547新证修订：T461 v2原件与canonical T468 writer确认合法空raw写盘后无条件md.update(empty)使收据链失败。本文当时“当前没有证据要求developer改saveRaw”的历史判断已被新证取代；按 `architecture-T547-create-and-empty-export-triage-v1.md` §5修工程后交developer。空raw允许，native/manifest不得为空，M2产品域仍NOT_REACHED；不改NO_PREVIOUS_CREATE/NO_TRACE准入。

2026-11-16，architect，不使用真机。结论 **包来源可核；证据采集未完成，功能保持NOT_REACHED；先复用现有诊断补证，当前不派kernel拆分或认定呈现同因**。原件指纹/布局与包内ELF核对见 `analysis-T521-m2-zero-ledger-evidence-v1.json`。

## 1. 来源与现场边界

逐位复算T482-final签包SHA256 `300bd52a97e360cf533e261bf35433b465f560329f4a73232d8e119cdb51381e`，9,454,076B；unsigned包 `fef97d3c94f50438848eaa4d54a5b9af8aa2506c9194fe4e361ded2106ecab73`。二者 `libs/arm64-v8a/libgrsm1.so` 相同，SHA256 `783cb115038e83e22897400c8e350b9562fba7783fa6fbe2b8c766c9249fa5b2`，1,753,184B；含T462诊断revision、m2CreateDiagnostics/m2CreateLog等实际字符串。T482修复SOURCE SHA `b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1`及同源代码由delivery/构建树核。此为本地实际artifact，不以重新编译的共享HEAD冒当时包，也不单凭声明证明设备加载ELF；现场仍需PID/进程/包声明对应。

`test/reports/t491/t461`有截图/五layout、files.txt；`create-diag.json`和`m2log.txt`本地均0B。files.txt只列cache目录及两个 `t468-create-1-1794666388084-0.log`、`t468-create-1-1794666422882-0.log`，没有ls -l、命令退出码、设备路径、native/manifest文件。设备端0B是tester报告观察，不能把该无尺寸列表当独立ls实证。

layout4/5文本和bounds集合一致，但文件hash不同（accessibilityId/hashcode有变化），显示PAUSED_CLOCK CLOCK_GAP/run0:1、四账0；只能确认UI某次状态，不含PID/epoch/session/root/原nativeGap。截图中 `t461_m2run.jpeg`实际为锁屏，`t461_real.jpeg`是STATE_LOST WAIT_RETIRED/run0:0，`t461_run2.jpeg`则显示CLOCK_GAP/run0:1和四账0；不能按文件名把它们混成同一次运行。当前/上一创建按钮在位并不等于实际命中、导出完成或属于该CLOCK_GAP scope。原始点击命令/坐标域/时间、WS_EVENT、boot/recovery/时间账没有保存，无法回溯首个GAP所在root或归因UI/compute/呈现。

## 2. 0字节原始program日志不等于0字节诊断

实际页面 `GpuSandWaterExperience.exportCreateDiagnostic`：读取m2CreateDiagnostics；previous=true时读取latest.previousSerial，为0则NO_PREVIOUS_CREATE；要求revision `T462_M2_CREATE_DIAGNOSTIC_V1`；随后依次存object0/1/2三个 `.log`，再存 `-native.json`，最后 `-manifest.json`，全成功后才写 `WS_EVENT t468_create {name,bytes,sha256}`。中途异常为CREATE_EXPORT_FAILED，类型/serial不匹配为INCOMPLETE。导出原始诊断按钮同时导出结构JSON和三个raw log，不应只拉取一个 `*-0.log`。

native `m2CreateLog(serial,object)`只对该Program.used字节编码hex。create_diagnostics.h中 `Program.raw` 初态used0，尚未尝试该object时hex为空；编译/link已成功但驱动无消息也可为空；binary加载路线未调用原create468日志查询时，queried可能false。`M2CreateColdFiles.saveRaw`明确允许hex=''，分配0字节并open/fsync，合法生成0B日志。它既不意味着compile失败，也不意味着成功。

结构诊断JSON是另一文件，必须含code/sealed/inCall/firstFailure、完整身份、programs sourceBytes/SHA/rawBytes、compileLog/linkLog的queried/reportedLength/copiedLength/queryError/emptyLog/truncated、能力probe/profileAdmission及events。即使NO_CREATE_ATTEMPT也应有具名状态响应，**正常结构JSON不能是空文件**。本地create-diag.json是手工拉取路径的0B文件，未见设备原生native/manifest及错误码，不能证明页面把完整诊断空写。files.txt只有object0可能是导出部分失败，也可能是取文件/筛选不全；两者均保留，待以下最小原件裁定。

## 3. 最小tester补证：先已有能力，不再盲试两次

复用同一300bd52a签包（如换包必须另建新scope，不拿新包诊断替旧run定因），设备串行排队。先落启动/安装/进程PID及设备加载来源收据；本地包/ELF hash与声明分列。记录页面layout尺寸和坐标域、原命令及exitCode，操作前后各一份layout/截图，时间可关联native clock。当前布局像素1280×2832：

| 操作 | 当前layout bounds | 取中点（仅该尺寸像素域） |
|---|---|---|
| M2入口 | [254,1414][1027,1554] | (640,1484) |
| 当前创建导出 | [304,2384][976,2524] | (640,2454) |
| 上一创建导出 | [360,2545][920,2685] | (640,2615) |

必须按本次现场layout重新查bounds，不硬套旧坐标。两个按钮要分别记录动作、UI operation和WS_EVENT。当前bootSerial1时上一为0，应NO_PREVIOUS_CREATE，不能把同一serial两次current导出冒previous。需要实际reset后新的create serial才能比较上一；reset需先保旧scope，避免覆盖故障。若按钮在当前屏幕外/覆盖，先用实际可见命中后记录，不能猜缩放。

每次导出保整个前缀所有文件（0/1/2.log、native.json、manifest.json），以及完整目录 `ls -l`/stat、原路径、fetch命令/exitCode、传输前后bytes/SHA、WS_EVENT t468_create收据、UI operation。空raw的SHA应是空字节SHA，bytes0合法；manifest/native必须非空且JSON可解析，receipt对齐实文件。不要把read命令stderr或shell空重定向当nativeJSON。

若只有object0而无后续文件，按顺序定位saveRaw object1的open/write/fsync/crypto/parse异常，先保UI CREATE_EXPORT_FAILED原String(e)；若WS_EVENT已有成功manifest但取不到文件，先查设备filesDir/传输路径与目录过滤；若manifest receipt bytes>0但实际文件0或hash不同，才有文件完整性缺陷证据，回architect后转developer。若有nativeJSON，按queried/empty/route精确区分未尝试、合法空、查询失败/截断、编译失败，禁止以raw0B替代这些字段。

同时用已有bootDiagnostics导出完整boot，已有recovery/timeline接口保状态和首GAP前后pulse receive/ui时间、READY/session交接、permission与root kind/nonce/plan/deadline、TRANSPORT_CREATE_ENTER/EXIT、scope和reset/request。该包缺较新T492增强项时先读取它已有的T456/T462/T467/T482字段，不凭空要求不存在endpoint；需要增强包时由manager沿现有T492/T510接力另scope，不重复开发已交增强诊断。取证期间真实GAP如实记录，读诊断不唤醒owner、不重置clock掩盖故障。

判读：

1. 同scope sealed/code非0＋firstFailure/具体GL调用原件 → architect核工程后developer，只有真实shader限制证据才讨论拆kernel。
2. code0资源就绪，但READY/session/pulse交接或首次双GAP失效 → 按真实控制证据归因；不能因M1也CLOCK_GAP就直接同因。
3. 有非空成功SOURCE/QUANTUM及合法publication，Draw/Swap/系统像素对照仍失败 → 才进入相应呈现路径分析。四账0/黑区自身不满足此前置。
4. 只取得文件/按钮/取证错误 → tester修证据流程，必要的工程缺口回architect，不伪派物理缺陷。

T477另一PID的冷暖创建成功只能证明那次program路径，不是T461该scope正常；T461原早期SHADER_FAILED与本轮coherent修复后CLOCK_GAP也不混因。四档能力、SOURCE13pass、准入/merge/NO_PAIR/QUANTUM/短接受/回滚/同token呈现继续NOT_REACHED/NOT_MEASURED。

## 4. 交付边界

首次归因范围完成，manager可把§3精确请求交tester续T461；产品根因仍需上述既有能力原件，补后回architect分流。当前没有证据要求developer改saveRaw对合法空日志的处理，也不新增manager验收门。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它项。

不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人状态有限例外，不改其它行列/滴答同名清单。本轮不使用真机、不实现业务、不重跑测试。CPU停投；前景背景全部XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。不改变250ms/GAP/120Hz quota/30fps目标。
