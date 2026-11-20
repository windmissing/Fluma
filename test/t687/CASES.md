# T687 G11 与 RX 离线及设备交接用例

离线入口为 verify.py 和 archive_units.py。设备执行负责记录实际 scope、初始配置和拒绝原件。

| 用例 | 同一产物必须记录的证据 |
|---|---|
| W01 | G11 独立页，默认 OFF，R1/DM 和 K/V 组合，静态生成输入闭包 |
| W02 | 每根1024B tail 原件，SOURCE/Q 的 rootKind 与 pass count |
| W03 | R1/DM 类型与输入身份，错 parent 拒绝 |
| W04 | 活动 program/cache 身份与 Expected/Observed 对照 |
| W05 | 双父 proof 同 scope，断言触发不得提交 |
| W06 | sealed writer plan 与四 map 完整顺序 |
| W07 | SOURCE replay BASE、marker 角色2/4和 barrier |
| W08 | 旧 P ghost union/代表选择与32word，对照 blend0/256 |
| W09 | 同代 SceneLease 和真实 fence，忙时拒绝下一 writer |
| W10 | 底材/glyph/真P与ghost/前景完整 candidate |
| W11 | 成功 swap 才发布 mapping，撤权后 stale/FAULT |
| W12 | bootstrap 四段，witness 及 ROOT/FULL 原件连同 manifest 与 SHA |

串行在实际 G11 页先导出 bootstrap 再 seal，按 W02-W11 运行原卡输入，单独导出 ROOT、FULL 和 witness。离线依次传给 python test/t687/read_archive.py MODE MANIFEST RAW。文件完整性与物理/像素另行判定。

RX 用 T687-rx 签包与原诊断 capture scope，保实际 loaded ELF 及 firstFailure。先复核编译到哪一阶段。完整可用冷包用 tools/g9rx/read_cold.py，保 T642/T652 对照。旧 RX 产物和 harness 另存原身份。
