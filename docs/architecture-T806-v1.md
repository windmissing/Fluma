# T806 · DM 前缀成本复算与复制候选裁定

2026-11-22 · 目标20 · P5 · architect · 离线复核

> T815 补充。T809 补到一轮 B 和四前缀。原始 copy 夹逼仍低于 20 ms，复制候选专用补组可停。下文精确命令保留给实际证据再次触发时使用。 [T815 裁定](architecture-T815-v1.md)

T804 交了 **一组四个独立前缀**。同组 `COPIES−OP9` 的原始 fence 完成时间差夹逼为 **6.108–8.592 ms**，尚未扣跨进程与截点提交偏差。第一笔 arena copy 的约 91 ms owner 开销，在没有提交 arena copy 的 OP9 前缀出现在新建退役 fence。**T799 保持 `COPY_COST_UNCONFIRMED`，暂不触发双目的 GPU 复制实施。** [原报告](../test/reports/t491/t804/v1-gpu-timing-prefix.json) · [逐次原值](architecture-T806-evidence-v1.json) · [T799 实施门槛](architecture-T799-v1.md)

## 原件与时序

每份有 29 行，末行是同 PID/scope/nonce 的 `GPU_SEGMENTS`。四个独立前缀均有 `DIAGNOSTIC_PREFIX STOP`，根状态 `WOULD_BLOCK`，诊断退出码 0。执行 fence 未创建，退役各自新建 fence，句柄 1 只在各自进程内有效。各有真实 `37147` 后到 `37146`，退役完成。四根均无 map/typed、commit/quanta 为 0、FIFO 留 1。实际映射 driver BuildId `09befda4647ef0d3b440ac5b880587d8a5c9e057`。T773 使用 T797 回执复读四份均给 `REJECTED`、执行 `NOT_ATTEMPTED`、退役 `SIGNALED`，设备全量部署字节仍记 `DEPLOYED_BYTES_UNOBSERVED`。原 `SOURCE_TIMING` 的 GPU 占位与实际 `GPU_SEGMENTS` 分开读取。

| 前缀原件 | 游标 / dispatch / 屏障 / copy 屏障 | B 根 wall / thread CPU，ms | 新建退役 fence wall / CPU，ms | 截点 flush wall / CPU，μs | 相对根起点的 TIMEOUT 入口至 signal 出口，ms |
| --- | --- | ---: | ---: | ---: | ---: |
| [OP8](../test/reports/t491/t804/p8.jsonl) | 9 / 9 / 9 / 1 | 1.179 / 1.177 | 96.218 / 95.893 | 4.063 / 3.906 | 284.184–285.371 |
| [OP9](../test/reports/t491/t804/p9.jsonl) | 10 / 10 / 10 / 1 | 1.183 / 1.181 | 92.181 / 92.045 | 3.750 / 3.750 | 315.671–316.880 |
| [COPIES](../test/reports/t491/t804/pcopies.jsonl) | 10 / 10 / 10 / 3 | 92.248 / 92.076 | 0.026 / 0.026 | 2.500 / 2.344 | 322.987–324.263 |
| [OP11](../test/reports/t491/t804/p11.jsonl) | 12 / 12 / 12 / 3 | 96.948 / 96.895 | 78.096 / 78.098 | 3.282 / 3.125 | 347.874–349.180 |

根 wall 用 `SOURCE_TIMING.root` 的 API 首尾，fence 夹逼以各自 `SOURCE_ROOT.rootBeginNs` 为零点。末次 TIMEOUT 用 API 入口作保守下界，首次实际 signal 用 API 出口作上界。计算区间为 `[322987344−316879531, 324263437−315671406] ns`。轮询次数只用于验序。COPIES 的首、次 arena copy wall/CPU 分别为 **91.033/90.865 ms** 与 **29.688/29.844 μs**，OP11 的第一笔为 **95.622/95.573 ms**。只凭调用墙时无法拆出前序提交、驱动工作和搬运，截点之间的开销迁移也会破坏直接相减的因果解释。这一组的原始区间上界低于 20 ms，只能作排查线索。历史 T801 是五次完整 B，提供超期重复性，无法提供五组分段波动。

