# T403 · T400 修订夹具 LAND 与同帧真机续证 — 三臂全达成：0° 三笔正授予床增 0.3、25° 三分列与离线逐位一致、零获准负控如实；同 publication CURRENT swap 两臂证实

- 卡面：承接 T393 旧包缺证，用 T400-delivery-signed.hap 按 T397+developer.md 验证 0° 原列 T3 正授予/退役/床增、25° 受限对照及同 publication CURRENT swap；8 成功步/16 回调/15 秒任一先达封档，不重复旧包长等待。
- 包：T400-delivery-signed.hap SHA256 `92f0d5f82c40c5975fda34cff37c1b328d2a306a17ea881cb073358e73ef3f9e`（本地逐位一致）。
- 路由：`p051v2=1 --ps singleSand 1`，pid 908。证据：`test/reports/t403/`（10 文件，SHA 见 t403_verdicts.json）。

## 三臂结果（全部封档导出成功，无长等待——每臂 2 步即达终态）

### 臂1 · 近床正控 0° — POSITIVE_LAND_CURRENT_SWAP（#1 · 2 帧 / 2 步）

重置空床 → 预约 ID 1/2/3 → 原干沙入口准入 H=0.3、x=63.5、y=0.004、seed=51：
- **准入收据**：admittedH=0.3、admittedL=0、lambda=1、rejectedReason=None、admissionTransactionId=1、sourceEpoch=1、physicsCommitId=3。
- **三笔出生**：slot 0/1/2，id 1/2/3，generation 1，质量 0.1/0.1/0.09999999999999998。
- **三笔正授予**（列 63，原列）：granted 0.1 → 0.1 → 0.09999999999999998，refunded 全 0，床 0→0.1→0.2→0.3，三粒全部 retired=True、verified=True、finalState=0（退役离表）。
- **床增 0.3 与离线预期一致**；dt=1/60、tilt=0、incomplete=False。
- **同 publication CURRENT swap**：firstPositive.presentation=`PRESENT_CURRENT_SWAP`，pub commit=5、substep=2、ledgerKey 含 runId/sequence=2/frameId=2/substepId=2，landCurrentSwap=true。

### 臂2 · 受限对照 25° — POSITIVE_LAND_CURRENT_SWAP（#2 · 2 帧 / 2 步）

x=63.99999、实际倾角 24°（参考复现 12°/24° 规划）：
- **三分列**：id1 全额 granted=0.1（床 0→0.1，retired）；id2 部分 granted=0.032121583881089655 + refunded=0.06787841611891035（床 0.1→0.13212158388108966，未退役）；id3 零 granted=0 + refunded=0.09999999999999998（床不变，未退役）。
- **最终床 0.13212158388108966 与离线预期逐位一致**；presentation=`PRESENT_CURRENT_SWAP`、landCurrentSwap=true。

### 臂3 · 预约首个 T3（普通触摸臂）— ZERO_GRANT_T3（#3 · 1 帧 / 2 步）

真实触摸注入后首笔 T3 封档：
- **零获准**：id2 granted=0、refunded=0.06787841611891035、remainder==requested；id3 granted=0、refunded=0.1——票据余量全额保真。
- **firstPositive=null**（负控正确）；presentation=`NOT_SELECTED_FOR_DRAW`（零获准帧未被选中上屏，如实）。
- 首个 T3 归档含 cross 收据（substep 2133）、lands 明细、dt=1/60。

## 封档语义核对

- **8 成功步/16 回调/15 秒任一先达**：实测臂1/臂2 各 2 步、臂3 2 步 1 回调即达终态封档——修订夹具彻底消除 T393 的 25 分钟长等待。
- **同 publication CURRENT swap**：臂1/臂2 landCurrentSwap=true + PRESENT_CURRENT_SWAP + selectedPublication commit=5/substep=2 与 firstPositive/firstT3 同帧；臂3 如实 false。
- **系统显示单列**：页面 XComponent 计数=1（唯一 surface），renderReceipt code=0、state=READY、surfaceEpoch=2、P013_NEWPIPE_ONLY_V1。
- **systemPresentVerified=False ×3**：平台 shell 无呈现校验通道，如实标注（非取证缺陷）。
- 重复预约拒绝/隐藏重置取消档/错 epoch 拒配：离线 units 已覆盖；设备侧三臂为正向主链。

## 结论

T393 的两项缺证（正授予 LAND 设备未产生、Z08 档未导出）**全部补齐**：修订夹具在设备上以 2 个成功子步产出三笔正授予 LAND（0° 臂床增 0.3 与离线一致）+ 同 token CURRENT swap；25° 受限对照三分列逐位复现离线；零获准负控如实。**主目标全达成，T403 完成。**
