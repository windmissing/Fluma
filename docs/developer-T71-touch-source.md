# T71 触摸零源最小修复交付

日期：2026-10-26。角色：developer。工程合同：`docs/architecture-P051-touch-input-revision-v1.md`。不使用真机，设备端到端交 T82。

独立无绘制输入 Stack 覆盖与 XComponent 同一父级/同一内容矩形，Block 命中；render 节点 None，不再依赖旧父 Stack/XComponent 触摸入口。几何首次真实测量前 `ready=false`，输入拒绝；测量同一个 vp W/H 和 geometryEpoch 并用于渲染 context。start 显式回执，单主指 Down/Move/Up/Cancel 与模型内部 stopGeneration 同步。仅沿原时钟产生真实干场请求，不改源率、准入、物理或 stale 决策。诊断复用 T72 的 256 ring 和受控 drain，记录输入事件、几何与身份，不另建队列。

`node test/run_t71_touch_source_units.cjs`：11/11，包括真实正源、Move-only 零源、多指、空 Cancel、边界、hide/reset/resize/stale/物理失败、诊断 ON/OFF 状态逐值一致。T59/T49-T51/T72 及 planning/gesture/input/particle/advection/S3/S4 共 12 个现行 Node 套件 exit0，详情 `test/reports/t71/regression.json`。T62 历史源码断言 `submitDualFieldSurfacePx` 1 项 FAIL，因为 T56 工作区将页面路由迁移到 `drawNewPipelineFrame`；原用例保留，独立报告 `render-repair.txt`，不将其记为 PASS。本卡业务修复的 11/11 不依赖旧源码断言。

hvigor `CompileArkTS + BuildNativeWithNinja + PackageHap` 在 T85 共同工作区构建零错误；完整现场命中、真实可见沙由 T82/T56 后续独立验证。T71 与 T56 共享页面：本提交仅精选输入 hunk，保留未提交场景渲染 hunk；当前工作区运行的页面同时包含 T56 候选，二者同页集成编译通过。
