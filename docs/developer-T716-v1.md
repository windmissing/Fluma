# T716 RX readonly 值实参

T713 原布局的内存 BuildId 和文件 SHA 与 T709 签包中的 RX ELF 一致。驱动原文两次报 0:1987 S0001 readonly。该行是 g9CompareStream 将 B[at+2u] 直接传入 g9EventMask 的 uint 值形参。按规范源的实际用户函数签名扫描，B 的同类边界仅此一处。

生成器在 T709 coherent 变换之后，以只读取并返回 uint 的 rxReadBaseWord 取值，保留原索引、读取时点、readonly、coherent、barrier 及 ABI。逆向移除这个访问器和单处替换后与 T709 规范源逐字节相同。包内源、reader 映射和实现身份同步。

实际 glslang、原生、ArkTS、unsigned HAP 编译通过。包内 RX ELF 个体及 GLSL 字节已核对。原件在 .codex/local-test/t716 与 .codex/local-test/t716-evidence，回执在 test/reports/t716/compile.json。交 T713 复测驱动编译、Create、READY 和首个有效 Swap。
