# T491 · GPU 场景缓存冷暖与上屏真机验证（v1：M1/M2 双路线冷缓存六 program 全 PERSISTED、暖启六 program BINARY_LOAD compile=0、暖启 READY 达成；上屏仍被首 root 期限问题阻断）

2026-11-13，tester，真机 62T0225B10010020，设备串行（本时段无其他卡占用）。被测=T485 双签包（commit `ee3c4be`）：M1 `c76c39f87a2293ded383c18e1cd84a2abe277946702e49ad961b4f853a169ee9`、M2 `70a21bda3a871ae52931d3ab0a2d35d3f4f7348fd7e6eb8e0cf4190d4dbc2ef3`（本地 sha256sum 与 `test/reports/t257/T485-*-handoff.json` 逐位一致，verifyApp=PASS）。入口 `--ps grsm1 true --ps gpusha <包SHA>`；冷缓存条件=`bm clean -n com.example.helloworld -d`；保数据暖启=`aa force-stop` 后重启。证据：`test/reports/t491/`（8 截图、4 份启动诊断 JSON、设备端缓存收据 2×6 JSONL 打包 t491-cache.tar/t491-cache-warm.tar/t491-m2cache.tar/t491-m2cache-warm.tar、hilog 转录）。

## 一、冷缓存（bm clean 后首启+新空场）

| 项 | M1 | M2 |
|---|---|---|
| 安装 | 成功 | 成功 |
| 冷启进程 | 51457 | 58853 |
| 缓存产物 | **6 program 全 PERSISTED**（SCENE_FULL/SCENE_PARTICLES/SCENE_WATER + SOURCE/INTERMEDIATE_QUANTUM/FINAL_QUANTUM，各 bin+jsonl） | **6 program 全 PERSISTED**（同六 role） |
| 编译路径 | graphics 冷编译（VERTEX/FRAGMENT_COMPILE_ENTER→LINK→REFLECT_OK→EXPORT_WRITE_SUBMITTED） | 同左；compute 含 PREPARE_OK/WARMUP_OK（FINAL_QUANTUM） |
| 页面状态 | STALE/CREATING_NEW → 新空场 run 0:1 | 同左 |

## 二、暖启（保数据 force-stop 后重启+新空场）

| 项 | M1 | M2 |
|---|---|---|
| 暖启进程 | 54379 | 60209 |
| **六 program 加载** | **全 BINARY_LOAD**；graphics 三 role REFLECT_PREPARE_OK **compileCalls=0** | **全 BINARY_LOAD**；graphics 三 role compileCalls=0；compute route=BINARY_LOAD |
| 启动诊断 phase | **READY**（resources=true，attempt3 无 firstFailure） | **READY**（resources=true） |
| SCENE_PROGRAM_PREPARE 耗时 | **4.90 ms**（PREPARE_ENTER→EXIT） | **3.05 ms** |
| 页面状态 | run 0:1 GPU_WORK_PENDING→物理推进（0:1806→0:2642）→数分钟后 V2_PULSE_CLOCK_GAP | run 0:1 GPU_WORK_PENDING→物理 0:267→V2_PULSE_CLOCK_GAP |

**缓存能力结论：M1/M2 双路线冷缓存自动 PERSISTED、暖启全 BINARY_LOAD compile=0、暖启 READY 达成、scene prepare 暖载后毫秒级（3–5ms）——缓存功能真机全部成立。**

## 三、上屏（如实：未达成）

暖启 READY 达成且物理持续推进（M1 两分钟内 physics commit 1806→2642），证明 compute/物理链路在缓存命中下健康；但两次路线均在新空场后从 GPU_WORK_PENDING 回落 `V2_PULSE_CLOCK_GAP`，画布保持纯黑，无有效内容帧。**该回落与 T448 v2 报告的首 root 250ms 绝对期限问题同族（ROOT_INTENT→ROOT_ACCEPT 真机 GPU 队列耗时结构性超期），不是缓存缺陷**——M1 attempt3 无 firstFailure 且 SCENE prepare 正常完成。按卡面规则：当前控制失败不伪造上屏，缓存能力证据与产品内容证据分列。

## 四、未行使（不冒通过）

- 层画面/Swap 逐层核验、首帧时长（无有效帧）、compute 缓存兼容正控的暖启细节（暖启 BINARY_LOAD 已证，refresh 路径行为随 GAP 撤权未到）。
- 缺可用 M1 入口的精确报告：M1 入口可用（READY 达成），已如实记录。

## 五、移交

上屏链归 T490（M1 首 root 超期与恢复再入归因，architect 已开工卡）；缓存证据链可供 T490/T448 复用。硬件环境：Maleoon 910，程序缓存目录 `/data/app/el2/100/base/com.example.helloworld/haps/entry/files/gpu-program-cache/v1/`。
