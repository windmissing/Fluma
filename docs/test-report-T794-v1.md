# T794 v1 · 目标 20 G13 同身份 BOOT 暖命中真机补证

日期：2026-11-21，effector，真机（011124264R000337）。
被测：T760 同签包 868106e2（映射 e5ec50ff、G13ResearchAbility、cache
key bacbc911/301961B）。**cache 缺失确认**（后续轮刷除）→按合同先补
冷轮。证据：`test/reports/t491/t794/`（diag.json 资格原字、warm-domain
.txt G13Init/G13Boot 全套原件、v1 判定）。

## 冷轮重建

T760-split-signed 全新安装 → REST 配置 420s → bacbc911 cache 重建
（2 条目）。

## 全新进程暖命中（本轮核心）

force-stop → 全新进程 **pid 28376**（processStartTicks 1064684 存活时
直采），映射 BuildId **e5ec50ff** hilog 验证：

**G13BootProgram 原件**：`route=BINARY_LOAD`，cacheKey bacbc911，
binarySHA 3368e191，**LOADED_FILE_OBSERVED + memoryBuildIdMatched=true**
（路径 /data/storage/el1/bundle/libs/arm64/libgrs_g13.so）——
**零源码编译/链接，纯 binary load 路线确认**。

- 对象：program 1/ctrl 7/work 6/fenceHandle 1
- 同 role 反射：REFLECT EXIT sourceSHA e1893369（T756 双 scope 布局）

## 原 FP32 终态（不变）

**OP65 result=27（attempt 103）**：op65 WORK
`[0,1065353216,1065353216,0,1065353216]`——+53 FTZ 与 250/196/180 逐字
相同；op66 mismatchMasks [12,12]；fence 37146、ctrl255 0xc0de1301、
ctrl7 27。G13BootOp65/G13BootReadbackV1/G13BootScope 三套原件 hilog
侧全采获。

## 合同遵守

无 RX 代用、无产品探针、无旧 PID——G13ResearchAbility 真实暖路径；
在 T770 覆盖之前执行（probe 不在设备上，无冲突）。

## 回执

同身份 BOOT 暖命中证据完整（BINARY_LOAD 路线+零编译链接+同 role 反射+
原 FP32 终态）——数值门 FTZ 族不变。
