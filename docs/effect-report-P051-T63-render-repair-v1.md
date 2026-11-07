# T63 DEMO拒绘修复后真机复测

日期：2026-10-26，09:05–09:12（Asia/Shanghai）。角色effector；原卡6ade9e08e4b0e6ce82bb1a90。路线P051_RENDER_GATE_REPAIR_V1，沿EC-T52-01~05及T60§6执行。

**结论：部分完成；尺寸启动拒绘修复有设备PASS证据，产品落沙演示仍FAIL。** 两次冷启及后台恢复均有真实draw/swap成功，未观测NONUNIFORM_SCALE；但自动中心/右壁滑动、用户确认的手动按住移动之后，源、粒子、床质量始终为零。无粒子使F2量化投影、state2/LAND、沙堆保留和可见双场重力响应无法验证。不能将零粒swap成功当作完整DEMO通过。

## 身份与设备

- 独立源码快照来自T62 `86366dd416c7d4b9dd66d4d9bcdbf5a6c4fa23ff`，包含`2ee110c`及composition重启修复。从git archive导出到`D:/hmprojects/t63-build`；source.zip SHA256 `782FDF322D04DA329DC8CFCB0EFF799579B7B6EC4735044C59863057608141B0`，逐文件source-sha256.txt归档。
- 接单发现共享工作树native/CMake已有他人未提交改动。最初主工作树PackageHap成功，但**该包未用于测试**；转独立快照全量编译ArkTS/native/PackageHap成功（build-clean.log），签名和安装成功。不回退他人改动、不修改产品或判据。
- 测试包t63-signed.hap SHA256 `6AAC39F187C5A74E992CD1EBA45258B56728F44E227512936F233D9EAE7D227E`。
- VYG-AL00，hdc 62T0225B10010020，API26，竖屏1280×2832；原生录屏868×1920。冷启`emptysanddemo=1`，EXPERIENCE真实sensor，无模拟姿态Want。单设备串行，结束停止录屏、force-stop释放。
- 本版仍初始化128×160=20480隐藏水库存；不混入T59，不宣称干空容器/空气触点精确出生通过。配置T50_DEMO_CANDIDATE_NOT_CALIBRATED_V1，alpha=.6、Aon=1/Afull=6m/s²、20ms订阅、250ms stale，保持未标定身份。

## 场景结果

| 场景 | 实际动作与结果 | 判定 |
|---|---|---|
| 零粒冷启 | 初始启动、录像内再次force-stop冷启；页面显示“新管线：按住发沙，松手停止”，实际swap成功 | PASS（零粒启动/尺寸门），零水不通过 |
| 首粒/中心触摸 | 09:08:20起，(400,1200)→(850,1200)，200px/s；容器内滑动后无沙，source=0 | FAIL_TOUCH_NO_SOURCE，首粒未形成 |
| 右壁触摸 | 09:08:28起，(1160,1200)→(1220,1700)，200px/s；同样无粒子/源 | 输入失败；右壁量化投影NOT_EXERCISED |
| 手动触摸 | 09:09:06请求用户在容器内按住约5秒并移动；09:09:31用户明确回复“已完成触摸和倾斜摇晃” | 仍无源/粒子/沙，手动失败与自动失败相互支持，不据此推定具体事件分发根因 |
| 真实手持方向 | 用户完成左右倾斜、回正、轻摇；日志有效角从约30°到-5.661° | sensor到有效角变化有支持；左右轴标定/手势时序无同步姿态真值，不签全方向正确；两场可见响应BLOCKED |
| 真实摇晃 | 用户确认执行；约每60帧demo抽样shakeInput最大0 | 未捕获非零强度，不证明整个摇晃期间恒零；实际强度/衰减证据不足，INCOMPLETE |
| Up/松手 | 自动滑动结束含释放 | 零源场景不能证明非零注沙被停止，BLOCKED |
| reset | 09:09:58点击(640,520)，零源/沙/粒子保持，未拒绘 | 空库存重置可用；非空清除未覆盖 |
| 后台恢复 | 09:10:02 Home，3秒后前台，不触摸再观察4秒；contextEpoch从1变2，继续swap | PASS（零粒恢复/重建）；非零源后台停止/不续发BLOCKED |
| 内部列边界、state2/LAND | 未形成首粒 | NOT_EXERCISED，不拿离线反例通过替代 |
| Touch Cancel、真实resize | 本轮未产生可辨Cancel事件或独立surface宽高变化 | NOT_COVERED，后台context重建不冒充resize |

与T52相同触发的可验证改善：启动不再陷入NONUNIFORM_SCALE，零粒帧有native成功回执。T52曾有注入的旁证，本版观察到触摸后零源是本轮新失败，不能因为修复门通过而忽略。落沙可用性0/10（及格6，实际无沙）；粒子连续性/床质感/两场协调N/A（没有非零样本可评），未闪退只限本次观察。

