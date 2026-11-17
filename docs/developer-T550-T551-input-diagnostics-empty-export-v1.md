# T550/T551 交付报告：GPU 严格输入首拒诊断与空日志导出修复

日期：2026-11-17。责任：developer（鸿蒙开发工程师）。
任务卡：T550（滴答 `6afc0930e4b0ed57d1250cdd`）+ T551（滴答 `6afc0930e4b02f369e61bb3c`）。
合同依据：docs/architecture-T546-napi-rejection-and-safe-reply-v1.md（§2-§5）+ docs/architecture-T547-create-and-empty-export-triage-v1.md（§5-§6）。
状态：**DELIVERED_COMPILE_ONLY**（开发/编译/提交完成；测试按角色边界未执行，交独立 tester；设备 NOT_RUN）。

## 1. T550 交付（T546 §2-§4 全项）

### 1.1 共享严格解码合同（§2 十阶段逐项观测）

- 新文件 `entry/src/main/cpp/gpu_resident/napi_words_contract.h` + `.cpp`（问题 T546/方案 v1）：
  - `ReadWordsResult<N>`：words 定长数组 + `Observation`（CB_INFO/IS_TYPEDARRAY/TYPEDARRAY_INFO/ELEMENT_TYPE/ELEMENT_COUNT/BYTE_OFFSET/DATA_POINTER/DETACHED_API/DETACHED_VALUE/BACKING_INFO/BASE_POINTER/BACKING_BYTES/WORD_COPY 逐阶段原始 status 与三态输出 VALID/API_FAILED/NOT_EVALUATED）+ `firstFailedStage` + `copyComplete` 资格位。
  - `ReadWords<N>` 按序执行十阶段判定：argc 读取容量 2 识别额外参数（不把容量 1 截断当 argc==1 证明）、不隐式转换 number[]/DataView、元素数（SDK 语义：length 是元素数非字节）、offset==0、data 非空、detached API 成功且 false、backing base==data 且 bytes==N*4、全过后 memcpy 到定长 native 数组（不保留 JS 裸指针）。**接受谓词与旧 m3Words/m5Words 逐位一致——只加观测层，零语义变更。**
  - `DiagnosticStore`：三 route（M3A/M3B/G9）各 lastAttempt + 本 generation 首失败两份 512B 快照（firstFailureLatched 首失败锁定）；快照含 revision=GPU_INPUT_DIAGNOSTIC_V1、boot/diagGeneration/attemptId、expectedN/expectedBytes、firstFailedStage、各 NAPI 原始 status、typed/type/length/offset、dataPresent/basePresent/baseEqualsData、backingBytes、detached、copyComplete、已复制时前 16 word 配置头；原指针地址不落盘；未执行项 null/NOT_EVALUATED 不伪 0。独立小锁，App 锁外记录（锁序合规）。
- 三条路线薄委托：`m3a/napi.inc`、`m3b/napi.inc`、`g9spf/napi.inc` 的 `m3Words/m5Words/g9Words` 全部改为 `grs::words::ReadWords<N>(..., Route::…, 0)` 薄委托（G9 用 Route::G9，不再被标成 M3）；bool 兼容签名保留，调用方零改动。

### 1.2 冷诊断端点（§3）

- `app_bridge.cpp` 新增 `gpuInputDiagnosticsV1(route, attemptId?)`：route 为 M3A|M3B|G9，attemptId 为十进制 U64 字符串选本 generation 首失败快照（缺省 lastAttempt）；不匹配返回 SNAPSHOT_EVICTED_OR_NOT_FOUND，绝不拿旧回执解释新调用；单快照 512B 展开（≤4096B 响应）；诊断请求不走被诊断的 typedarray 解码器、不覆盖被查输入回执。M0/M1 无条件注册。
- `index.d.ts` 声明 `gpuInputDiagnosticsV1:(route:string,attemptId?:string)=>string`。

### 1.3 JSON 端点安全错误语义（§3 JSON 端点类）

- 状态端点（m3Configure/g9Configure/pulse/target/offer 等）保持原具名字符串/十进制返回（它们不被 JSON.parse），兼容不变。
- G9/M3/M5 启动入口改为按成功 token 核准（见 1.4），错误经页面 parse 守卫生成受控失败而非崩退。native 侧普通拒绝路径未改（拒绝即返回原 token 字符串）——T546 §3 的 GPU_NAPI_ERROR_V1 对象 JSON 由后续证据版本按合同跟进（本卡范围：页面侧已能识别 GPU_NAPI_ERROR_V1 并停止，LEGACY_NON_JSON_REPLY 有界显示）。

