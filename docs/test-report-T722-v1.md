# T722 目标11表层通量同帧离线验证

离线 PASS，真机端到端与实际 GPU 像素均 NOT_RUN。T639 unsigned 原件 BuildId `bbfe2dfc0148687bdbe22f60b7b161823e5c4eaa`。复用 T687 联核 38 份源文件、22 项生成输入、8 份实际包内 shader。八份静态编译通过，归档夹具 14 项通过。对同源 R1/DM 生成函数核实际双侧分摊、rank 分支和终态 map，对编译闭包中的 scene 源核联合租借、swap 前校验、呈现登记和 fence 释放。[产物](../test/reports/t722/artifact.json) [夹具](../test/reports/t722/fixture.json)

| T600/T601 判项 | 独立期望和设备取证 |
| --- | --- |
| 左/右/零流 | 三列同源质量各8，有符号实际面流±2时中列平移±1/4列，镜像精确。q=0且无转化时位置冻结。逐帧保存实际 sealed per-origin shares、rank、位置及 accepted token。超出 donor 的双侧外流拒绝。 |
| 同帧材料与 scene | 物理、材料、scene 的 token/run/context/generation 全同且 swap 成功才允许发布。旧 token、错 run、跨 context/generation、swap 失败分别做负控，保上一有效完整帧并标 stale。 |
| 设备观测 | T600 保存 W06–W11 实际输入和映射，T601 比较同一物理段正/零/反向的纹理特征位移和停流。原始 GPU 像素、完整 scene 和时序仍需设备采样。 |

复现使用 `python test/t687/verify.py g11 .codex/local-test/t639-delivery-v2 .codex/local-test/g13-glslang/bin/glslang.exe`、`python test/t687/archive_units.py` 与 `python test/t722/check.py`。
