# T721 目标13流场协议与基准离线联核

离线 PASS，真机端到端 NOT_RUN。对 T690/T704 实际包内 ELF、ABC、shader 和 BuildId 做字节联核，重跑 GLSL 静态编译退出 0。包内生成布局定位 Config128 黏度 word18、Completion256 黏性残差 word104、散度 word106、压力证书 word108。用独立算术核动力黏度到无量纲运动黏度转换、速度/压力尺度，复算 T631 F06 尾界和 F07 流量 1/6。[产物证据](../test/reports/t721/artifact.json) [联核证据](../test/reports/t721/join.json)

原 T654/T672 四页以严格 base64 解码和 SHA 绑定，boot 287785986262658，Configure 票据 216，CAPABILITY 原 rawGL=1280。同步 Configure 回执和前后锚未随历史原件保存。217 是 status 调用号。该历史轮精度及可信流场保持 NOT_MEASURED。

| 判项 | T609/T654 复用规则 |
| --- | --- |
| 同票据四页 | `python test/t721/check.py DIR ROUND`。宽 ID 使用十进制字符串。原始状态、stage、context、owner 与前后锚都须匹配。相同锚只支持 ANCHORS_EQUAL_NOT_ATOMIC。 |
| 能力与精度 | blocks/bindings≥4，invocations/laneX≥64，shared≥4096B，现有判定器用 2331392B 覆盖五个 profile 的 WORK 最大值。逐 profile 记录其自身 WORK 下限。顶点声明 23 仅预筛。FP32 需要同 context/owner 的真实两轮共16 words、三处阶段退出及无原始 GL 错误。 |
| 数值门 | J∈1/2/4。散度 RMS/max 各≤2^-14/2^-12，黏性 rH 各≤2^-16/J、2^-14/J。压力 RMS 证书≤2^-16，投影速度证书≤2^-16/J。压力证书先含真实残差与 RHS 扰动界。冷档先经 `python tools/g13/read_archive.py` 联核原始字节，再按 T631 逐场基准验值。 |
| 复现 | `python test/t704/check_artifact.py`、`python test/t704/units.py`、`python test/t672/analyze.py --selftest`、`python test/t721/check.py`。合成通过、失败及缺证仅登记为判定器输出。 |
