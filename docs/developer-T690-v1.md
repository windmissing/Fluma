# T690 G13 精度查询与 compute 资格

CAP_COMPUTE 在当前 context 查询六项资源限制。合法的 VERTEX/HIGH_FLOAT 查询保留 23 位预筛门，并分别报告资源查询和声明查询的实际 GL 错误。查询出错时声明精度和范围为 null。

同一个已链接 compute 程序在冷初始化执行两次独立的 highp 探针。证据包括 1 ULP 加法、半 ULP 偶数舍入、次正规输入保留及下溢生成、有限值/Inf/NaN 分类。两组结果分别写入冷尾临时区域，复用初始化 fence 和 1024B 读回。原冷场审核、首次错误和逐根审核继续生效。只有探针封印、预期原始位型、重放一致与 raw GL 门均满足才进入 ready。CAP_MATH、CAP_SCENE 与真实呈现仍由各自证据确定。

T672 真实页面生成树完成 native、ArkTS 和 unsigned HAP 构建，45.400 秒，零错误。静态 GLSL 编译通过，包内 ELF 的 shader 原串与实际生成源一致。产物见 `test/reports/t690/compile.json`。真机 compute 证据由 T654 继续采集。
