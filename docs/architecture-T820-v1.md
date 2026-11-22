# T820 · G13 RESET 实际编译停点与等价角色拆分

2026-11-22 · 目标20 · P5 · architect · 离线首次归因

**T817 已把停点定位到 `glCompileShader` 的调用边。** EGL 初始化、私有 context 建立、角色输入 SHA 校验和 `glShaderSource` 均已返回。第一个未闭合调用是为 RESET Q5 准备 **48,695 B `OP_00`** 的 `glCompileShader(shader=2)`。取消请求后仍看不到该调用退出、owner ACK 或退役。T813 为分辨 EGL 和角色路径的同角色对照已经没有必要。下一次直接验证只含 RESET 两个原 opcode 及原资格段的 **7,381 B** 闭包。 [T817 原报告](test-report-T817-v1.md) · [逐项证据与生成实测](architecture-T820-evidence-v1.json) · [T813 备选边界](architecture-T813-v1.md)

## 实际工件、调用和取消

T811 未签 `0ba9bf20…` → T817 已签 `d42401c2…`。13 个 payload 逐字一致，实际进度/可信诊断标志在 `libgrs_g13_visibility.so`，SHA `070c5452…`、BuildId `25e750c83d9e3991842c08dd5a9b6fae8d6c42e4` 与原进度相同。PID **53577** / start **8535146**，owner TID **54453**，RESET attempt/generation **1/1**，进度容量 **192**、dropped **0**。原 `G13_VISIBILITY_PROGRESS_V1` 逐边通过带 BuildId/attempt/generation 的 T812 reader，报告最后未闭合调用为 seq 44。进度 schema 自身无 role SHA/nonce；实际 role 是从已签包、冻结的 RESET 路由和通过的 `ROLE_SHA` 边联合确定。未取得原 RESET 最终语义原件来断言通过。 [T811 编译回执](<D:/hmprojects/HelloWorld-T811/docs/receipt-T811-compile-v1.json>) · [T817 签包](../test/reports/t257/T817-probe.json) · [T812 reader](<D:/hmprojects/HelloWorld-T811/test/t812/read_progress.py>) · [原进度](../test/reports/t491/t817/probe-reset-t812.json)

| 同代原边 | 结论 |
| --- | --- |
| seq 4→31 `Open` | actual wall **30.535 ms**，出口 result 1，已过 EGL、GL 版本/字符串和原 context 检查。 |
| seq 34→35 `ROLE_SHA`、seq 36→37 `glCreateProgram` | 角色 SHA 匹配；建 program 已返回。第一个 `Prepare(5)` 取 Q5/OP12，Q6/OP13 原本复用该角色。 |
| seq 40→41 和 42→43 | compute 类型 `GL_COMPUTE_SHADER` 的 shader 对象为 2；同对象经原 `glShaderSource(shader, 1, &source, nullptr)` 返回。后续源码长度由实际 C 字符串计，SHA 对应已签 ELF 中的完整 **48,695 B**，不含终止 NUL。 |
| seq 44 | owner 在 **85401378425000 ns** 记 `glCompileShader ENTER`。三份早期快照和三份取消后快照都无匹配 EXIT，未见 compile status、link、reflection 或任何 RESET dispatch。 |
| seq 45 | UI TID **53577** 在距 compile ENTER **284.869 s** 发 `CANCEL_REQUESTED`。T812 判 `REQUESTED_UNACKNOWLEDGED`，retirement `UNOBSERVED`、leaseHeld 为 true。UI 线程 CPU 与 owner CPU 不作差。 |

