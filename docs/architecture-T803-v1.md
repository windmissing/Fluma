# T803 · DM 分段原件核验与补采

2026-11-22 · 目标 20 · P5 · architect · 离线复核。

**T801 交了五次完整 B 基线，四类独立 prefix 和 GPU timestamp 原件均未交。** 五次都用原 250 ms 到达 cursor 12，执行 wait 为 TIMEOUT，随后在退役阶段复用同一执行 fence 才观察到 signal。T799 的 copy 专属成本下界仍不可计算，候选继续 `COPY_COST_UNCONFIRMED`。已核到 T797 工件包含所需诊断入口，先用现有包补采并联核原件。 [T801 原报告](test-report-T801-v1.md) · [逐次核验数据](architecture-T803-evidence-v1.json) · [T799 触发条件](architecture-T799-v1.md)

## 五次原件与工件身份

实际本地 T797 driver、HAP 内剥离 DSO 和配套 libc++ 的 SHA/长度均与编译回执一致，bootSource HAP 中的 DSO 与分发原件逐字相同。五份原件实际映射 driver BuildId `09befda4647ef0d3b440ac5b880587d8a5c9e057`、BOOT DSO BuildId `962b06ec8d6a673e99d6dbcbf760e576748a3d99`，与本地 ELF 匹配。报告中的 `2855f567…/b0df4529…` 与本地 MD5 对应。归档缺各进程的全设备字节回执和实参原本，T773 因此仍标 `DEPLOYED_BYTES_UNOBSERVED`。 [T797 编译回执](<D:/hmprojects/HelloWorld-T771/docs/receipt-T797-compile-v1.json>)

| 原件 | PID / start tick | 实际 B 根墙时 / owner CPU，ms | 首笔 arena copy / 末批首次 flush，ms | 退役时首次观察到 signal，距根起点 ms |
| --- | --- | ---: | ---: | ---: |
| s1 | 2097 / 5900718 | 250.164 / 174.457 | 74.660 / 74.943 | 332.630 |
| s2 | 5932 / 5954140 | 250.153 / 175.496 | 80.728 / 73.599 | 338.525 |
| s3 | 9406 / 6006142 | 250.120 / 177.216 | 84.452 / 73.028 | 338.504 |
| s4 | 12246 / 6059178 | 250.109 / 176.705 | 81.258 / 72.967 | 338.634 |
| s5 | 15278 / 6113412 | 250.052 / 176.258 | 80.330 / 73.184 | 336.947 |

五份不同 SHA、PID/start/scope，均为同 K192、SOURCE/roleSet、参数和布局。T773 `b41656c1` 逐份复跑均为 `B / DEADLINE / execution UNSIGNALED / retirement SIGNALED / admittedQ20=null`。每份实际 250 μs cadence、12 dispatch/12 dispatch barrier/3 copy barrier、advanced cursor 12；首拒 `PUMP_ENTRY_GUARD / DEADLINE`，最后通过 guard 时真实诊断 heartbeat 年龄 2.184–4.495 ms。SOURCE 无 map、无 typed、commit/quanta=0、FIFO=1。正常执行 fence 有创建记录及 TIMEOUT `37147`，退役 `reusedExecution=true`、`attempted=false`、`created=false`、同 handle `1`、最终返回 `37146`，`retirementFenceCreate=null`。正常根的 drain 各为 72.381–81.600 ms，包含撤权、轮询及 owner 观测开销。 [原件示例 s1](../test/reports/t491/t801/s1.jsonl) · [T773 原 reader](<D:/hmprojects/HelloWorld-T771/test/t761/read_source_timing.py>)

### 三处证据缺口

