# T541 v2 · T564 真源差分 harness 重建受阻（工具链死锁）

日期：2026-11-18，tester，host 构建（未到真机步骤）。
目标：按 T564 真源（t564-stage-tree）重建 g9_differential 独立差分 harness。
证据：`.codex/local-test/t564-stage-tree/build.log` + 本报告记录的进程级观测。

## 执行与受阻经过

1. T549 frozen 的 `t549_harness.cpp`（8377B）迁移至 T564 真源树，CMake 增
   `add_executable(g9_differential)` target（测试设施，含
   `libEGL.so libGLESv3.so libace_ndk.z.so` 链接修正）
2. t564 树 hvigor 全量构建：COMPILE 阶段在 **g9spf/kernel_assets.cpp** 挂死——
   clang++ 进程 CPU 累计 **0.05s / 38 分钟**（4 线程全 idle，确认非慢算）
3. 杀 clang++ 后 ninja 重试进入 **LINK 阶段**：ld.lld 挂死 CPU **0.03s / 60.5 分钟**
4. 两处死锁均为**同一模式**：>140KB 内嵌 R"()" GLSL 字符串单元
   （g9spf/kernel_assets.cpp 与 T573 的 g9dm/kernel_assets.cpp 同类，源 SHA
   11276222… 两树一致）在 OHOS 26.0.0 工具链（clang++/lld）编译与链接时 CPU 零
   挂死；主树上午 g9dm 单元同样挂死（5 小时 33 分后杀），其 foundation.a 里的
   .o 实为旧编成缓存

## 结论与回执

- **T564 真源差分 harness 在当前工具链下不可重建**——差分三分域（COMPUTE_ONLY/
  SCENE_ATTR/PRESENT）与完整独立差分 **NOT_REACHED（工具链阻断）**，不冒旧
  T549 frozen（3640304B，T579 v1 交接过但 manager 已注明不能以旧 frozen 推新版）
- 精确原错回 **architect 首次归因**：
  `clang++/lld (OHOS 26.0.0) hangs CPU-zero on >140KB embedded R-string TUs`
  （g9dm/g9spf kernel_assets 两单元同证）；需工具链规避方案
  （拆分 GLSL 字符串单元 / 换编译器版本 / 编译期宏分片），属工程设施缺口，
  tester 不补产品实现
- 本轮完成的准备（保留）：t564 树 CMake 差分 target + harness 源迁移 + link 库
  修正——规避方案就位后可直接续跑
