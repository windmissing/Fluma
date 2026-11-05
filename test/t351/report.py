from pathlib import Path
import json
r=Path(__file__).resolve().parents[2];d=r/'test/reports/t351';delivery=json.loads((d/'delivery.json').read_text());budget=json.loads((d/'budget.json').read_text());rows=budget['profiles']
table=[]
for s in [p['scenario'] for p in rows if p['mode']=='before']:
 a=next(p for p in rows if p['scenario']==s and p['mode']=='before');b=next(p for p in rows if p['scenario']==s and p['mode']=='after-on');label='持续输入' if s.get('held') else '停源/空场'
 table.append(f"| {s['n']} / {a['startLive']} / {label} | {a['wallP50']:.3f} / {a['wallP95']:.3f} | {b['wallP50']:.3f} / {b['wallP95']:.3f} |")
over=max(p['enabledP50']-p['disabledP50'] for p in rows if p['mode']=='after-alternating')
text=f'''# T351 · V2 帧预算与安全发布比较优化交付

2026-11-05，developer；依据 `architecture-T349-v2-frame-budget-v1.md`（848be9a）。不使用真机。开发范围完成，结束待验收；设备流畅性由 T352 验证，本文不声明产品 30fps/PRESENT 通过。

## 改动与严格边界

1. `DerivedCommitV2` 为两个私有 bank 与 canonical 的 13 池+8 field buffer 冷预绑定共 **63 个 Uint8Array 视图**，正常每次发布新建视图由 42 降至 0。已知位置 O(1) 检查实际 buffer 身份/byteLength/视图长度；发生替换或 detach 时保留原始完整比较构造路径与原 TypeError/错误结果，不使用失效缓存。不同长度仍首先返回 false；每个字节仍逐一读取比较，包括 ±0 和全部 F64 位型。
2. authority 比较的 diagnostic `counters.add(16)` 从每字节一次改为每完整 buffer 一次；首个不同比特处加 `i+1`，保留原比较前缀计数。无用户回调插入该字节循环，typed view 普通读取期间不会发布或冻结故障；原 first-fault、结构错误优先级和冻结原始 buffer 对照通过。N512 全量发布字节比较仍为 38,400 字节；示例 28 帧活跃 profile 原 2,073,600 次计数调用降为 1,134 次，计数值相同。
3. 不融合 fill→verify→matches：中间仍保留完整 public verify 和最终逐字节对拍，外部 writer 注入用例仍拒绝并回滚。不融合 structure→audit：结构失败必须在写 audit scratch 前冻结，擅自合并会改变冻结 raw。保持每个新公共边界全量结构、四趟 radix ID 去重、exact70 全桶/limb/计数/cache 位型重建与严格域检查。没有引入 certificate/serial/checksum 代替证明。
4. `copyDerivedPayloadV2` 按每次实际成功 `.set` 的源 byteLength 计数；包括两个 shakeSource 数组是否存在的分支。field fill 在实际 `.set`/池复制完成处计数，派生 hs/hl/sandTop 的写入单独计账，失败后的已完成前缀不会被误记为零。诊断 counter13 更新为成功 payload 的真实 typed-array copy bytes，**这是唯一有意变化的旧统计字段**；物理状态和其它旧 counter 对拍不变。N512：fill 源复制 37,376B + 派生写 3,072B；consumer copy 有双 shake 数组时 42,496B，缺少时按实际路径扣除，原 43,520B 固定估值退役。标量/对象/string 与 native 隐式复制不计入该数组字节值。

## 同帧收据与取用

`FrameBudgetV2` 固定最近 128 帧×128 F64，32 层预分配栈；typed payload 132,736B。run≤256 字符、token≤512 字符，历史 UTF16 上界 196,608B，另有引用/对象/当前行，不冒充总堆/RSS。满后覆写最近环并显式 `overwritten/incomplete`；倒钟、非法计数、栈不闭合或过长身份显式 invalid。覆盖声明 `RECENT_128_FRAMES_NOT_FULL_RUN`。

row[0:23] 含 callback serial、原 callback ns 间隔、原物理 dt、commit/epoch/N/live、Q/A/R、fill/consumer 真实复制字节、派生写、实际 hot begin 尝试、rollback counter 增量、CURRENT/STALE/FAULT/step/copy/exception outcome、比较字节与 fallback view 次数、wall/clockCalls/zeroSegments/invalid。Q/A 在 `commitPrepared` 成功切换时计数；R 是实际递交 apply 的 request rows，含尝试/被拒请求，不等于获准质量。

row[24:44] 是 **互斥 leaf 时间之和**；[44:64] 是各调用独占时间最大值；[64:84] 是实际调用次数；[88:120] 为旧 32 counters 的帧内增量。段名：callback尾、input、injection、step、preview、collect、hotApply、field、fill、publicVerify、authorityCompare、seal、consumerCopy、sceneSetup、pack、nativeDraw、structure、exact70、strictDomain。嵌套父段扣除子段时间，不把 T311 inclusive 和加进来。Q 同帧区分子步数量，调用次数保留 preview/collect 重试，不将其冒作成功子步。

run/frame/commit/epoch 与 native 实际请求 `frameToken` 同行绑定；保留 raw callback 间隔与 dt 两列。**计时为 Date.now 整数毫秒**，有 zeroSegments/clockCalls；不宣亚毫秒阶段精度，不据模拟秒反推 fps。physics 从不读取这个诊断时钟。最大时钟调用在本离线 workload 为 {max(r[20] for p in rows for r in p['rows']):.0f}/frame，不随逐字节计数增长。

仅隔离 V2 页面开启，Facade 默认 disabled；reset 后保留同一预算对象并重新冷绑定 counters。页面按钮“导出帧预算”按需经 DiagnosticColdFiles 原校验和+fsync 文件出口保存；不自动逐帧 JSON/全池导出。移除 T218 每次 held 注入 hilog 和 T347 每120帧冷全池导出；V2 scene 成功/失败日志限制≤1Hz，其余默认页面诊断策略不变。

## 非机对照与计量

基线 `5970fbd`，`test/t351/baseline.cjs` 可从 git 独立重建；生产数学未移植。新旧 differential **23/0**：N0 原样拒绝（同时检查零池 payload 复制）、N2/16/64/192/512、live0/低/128/512、Q0/1/2、A0/1、重复/失败输入、接触 R/N、替换/detach/alias、外部 writer、回滚，逐位 checkpoint/acc/count/cache/source/commit/elapsed/pool payload、原 counter 除13、第一错误及冻结 buffers 对拍。观察 ON/OFF 对拍；实际复制公式与 Q/A/R 闭合。

`scene_units` **5/0**，CURRENT/STALE/FAULT/异常/无效封签。脚本注入 setup6ms、真实 pack2ms、native 替身7ms 时，总 wall15ms，与互斥段精确闭合；不接受旧 token 冒 CURRENT。

既有 T216 **7/7 套件**全绿：exact70（3208独立 BigInt oracle cases）、tuple、owner、checkpoint、publication、material preparation、allocation。证据重定向到本卡目录，保留其它会话的脏报告。

`budget.cjs` 32 profiles（8负载×before/after-OFF/after-ON/交替ON-OFF），每 profile28帧去前4帧，合计768测量帧。下表单位 ms，为 Node host 整帧 `performance.now`，阶段仍是 Date 整数ms；前版用非机 wrapper 分段，seal 留在其父剩余段。使用 **真实生产 pack + NAPI 替身**，native GPU/提交/显示时长 **NOT_MEASURED**，不得将替身短时长写成设备0ms。不是统计显著性或设备承诺。

| N / 初始live / 负载 | 修改前 p50 / p95 | 修改后观测ON p50 / p95 |
|---|---:|---:|
'''+ '\n'.join(table)+f'''

完整各段 p50/p95、Q/A/R/bytes、每帧原始行、各统计调用数见 budget.json/summary.json。交替采样可见的最大 ON−OFF p50 差为 {over:.3f}ms，含 JIT/计时量化与运行噪声，不宣零成本。早一轮与 hvigor 同时执行的样本另存 budget-with-build-contention.json.gz，未作主表。任何单档噪声或回退如实保留，不挑最好样本、不据这一表宣设备性能 PASS。

剩余工作界保持每边界 O(N+4L+256×70+C)，接触逐行 hot audit 仍有 Ω(R(N+C))，没有用删校验换预算。在 PC 近满池档 exact70 与 hotApply 占主要 CPU 段；pack 并未在该替身环境证明占主导，native 未测。**不能据 host 低于33.3ms 推真机可达**：T347 空场约82ms的差额仍需 T352 原位拆分。

## 构建、签包与 T352 接手

生产 PackageHap 与隔离完整工程（含 BuildNativeWithCmake/CompileArkTS）均 BUILD SUCCESSFUL。生产默认入口/V1 未切换；隔离 HAP 仅入口固定 V2 页面。verify-app/profile 均 PASS，原配置与签名材料未改，不安装、不启动设备。

生产签包 SHA：`{delivery['artifacts']['production']['sha256']}`。

隔离签包 SHA：`{delivery['artifacts']['isolated']['sha256']}`。

绝对路径、source hashes、签名校验、构建压缩日志在 `test/reports/t351/delivery.json` 与 production/isolated-sign.json。隔离包入口直接 V2，保持真实触摸→held→原 admission 数学；无 debug 注入绕过。按需导出最近预算文件，显式记录覆盖数/溢出，不能把128帧环称完整 run。

T352 使用同一隔离签包/SHA/渲染参数，分别空场、保持真实触摸、停源后低/中/高 live 固定短窗，比较原 callback 间隔 p50/p95、Q/A/R/实际字节及互斥段份额；同时核 LAND/床量、原 token CURRENT/STALE、无新 fault。若 sceneSetup/pack/native 占真机主导，请保留该段原接口调用、绑定/prepare次数、same-token receipt和累计时间再交 architect，而不是把本卡 byte 比较优化称完整帧率修复。开发当前范围无阻断；真机验证不作为本卡关闭前置。
'''
text=text.replace('typed payload 132,736B','另有256子步×64 F64和128 F64 scratch，typed payload合计264,832B')
text=text.replace('run/frame/commit/epoch 与 native 实际请求', '子步行含父 callback serial、真实 p011SubstepId/physical attempt、原错误码、commit前后、R/Q/A/复制/回滚以及hot stepEpoch/attemptToken；每子步独占阶段累计与调用次数单独保存，且和父帧可按serial关联。异常未完成子步在帧尾记-1，不冒成功；子步环覆写另记substepOverwritten。旧帧/子步环是两个有界最近窗口，缺失父帧时不能补造token。\n\nrun/frame/commit/epoch 与 native 实际请求')
text += '\n## 并行交付版本说明\n\n共享工作区在本轮收尾期间已有 f06de96 收入部分 T351 修改。T352 正式报告已测试此前隔离 SHA `18399de86843f21780a9cf113ea086e656fca85f084bc4c7e1a47b96f68c976c`：设备空场/触摸帧率未有可测改善，性能 FAIL 保留；该报告没有消费本次最终交付 SHA，不将其改写为新包 PASS 或重写原件。最终包补齐子步身份与互斥账，physical attempt 原值 string/null 保存在有界平行数组（null 表示未启用该物理 ledger 身份，不造号）；hot step/attempt 与子步序号另有数值原值。设备失败首次分流归 architect，开发不据 host 快样本宣布帧率修复。本卡当前范围是工程改造、离线证明、编译签包；后续取证/分流由 manager 按真实前置调度。\n'
text=text.replace('证据重定向到本卡目录，保留其它会话的脏报告。','最终运行通过 NODE_OPTIONS 使所有子进程证据重定向到本卡目录。初次运行的子进程遗漏重定向，覆盖 allocation.json/owner.json 两份既有生成报告；已按开工前5970fbd确定性重生成恢复，allocation 与未被覆盖的原 allocation.txt 容量/缓冲数摘要吻合。没有初始逐字节备份，故不声明恢复字节与原未知字节校验一致。report-recovery.json 与压缩证据保留原错及恢复方法，最终子进程回归后再次校验恢复文件未变化；未覆盖或恢复其它人的源码。')
(r/'docs/developer-T351-v2-frame-budget-v1.md').write_text(text,encoding='utf-8');print('report written')