### 1.4 UI 启动事务与解析边界（§4）

- `GpuResidentG9.ets`：新增 `parseNativeObject(raw, api, allowedRevisions)` 共用守卫（字符串/长度上界/caught JSON.parse/非空非数组对象根/revision 门/words 恰长且 safe integer 0..0xffffffff 才建 Uint32Array）；`start()` 整体 try/catch——清理（cold.cancel/closeArchive/setActive）各自 catch 不覆盖首因；旧 frame owner 置空 frame=null；仅 `G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED` 成功 token 才安排新 pulse；g9Session 解析同样走守卫+identity 逐字校验。
- `GpuResidentM3.ets`：start 整体 try/catch + `M3_NEW_EMPTY_RUN_REQUESTED` token 核准 + 失败附诊断提示（gpuInputDiagnosticsV1('M3A')）；m3Session 解析 catch 守卫。
- `GpuResidentM5.ets`：同型 `m5ParseNativeObject`（T517_EXPLICIT_RESEARCH_CANDIDATE_1 字符串 revision 分支 + words 逐字校验）+ `M5_NEW_EMPTY_RUN_REQUESTED` token 核准 + start catch；m5Session 解析 catch 守卫。
- 失败路径：不安排新 pulse、不自动重 configure、清理错误另列、首因保留；页面呈现明确 FAULT 文案。

### 1.5 同源 generator（§5）

- 新 `test/t550_words_contract_overlay.cjs`：复制 canonical `napi_words_contract.{h,cpp}` 入交付树；**折入交付树 grs_foundation 源列表**（不是新链接库——OHOS 工具链按链接库条目复制 linker flags，链接命令已在 cmd.exe 8191 字符边界上，新增库条目会越界；折入使链接行与 T550 前逐字节一致）；向交付 app_bridge.cpp 注入 include + gpuInputDiagnosticsV1 + 属性注册（幂等、锚点唯一校验）；追加 d.ts 声明。
- `test/t522/bridge_overlay.cjs`、`test/t529/bridge_overlay.cjs`、`test/t517/bridge_overlay.cjs` 末尾各加一行 `require('../t550_words_contract_overlay.cjs')(main)`——三交付链同源生效；m3a/m3b/g9spf 目录由 generate.cjs 从 canonical 复制，新委托自动携带。

## 2. T551 交付（T547 §5-§6 全项）

- canonical `test/t468/M2CreateColdFiles.ets` saveRaw 修订：保持原 filename/hex≤8192/偶数/小写门、write 循环/fsync/finally close 原样；**仅当空文件已成功 fsync+close 后**（bytes.length===0）返回数学常量 SHA256(empty)=`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`，**不调用 md.update 空数据**（真机行为：空 update 抛 "update failed" → CREATE_EXPORT_FAILED，T461 现场已证）；非空数据仍走原 crypto SHA256；I/O/hash 失败仍如实中断。该常量仅合法零字节域，不用于非空或 I/O 失败兜底。
- 原协议链（三 raw + native.json + manifest + WS_EVENT）不变；page.inc（exportCreateDiagnostic）零改动——空 raw 由此成为合法回执（bytes=0 + 标准 SHA）。
- generator 同步：`test/t468/generate.cjs` 逐字复制 canonical → 交付树自动携带修复；T482 历史交付树不回改（按 T547 §5 只修 canonical 与生成链）。
- 旧 `test/t468/page_units.cjs` 的 fakeCrypto（空 update 接受）属测试设施，按 T547 §5 交独立 tester 换「空 update 抛错」平台替身并补九类用例；developer 未动测试判据。

## 3. 构建验证（环境与结果）

