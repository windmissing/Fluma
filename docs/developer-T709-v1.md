# T709 RX coherent 值实参

T707 已加载源码 SHA 5d9cd082…，第 1097 行在 g9DepositMap 将 C[at+3u] 直接传入 g9MapAdd 的 q 值形参。按当前 GLSL 定义核对同类边界，共 43 处，涉及 g9MapAdd、g9ProvisionalAdd、g9U32、g9ColumnPotential、dmPut、g9PutColumn。每处改经同类型的只读访问器取值，读仍在原实参求值点，短路、索引和访问次数保持。保留原 coherent 声明、屏障、写回和 T699 两个局部 prior。类型构造器、内建函数和 lvalue 路径不属于这批有声明的值形参。

生成器逐处记录原行、新行、实参序号及前后文本，并拒绝清单漂移。实际 native、ArkTS 和 unsigned HAP 构建及 glslang 静态编译通过。包内 RX GLSL 与真实生成源码字节一致，原件在 .codex/local-test/t709 与 .codex/local-test/t709-evidence，交 T645 复核 Maleoon 驱动实际结果。
