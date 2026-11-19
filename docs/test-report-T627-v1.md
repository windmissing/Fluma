# T627 v1 · 共享 cold broker 跨 DSO 运行共享独立验证

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T600-composite-signed.hap（正树 grsm0+grsm1+libgrs_cold_slots.so 同包，
SHA 334fd90f…），bm clean 后冷启 grsm0 进程 PID 21802。
证据：`test/reports/t491/t627a/`（maps-grsm0.txt /proc/21802/maps 全文、
M0 时间线 manifest+part0、布局快照）。

## 已证事实

### 1. NEEDED 链（llvm-readelf，包内 ELF 直读）
- `libgrsm0.so` NEEDED: … **libgrs_cold_slots.so** …
- `libgrsm1.so` NEEDED: … **libgrs_cold_slots.so** …
- `libgrs_cold_slots.so` NEEDED: 仅 libc++_shared/libc（无递归）

### 2. 设备加载映射（/proc/21802/maps，真机运行时）
- **libgrs_cold_slots.so 单实例**：5 段映射同一 inode（104:87 51578）、同 vaddr
  基址 5a30600000——dlopen 单份静态数据（broker 的 slots[]/mutex/gpuTicket）
- **grsm0 与 grsm1 同进程同时装载**（各 5 段）——两 DSO 引用同一 broker 地址空间
  实例；进程级单实例由加载器保证（非 NEEDED 静态声明）

### 3. grsm0 运行 cold 链（同进程活跃证据）
M0 页原生时间线 34 行：InitBegin×2 → ShaderEnter/Exit×2 → ProgramEnter/Exit →
ResourceEnter/Exit——cold slot 资源域在运行中真实经过 broker API
（state=INCOMPLETE 为活 owner 诚实截断）。

## 不可观测域（如实）

- **跨模块 ticket 交叉连续性**：grsm1 的 GL 域未在本进程激活（页面路由只有
  M0 单页可进，M1 无法同进程切换），timeline/g13Status 均无 ticket 输出域——
  broker ticket 交叉证据需要 grsm0+grsm1 **同时**跑 cold 链的可观测端点，
  现有产品无此观测面（缺接口，保原样回 architect）
- broker 组件逻辑退账≠driver 物理 free（T617 v1 已分列）

## 判定

- **加载映射级运行共享证据成立**：单 DSO 实例 + grsm0/grsm1 同进程双装载
  （超出 NEEDED 静态声明的运行时证明）
- **运行时 ticket 交叉**：NOT_OBSERVABLE（缺观测接口）——若 architect 判需，
  由 developer 暴露 broker ticket 快照端点（如 g13Status 同类的只读诊断）后
  tester 续测
- 与 T617 v1（host 组件 21/21 PASS）合并为 broker 合同两层证据
