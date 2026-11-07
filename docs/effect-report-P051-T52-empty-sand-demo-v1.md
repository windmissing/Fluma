# T52 空容器DEMO真机效果基线

日期：2026-10-25 23:14–23:22（Asia/Shanghai）。角色：effector。原卡：6addc96fe4b0e6ce82b176ed。

结论：**本轮效果验证执行完成，产品效果未通过（FAIL_RENDER_GATE）**。冷启动和触摸后容器持续空白，页面明确报告 `NONUNIFORM_SCALE`；首轮注入后另见 `INVALID_PARTICLE`。不能交付成功落沙演示，已交付原生连续失败录像。无业务代码修改，无判据放宽。真实手持动态未覆盖，不假定轴映射已标定。

## 版本与环境

- HEAD：`340e7cf3d9e456ca25dd69e956eafd7fa158b0b1`；entry/src 最近提交 `952e148fd35c5de5a2cad56c398ff1c441717fcf`。打包后 entry/src tracked diff 为零；逐文件源码哈希见证据目录 `source-sha256.txt`，共享工作树的其它文档改动不计入产品版本。
- `default@PackageHap` 成功，既有证书签名、覆盖安装成功；初次构建因java未在PATH失败，补齐现有JBR环境后成功，无产品修改。初次签名链输入失败，按既有 `p001-sign-cert.cer` 流程签名成功。
- HAP：`test/artifacts/t52-empty-sand-demo-20261025/t52-signed.hap`，SHA256 `AF092FC0E97D2CC9E62B8450279D4CE91D115693B5EC325F6EAEB03F818D576B`。
- 真机 VYG-AL00，hdc `62T0225B10010020`，API 26；屏幕1280×2832，竖屏且系统旋转锁定。仅此设备；未测其它方向/机型。执行前未检测到本应用PID或活动采集命令，同机串行执行；结束已force-stop释放设备。
- 入口 `aa start -a EntryAbility -b com.example.helloworld --ps emptysanddemo 1`。真实sensor，无模拟摇晃Want。候选配置 `T50_DEMO_CANDIDATE_NOT_CALIBRATED_V1`：alpha=.6，Aon=1、Afull=6m/s²，20ms订阅、250ms stale；均不宣称已标定。

## 场景与判定

用例追加于 `docs/effect-cases.md` EC-T52-01~05。同路线首次设备基线，无同路线历史分数可比较；拒绘硬门槛触发后不强行给0~10动态观感分。

| 场景 | 实际执行/证据 | 结论 |
|---|---|---|
| 01 冷启动空容器 | 两次冷启均见空白框和NONUNIFORM_SCALE；初始源/沙/粒子抽样为0 | 新管线准入FAIL；零沙仅遥测支持；无水FAIL（已知隐藏水） |
| 02 按住/移动落沙 | 首轮(400,1100)→(900,1100)，1000px/s；录像轮200px/s；均无可见飞落/沙堆 | 可见落沙/累积FAIL；跟手、连续性N/A，拒绘硬阻断 |
| 03 松手/后台恢复 | 首轮源累计保持；录像轮松手≥6s，Home→回前台，不触摸 | 无可见沙堆可核验；后台期间同run源证据不齐，INCOMPLETE，不判通过 |
| 04 重置 | 点击(640,520)，源/粒子/沙抽样归0，仍NONUNIFORM_SCALE | 归零仅遥测支持；可见残留清除N/A（重置前已空白） |
| 05 真实倾斜/摇晃 | 仅静置真实sensor；观察target=89°、effective≈30° | 无实际运动/可见双场，NOT_COVERED；不判轴正负或动态手感通过 |

已知零水/空气出生缺口沿用T58→T59，不重开已完成开发卡。模型初始化有128列×160=20480水库存，未绘水不等于无水；空气触点夹水域边界不能当触点精确出生通过。

## 遥测旁证与质量边界

首轮PID35614：23:17:16.339 commit943，sourceEpoch4，外源H=3.2876502418518063，expected相同，actual=3.287650241851807，particleMass=3.287650241851807，deposited=0。23:17:31.188 commit1063：deposited=1.3547005383792516、particle=1.9329497034725551；至23:18:25.895 commit1423源、床和粒子抽样相同。支持“有注入、部分落床、松手后源不增”的采样事实，不能证明屏幕可见或长驻留已排除。

两PID不得拼成同run。录像轮PID38325在触摸至恢复区间缺连续demo样本（最终dump仅4条），不外推源生命周期。reset后23:19:09.218/12.254/15.262源、expected、actual、particle、deposited均0。

所有提取样本 `metricCommitKey=null`，页面diagnostic不含runId、有效帧/swap或逐子步完整账本。故**同帧质量依据不足**：物理commit不能与成功屏幕帧对应，不能签守恒/预算全程PASS或双场同帧PASS。`tools/analyze_t52.py`仅提取旁证、哈希与视频解码，不新增产品判据；完整解析样本见demo-samples.json，原始日志保留。

## 缺陷最小复现与交接

1. **F1 可见拒绘（稳定两次冷启）**：安装上述HAP→冷启emptysanddemo=1→静置3s；页面NONUNIFORM_SCALE，整框空白。截图start.jpeg/after-touch.jpeg/reset.jpeg；连续录像约24s起为第二次冷启段。错误产生位置可定位 `P013RenderController.ets:117`，这里只报告观测，不做整体根因结论。交designer分流；如需客观证据请tester补surface像素尺寸、页面逻辑尺寸及sx/sy、draw/swap逐帧身份。
2. **F2 注入后INVALID_PARTICLE**：首轮冷启→上述1000px/s滑动→松手静置；record-start.jpeg/record-confirm.jpeg显示INVALID_PARTICLE，同时物理抽样有粒子质量。错误可定位 `P013DualFieldAdapter.ets:34` 校验分支；未保存逐粒快照，不能指定哪项校验失败、不能断言物理坏粒子。请designer决定补测，tester需同commit的粒子字段/Float32边界与拒因证据。
3. **F3 正式演示证据未达成**：F1/F2阻断可见落沙；动态手持和同帧质量未覆盖。修复后沿同用例复测，不用本次视频宣传成功演示。隐藏水/空气触点另由现有T58/T59处理。

## 交付证据

目录 `test/artifacts/t52-empty-sand-demo-20261025/`：安装包、build/sign日志、源码清单、actions.txt、三份阶段原始hilog及首轮hilog-touch、截图、demo-samples.json、analysis.json与原生 `continuous-failure.mp4`。

视频经OpenCV完整顺序解码：868×1920，537帧，约68.080秒，平均7.888fps（原生录屏可变帧率）；不是性能fps测量。含冷启动、触摸、等待、Home/恢复与重置的连续失败过程。5/30/40/50/60s抽帧已导出，人工审看关键帧与设备截图一致。录像开头继承首轮INVALID_PARTICLE，24s左右重启进入第二轮NONUNIFORM_SCALE；不拼接两个PID质量数据。文件哈希见analysis.json。

T52本轮工作结束，状态“结束待验收”仅作交付登记；原卡报告执行完成/产品未通过及上述未覆盖项，由manager据此登记并派后续，无新增验收门。
