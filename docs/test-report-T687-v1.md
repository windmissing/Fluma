# T687 G11 / RX 独立离线交付

2026-11-20。两份真实 unsigned 原件的 SHA 与开发回执一致，分别签名和 verifyApp。签包路径、ELF、ABC、编译对象、生成输入及响应参数见 test/reports/t687-g11/ 和 test/reports/t687-rx/，签包原件收据见 test/reports/t257/T687-g11.json 和 T687-rx.json。

G11 核 38 源文件及 22 生成依赖。八份源码 SHA 与实际包内程序完全一致，2 compute 和 6 graphics 均重新静态编译通过。BuildId bbfe2dfc0148687bdbe22f60b7b161823e5c4eaa。交 T600，按 test/t687/CASES.md。read_archive.py MODE MANIFEST RAW 联核实际 worker 原件串流，ROOT/FULL 11/17 分段，bootstrap 四段，witness 有界。14 项合成设施正反控通过，实际 GPU 原件由设备执行产生。

RX 规范源 145458B SHA 4faf48aa…，生成 comp/header/reader/实现相同，包内实际字节同一。二次转换拒绝 RX_CAPTURE_LEGACY_FUNCTION_SHA，旧 frozen SHA 不匹配新身份。交 T645，保 T642/T652 原基准。另 T594 真机旧执行已观察本 ELF 消除 slot 重声明后触发 coherent qualifier 编译错误，参照 f89a7802。该事实交 architect 跟进，新签名不改变 shader；涉及后续物理的设备结论仍需原卡执行。