- **实际模式。** T801 报告记载 `--timing=B`。原 28 行均止于 `SOURCE_TIMING`，无实际 argv、`GPU_SEGMENTS/T797_DM_SOURCE_GPU_V1` 或 `DIAGNOSTIC_PREFIX STOP`。运行形态与 B 完整执行匹配，采证没有证明诊断 flag 被实际调用。即使末行被过滤，这五份也都实际创建了执行 fence 并在退役时复用，独立 OP11 prefix 则会在相同 cursor 12 转入新建的退役 fence。
- **GPU timestamp。** T797 的兼容 `SOURCE_TIMING.gpuClock` 字面固定为 `UNOBSERVED`。实际 GPU 时间能力、不可用原因、disjoint、bit width 和 markers 只由显式 `--gpu-timing` 的 `GPU_SEGMENTS` 行提供。没有该行只能登记模式未获原证，无法确认设备不支持，也无法以它作为已尝试 fallback 的依据。 [GPU 观测字段](<D:/hmprojects/HelloWorld-T771/test/t670/dm_source_run.h#L745>)
- **耗时归属。** 第一笔 copy 约 74.7–84.5 ms，后一笔只有约 29–36 μs；末批重 flush 约 73.0–74.9 ms。两个开销仍同时存在，标准运行 API 原边只说明调用持续多久，无法扣除已排队的 op8/op9 或拆出 GPU 搬运。故 `5/5` 是完整 B 超期重复，将分段计为 **timestamp 0、有效独立 prefix 0**。 [T801 原判定](../test/reports/t491/t801/v1-five-samples.json)

## 下一次设备执行

**Effector 可直接复用 T797。** 离线从实物 driver 读到五个诊断参数与 `GPU_SEGMENTS` schema 字符；源码在解析后把诊断模式绑定到 B，四个停止点由原 `Dispatch/Pump` 接线实现。先在同样输入和设备环境取一次 `--gpu-timing` 原件。有效时将该模式与 B 交错重复至 T799 所需组数。真实 GPU clock 不可用、disjoint 或端点缺失时，按下方四个独立命令取证。先每模式各一份确认停止端和原件导出，再扩为 T799 五组交错；进入新设备窗口补同窗 B，以 T801 当历史重复。 [T797 交接](<D:/hmprojects/HelloWorld-T771/docs/developer-T797-v1.md>) · [参数解析](<D:/hmprojects/HelloWorld-T771/test/t670/harness.cpp#L47>)

以下为设备 shell，在同一授权 HDC 目标上执行。`INPUT` 填 T801 实际 K192/S40 输入的已核目录，先保存该目录路径和输入 SHA，避免按猜测混入新输入。每调用用唯一 `SERIES`/输出目录，先保实际 argv 和散列，再采远端真实退出码。按 T801 的默认无显式程序缓存目录运行。

```sh
BIN=/data/local/tmp/t801bin
INPUT=/data/local/tmp/verified-t801-input
SERIES=t803-first
export LD_LIBRARY_PATH="$BIN${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
sha256sum "$BIN/g9_dm_differential" "$BIN/libgrs_dm_t647.so" "$BIN/libc++_shared.so"

run_one() {
  tag=$1
  shift
  out="/data/local/tmp/$SERIES-$tag"
  test -d "$INPUT" && test ! -e "$out" || return 2
  printf 'ARGV <%s>\n' "$BIN/g9_dm_differential" "$INPUT" "$out" "$@"
  "$BIN/g9_dm_differential" "$INPUT" "$out" "$@"
  rc=$?
  printf 'REMOTE_RC=%s OUT=%s\n' "$rc" "$out"
}

run_one b --timing=B
run_one gpu --gpu-timing
```

确认 GPU timestamp 不可用或缺端点后，在同一 shell 运行四个独立截点。

```sh
run_one op8 --prefix=op8
run_one op9 --prefix=op9
run_one copies --prefix=copies
run_one op11 --prefix=op11
```

每次保 `execution.jsonl` 原文件、原 stdout/stderr 与远端退出码、设备文件 SHA/长度/映射路径。正常退出也拉完整输出目录，保最后一行。device 和 host 均核文件 SHA。后续交错旋转执行顺序并换唯一 `SERIES`，禁止从 `s1…s5` 文件名或超期次数补填模式。保三件套真实设备与进程映射，完整 `pairedDsoFromHap` 回指 T797 回执的 `bootSource.packagedElf`；同目录的 T773 旧部署接口依旧用 `pairedDsoFromHap` 和 `pairedCpp`，可从原回执做留源的字段适配。

| 运行 | 必须观察的原件与停止位置 |
| --- | --- |
| `--gpu-timing` | `mode=TIMESTAMP`，同 PID/start、scope/nonce/deadline，明确 `GL_TIMESTAMP_EXT`、单位 ns、位宽、disjoint/error。只有读到有效、同队列、时间顺序可核的 `op8/op9/arena1/arena2/op11` 必需边才归因；状态 `UNOBSERVED` 或 null 不补时间。 |
| `--prefix=op8` | `mode=OP8`，`prefixReached=true`，停 op8 屏障后，cursor/dispatch/barrier **9/9/9**，BASE copy 后共 **1** 笔；无 op9。 |
| `--prefix=op9` | `mode=OP9`，停 op9 屏障后、arena copy 之前，cursor/dispatch/barrier **10/10/10**，共 **1** 笔；无 op10。 |
| `--prefix=copies` | `mode=COPIES`，停 op9 两笔 arena copy 的屏障后，cursor/dispatch/barrier **10/10/10**，共 **3** 笔；无 op10。 |
| `--prefix=op11` | `mode=OP11`，停 op11 屏障后，cursor/dispatch/barrier **12/12/12**，共 **3** 笔。应由退役新建 prefix fence。 |

四类 prefix 每类都须是独立 PID/start/scope，`prefixReached=true`，原 `DIAGNOSTIC_PREFIX STOP`、停止点 `WOULD_BLOCK`（root result 2）及 terminal 互核，正常执行 fence 未建。退役单独创建一次 fence（`attempted=true/created=true/reusedExecution=false`），保句柄、`retirementFenceCreate`、`prefixFlush` wall/CPU、实际 poll 原返回，无 map、无 typed、无提交。程序退出 0 代表该诊断根已安全退役。未到达、空 fence、GL/EGL 错、超时或 no signal 按原阶段逐条保未达。T773 复用 B 的标准判定，prefix 的正常提前停批需按 T797 证据单独判，不能以 B 的 flush 和 DEADLINE 成功样式覆盖它。 [停止和退役接线](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L953>) · [独立退役 fence 接线](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L1218>)

## 分段判定和精确补证

- 取同组的 op9 后与 copies 后 fence 观测区间。保守下界采用 `copies` 最后一个 TIMEOUT 的 **API enter**，减去 `op9` 第一个真实 signal 的 **API exit**，两者均相对各自根起点。TIMEOUT 返回时点用于识别原边，TIMEOUT API 入口才是保守下界。首 poll 即 signal 的样本缺实际 TIMEOUT 下界，只报上界和首次观测。按原 T799 再扣可见采证与跨进程开销，特别是截点后停止 heartbeat、owner drain 插 fence 的延迟、退役轮询计划间隔 1 ms 与实际调用时间，以及 prefix 自身提交重排。有效 GPU timestamp 也要联 API 分段以辨明提交空隙。如果无法扣出前序排队，同组差保不归因。
- 得到有效同组界后才应用 T799 的 **≥20 ms 且≥两倍波动**。发现 op8/op9 排队主导时分到 replay/compare，末批共享提交主导时分到驱动/owner 提交；适用同根、原 250,000,000 ns、真实 33,333,333 ns pulse/receipt、原 typed/oracle。晚信号只退役。诊断根不作为 SOURCE 或首 Swap 正例。
- 若实际 driver/argv 与完整拉取文件都匹配，仍缺 `GPU_SEGMENTS`、具体首阶段或比较必要的端点，保原命令与退出码并转 developer 修追踪。最小修订在冷编译前 flush 一条请求及 native 解析的 mode/arm 记录，带 PID/start、原构建身份；在独立 prefix 保 Stop 边、heartbeat join 及新 fence/flush 的 wall/CPU 和 FIRST/LAST TIMEOUT/首次 signal 原位。采集只扩现有固定槽，不加 GPU 命令、阻塞采样或更改 root；短暂截尾/零 TIMEOUT 仍按原观测留空。Tester 对 A/B legacy 兼容与 prefix 的模式/边界负控独立判，禁止以预期值补原件。

离线已复核实物 T797 三件套、T801 五份 T773 结果和原始 fence 时序。T773 既有 T762 归档、15 个时序夹具和 27 个证据负控复跑通过。T803 未使用真机。 [复核数据](architecture-T803-evidence-v1.json)
