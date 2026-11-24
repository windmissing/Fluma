# T933 · 集成 Demo、G15 资格和私有重放

## 单一 Demo 候选交 T935

工件目录为 `.codex/local-test/t933-integrated-demo`，唯一启动入口为 `Start Demo.cmd`。清单固定同一 Windows PE、SOURCE、候选 QUANTUM、独立审计/replay 和启动参数。启动器逐件校验 SHA-256，并将同一版本清单及真实运行日志写入它打印的目录。此包是 T935 的唯一送验档，体验版放行以 T935 的实际 PC 结果为准。

- 保紧凑盒、转盒/复位、真实长按和旧 fence 退役恢复。默认使用 35 派发和既有分频呈现。每个成功 Q 仍消耗 1/120 s，沿用真实 freshness、原 250 ms、同代 fence 和入料撤销账。
- 候选与独立 replay 的宽除法用等价的 32 位分块除法。小分母以整段处理低字，大分母沿用逐位进位。SOURCE 字节保持原版，除法余数、取整、守恒、事件顺序与接触公式保持原合同。
- 独立审计的库存扫池后按列归约，按材料分别稳定排序 ID 并核相邻重复。角查找使用上下界二分，上传前核全部 15,361 个切线原字非递减。来源前缀、有效槽、空槽字段与独立 spare 的逐字校验仍完整执行。分段失败保原逻辑 stage 21、原原因和原行序。
- 内部 `--m1-audit-breakdown` 使用配套 38 派发模块，分别计库存、角域、来源、余下审计、spare copy、物理 replay 与 metadata。普通 `--m1-stage-timing` 可在 35/127 派发中量同一其余点位。收据记录 query reset serial、源前缀和事件范围。`t933_event_work.py` 从同根归档给行类型、触及列和按相邻列/槽估计的局部依赖链上界。

T935 以启动包原参数量同根像素和 Down 至首次达标 Present 的实际墙钟，随后看 60 秒后真实 SOURCE 与 Q、松手留存、转盒与复位。需测迟退役续等时，须取得首次等待超时、后续同 fence 成功和新会话继续实收的一条完整链。此次未在 PC 启动 Demo 或宣告效果通过。

### 构建和内部计量

在 MSVC x64/Ninja 环境中运行。`$vk` 指本机 Vulkan 头文件目录，`$glslang` 指支持 SPIR-V 验证的编译器。

```powershell
$l = '.codex/local-test'
python pc/vulkan/build_t918_compact.py --assembled "$l/t280" --compiler $glslang --out "$l/t933-shaders" --integrated
python pc/vulkan/build_t918_compact.py --assembled "$l/t280" --compiler $glslang --out "$l/t933-timed" --audit-breakdown
cmake -S pc/vulkan -B "$l/t933-ninja" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DG13_GLSLANG=$glslang" "-DVULKAN_HEADERS=$vk"
cmake --build "$l/t933-ninja" --config Release
python tools/package-pc-integrated-demo.py "$l/t933-ninja/hello_world_pc.exe" "$l/t933-shaders" "$l/t933-integrated-demo"

$exe = "$l/t933-ninja/hello_world_pc.exe"
$timed = "$l/t933-timed"
& $exe --m1-window "$timed/full_source.spv" "$timed/quantum_candidate.spv" "$timed/quantum_commit.spv" --m1-compact-demo --m1-audit-breakdown --m1-one-quantum-grant --m1-source-trace --m1-dual-source
```

127、T926 35、T933 35 和 38 诊断档分开核工件、实际 grant 和时标。静态验证涵盖 SPIR-V、100,080 组宽除法商和余数差分以及既有根归档的行解析。实际同 BASE GPU 原字等价、GPU 分段时标和长时吞吐交独立复测。

## G15 opcode 23 私有建管线资格

同一 G15 Release PE 执行 `--qualify-g15-op23 MODULE [PREFIX]`，复用正式 owner 的 4 SSBO + 1 UBO、布局与 20 B push。资格入口只建管线，输出真实模块、布局、flags/cache、owner TID、QPC 和 VkResult，private 结果无 BOOT、业务根或发布。

独立准备脚本验证原版 123,696 B，只移除精确的一条 52 B `OpSourceExtension`，产物逐字等于 baseline。第二组固定去记录后的目标模块，交错对比经完整摘要固定的 DM2/G15 0–22 前缀。默认仅准备工件。effector 加 `--run` 启动 ABBA 子进程，复用 20/45 s stack dump、owner CPU 和已加载驱动映像收据。

```powershell
cmake -S pc/vulkan/sand -B "$l/t933-g15-release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSAND_FAMILY=dm2 -DSAND_ROLE_VARIANT=g15-dm2 "-DG13_GLSLANG=$glslang" "-DVULKAN_HEADERS=$vk"
cmake --build "$l/t933-g15-release" --config Release --target pc_sand_dm2_host
python pc/vulkan/t933_g15_qualify.py "$l/t933-g15-release/pc_sand_dm2_host.exe" "$l/t924-dm2-baseline/sand_assets" "$l/t909-g15-dune-release/sand_assets" G15_OP23_RUN --run
```

正式 G15 包保持原身份。待实际确认 opcode 23 可返回，再做正反砂丘、typed Q、守恒和同根图形。

## OFF 实采样私有重放

参考固定为 T927 OFF1 action 7 以及 action 6/8 的真实相邻 callback/receipt。封存原日志摘要、频率、producer、原 32 字 SOURCE、SOURCE/Q completion。原相邻采样间隔 15,026,700 ns，实际接纳 126,053 Q20。历史原件只有日志中的前态承诺，重放进程先用相同非紧凑模块和新私有身份，在 GPU 初始化空 BASE，真实 fence 后回读校验全字。当前运行的 QPC、授权、预算、提交、等待和退役独立记账。

`--m1-private-off REF OUT OFF|ON` 只复用参考的坐标、物料与物理请求字，用当前真实脉冲取 SOURCE grant，再取一次实际 QUANTUM。原时戳留在参考文件，当前命令使用当前时钟。输出 GPU 空 BASE、SOURCE 命令及回执、Q 的 BASE/候选/独立 spare、来源账、两类事件和同根 publication。ON 在相同物理根结束后额外走原冷观察归档。结果 JSON 保 `acceptedRoot:null`，无窗口发布。

```powershell
python pc/vulkan/t933_off_reference.py test/reports/t927/runs/off-pair-t926-full/off-1.jsonl "$l/t933-off-reference.bin"
$full = "$l/t830-m1-20261123"
& $exe --m1-window "$full/full_source.spv" "$full/quantum_candidate.spv" "$full/quantum_commit.spv" --m1-private-off "$l/t933-off-reference.bin" off-1-private OFF
& $exe --m1-window "$full/full_source.spv" "$full/quantum_candidate.spv" "$full/quantum_commit.spv" --m1-private-off "$l/t933-off-reference.bin" off-2-private OFF
& $exe --m1-window "$full/full_source.spv" "$full/quantum_candidate.spv" "$full/quantum_commit.spv" --m1-private-off "$l/t933-off-reference.bin" off-observe-private ON
```

先比 OFF/OFF，再比 OFF/ON。按字典保留原时钟、授权、身份与业务摘要，逐字核实际物理输入/输出及事件。私有对照不取得 T865 正式四臂资格。T496、T504 继续按原前置。
