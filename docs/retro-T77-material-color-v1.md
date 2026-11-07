# T77 材质色彩差异归因复盘 v1

日期：2026-10-26。designer；路线 `P013_COLOR_TARGET_ATTRIBUTION_V1`。归因设计交付，未改业务、未跑GL或真机，未标VALIDATED。

## 候选与取舍

| 候选 | 证据／代价 | 决定 |
|---|---|---|
| 因default暗就给材质shader加gamma | 默认最坏点具有线性码特征，但同shader在SRGB8 FBO简单OFF近乎精确；加OETF可能在正常目标双编码 | 拒绝。先分目标写／读链，保留唯一硬件OETF合同 |
| attachment报告SRGB所以default测量可忽略 | 实际145→72，与预期编码不符；属性与功能／屏幕证据不等价 | 拒绝。保留异常，常量fragment和实际状态三角补证 |
| 1.956码看着小，统一算通过 | 未冻结数值阈；且最坏样本有baseByte172/173的离散分支，并非纯输出半码量化 | 拒绝。冻结F64主参考不变，F32影子参考只作诊断 |
| 调palette/gain/纹理或提高容差 | 会掩盖目标域和精度边界，还破坏签收配方／Q增量解释 | 拒绝 |
| 把原因定成ANGLE bug／Harmony也错 | 只有Windows ANGLE离屏证据，写／读和状态未分开 | 拒绝唯一平台定因。报告最小域假设及缺失事实 |
| 先停T56等所有色彩结论 | 结构／FrameToken／LastGood与色彩数值验证可分别推进，没有证据要求阻塞结构实现 | 不采用。T56继续，产品颜色结论留T57同帧A/B/C |
| 分成目标矩阵与逐级数学探针 | 可以用简单OFF、常量shader和一个精度反例定位首个差异，避免改配方 | 推荐，测试目录独立、原始raw不覆盖、记录全部格式与来源 |

## 本轮发现

1. 两种GPU目标共用scratch目录，当前raw只完整匹配FBO。default JSON摘要仍可分析，但不能伪造成default全图复算；后续每target/run独立归档。
2. 源码raw hash不相等是CRLF/LF差异；规范换行后与T55提交一致，组合shader hash也吻合。身份核对必须说明规范化，不能写raw一致。
3. CPU使用实际Float32 profile后，JS double乘法仍与GLSL float乘法不同。150×1.149999976158142在double下位于172.5以下，binary32可落到172.5；取整的1码再经Q增量可放大。只说“CPU已经用Float32输入”不能排除数学分支差异。
4. 默认GL_BACK查询是初始化一次CHECK，FBO格式虽明确，实际逐目标read/draw/write-control与读回域还没有完整归档。只有一次SRGB标签不能证明实际数值链或显示链。

## 后续边界

tester分别补H1常量目标矩阵、H2 GPU中间量，整体归因仍由designer负责。若确认具体实现问题，再按主报告责任hunk最小修复；不动T75上传合同，不借T56重写配方。T76的bridge环境阻断单独处理，不绕过系统保护。T57必须将FBO、window、最终新管线画面的A/B/C同帧关联；所有背景前景shader输出，无Canvas回退。本轮没有产品／设备通过结论。
