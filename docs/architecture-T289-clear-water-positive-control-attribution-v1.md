# T289 明水正控与量化续证首次归因 v1

2026-10-30，architect；原卡 `6ae3f6c7e4b0f69bc56bc7a9`。不使用真机、不重跑测试。消费 `docs/test-report-T286-cw-coverage-resample-v1.md`、`test/reports/t286/arm1v2_diag.json` 与 `arm2_diag.json`、T268 §5、T270 诊断源码和 C V3 shader。本稿只判断 T286 的正控/ROI/量化推断；T271 四笔正水 ACK 和同 token W→native 数值链的窄结论保留，GPU M1 效果仍优先。

## 1. 两臂必须按各自 run 的逐列 profile 核对

T270 `cwdiag::sample()` 的 `nativeWater` 是遍历128列 `profile[8*c+1]` 的**全局和**，`ledgerWater` 也是全局账本；二者相等不代表 ROI 中有水。T286 的 ROI `(740,0,4,4)` 在未倒置、box1178×2026时 x 只覆盖模型**列80**（`floor(x/1178*128)`）；不能笼统写“列80–82”。

| 证据臂 | 同 run/frame 的实量列 W | ROI 判定 | 允许结论 |
| --- | --- | --- | --- |
| arm1v2，run `cw-run-1793324007330-1-1`，frame700–702 | 全局 W≈0.033364079，几乎全部集中在**列127**；列80≈`2.5e-15`、列81≈`3.4e-15`、列82=0 | ROI列80**无实量水**；`nativeWater=0.033364`只是全局和 | 三层背景是无水点位观察，不能作自然可见正控、不能推公式/量化根因 |
| arm2，run `cw-run-1793324137619-1-1`，frame260–261 | 全局 W≈0.06673335；列80≈0.000885415，列81≈0.006506775，列82≈0.006409149 | ROI列80有极薄实量 W，但并未采到列81/82；frame262列80=0 | 此低量 ROI 的三层背景成立；未提供理论可辨正控 |

这两臂不能相互借列 W：arm1 的全局 W、arm2 的列80与 T271 旧 run 的列80–82均属不同 run/token。arm1 原始首次导出 frame260–262 `nativeWater=0`，正文引用的是 arm1v2 的 frame700–702，须明确分开。T286 报告称“自然正控 ROI 与实量 W 列相交”和“实现与公式吻合，非工程缺陷”均超出原件。

## 2. 源窗口与像素判线仍缺关键证据

arm1v2 声称自然长按20秒，frame700 的 `frameData` 为 sourceCommit=449、physicsCommit=2、全局 W≈0.03336；同档 `count=256,dropped=1528`，保留下来的 rows 仅 `SELECT/TICK/STOP`，没有 DOWN、CLOCK_WINDOW、ENQUEUE、TERMINAL_QUERY。无法由墙钟20秒或 commit 计数证明持续有效源窗口、真实 requested/admitted 总量、拒绝/停止原因。arm2 frame260 的 sourceCommit=178、physicsCommit=4、`dropped=540`，其 terminal 行亦已不在保留环中；T271 的旧 run receipt 不能补新 run 的缺失。

即使某列 body alpha `1-exp(-sigma*W)<1/255`，也不能单凭这个不等式推出最终 `SRGB8_ALPHA8` 字节一定不变：body 只在像素中心位于 `[S,S+W]` 时绘制，surface 层另以像素面积/坡度/Fresnel/光照计算，随后与背景做预乘混合及编码/量化。arm2 列80的 W≈0.000885 相当于约0.007个垂直像素；这只是**薄层负例**，不满足“理论可辨且实际应覆盖”的正控。报告所用全局 W=0.033/0.066 不是 ROI 单列 path，`path≥0.158` 的单一阈值也不能替代两层与实际 sRGB 背景的逐通道预期。无水 arm1 和极薄 arm2 的相同背景不能证明 shader 正确，也不能定位其故障。

T286 的 T271 run 勘误还反向写错：`test/reports/t271/t271_diag3.json` 的 rows/frameTokens 实际 run 是 `cw-run-1793274115973-1-1`；`cw-run-1793273652501-1-1` 是 T271 报告首页所写值。T286 报告把后者又称“第三轮实际 run”，需 tester 修正而不改写原证据。T286 还称 `0.066733` 高于 T271 的 `0.066748733`，数值方向相反；这不影响两档各自原值，但不能当比较证据。

## 3. 分流与下一次可判定的正控

本轮为**测试点位和正控定义问题**，交 manager 裁决修用例/报告；当前没有证明新 shader/源工程缺陷，也没有足够证据派 designer 调 sigma、水量或积分窗口。tester 用现有 `cwdiagX/Y/Frame` 和同一个新 run 的封存 profile，先从**该 run、该 frame** 的 W>0 列反推 ROI、box/镜像/GL左下原点；记录 ROI 覆盖的具体列和像素、S/W/eta、两层各自理论线性输出与预乘混合后的 sRGB8 期望字节。自然源窗口需封存 DOWN→有效 clock→每笔 terminal 的同 run 请求/准入/停止及 sourceCommit，在 rows 容量被 TICK/STOP 淹没前导出或用版本化有界补口；不得用20秒墙钟或全局 ledger 代替。正控必须有**真实产生的**水覆盖足够像素中心/面积，且按现有公式与目标格式预测相对层2基线有明确非零字节差；再同 token 采层2/3/4 raw ROI 与屏幕原始图。若有效源输入与准入量不符，先把输入/时钟/拒绝首码交回 architect；若应可见的正控实际仍零差，带 profile/首错回 architect 分工程层；若逐层与公式一致而体验仍差，才交 designer 与 effector。旧代终态仍需另造真实待决旧请求场景，不与本卡薄水负例混算。

不改变水量、物理、光学参数或 endpoint 协议来制造 PASS。所有绘制仅新 XComponent+shader（含背景前景），失败 FAULT 或完整旧 token 显式 STALE，禁止 Canvas 上屏/回退。
