# T317 · 首 root 初始化前置与提交边界修订

2026-10-30，developer；依据 `architecture-T313-gpu-first-root-window-v1.md`。不使用真机。交付为 **PARTIAL / 看评论**，尚不能声明有界真实 Submit 已完成或解锁完整产品 PASS。

## 已实施

- M0、M1 source、M1 quantum 的真实 transport 发出 Submit/Poll、compile/link、resource、prepare、dispatch、fence 边界。沿用固定 1024 行首段保留，满后显式 dropped；原 ns/U64 十进制串、原 worker OS TID、所属 transport epoch/run/surface/root 身份。Windows host 的 TID 是 thread hash，仅用于离线替身。
- 字典版本 `T317_STAGE39_V2`，保留旧 1–23 数值；新增 24–39 为 SubmitEnter/Exit、ShaderEnter/Exit、ProgramEnter/Exit、ResourceEnter/Exit、PrepareEnter/Exit、DispatchEnter/Exit、FenceEnter/Exit、PollEnter/Exit。旧 RootSubmit 仍表示 Pump 返回后的观察点，不改名掩盖含义。`code` 在 Submit/Poll 是原 TransportCode，在 compile/link 是查询返回的 status，在 Prepare 是 raw GL，在 fence 是 null 标志；资源异常提前返回为 UINT32_MAX（无完成码）。`carry` 在 transport 行是 dispatch ordinal，不是时钟 carry。时长仅 CPU wall；无 GPU timer 数据。
- 合法冷准备在同一 GL owner 上、READY/首 anchor 前完成：资源全部建立后零工作组 dispatch + glFinish，零 invocation，无库存/事件/nonce/owner Begin/accept/publication 写入。M1 最终替换后的 quantum program 也独立准备。**不承诺零组准备已消除设备驱动的全部延迟编译。**
- uniform 名称在 Create 冷反射进固定 128×96 名称表，超界拒绝；每 pass 查固定 CPU 表，不再反复调用驱动 glGetUniformLocation。M1 替换 program 后重建表。原 uniform payload、dispatch/barrier/审核/owner/lease 顺序不变。
- Pump 返回后、Draw 前增加原 anchor 的真实超窗检查：不等待下一轮再撤权；沿用 Pause/隔离晚结果，保原始 delta，不钳值、不补跑、不重置运行中 anchor。已主动 Suspend 且 last=0 的路径不制造新 CLOCK_GAP。
- T306 冷出口同时支持旧 23 / 新 39 字典，拒绝未知 stage 和分页期间字典切换；不调用 reset/offer/wake。

## 非机证据

`test/t317/build.cmd`：原 App loop + 真实 Run/clock + EGL/transport 替身，11/0。覆盖正常 8ms 等待、40ms 等待/工作、187ms 初始化、初始化中旧 epoch 撤权、78ms Submit、40ms Poll、BUSY 重试、提交未接受的 Pause drain。初始化前置不污染首 anchor；真实超窗仍撤权；晚结果不接受。

`test/t295/build.cmd` 原回归 16/0；`test/gpu_resident/build_t281_run.cmd` 7/0；T228 原 UI/native pause/source-discard/suspend/resume 10/0。

`node test/t317/cold_units.cjs` 23/0，含 T306 原 20 项和 39 阶段、未知 40、分页换字典。

M0/M1 各 6/0：`test/t317/build-m0-gl.cmd` / `build-gl.cmd`，PC ANGLE/GLES pbuffer、脚本时钟、真实生产 App/transport/Run/shader/scene。均有 sourceCommit=1、physicsCommit=2、相等 requested/rendered token、rawGL/EGL=0，且真实新增阶段齐全。此证据仅离线接口/GL 执行，不是设备 33ms 性能、XComponent present 或产品体验 PASS。PATH 需包含本机 `C:/Users/admin/AppData/Local/Programs/CokePLUS` 的 ANGLE DLL。共 79 项检查（不重复计另行保留的旧冷出口 20 项）。

构建日志、签包 SHA、原始检查输出见 `test/reports/t317/delivery.json` 与同目录。hvigor PackageHap 必须 BUILD SUCCESSFUL；签包 verify-app/profile 通过。未安装、未启动真机。

## 未完成与须 architect 接手的具体合同

**未完成：保证运行中每个真实 root 的实际 Submit 不跨 33,333,333ns。** 非机复现 `bridge_test.cpp` mode7 在 SubmitEnter/Exit 间注入 78,000,000ns，现修订正确返回 CLOCK_GAP，不能称该目标 PASS。实际设备剩余耗时属于 driver/JIT/CPU/调度哪一项仍未知。

源约束：`ResidentRun::Pump` / `m1::Run::Pump` 仅在整个 Submit 返回 OK 后写 inflight/submitted；M0 `SubmitRoot` 在 owner.Begin 后遍历整份 plan；M1 `Enqueue` 内含多个 Sort/Pass，FullM1Transport 再执行审核、发布与封尾，之后才创建 fence、标记 active。现有 Poll 只接受完整 fence 的 sealed tail，Stop 的 quarantine/drain 同样以已建立的 flight 为基础。

已经尝试的合法路径：一次性资源/driver 冷准备前置；清除热 uniform 驱动查询；分清实际调用段；返回后及时撤权。仍不能抢占单次 glDispatchCompute/glFlush 等同步 driver 调用。直接在中间 pass 返回 BUSY 会使 Run 下轮重试同 nonce 并重复 Begin；返回 OK 则把“未封尾、无 fence”的中间态冒作现有 inflight，现 Poll/Stop 不能正确 drain。把剩余工作转另一线程又违反 current context/原 GL owner 及 App surface/lease 串行合同。不能用这些方式伪造有界成功。

请 architect 定义可执行的 partial-submit 状态/单批 pass 预算、相同 nonce 的 continuation、未封尾取消的 fence/drain、撤权与 publication/lease 时序，以及跨多批总墙钟与当前 GAP 合同的关系；或依据新阶段证据明确可前置的具体 driver 操作。若判断需改变时钟/物理策略，再转 designer。此处是明确合同前置，不是等设备作为开发门；现有原始超窗判定和所有物理审核继续保留。
