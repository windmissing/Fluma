# T715 DM coherent 值实参

T712 报告独立 g9_dm_differential 已到 Maleoon 935F 并在 CREATE 返回 8。离线核对 T706 独立 ELF、配套库及 T712 签包内 ELF，规范 GLSL 一致。第 1063 行是 g9MapAdd 将 C[at+3u] 直接作为 g9AddMass 的 uint 值实参。部署回执与 ELF 大小相符，原件未附进程映射或已加载 BuildId。

DM 生成器在 T706 捕获作用域变换之后，按声明和实参位置识别 45 处直接 coherent 值读取。使用仅返回一个 uint 的访问器，保留每次读取的求值位置、索引、短路、coherent、barrier 和写回。生成器拒绝未核对的值边界与 out/inout 形参。生成身份、reader 字典和 cache 输入同时更新，冻结参考保原字节。

glslang 和 hvigorw 原生、ArkTS、unsigned HAP 构建通过。独立 ELF DT_NEEDED 指向 libgrs_dm_t647.so，规范 GLSL 与真实链接库和包内 ELF 均一致。原件目录 .codex/local-test/t715，编译回执 test/reports/t715/compile.json。交 T712 核实际装载、驱动编译及后续物理结果。
