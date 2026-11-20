# T718 DM readonly 1987 生成修复

T591-v5 原件记录 DM CREATE 返回 8，驱动报告 `0:1987: S0001: Function call discards 'readonly' access qualifier`。按 T715 编译与签包回执核对实际 ELF、HAP 和内嵌 GLSL 字节，第 1987 行由 `g9EventMask(B[at+2u])` 传入 readonly SSBO 元素。本轮原件记录设备已刷新对应工件，未保存进程映射 BuildId。

复用 T716 的按声明扫描及值复制变换。在原调用表达式中先由 `dmReadBaseWord(at+2u)` 读取 uint，再将值交给 `g9EventMask`。撤销该处替换与新增 helper 后，实际生成 GLSL 与 T715 逐字节一致，包含 readonly/coherent 声明、所有 barrier 和原有控制流。生成的 reader identity 记录变换及新行号。

Hvigor assembleHap、原生动态库及独立 differential ELF 编译和最终 GLSL 静态编译通过。工件路径、BuildId、SHA256 及旧包关联见 `test/reports/t718/compile.json`。交 T712 签包及真机续测，以实际加载的 library/differential 对应 BuildId 和新的 CREATE 返回与驱动原文核对结果。
