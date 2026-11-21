# T791 · RX 冻结动态设计 manifest 归因与构建合同

2026-11-21 · 目标 9 · P3 · architect · 离线归因。

**工程问题，转 developer。** 动态动员 manifest 属于 RX 的冻结上游物理契约。T637 已登记原路径和批准 SHA，现有 RX 只把当前文件的 SHA 写进生成依赖。重新生成后，`verify_assets.py` 又用这份新摘要比对同一损坏文件，因而放行。修订范围是把已有冻结校验接进生成和验证，保留 T781 正常重建和生产 GLSL。 [T787 测试报告](<D:/hmprojects/HelloWorld-T787/docs/test-report-T787-v1.md>) · [T791 复核原值](architecture-T791-evidence-v1.json)

## 原因与批准身份

| 对象 | 当前行为和证据 |
| --- | --- |
| T637 clock manifest | Generator 先从 T641 四表读取 `design` 描述，比对原文件字节数及 SHA。缺失或改写会在生成期失败。受核验的 T637 文件已经包含 `specHashes.physicalSpecFile` 和 `physicalSpecFileSha256`。 |
| 动态动员设计 | `docs/design-G9-dynamic-mobilization-manifest-v2.json` 同时是 T554 `parentDesign` 与 T637 `physicalSpecFile`，批准 **13,045 B**，SHA-256 **`b17060c0441e481ec99f3279c76c8b8c94a1013b0fd9045d6b01b6cb827dac77`**，即 T781 从原 T763 构建取回的 CRLF 原字节。现实现只执行 `deps[relative] = sha(currentBytes)`，没有沿已验证 T637 的引用再核一层。 |
| 后续 verify 和 Hvigor | `verify_assets.py` 将当前文件与生成的 `inputs.json` 对比。坏输入配旧资产时有 `RX_STALE_INPUT`。fresh generator 接受坏文件后，互相匹配的坏输入和新收据照样通过。CMake configure 依次调用 generator、verify，两次返回 0，Hvigor 成功出未签 HAP。 |

离线在一次性目录复跑 approved、`{invalid-json`、合法 JSON 但 `formula.iterations` 从 32 改 31 三种输入。两种改动均使旧资产验证失败，fresh generate 和 fresh verify 却均返回 0。三次实际 canonical GLSL 均为 **146,301 B**、SHA `2cb9a84813833027f68ee7434f98f833dae01e310d9ea053e57e2870d133778e`。T787 还实际构建了坏 JSON 对应的未签 HAP。本轮据此定位冻结身份缺口，保留已有正常 native 构建和 shader 证据。 [生成器](<D:/hmprojects/HelloWorld-T787/tools/g9rx/generate_assets.py#L23>) · [只读验证](<D:/hmprojects/HelloWorld-T787/tools/g9rx/verify_assets.py#L13>) · [T637 冻结引用](<D:/hmprojects/HelloWorld-T787/docs/design-T637-manifest-v1.json#L1095>) · [T554 批准原件](architecture-T554-g9-dm-manifest-v2.json) · [T781 编译回执](<D:/hmprojects/HelloWorld-T787/docs/receipt-T781-compile-v1.json>)

## Developer 实施合同

1. 在 RX 构建侧复用一处只读冻结输入校验，由 `generate_assets.py` 和 `verify_assets.py` 共用。先用现有 T641 批准的字节/摘要核 T637，再从已核验的 T637 原字节读 `specHashes.physicalSpecFile` 和 `physicalSpecFileSha256`。路径必须是已登记的 RX 动态设计文件，SHA 为合法 SHA-256。以该独立批准值作真值，`inputs.json` 保留为产物依赖一致性记录。先读一次动态设计原字节，使用同一份快照做 UTF-8、JSON、schema 与 SHA 检查，生成端只登记这份验证通过的 SHA。保持原 checkout CRLF 属性。
2. JSON 必须为严格 UTF-8 JSON object，有效 schema `G9_DYNAMIC_MOBILIZATION_DESIGN_V2`、route `gpu-spf-dynamic-mobilization-v2`、profile `G9_SPF_DM_Q20_V2`，`units`、`formula`、`research` 为对象。拒绝重复键、非法常量和截断。随后用**文件原字节** SHA-256 对比 T637 的批准值，确保字段、值及版式都与冻结版一致。结构检查负责诊断，精确内容由批准 SHA 约束。原 manifest、T637/T641/T554 的批准内容及 DM 原路线继续沿用。设计变更须有独立批准与版本化引用，再由 architect 更新构建契约和 tester 更新基准。
3. Generate 在写 `inputs.json`、派生源码或依赖列表前校验。Verify 在接受任何本地生成物前独立从同样的批准来源校验，再执行原 stale/hash/shader 对账。错误带具体路径和阶段，缺文件保原 `FileNotFoundError`，UTF-8/JSON 错误用 `MALFORMED_MANIFEST`，对象形态/版本/标识错误用 `DESIGN_SCHEMA_MISMATCH`，合法但原字节 SHA 与批准不同用原 `DESIGN_IDENTITY_CHANGED`。CMake 保持非零必停，保留实际 Python 首错，并维持构建时 `rx_source_identity_guard`。脚本和批准依赖纳入现有 configure depends 与 fresh overlay 复制闭包。失败构建不产新可交付 HAP，现成旧 HAP 按此前自身收据归档。
4. 同一 common gate 覆盖 default、`--test-hooks`、`--boot-only`、`--boot-source` 的合法组合。只修构建输入批准与证明链，生产 accepted shader 原字、ABI、配置索引、时钟、GPU 驻留、原 oracle 与 XComponent+shader 渲染约束继续有效。developer 在 fresh overlay 实施并编译，带原工具命令和结果回执。 [RX CMake 原入口](<D:/hmprojects/HelloWorld-T787/entry/src/main/cpp/gpu_resident/g9rx/CMakeLists.txt#L31>)

## Tester 离线回归

| 用例 | 预期 |
| --- | --- |
| T781 原件，fresh configure/build | 两份批准 manifest 原字可读，正常 generate、verify 和 Hvigor 通过，RX canonical GLSL 与 T781/T787 正例一致。保持包内入口、ELF 与编译收据配对。 |
| 无生成物的 fresh overlay，缺文件、坏 UTF-8、坏 JSON、错 schema、合法内容更改、LF 异于批准 CRLF | 分别在冻结输入阶段以预期原错退出。至少对 T787 原坏 JSON 重做无历史产物的真实 Hvigor 反例，保首错、非零结果与无新 HAP。另核各合法构建变体通过相同校验入口。 |
| 先有正确资产，后破坏输入 | 使用实际只读 verifier 新进程拒绝，已生成描述不能覆盖独立批准值。 |
| T787 原坏输入与当时成功生成的相配新资产，或测试中重写 `inputs.json` 使其与坏输入摘要一致 | Verifier 仍在冻结阶段拒绝。合法 SHA 改写反例同时改 T637 引用，由原 T641 固定摘要先拒绝改写后的 T637。 |

负控只在独立工作区进行。保 T787 已有报告及坏包为反例。目标 9 后续继续离线，T791 未使用真机。 [T787 独立 checker 与原判定](<D:/hmprojects/HelloWorld-T787/test/t787/check.py>)
