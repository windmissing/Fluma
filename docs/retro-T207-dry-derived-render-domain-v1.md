# T207 干场派生量与严格渲染域复盘 v1

2026-10-28，designer；P011_DRY_DERIVED_DOMAIN_V2_DESIGN；无真机／无实施。

T202已经闭合直接拒绝谓词：微负粒子列cache进入dry包络，strict renderer在F32/GL之前拒绝；并非没有导出或没有新管线接线。完整pool和首残差writer未捕获，因此本篇只处理已证跨域合同，不猜旧列无粒子，也不把一个合成减法序列说成设备唯一首因。

比较renderer加EPS/钳零、逐事件全池fold、UI-only正包络投影、固定slot sum-tree与exact70成员归约。选择exact70：成员变化按实际before/after tuple更新，最终RN一次；空集合为+0、非负集不会负、局部更新固定字宽，publication owner统一封存。代价是cache位型及边界grant可能改变，必须显式数值V2，不能用“派生量”三个字冒旧物理轨迹逐位等价。

保留原权威F64 writer、source成功／step回滚、H/drift/预算规则和原审核cutpoint；新增V2 derived verifier只在public接受边界重建一次，不每UI或每事件加全池扫描，所有字宽、copy/undo与分支成本接入T197/T205。

严格renderer不放宽；非负床和正live质量可证明包络次序，负床／超capacity／旧schema仍拒。基础新管线已完成与双态沙稳定展示未完成分清。下一步是architect工程化、开发、独立和同DEMO窄设备证据，真实效果／性能未到不写VALIDATED。
