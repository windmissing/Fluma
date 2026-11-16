# T529 开发交付

2026-11-16，developer。依 T515/T524，独立 `gpu_resident/g9spf`，版本 `G9_SPF_Q20_V1`。本轮用户明确要求只开发、编译、不负责测试；未执行物理测试、oracle、真机或端到端验证。开发交付不表示动力学、设备吞吐或效果指标已通过。

## 实现范围

- 同源 ABI：`test/t529/generate_schema.py` 从 T524 机器表生成 native/GLSL/reader 字段、事件 used mask、K192/K512 八段 state、十五段 WORK、五完整 bank 和固定 pass 表。独立 magic/profile，不转换旧 V4/V5 物理尾。
- 整数核：四 U32 limb 的 checked U128/S128 加减乘除、TZ/RNE/定向舍入、ceil-isqrt；p²/F 先除再移24。局部转换与闭合 micro 分开核能量，相同项先抵消；重力、触摸、来源、支撑墙冲量分别记账。
- S/F/P 全 writer：top-first S→F、有序来源 F 流动、停止后的完整 F→S、模型冲量 F→P、解析 P 飞行及有序整包 P→F。S 不直接搬到邻列，P 不直接落旧 LAND/bed；旧路线不与本库存同推。容量、来源表、槽/ID、输入身份和封存失败均保留独立原因。
- 流动：HR/Rusanov、共享 q/jp、床修正/重力、REST 静止掩码；left-first remaining-Hamilton 来源预演后配对提交。摩擦 proximal 不翻号增能，restTicks 按 32/J 累进并饱和于显式配置。
- 飞行：冻结表面、完整有理常加速度、解析 TURN、T16 lower_bound、TOP/CEILING/WALL/CROSS、最新床面捕获及投影能量门。每包每 micro 最多8 PATH；新 launch 不在出生 micro 再运动。粒子归属按 lane 的 slot stride64；共享游标和环境账按 lane 顺序同步提交，是保守定序实现，不声称并行收益。
- trial/root：J1/2/4/8/16/32 发现、首次成功选择、权威重算、独立拒绝前缀重算、语义 replay，完整 state 与 EVENT/PATH 原件比较；只规范化 role 位。全失败也独立重算拒绝前缀。SOURCE 独立12 pass，零物理时间；QUANTUM固定2024 pass，无 host 读 J 缩短调度。
- EVENT64/PATH16：planned/sealed 联合前缀、ROOT_BEGIN/END、逐事件字段 mask、首错位置。正常拒绝私有 compound 只保留最后已封前缀；FAULT 不转成正常 BLOCKED。
- native：每批最多10顶层 pass/8ms、每提交与关键 GL 操作检查 permit/原250ms期限；一个 fence、单1024B completion 回读。G9 completion_gate 核完整身份、四 SHA、计数/审核族/封存和 outcome 后，私有构造中立 blocked 证明；复用 T508 时间账，不把 G9 数字 outcome 强转旧 V4 证书。
- 输入：原始 INPUT128/binary64 由 GPU 解码；独立 source/impulse 单调序号、完整命令原件去重。已消费重复不成为新 SOURCE 意图，IMPULSE 入队不解锁 blocked；相同 target 心跳只更新明确模型 pose 的时域，不增加 meaningful input。
- scene：同 CURRENT lease/token 直接读 S/F/p 与真实 P 位置/质量，仅 XComponent+shader，背景亦 shader。独立 graphics ABI/cache 身份、FBO 双缓冲、Swap 前后资格核对及 fence。失败报告 FAULT/STALE，无 Canvas 或软件 GL 产品回退。
- 冷档：独立 `G9_SPF_COLD_V1`，两 slot/一 builder/一 reader、60s原请求 TTL、预留与 detached 信用、caller-span 直接捕获。ROOT含32KiB prefix、BASE/FINAL、SELECTED/权威 EVENT/PATH；FULL加六 discovery pair，未执行段明确置零。累计两 FULL 预算359342080B，prefix 内保 input/config/quota/summary/descriptor/原 GL/EGL 错误。prefix 不描述自身，外部 archive reader 单独计算 prefix SHA；各 payload 段 SHA 封在 descriptor23..30。
- 冷 collector/reader：ArkTS 每次一页、≤1024 U32/16KiB、序号/offset/身份连续、实际 ASCII/UTF8 字节额度，finally End；完整写盘/fsync 后才重命名 `.part.jsonl`。`test/t529/read_cold.py` 流式落临时文件，校验版本、完整范围、段 SHA、配对原件和 quota/state 身份，给出实际 archive/prefix SHA，不运行 CPU 动力学。切换路线拒绝仍 pin 的读者，退休旧 slot 后才进入新预算。

## 研究入口与参数

从隔离工程原 GPU 入口选择“G9 S/F/P 独立研究”，再显式新建 K192/K512 或 FULL 空 run；默认仍为 M2。原页面和旧能力不被当作 G9 能力。

`g9ResearchConfig` 明示候选1：g=16Q20、muStart=32768、muStop=16384、band=Q20、entrainRate=4Q16、shear=Q16、settleTicks=384、revision1；全部在已冻结支持域。SHA 覆盖具名研究参数，种子 SHA 随原始空 seed 或有界显式 raw seed 记录。这些值待定标，不是产品默认。首版外部输入只接受明确 SYNTHETIC 模型域；没有 sensor 时域和像素→冲量标定证明时，不伪称真实输入资格。页面提供模型掉沙/脉冲和手动目标角。

## 编译与复现

生成器必须给一个**尚不存在**的 `.codex/local-test/` 子目录；不会覆盖既有工程，也不执行测试：

```text
python -X utf8 test/t529/assemble.py
node test/t529/generate.cjs .codex/local-test/<fresh-project>
hvigorw.bat --mode module -p product=default -p module=entry@default assembleHap --no-daemon
python -X utf8 test/t257/sign.py <fresh-label> .codex/local-test/<fresh-project>
```

生成器复制规范源文件并记录逐文件 SHA。schema/control/planning/cold 的 `generate_*.py` 是建立模块时的迁移生成辅助，不是每次构建都重新覆盖手工接线的步骤。交付构建使用现有 canonical 文件；不重新生成旧物理内核替代 G9。

完整工程 `.codex/local-test/t529-delivery`，首次34 tasks全构建 `BUILD SUCCESSFUL`；最终改动后33 tasks构建 `BUILD SUCCESSFUL in 14.495s`，零编译错误。已有 ArkTS/SDK 警告保留。compute 和两组 graphics shader 均独立编译/链接通过；编译工具不 dispatch、不 draw、不读模拟状态。ANGLE仅作编译设施，不是产品回退。

签包与 verify-app 通过，HAP SHA256：
`9290f550ee92a617101bbfa826537d0b03ec7e868c08055ef8df72838f5e8e82`。

逐文件源 SHA、编译工程一致性、packaged ELF SHA、日志位置见 `test/reports/t529/development-compile.json`；签包原始回执 `test/reports/t257/T529-development.json`。签名材料和根工程配置未改，不安装应用。

## 后续验证边界

tester/effector 后续负责 T520 对照、完整长序列、各错误 cut、cold collector/reader 实跑、真实设备资格/峰值/250ms/30fps与观感。上述均为 NOT_RUN/NOT_MEASURED；本报告没有把 shader 编译或签包当作物理 PASS。T517 的 T527 prefix 自引用摘要冲突留在 T517 原卡，与本独立 G9 实现分开处理。
