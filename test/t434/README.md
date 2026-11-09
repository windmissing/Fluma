# T434 · developer 交付与复跑

不使用真机。消费 T433 §7；基础 native/page 固定为 T421 commit `0228a7d`，只覆盖本卡四个 transport/candidate 文件。`frozen-source.json` 保存基础逐文件 SHA。T410 已装包、公共 app bridge、页面、CMake 未修改。新生成目录必须为空。

```powershell
cmd /c test\t434\build_host.cmd
node test/t434/run_host.cjs
node test/t434/generate.cjs .codex/local-test/t434-reproduce
# DevEco JBR/node 加入 PATH 后，在新目录执行
hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon
# 授权本地签名，不操作设备；label 必须唯一
python -X utf8 test/t257/sign.py T434-reproduce .codex/local-test/t434-reproduce
```

实际 GL：ANGLE SwiftShader GLES3.1；四档 K192/512 × Kw64/128，每档完整/前缀同源差分。192/64 跑14类现有故障注入，其余档保留 SOURCE/QUANTUM、LAND/ARRIVE、连续量子、拒量、末ID、持lease、迟尾撤权和 completion 身份变异。每次正常接受冷抓 state/event64/completion256，二进制逐字节比较。独立 tester T435/T436 与真机收益 T437 仍分别报告。

额外实际 GL staging 检查 n=0/1/4095/4096/4097、错run回退；向 WORK 后缀写非零毒值，n<4096 时断言未被覆盖。这些大 n 是**仅搬运 fixture**，没有冒充4096次真实 SOURCE admission。真实满账/非法origin精确首错由独立差分继续覆盖。

## 路由与计数

默认完整控制路；页面“光学/诊断”冷选“下轮来源账：前缀/完整”，然后点“新空场”。`setLedgerPrefix` 仅更新下一 run 配置；当前 run 使用 Create 的不可变选项。V2 同样只冷选下一 run。本独立生成 profile 的 `T434_EXPERIMENTAL_V2` option 默认 ON；公共 CMake 未启用。

`copyAccounting()` / “导出复制计数”写 `T434_COPY_ACCOUNTING_V1`；不扩展/混用旧 timeline schema。每行完整 run/owner/context/surface/geometry/config、hash words、nonce/control/frame/kind，保留最多128行及 dropped。数值均是 issued GL API payload，不是驱动流量/耗时：

- `ledgerCopyBytes`：完整262144；有效前缀64*n；0条跳过copy但保原barrier；资格不明 route=2 走完整。
- `eventPrefixCopyBytes`：原有 count*256 条件复制；没有宣称本卡新优化这部分。
- `stateCopyBytes`：**GL_COPY_BUFFER_ONLY_SHADER_STORES_EXCLUDED**，当前为0。state/candidate/spare/authority/publication仍由原 shader 搬运，不是总状态流量为0，也不是每root没有审计副本。
- `completionMappedBytes`：本root实际成功map的1024B，SOURCE与完整QUANTUM均计；未map拒绝0。不会把standalone64B Poll充当完整接受。
- `coldMappedBytes`：显式Freeze/Capture对应最后root的bank+有效events读回；初始化/JIT准备不归到业务root。正常root不新增pool下载。

## 来源读取审计

所有 shader/pass/首错保持原字节：`quantum_stages.glsl::inventory`先核 header n≤4096，slot origin 非零/高位0/≤n后读16word；`quantum_core.comp::emit`先核origin≤B.header34；`quantum_replay.glsl::quantumAudit`先核来源/去重界再循环 i<n；`replayQuantum`逻辑&&短路先核origin>0且≤B.header34再读取。SOURCE完整审计未改。无CPU后缀清零、增量跨root缓存或新早拒绝。未知certificate沿原完整copy和GPU审核失败切点。

资格仅由同GL owner接受后的native Header构造，核magic/字段版本/features、六个identity、三套hash、接受bank及n≤4096。Create初始已核空bank的n=0也可使用；未接受candidate不会提供n。SOURCE拒绝不改变已接受账；连续QUANTUM每次重复制；新run创建独立WORK/选项，Stop/epoch/lease/回放和原barrier/fence/flush不改。

所有前景背景仅XComponent+shader；不增加Canvas/CPU物理回退。字节减少不等于fps、GPU耗时或设备功能PASS。最终编译、签名SHA及本轮GL结果见 `test/reports/t434/`、`test/reports/t257/T434-release.json`。
