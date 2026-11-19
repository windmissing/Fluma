# T663：M2 创建真实单 API 游标

2026-11-19，developer。开发与编译完成；不使用真机、不执行测试/GL/产品，不签名部署，不声明创建/物理 PASS。

`m2/create_diagnostics.h` 在现有 Attempt 尾部追加精确128B ApiCursorV1，保持旧成员偏移；三槽显式 payload 和 Store 实际增量均384B。Store 原67200B，现67584B，mutex40B，Attempt22512B。未触发外部三槽 fallback，未把 Store 冒成64KiB。

Begin 在原 mutex 下清零新槽并绑定真实 tid/epoch/window/context/bootSerial；未进入 edge0，未知 returned/rawGl=-1。单 API ENTER/EXIT checked 推进 U64 sequence、真实单调时钟和实际对象 handle，保留32B零。开放调用不能被嵌套覆盖；序号超限/边界不符标 diagnostic incomplete，原 GL 仍照原流程执行；sealed 后原 Change 门拒绝更新。

真实 Compile/Link、GetShader/Program-status、原有 GetError、ReadLog 原有 length/log 查询均在原位置前后记 cursor。没有新增或重排 GL/error/log API、额外等待/线程/热文件；observer/current/events/Finish、pending error 与返回码保原行为。cursor 是最后一个 API 的位置，不把旧 LINK 大阶段拆成假成功。已有 m2CreateDiagnostics 追加可选 apiCursor，保主 revision/参数，U64 十进制 string；m2CreateLog 接口不改。

`test/t468/generate.cjs` 将真实 header 和 ABI TU 写入真正 m2route/App 生成链，CMake target_sources 把 ABI 链进 libgrsm1；manifest 记录新 header SHA/128B/384B。`tools/m2/prepare_cursor_build.cjs` 从既有 T468/T452/T438 冻结生成链创建全新实际 M2 产品工程，不是只编未链接副本；`build_cursor_receipt.py` 只读取生成源/包/ELF，核实际ABI和源SHA，不运行产品。

复现：`node tools/m2/prepare_cursor_build.cjs .codex/local-test/t663-new`，在输出目录执行 `hvigorw.bat --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。实际目录 `.codex/local-test/t663-delivery` 全 native/ArkTS/unsigned **47.104s**，最终布局修订后增量 **11.995s**，零错误，有既有SDK警告。编译静态核原/新 Store 差384B；包内真实ABI为 `[1,128,3,384,22512,67584,40,22384,32,96]`。

| 实际原件 | 身份 |
|---|---|
| unsigned HAP | `.codex/local-test/t663-delivery/entry/build/default/outputs/default/entry-default-unsigned.hap` |
| HAP SHA | `d2a522e2733d4e5083452d9e74f3da6c3fdab11b6b71248a2865f5f02eb57828` |
| 包内 libgrsm1.so SHA / BuildId | `7a0bcbaab37c3fabcff3a5874164bbea1767e2877db903ad867477fdb781253f` / `c23e231c781476010af91f631c60d2239371205c` |
| ABC SHA | `a64b403ad3ba061e8540ea24fa7fd6dcb2d3b5be9a52d68f1538ee5924a4948c` |
| 正式字典 / 实际源与日志 | `test/reports/t663/formal-contract.json`、`generated-sources.json`、`compile-receipt.json`、两份原始 build.log |

包指纹不是实际 loaded-module 证明。T591 tester 后续用新 artifact 补同 attempt cursor/current/events/terminal 和真正 loaded ELF；旧40s LINK样本不能据此反推某单 API 未返回。开发不替tester运行或把设备结果加作关闭前置。未改任何物理、线程/owner、渲染或阈值。
