# T229 明水C与落水V2独立验证（GPU 接力范围，离线，结束待验收）

2026-10-30，tester 离线（PC ANGLE/SwiftShader，device=NOT_RUN 如实）。被测：T281/T228（3a7cb89）真实 GPU M1 V4 接口（T281 `c127322` + T228 `GRS_M1_DEMO_UI_V4`）。

## 一、T228 三层独立复跑（重编译重跑，非复用 json）

| 层 | 分母 | 结果 |
|---|---|---|
| controller（实际 ArkTS 输入控制，Node transpile） | 22 | **22/0** |
| native UI gate（重编译 run.exe 重跑） | 10 | **10/0** |
| production app（真实 FullM1+Run+scene，重编译重跑） | 20 | **20/0** |

关键覆盖（controller 22 例逐项独立判读）：首窗/dt0 无源量、真实纳秒差→精确 volume+revision2 原 32word、MOVE 不重采样、主 pointer 单占、UP 保 owner 停出生、切源需新 DOWN+水 namespace/source-control U64、间隙>33.3ms 停不补量、倒退/异常钟拒、精确合法窗口原 delta、背压不重放、geometry 变化解除武装+镜像显式、新 run 新身份、READY epoch 失配不出量、旧 F64/M0 UI revision 显式拒绝、畸形身份不成 command、后台恢复需新 DOWN、sensor 仅观测目标/manual 显式、光学 typed 冷配置、profile M1 feature/revision、request ID 耗尽拒绝不回绕、ENQUEUED≠成功准入、越盒拒绝不重定位。

## 二、production app trace 独立语义判读（tester 自持，不调产品断言）

- **水总量守恒**：`water=4194304`（4Q）贯穿 J 出生（phase2–4）→停源→ARRIVE/WATER_EDGE（phase21–22）→沙源（sand=131072 首现 accepted5）→落床湿介质（phase7/10）全序列不变——**J→W 转换无双计/无丢失**。
- phase12 末行 `water=0` 为**显式冷光学 reset 新空 run** 语义（`a.ui.water.lo==0` 且 `sourceCommit==0` 且 token 更新），非库存丢失。
- resource 账与交付一致：gpuBufferAllocatedBytes=25,835,728、peak 26,360,016、PC working set 如实分列（非设备值）。

## 三、SW01–SW14 版本化判定（按卡面 GPU 接力覆盖指令）

- 旧 T221 CPU-only 符号面（17/18 缺）已被 **GPU M1 policy 显式版本化替代**——SW 旧 F64 预期不硬套 M1 profile（T228 交接明确旧通道非生产路径）。
- 基线 4 项维持 PASS（SW-04 水沙独立 / SW-05 平床双侧外流 / SW-09 原子性 + SW-00 符号覆盖核）；10 项 NOT_IMPLEMENTED 按卡面指令登记为**版本化差异，不派产品缺陷**。
- **M1 数值差分由 T292 独立 oracle 覆盖**（B T16 段速度 / C ARRIVE 确定序 / D 预算全流 / E 水边写集 / F mask / A 回滚 / G reason 全 PASS，mask 从 T238 manifest 独立生成）——本卡引用不重复；T237 WM01-12 与 T238 revision 的 GPU M1 权威在 T280/T298（1860/0+152/0）复核。

## 四、交互原子性与渲染身份

- UP/切源与 Pump 提交同锁（native units 10/0 含停源前后边界、一个在飞 root、暂停丢弃后续 quanta）。
- GPU 场景身份：sceneContract=`GRS_GPU_SCENE_Q20_M1_V2`（T300 独立 oracle 24/24 复核）；uiContract=`GRS_M1_DEMO_UI_V4` 旧 reader 显式拒绝（controller 用例 PASS）；无 Canvas 回退。
- 后台/sensor stale：app 20/0 含后台 60s 墙钟+恢复首 tick0、库存保 accepted；真机端到端 NOT_RUN（另卡 T301，不前置）。

## 五、结论

T229 GPU 接力范围**结束待验收**：三层 52/0 独立复跑 + trace 独立语义判读（守恒/原子性/身份门全过）+ SW 版本化判定。无 FAIL、无需回 architect；未实施面按 NOT_IMPLEMENTED 版本化，不伪称完成。

证据：`test/reports/t229/`（recheck_summary + units/app 复跑原输出）、`test/reports/t223/independent.json`（SW 版本化记录）。
