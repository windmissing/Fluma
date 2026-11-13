# T486 · GPU 场景缓存实际渲染独立差分（v1：离线独立重算 9/9 全 PASS，不使用真机）

2026-11-13，tester，离线不使用真机，被测对象=T485 实际交付（commit `ee3c4be`，M1/M2 双 handoff 签包 SHA `c76c39f8…`/`70a21bda…` 与 `test/reports/t257/T485-*-handoff.json` 逐位一致，verifyApp=PASS）。本卡全部结论由**独立脚本重算**得出（`test/t486/independent_pixel_diff.py`），不复用 T485 开发断言；原始字节权威来源为 `pixel-originals.zip`（288 份，SHA 与六路运行目录逐份核对一致，Z1 PASS，防复写）。

## 独立差分结果（9/9 PASS）

| 判据 | 结果 | 实测 |
|---|---|---|
| Z1 原件完整性 | PASS | zip 288 份 ↔ 六路目录逐份字节一致 |
| E1 禁缓存基准 | PASS | off 路 4 场景×8 层×2 route=64 层全部尺寸=宽×高×4 有效 |
| E2 三路像素一致 | PASS | off/cold/warm 同 route 同场景同层 128 对比较零差异（缓存路径不改变渲染输出） |
| E3 M1/M2 跨 route 一致 | PASS | 96 对比较零差异（共享三 graphics binary key 的结果面） |
| E4 业务 token 路内稳定 | PASS | 8 组场景 token 三 mode 逐位相等（token 与缓存路径无关） |
| E5 反射绑定 | PASS | SCENE_FULL.publicationBinding=0；SCENE_PARTICLES aState location=3/type=5125（或 inactive 合法）；三 role color location=0/type=35666(vec4)/size=1 |
| E6 编译路径 | PASS | warm=BINARY_LOAD compile=0；cold=SOURCE_COMPILE×2；off=CACHE_DISABLED_SOURCE×2（禁缓存仍编译，不读盘） |
| E7 失效回退 | PASS | bad_hash/truncated 新进程 REJECT_THEN_SOURCE_ONCE 两 stage 源码回退，回退后像素与冷基准逐位一致 |
| E8 共享 key | PASS | M1/M2 缓存目录三 key 集合相等且与 pixels.json 声明一致 |

## 边界

- 像素来自 T485 冻结 publication 的 host fixture（真实 lease+GL buffer+实际 Scene GLES 绘制）；compute 准入正确性与真机端到端不在本卡范围（T491 承接真机）。
- 期望全部由本脚本独立定义后与原件对照；判据修正过程（E6 初版误用 PREPARE_OK 判冷路，实际同 key 收据文件被后续进程追加，改用 REFLECT_OK 判冷路并补禁缓存路断言）如实记录于脚本注释，未放宽任何判定。
- 报告 JSON：`test/reports/t486/t486-independent-pixel-diff.json`（含每项实测明细与来源清单）。
- 未宣称：设备命中、当前上屏、冷启动收益（T491 真机项）。
