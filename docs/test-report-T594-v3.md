# T594 v3 · V5 独立入口真机直达——configure 接受后 M5_NOT_READY 持续原件

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T669-v5-signed.hap（T653 unsigned 基+V5 native libgrsm1 3201504B/
BuildId 7f31c1de，T691 指定 V5 工件；signedSha ce9ba9af，sign=PASS）。
证据：`test/reports/t491/t594b/`（c1–cD 布局、v5-m3b-first/last.json、
v3-v5-entry.json）。

## 入口路径（T691 合同实测打通）

无参启动 → Index「**开启独立 GPU M2 V3 空场**」→ M2 V3 页「**打开 V5 独立
研究**」→ **GpuResidentM5 页直达**（V5 独立研究：真实来源层与活动粒子；
ROOT/ FULL/投沙/加水/合成信号/冷档状态/导出 V5 冷档 按钮全在）。

## 执行链与原件

| 步骤 | 实测 |
|---|---|
| ROOT 新空运行 | m5Configure **ok=true / expectedN=32**；reply state=**M5_NEW_EMPTY_RUN_REQUESTED** / requestTicket=1（M3B-last 原件） |
| 冷档状态 ×3（间隔 30s+） | **M5_NOT_READY / API_SEMANTIC / qualification=NOT_EVALUATED**，attemptId 2→3→5 递增 |
| boot 导出 | V5 页无 t456-boot（M3B-first/last 通道）；M3B-first state=COMPLETE / NO_FIRST_FAILURE |

## 判定

- V5 创建链：**configure 已被接受**（ok=true）但 m5ColdCapabilities 在
  attempt 2→5 间持续 M5_NOT_READY——创建臂未发布 READY，ROOT/FULL 冷档
  链卡在 **M5_NOT_READY 门**
- 与 T593 v5 CURRENT 窄窗、T541 SPF WAIT_RETIRED 构成第三条 create/ready
  路径流——三流汇聚回 architect
- RX shader 缺陷（T594-rx 树）与 V5 域**独立**：V5 包是另一 native
  （libgrsm1 3201504B），冷档验证可继续沿 V5 域推进（不等 RX 修复）

## 回执

V5 入口/页面/按钮/请求链原件齐；M5_NOT_READY 持续原件交 architect 定
V5 资格链缺口（API_SEMANTIC 域）。


## v4 追加（2026-11-20，effector；T707 RX 修订产物设备验证）

日期：2026-11-20，真机。被测：T707-signed（RX 修订产物，signedSha dfc7f60e；
libgrs_g9rx.so 1404664B/SHA 4728e218/BuildId 624ba445）。证据：
`test/reports/t491/t594b/`（d1–d4 布局、h594d.txt、v4-t707.json）。

### 执行链

grsm1 直达 RX 研究页 → 启用研究：
- **LOADED_FILE_OBSERVED：memoryBuildIdMatched=true**（T707 修订 so 正确加载）
- **CONFIGURE_FAILED_8**：driver `0:1097: S0001: Function call discards
  coherent access qualifier`——错误行从 1063 移到 1097（同 S0001 类）

### 判定

- T691 修订变换（两处 merge 分支局部化）已进包并加载，但 **coherent 拒因
  仍在**（新行 1097）——第三个未变换调用点存在，或变换未覆盖实际被拒构造
- 修复链：S0022（slot）FIXED → S0001（coherent）仍 OPEN @1097
- 回 architect：T691 变换不完整，1097 行需重查（第三分支 g9AddMass 调用或
  已变换点仍被拒）——同 S0001 族非新缺陷类


## 复测追加（2026-11-20 10:50，effector；用户点名 T594/T591/T654 再测）

复现性确认（当前设备最新包，rerun-all.json）：
- **T594 RX 臂**：readonly@1987 **精确复现**（hilog 同行同类）——稳定
- **T591/T712 DM 臂**：ACTUAL_GL_EXECUTION+CREATE result=8+coherent@1063 **精确复现**——稳定
- **T654 G13 域**：G13 页在当前 RX 包不可达（无路由）——NOT_EVALUATED 维持，需 G13 包取配置化快照

无状态漂移；阻塞臂与上轮一致（等限定符变换交付）。


## v5 追加（2026-11-20 11:32，effector；T716 readonly 修复包验证——**编译门首次通过**）

日期：2026-11-20，真机。被测：T716-rx-signed（readonly 修复，inputSha
d07b9eda→signed b4b4e7a3，sign=PASS；T716 compile receipt BuildId
317645e3）。证据：`test/reports/t491/t594c/`（r1–r4 布局、h716.txt、
v5-t716.json）。

### 里程碑

- 启用研究后状态 **`CONFIG_QUEUED`（不再是 CONFIGURE_FAILED_8）**——
  **shader 编译门首次通过**：hilog 零 S0001/S0022
- 限定符链设备闭环：S0022（slot）→ coherent@1097 → readonly@1987
  **三层全部清除**
- CONFIG_QUEUED 持续 55s+（重复点击无跃迁）——队列未消费

### 判定

- 编译门通过=T594 系列首个 configure 阶段非编译失败状态
- 下一门=**队列消费/apply 路径**（T710 worker 握手族或主线程 apply）
- 回 architect：查队列消费路径；Create/READY/Swap 如实 NOT_REACHED
  （卡 CONFIG_QUEUED）