实际 timestamp 轮为 `GL_TIMESTAMP_EXT`、64 bit、初末 disjoint 均 0、GL 错 0，`INCOMPLETE`。只有 op8 两个端点，间隔 **6,094 ns**，其余端点为空。op8 dispatch wall **49.062 μs**，barrier wall **2.656 μs**，barrier 出口到 `DISPATCH_POST_GUARD DEADLINE@8` **196.136 ms**。根 wall/CPU **263.926/248.195 ms**，比期限晚 **13.897 ms** 才记录首拒，9 次物理 submit，游标 8。无法由此确认 timestamp 本身的开销、可复现性或其余段成本。诊断前缀的 GPU 状态应为 `NOT_REQUESTED`。 [timestamp 原件](../test/reports/t491/t804/g1-gpu-timing.jsonl)

## 续采合同

Effector 复用 T797 与 T804 同一 K192/S40 输入、物理 seed/参数/布局、同台设备、同 owner 入口和无显式缓存目录。T804 这组来自同次历史采集，但归档缺组内完整 B、实参和设备 SHA 账。若能恢复原始同窗配对及设备证据，至少补四组。当前按**五个完整新组**排，每组独立进程各跑 B、OP8、OP9、COPIES、OP11，循环旋转起始命令。以下直接复用 raw 映射下的 driver。将两个位置参数填为已核验的原目录与唯一轮次名。

```sh
INPUT=$1
CAMPAIGN=$2
BIN=/data/local/tmp/t801bin
test -n "$CAMPAIGN" && test -d "$INPUT" || exit 2
export LD_LIBRARY_PATH="$BIN${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
wc -c "$BIN/g9_dm_differential" "$BIN/libgrs_dm_t647.so" "$BIN/libc++_shared.so"
sha256sum "$BIN/g9_dm_differential" "$BIN/libgrs_dm_t647.so" "$BIN/libc++_shared.so"

run_one() {
  group=$1; tag=$2; shift 2
  out="/data/local/tmp/$CAMPAIGN-$group-$tag"
  test ! -e "$out" || return 2
  printf 'ARGV <%s>\n' "$BIN/g9_dm_differential" "$INPUT" "$out" "$@"
  "$BIN/g9_dm_differential" "$INPUT" "$out" "$@" >"$out.stdout" 2>"$out.stderr"
  rc=$?
  printf 'REMOTE_RC=%s OUT=%s\n' "$rc" "$out"
}

for group in 1 2 3 4 5; do
  case $group in
    1) order='b op8 op9 copies op11' ;;
    2) order='op8 op9 copies op11 b' ;;
    3) order='op9 copies op11 b op8' ;;
    4) order='copies op11 b op8 op9' ;;
    5) order='op11 b op8 op9 copies' ;;
  esac
  for tag in $order; do
    case $tag in
      b) run_one "$group" "$tag" --timing=B ;;
      *) run_one "$group" "$tag" "--prefix=$tag" ;;
    esac
  done
done
```

每组随原设备和输入完整文件 SHA/字节数留实际 argv、退出码、stdout/stderr、未筛选的 `execution.jsonl` 及映射 ELF BuildId。保 stop、实际中间/截点 flush、owner 生命周期、独立退役 fence、最后 TIMEOUT API 入口和最早真实 signal API 出口。报告探针扰动、截点到 fence 的提交和线程调度、设备负载、组内差、组间范围及失效分母。需复查 clock 时可额外执行 `run_one clock gpu --gpu-timing`，保持新进程，采全域实际 marker/disjoint 并与 B 对照，单个端点对不进入成本门。

只有扣除前序已排队命令及采证偏差后，连续取得至少五组有效配对，copy 特有成本下界 **≥20 ms** 且 **≥2× max(有效组间范围, 采样分辨率)**，才开 T799 研究臂。若明确耗时在 compare/replay 或公共提交，按对应段转 developer。若 copy 前排队无法扣出，保待触发。全根的 250 ms、真实 pulse、typed/oracle 和 GPU 驻留继续照旧。真机执行及结果留原角色，本卡未使用真机。