## 设备回执、几何和证据范围

严格解析多阶段原始日志、按整行去重，分PID/run保留；解析器`tools/analyze_t63.py`，输出parsed-events.json、analysis.json。没有修补截断JSON、没有从抽样推全程守恒。

| 旁证 | PID31761（初次启动） | PID32518（连续录像主run） |
|---|---:|---:|
| frame日志样本 | 27 | 111 |
| code=0且swapSucceeded=true | 27 | 111 |
| demo抽样 | 8 | 31 |
| 最大render粒子数/源H/粒子质量/床质量 | 全0 | 全0 |
| projection最大correctedCount | 0 | 0 |
| surface像素 | 1178×2026 | 1178×2026 |
| surfaceEpoch/contextEpoch | 2/1 | 2/1及2/2 |

geometrySpace=SURFACE_PX_V1，projection=COLUMN_PRESERVING_F32_RENDER_V1。原始日志中未发现p013_dual_rejected，故没有“首拒slot”可报告；**这不验证非零合法源、非法源或边界投影**。renderPipelineId=DUALFIELD_BASIC_V1、colorPipeline=BASIC_SRGB_DISPLAY_V1，shaderHash=7fb5df888208829f。

native回执含runId/stateEpoch/physicsCommitId、frameId/uploadedFrameId/submittedFrameId/lastSwapFrameId；geometry也含同physicsCommitId。两PID分别有26/27、109/111组可按同毫秒时间与同commit配对，其余只有同commit关联，不强凑同毫秒。例如初run09:07:54.882，commit179/frame93，上传/提交/lastSwap均93，code0/swap=true，对应geometry1178×2026。日志可关联packet→native swap的采样身份；**屏幕没有commit水印、readback探针或present correlation，因此截图/视频只能按时间关联，不能宣称特定显示帧与commit逐帧精确对应**。真实logical vp宽高、DPI值及同屏帧探针未完整采集，保留缺口，不能从全屏像素替代XComponent权威尺寸。

所有demo抽样metricCommitKey=null；无完整逐子步账本和预算身份，不能签全程守恒/预算通过。粒子未形成，LAND原子性与粒子/床同包同屏也未实证。不得用T44另一隔离基线替代本run事实。

## 最小复现与建议路由

**F63-TOUCH_NO_SOURCE（产品FAIL）**：安装上述T62快照包→冷启emptysanddemo=1→确认新管线成功状态→容器内部中心滑动/右壁滑动，或手指按住约5秒并移动→屏幕始终空容器；demo sourceEpoch/sourceInjectedHeavy/expectedMass/particleMass/depositedMass均0，native particleCount=0。center.png、right.png、manual-end.png和连续录像、触摸/手持时间记录支持。

effector仅报告行为，不断言一定由XComponent吞事件、sensor stale或物理准入拒绝造成。建议manager派designer区分触摸接线/坐标/生命周期/源事务；缺证据时由tester补Down/Move/Up/Cancel接收、model.start/tick/stop时间、归一化触点、inject请求/准入拒因及同commit字段。明确后developer最小修复，另立真机复测，不将设备验证作为开发卡关闭前置。

**F63-EVIDENCE（部分验证受阻）**：没有非零粒子使F2、首拒slot、右壁与内部列边界、state2-LAND、非空reset/后台停止均无法测试；同屏commit、实际resize/Cancel、真实sensor高频摇晃强度需补采。保留原未标定约束；不改shake阈值或物理预算求通过。

## 交付

目录`test/artifacts/t63-demo-retest-20261026/`含源码快照/清单、clean构建/签名日志、HAP、actions.txt、start/touch/pre-manual/manual/final原始事件、无损PNG、parsed-events.json和analysis.json。原始屏幕截图逐张审看，均无可见沙，页面成功状态与零粒swap旁证一致。

连续原生录像continuous-retest.mp4约119.417秒，4214帧，完整顺序解码成功；868×1920，平均35.288fps为录屏统计，不是应用性能结论。包含重启、自动触摸、用户手持操作期间、reset和Home恢复；屏幕录像没有外部手持摄像，真实动作由用户回执及sensor角度变化旁证，不能从录像单独恢复手机真实姿态。无损抽帧及所有文件SHA256见analysis.json。没有成功落沙累积演示可交付，交付的是修复后失败复现录像。

录像SHA256：`72B511584991D9F17DB160588428A3CF1BF48AB4C3C7FA4C33F6FE0AC757E0B1`。

本轮部分完成，状态“结束待验收”仅是执行交付登记；原卡报告FAIL/证据不足供manager读取结论、裁决后续，无新增manager验收门。
