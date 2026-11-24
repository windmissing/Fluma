# T935 · 单一集成 Demo 体验版 PC 放行（effector）

2026-11-24。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。送验包 `.codex/local-test/t933-integrated-demo`（manifest 逐件 SHA 核过：PE `4a40a141…`、source `e9c7c123…`、candidate `bf024ac5…`、commit `64ba25f4…`、launcher `8494cb8d…`）。整轮主验收用启动器同参数（`--m1-compact-demo --m1-integrated --m1-dual-source --m1-source-trace`），无诊断变体切换；另跑一轮同 PE/同 shader/同 argv+observe 仅作像素取证（`runs/pixel-round`，argv.json 标注）。GPU 驻留、shader 渲染，无 Canvas 回退。

## 整轮判据（runs/integrated-round，墙钟 106.7s+）

| 判据 | 实测 |
|---|---|
| 空小盒可见 + READY | `WINDOW_AVAILABLE`（624×431）→ `M1_POINTER_READY` 先于首次 Down |
| 2–4s 明显沙堆 | 3.0s 持按 140 量子入账；**像素取证轮**（3s 持按、帧 50）同根 `FENCED_READBACK` 沙色 (89,145,184) **4+9 px**（ROI 296/312×308，T920 同位判据） |
| Down→首达 | **接受 grant 99ms、首次呈现 140ms**（墙钟自 Down 计） |
| 松手自然落床留存 | `settleRetained=true`；末态 lease 沙床 66,961,882 Q 保持 |
| ≥60s 反复长按后段真实入料 | 106.7s 墙钟 2059 量子（**19.3 Q/s**，较 t926 前版 ~5 Q/s 大幅提升）；末段探针后总数 2616（仍在涨） |
| 左右转盒/边界反向/复位 | 18 条 `M1_GRAVITY_INPUT` 全 `ACCEPTED_INPUT`；请求角时间线 −1280→−7680（6 左）→反向→+5120（10 右）→0（R 键）→0（右上按钮）；granted 逐一跟随；边界反向过（−7680→+5120 跨越 0）；2 条 `M1_BOX_VIEW_PRESENT_QUEUED` |
| 迟退役恢复 | 完整链 ×1：SUSPEND(4)→**RETIRED→NEW_BASELINE→HOLD_REVALIDATED**（首次等待超时→同 fence 成功→新会话续收）；无重复旧账（预约 AUTHORITY_CHANGED/新会话重验语义沿 T927） |
| 入料与 LAND 守恒 | 末态 `sandAdmittedTotalQ(66,961,882) == sandStockQ(66,961,882) + activeSand(0)`——闭合；liveSand 峰 23 全落床 |
| 版本/GPU/命令/日志 | manifest+argv.json+demo.jsonl（14,023 行）全档 |

## 结论

**达标：本默认启动器（Start Demo.cmd）可作为用户体验版放行**。集显 PC 全判据过；独显持续帧率按卡面由用户目标环境实测（不在本卡范围）。

备注：集成版材质默认沙（waterAdmittedTotal=0，materials={0}）——紧凑盒干沙体验路径，与卡面"沙堆"判据一致；水线为独立模式不在本验收范围。