T817 报告记 owner R 态及 `utime 13402→36524`，约 90 s 的采样间进度不变。归档未保两份完整原 stat、采样 HZ 或符号化 PC/栈，足以把“报告的 CPU 活跃”和“原件证明的调用未闭合”分开，仍无法定位驱动内部循环、锁或编译器具体 pass，也无需用相同 48 KiB 原编译再等一次来决定是否做最小等价拆分。 [取消原件](../test/reports/t491/t817/probe-cancelled.json) · [实际 Compile/Link/Prepare](<D:/hmprojects/HelloWorld-T811/.codex/local-test/t811-probe/entry/src/main/cpp/gpu_resident/g13_visibility/probe.cpp#L287>)

## 第一阶段用原 RESET 跑小闭包

Developer 在现有 `tools/g13/split_shader.py → generate_assets.py → pipeline_source.h / assets_manifest.json` 生成链加显式诊断变体。从已核原父 shader、原 opcode 表及同一 token 级 specialize 和传递可达函数求闭包。为 OP12（`TRIAL_GATE`）、OP13（`TRIAL_RESET_FROM_BASE`）分配一起的 `RESET_12_13`，保原私有 OP66 段。原 `OP_00` 的其余 opcode **0–11、14–17** 走余下的闭包，其它角色原字节保持。唯一 runtime opcode 映射覆盖 0–66，OP66 的常规入口仍归 BOOT，其他角色中的 OP66 仅保原资格用途。禁止同时将 OP12/13 注册到原 role 和新 role 或在同 attempt 用旧 shader 回退。新 mapping、角色名/边界、实际生成 SHA/bytes、后端 ABI 进入各自构建回执和缓存身份。

以下从 T811 实物的 `pipeline_assembled.comp` 复算，首先用原 splitter 重建并逐字核全部既有角色。试验仅改分组。在新 emit 中逐 opcode 比对原专化分支、OP66 和保留的函数/全局声明，所有条目均通过离线 GLES glslang 和 Vulkan 1.1 SPIR-V 静态验证。未生产 HAP 或运行新真机着色器。

| 可落地分组 | GLES 完整输入 | Vulkan GLSL / SPIR-V | 用途 |
| --- | ---: | ---: | --- |
| 原 `OP_00` | 48,695 B，`1b8ce7f6…` | 原工件另账 | 冻结历史对照。 |
| OP12 + OP13 + OP66 | **7,381 B**，`e270a9c7…` | 7,699 B / 22,008 B | 首选修订，少一个跨 program 切换，RESET 的 Prepare(6) 同代复用。 |
| OP12 + OP66 | 6,571 B，`d5d00043…` | 6,889 B / 17,064 B | 合并闭包仍卡编译时才单独切分 gate。 |
| OP13 + OP66 | 6,364 B，`f050c649…` | 6,682 B / 15,968 B | 同上，单独切分 reset/copy，保原 gate→copy 的可见性。 |
| 原 OP_00 余段 + OP66 | 46,928 B，`e1a72d7f…` | 47,246 B / 252,484 B | 首轮 RESET 不准备此 role。逐原 case 实测后再按实际调用分段，避免把下一大组隐性移入首轮。 |

每个实际完整输入继续过 **49,152 B** 上限，RESET 合并闭包相较原 48,695 B 缩减约 **85%**。首轮采用固定组及重新生成的 manifest，不把静态省字当真机编译时间收益。GLES 仍要求真实 `uOp/uScope/uIndex`、64×1×1、Control binding 3；实际反射核新闭包的活动项，取所需 `uL` 活动长度上传。离线反射合并闭包活动项为 Base 0、Candidate 1、Control 3、`uL` 长度 5；Work 仍在原 ABI 声明与四个真实资源绑定中，诊断小闭包允许原反射规则已有的非活动块。保原 `ResolveRootBinding` 和真实范围，不以 glslang 输出代替设备实际反射。 [当前 splitter](<D:/hmprojects/HelloWorld-T811/tools/g13/split_shader.py#L153>) · [原 locations 与四绑定](<D:/hmprojects/HelloWorld-T811/.codex/local-test/t811-probe/entry/src/main/cpp/gpu_resident/g13flow/gles_submit.h#L21>)

Vulkan 使用同一闭包来源，按 T731/T741 映射 set 0 binding 0–3、布局 UBO binding 4、原 push offsets 0/4/8/12/16/20、stride 16 与 local size 64×1×1。离线已核三项 FP32 32-bit modes、原精度约束和 SPIR-V 结构。GPU 现场仍须走该设备支持检查、原实际 pipeline/屏障、原字和独立发布链。后端照原工程资格阶段选择。这一步为隔离编译/验证，产品 BOOT 的 OP65/66、逐角色资格和实际首根条件保持。 [T731 后端工程合同](architecture-T731-v1.md) · [T741 数值资格](architecture-T741-v1.md)

## 验证顺序与退役

1. **Developer** 先提供带新角色映射的同签诊断构建、T811 逐调用诊断与单独的实际输入标识。为编译调用保 role、原 ordinal/opcode、type、实际 string count、完整输入字节 SHA/长度、program/shader 对象、context、owner 和 generation。仍分别记录 source、compile、compile status、link、link status、反射、原错误和实际缓存各阶段。短时快照与工作线程分离，编译进行中不持生命周期锁调用驱动。T812 原进度格式保持兼容，角色装载证据独立关联同一 attempt。
2. **Tester / effector** 先验真实 signed HAP/映射 ELF，再直接跑原 RESET。沿 `Q5→Q6→Q410→Q411`、ACTUAL/REPLAY 和原独立对象、原四绑定/有效范围、dispatch 与原 storage barrier、tail、同代 fence、actual map/unmap。按 T769 核 BASE、CANDIDATE、REPLAY word 319/320 和原 tail 证据，覆盖错 scope、错代/role、错绑定以及缺 barrier。新 manifest 显式绑定新角色 SHA，禁止把旧 SHA 设为通配。证据进度可逐段报告 source 调用退出、链接及反射退出、原 RESET 原字和退役。只完成编译的阶段无 RESET PASS，也不计产品首个 Swap。
3. 若 7,381 B 合并 role 仍出现同一未闭合 compile，才做两个单角色闭包的定向比较，并优先保真实栈/PC、原 stat/HZ 或不可采原因。两角色都可运行时，原跨 program 的 gate→copy 保相同队列、scope/物理对象、顺序和 storage visibility。若缺端点只是日志问题，先补日志。另按原设备资格逐步验证 COMPARE/PACK 等仍需余段的场景。若换 Vulkan，仅由同源实际设备精度资格及完整 RESET/退役结果支持对应结论。
4. Cancel 只由原 owner 在正确 attempt 的实际检查点 ACK。长调用未返回须保 `CANCEL_REQUESTED`、开放的 ENTER 和 `OWNER_BUSY`，禁止跨线程销毁或宣称同代退役。未发 dispatch 的失败就按原未提交分支清理。已有真实提交须覆盖最后资源访问的 fence 或原安全隔离，才释放 lease。已提交的场景与 BOOT/产品 cache 跨代隔离，正例仍要通过 T769/T782 及原产品数值门。GPU 驻留、CPU 停投、产品只用 XComponent+shader 上屏。 [T764 探针合同](architecture-T764-v1.md) · [T769 原 oracle](<D:/hmprojects/HelloWorld-T811/docs/test-report-T769-v1.md>) · [T782 工件验法](<D:/hmprojects/HelloWorld-T778/docs/test-report-T782-v1.md>)

本卡完成离线身份、逐边、输入及闭包生成验证。未修改产品代码或使用真机。
