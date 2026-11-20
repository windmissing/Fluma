# T732 G13 Boot INIT_ERROR_27

## 原件与可判定范围

T730 SPLIT 原签包 `707a751b…` 的 ARM64 ELF BuildId 为 `a9363c84680c230ac8ecfe92d594d7c8e5889146`，内嵌 BOOT 为 23,834 B、SHA256 `a93aecf86ddb1f782b337067f5b5c54fe7ac5676e2f9c24d8c25f8c8af4b24bb`。单程序原臂的 259,025 B、`7563c614…` 不属于该 cache 记录。角色记录仅支持 BOOT 冷 build 116.491 s、后次同 binary build 10.259 ms、其中 binary load 5.645 ms。其记录缺同配置票据、进程映射与新进程配对原件。退出原因未知。

原 `s4.json` 绑定 queuedTicket/firstFailureAttempt=209，记录 `INIT_ERROR_27`、`INIT_TAIL_VALID:EXIT`、bootReadyNs=0。后采 `s5.json` queuedTicket=1843，firstFailureAttempt 仍为 209；这条历史 firstFailure 不能直接绑定给新票。旧实现中，进入 CPU 对 op 66 的独立 FP32 拒绝前必经 `INIT_FP32_RESULT`。209 的末阶段优先指向 op 65 初始化尾部拒绝；原封样本缺 cold[255]/cold[7]、WORK 五项和两轮探针原始位，具体谓词待取证。

## 工程修复

1. op 64 以 SSBO 写入 bank 后，CPU 用 `glBufferSubData` 上传同一 bank 的种子头。现于重叠更新前补 `GL_BUFFER_UPDATE_BARRIER_BIT`。64→65→66、原 SSBO barrier、init fence、反射与退役顺序原样。 [OpenGL ES 3.1 §7.11](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)
2. Boot 后的逐角色 FP32 读回原来把 `18632` 字乘 4 后传给还会乘 4 的 `CopyBuffer`。旧映射起点为 298,112 B，超过 74,752 B CTRL。现在 helper 统一接收字偏移，实际映射起点为 74,528 B，16 个字结束于 74,592 B。Boot 的原 init tail 和完整根尾部仍经正确的 18,432 字读回。此缺陷位于 Boot 之后，旧票据 209 尚未到达。
3. 仅在 init fence 返回后读回 CTRL，终态保留 `G13BootReadbackV1`，包含真实 seal/error、两轮 ctrl[200..215]。失败且 seal 可验时，额外读现有 WORK 样本的 35/41/47/53/59，顺序为 sin(0)、cos(0)、sin(π/2)、次正规保留、tie。该补证与生产首个 GL 错误分列。未读使用空数组或 −1，拒绝码与原谓词保持一致。`G13InitDiagnosticV1` 的四页仍为 32768 B，新增证据在状态与 `G13Boot` hilog。保留实际 `INIT_TAIL_VALID`、`INIT_FP32_RESULT` 及终态 fence 行。

两臂未签 HAP、BuildId、SHA 与原件索引见 `test/reports/t732/compile.json`。Hvigor 原生、ArkTS、最终 HAP 与 GLSL 静态编译通过。Boot、其余角色及图形 GLSL 与 T728 逐字节一致，FP32 与数学门未改。

## T730 续测

优先用修订 SPLIT 签包，固定原 REST 输入、profile 和冷暖条件，匹配同 PID/start、映射 BuildId、boot/attempt/queue/run/context、BOOT SHA。只发一次 Configure，保留原返回与 `G13Init`、`G13Boot`、原四页及其采样身份。取 `INIT_FENCE_POLL` → `INIT_TAIL_VALID` → `INIT_FP32_RESULT`，按原字判定。若 op 65 返回 27，用 WORK 样本定位哪个原谓词拒绝；若 op 65 成功而 op 66 失败，对照 probe 的两次 8 字签名、GL 原码和首个差异。如果 Boot 通过，分别记录每个后续角色的 FP32、COMPUTE_READY、首个非零完整 quantum 与同 token 的有效 Swap。开发侧未签包、未使用真机。
