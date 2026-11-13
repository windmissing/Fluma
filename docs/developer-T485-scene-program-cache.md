# T485 场景 program 缓存交付

2026-11-13，developer，不使用真机。实现依据：`architecture-T480-scene-program-cache-v1.md`。本卡范围为 M1/M2 三个 graphics program 的缓存、冷准备和必要离线检查。

## 实现

- `program_cache/graphics_program_factory.h` 提供双 stage descriptor、独立 `T480_GRAPHICS_KEY_V1` key、graphics 收据和当前 context/generation/owner 约束。key 编码实际 vertex/fragment 字节、stage、link 配置、ABI 与真实环境指纹。保留 T475 compute 工厂、key、收据和二进制格式。
- `m1/scene.cpp` 的 `ScenePrograms()` 定义 `SCENE_FULL`、`SCENE_PARTICLES`、`SCENE_WATER`；`PreparePrograms()` 用局部句柄准备三 program、反射 uniform/attribute/SSBO/output 并创建 VAO，全部成功才提交。失败清理本次对象；已合格的独立 role 可持久化。`aState` 优化为 inactive 合法，active 时校验位置和类型。
- Draw 从当前代 uniform 表读取 location，逐帧照原路径上传全部值及重设 SSBO、属性、texture 和 VAO 状态。shader、光学参数、pass 顺序、业务 token 与 lease 条件保持原合同。
- `test/t485/generate.cjs` 将以上接入 M1 和 M2 当前生成链。实际 GL owner 在 transport Create 后、READY 提交前调用准备；每 role 前后检查 candidate，编译期间不持控制锁。失败输出 `SCENE_PROGRAM_CREATE`、role/detail 和耗时。冷准备不 Draw/Swap、不接受 publication、不设置 lastGood，也不制造 quota/pulse。
- 缓存损坏、未知格式、驱动拒绝走一次原 GPU GLSL 回退；导出/I/O 失败保留合法 program。三 role 共用 T475 的 32 条、单条 32 MiB、总 payload 128 MiB 限额和持久化线程，无第二缓存目录。产品所有前景背景仍为 XComponent+shader。

## 非机证据

机器收据：`test/reports/t485/delivery.json`。其 sourceManifest、六 stage 字节清单、三 role ABI/key、包/ELF hash、签名收据构成来源链；签包 ELF 中逐个核对实际 shader 字符串。CMake 在 Windows 写 CRLF，C++ 翻译阶段会将换行归一化，collector 按相同规则比较宿主头文件。

- 实际生成 M1/M2 Scene 各跑 cold/warm/off，四场景各八个层/最终 readback，共 192 份像素缓冲，逐字节一致且每路线业务 token 相同。覆盖空场、床+飞沙+W/J、镜像和 viewport。`pixels.json` 保存 hash、收据和退回记录，`pixel-originals.zip` 保存原始像素和运行日志。
- 每次运行检查八类拒绝路径：坏 publication、无可用 lease、旧代、context 不匹配、Swap 失败、迟到真实 RootIsolation 许可、Draw GL 故障、候选中途失效/清理。不得发布新 CURRENT。
- 工厂 13 组检查通过：stage/key 变更、VS/FS 编译失败、link 接口不匹配、反射失败、格式/driver 拒绝、原始 GL 错误、I/O 故障、代际与 owner、context、候选失效、非法 descriptor、无 binary format 等。日志保留原始编译/link 错误。
- 坏 hash 与截断档各用新进程验证一次源码回退，编译计数均为两个 stage，修复后像素与基线一致。
- 真实旧 compute binary 仍以原 key `7234cbb48b952df7966f31a83f07bde24db8f7d210cc2ecbb64f4e0b4c86d153` 命中。原 T475 非机检查通过。
- 生成 App 的 M1/M2 均做 cold/warm 检查：三 graphics 已 prepared 时仍未 READY；新 session 的首个真实 host pulse 后才 READY，generated quota 为 0。M1 复用 compute binary，graphics 仍从 cold 编译开始。宿主显式优先包含本次项目 CMake 生成头文件。
- M1/M2 `hvigorw default@PackageHap` 构建成功，handoff 两签包 `verifyApp=PASS`。签包路径和 SHA256 见 `test/reports/t257/T485-m1-handoff.json`、`T485-m2-handoff.json`。

## 边界与接续

像素测试使用冻结 publication 的 host fixture，经真实 transport lease、GL buffer 和实际 Scene GLES 绘制；这不重新估计 compute 准入正确性。ANGLE 仅作离线测试设施，不是产品软件 GL 回退。没有运行真机，未宣称设备命中、当前上屏或冷启动收益已通过。

T486 可消费源码、原像素及收据独立离线验证；T491 消费两份 handoff 签包，串行设备验证各 role 的持久化/暖命中、准备耗时及完整 Draw/Swap。原 session/permit 阻断仍应按其原卡处理，本卡未将缓存成功当成该问题根治。
