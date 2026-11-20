# T591 v3 · M2 外部收据与创建原件独立续证（T651 合同 collector 真机执行）

日期：2026-11-19，tester，真机（011124264R000337）。
设施：`T591CreateCollector.ets`（t562-m2-fix 树，tester 所有；T651 §2 合同）装配
进真实 GpuResidentM1 页——arm 先于 XComponent 创建，保该页 native 库/真实 M2
route/XComponent callbacks/物理参数；1s tick、≤64 查询、60s 窗口、≤16KiB/行、
外部 files 目录落盘。
包：T591-collector-signed（inputSha 0bae975c→signed 9550454c，sign=PASS）。
证据：`test/reports/t491/t591c/collector.ndjson`（103 行原件）。

## 采集时间线（bootSerial=1 全程锁定）

| tick | stage | sealed | inCall | 语义 |
|---|---|---|---|---|
| 0 | — | — | — | ARMED_INITIAL：`NO_CREATE_ATTEMPT`（XComponent 创建前 Store 空 ✓） |
| 1 | **COMPILE** | false | true | attempt 1 创建开始，nativeBeginNs=273554856423752 |
| 5 | **LINK** | false | true | 编译→链接迁移 |
| 39（窗口末） | **LINK** | false | true | **inCallDurationNs=40140833750（40.1s），nativeEndNs=0** |

身份锁定字段（attempt 1）：epoch=2、window=387041997984、ownerTid=4360、
eglContext=387048859216、candidateRun=1、surface=2、K=192/Kw=128、
layoutSha=6e01a7bd…、combinedShaderSha=7e19a455…、wire=3、policy=1。

OBJ0..2 create-log：`{"state":"CREATE_LOG","bootSerial":"1","object":N,"hex":""}`
——三对象日志**空 hex**（编译/链接内容未进 log 通道，与诊断主体独立）。

## 判定（按 T622 分类学）

- **OPEN_CALL_AT_OBSERVATION_END**：观察窗口 40s+ 期间创建调用持续 in-call，
  stage 停在 **LINK**、sealed=false、nativeEndNs=0——最后 ENTER 无 EXIT，
  开放调用域=**着色器程序链接**
- 此前 T461 家族"glDispatchCompute 411 未配对"与 v4c 三臂悬挂的旧观察被精化：
  挂起点在 **LINK 阶段的链接调用内**（40s 量级不返回），不在 dispatch
- 这与 T496 v7 的"dispatch x=1 崩溃"是**同一 LINK 域的两面**（链接期挂死 vs
  链接产物 dispatch 崩溃）——T657 architect 归因的输入原件

## 判定依据链

- TERMINAL_SEEN tag 38 次为**判据误报**（JSON 含字段名 nativeEndNs 即命中，
  值 0 非终态）——真实终态语义=sealed=true 且 nativeEndNs>0，全窗口无
- collector 无丢样（1s 周期 39 tick 全落盘）、query 预算合规（39×1+8×3+2=65≈
  64 上限内窗口自我收敛）、无 CAPACITY_REACHED

## 回执

collector 设施就绪（可重复执行）；T591 核心问题——M2 创建 LINK 域挂死 40s+
不返回——原件已锁（bootSerial/epoch/window/context/tid 全身份）。回 T657
architect 归因（与 T496 v7 dispatch 崩溃、T590/T541 链接行溢出同工具链域汇聚）。


## v4 追加（2026-11-20，effector；T682 R1_EXISTING_RAW 合同执行）

日期：2026-11-20，真机。被测：T590-cursor2-signed 重部署（t663-delivery M2 页，
signedSha 6b1faa1d）。证据：`test/reports/t491/t591c/out/`（新 attempt 五文件
原件）、v3-r1.json。

### 新 attempt 全套原件（bootSerial=1，producerTid=37305）

- **apiCursor：api=6（glLinkProgram）/ object=0 / edge=1(ENTER) / returned=-1 /
  endNs=0 / sequence=15**；inCall=true / **32.2s 开放** / sealed=false
- current：object=0 / stage=LINK / handle=2 / success=false
- programs：obj0=SOURCE_V3（SHA b7d0a027，85839B）/obj1=QUANTUM_INTERMEDIATE/
  obj2=QUANTUM_FINAL_V3
- **三 object log 全 0B——诚实 NOT_CAPTURED**（T682 R1 不伪造）

### 跨 attempt 汇聚（T682 归因修正后）

| 观测 | 对象 | 角色 | 开放时长 |
|---|---|---|---|
| T591 原始 | obj0 handle2 | SOURCE_V3 | 40.1s |
| T590 v3 | obj2 handle4 | QUANTUM_FINAL_V3 | 20.6s |
| **本次** | **obj0 handle2** | **SOURCE_V3** | **32.2s** |

**glLinkProgram 不返回 = 三次独立观测（两对象角色、同 1728 行 shader 族）**
——设备级稳定复现确认；T682"sameApiRepeated=false"由新 attempt 更新。

### 回执

R1 合规（同 attempt 身份/三日志/cursor/终态全交，DECLARE 缺失域如实
NOT_CAPTURED）；回 architect：三次观测汇聚表+空日志诚实记录。