| 项 | 结果 |
|---|---|
| ArkTS 编译（hvigorw CompileArkTS，本机） | **BUILD SUCCESSFUL**（36.9s；再跑 5.5s up-to-date 复核绿） |
| native 交叉编译（clang OHOS，grs_words TU） | **通过**——libgrs_words_contract.a 产出（ninja grs_words_contract target 2/2） |
| host 语法门（MSVC 14.51 + 最小 napi stub） | napi_words_contract.cpp **CONTRACT_TU_OK** + 委托实例化 TU **DELEGATION_HOST_SYNTAX_OK**（stub 把 napi_env 定义为 int 曾致 C2672 假阳性，修为指针 typedef 后通过——纯 stub 问题非产品问题） |
| 产品 .so 链接 | grsm0/grsm1 链接 exit=0；llvm-nm 确认 `grs::words::SnapshotJson`、`grs::words::Snapshot::JsonEncode`、`DiagnosticStore::Instance`、`gpuInputDiagnosticsV1`（匿名命名空间 local）均落位；研究路线函数（m3/g9/m5）按交付模型由 t522/t529/t517 overlay 注入交付树，产品 .so 本就不含（与 T550 前一致） |
| **链接环境缺陷（如实登记，非本卡引入）** | OHOS 工具链在 build.ninja 生成时把 `-rtlib…-Wl,-z,noexecstack` 47 次×167 字符重复注入 link 命令，总长 8533 字符超 cmd.exe /C 8191 上限——**p013hello（不含本卡任何代码）同样失败**，证明是环境级工具链缺陷。规避：①本卡把合同 TU 折入 grs_foundation 源列表（不再新增链接库条目，链接行与改前逐字节一致）；②`test/t550/direct_link.ps1` 用去重参数数组直接调 clang++ 完成 grsm0/grsm1 链接（语义等价，零 flag 内容变更） |
| 运行验证 | 设备 NOT_RUN（本卡声明不使用真机）；启动/不闪退由后续 tester 卡换正确签包验证 |

## 4. 修改文件清单

| 文件 | 改动 |
|---|---|
| entry/src/main/cpp/gpu_resident/napi_words_contract.h（新） | 十阶段共享解码合同 + 512B 快照 + DiagnosticStore |
| entry/src/main/cpp/gpu_resident/napi_words_contract.cpp（新） | 快照有界 JSON 编码 + SnapshotJson 端点逻辑 |
| entry/src/main/cpp/gpu_resident/m3a/napi.inc | m3Words 薄委托 Route::M3A（谓词逐位不变） |
| entry/src/main/cpp/gpu_resident/m3b/napi.inc | m5Words 薄委托 Route::M3B（消除重复实现，谓词逐位不变） |
| entry/src/main/cpp/gpu_resident/g9spf/napi.inc | g9Words 独立委托 Route::G9（不再经 m3Words 标成 M3） |
| entry/src/main/cpp/gpu_resident/app_bridge.cpp | include 合同 + gpuInputDiagnosticsV1 冷端点 + 属性注册 |
| entry/src/main/cpp/gpu_resident/CMakeLists.txt | 合同 TU 折入 grs_foundation 源列表（链接行零增长） |
| entry/src/main/cpp/types/libgrsm1/index.d.ts | gpuInputDiagnosticsV1 声明 |
| entry/src/main/ets/pages/GpuResidentG9.ets | parseNativeObject 守卫 + start 事务 catch/finally + token 核准 + session 守卫 |
| entry/src/main/ets/pages/GpuResidentM3.ets | start 事务 catch + token 核准 + 诊断提示 + session 守卫 |
| entry/src/main/ets/pages/GpuResidentM5.ets | m5ParseNativeObject + start 事务 catch + token 核准 + session 守卫 |
| test/t550_words_contract_overlay.cjs（新） | 交付树同源注入器（复制/折入 CMake/注入 bridge/追加 d.ts） |
| test/t522/bridge_overlay.cjs、test/t529/…、test/t517/… | 各加一行调用注入器 |
| test/t550/direct_link.ps1（新） | cmd 长度限制规避链接脚本（环境缺陷 workaround） |
| test/t468/M2CreateColdFiles.ets | 空 SHA 修复（§5 唯一合同） |

## 5. 红线核对

- 未改任何物理 dt/阈值/守恒；未删任何 typedarray 门（谓词逐位保留，纯加观测层）；未猜根因（M3/G9 具体拒绝谓词仍 UNKNOWN 待 tester 补证）；未改期限/GAP/120Hz 配额/30fps 显示/defaultOFF；未改 renderer；未动协议正文；未执行测试（角色边界：测试归 tester）；未用真机。
- T551：未改判定式；未加新 endpoint；未扩 4096B raw/4MiB 诊断信用；空常量仅限合法零字节域；非空 crypto 失败仍如实中断。

## 6. 变更记录

- 2026-11-17 v1：初版。T546 十阶段解码合同+512B 快照诊断+冷端点+三页启动保护+三 generator 同源注入；T547 空 SHA 导出修复；链接环境缺陷（工具链 47×167 字符重复注入）如实登记并给出零语义 workaround；CompileArkTS+native TU+host 语法门全绿；签包未产出（SignHap 00303116 既有失败，交 tester 按 T151 手签）。
