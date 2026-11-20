# T699 RX coherent 实参

RX 生成器在冻结父源与 T665 变换之后，严格定位 g9MapAdd 和 g9ProvisionalAdd 的两个 merge 分支。第 1063 和 1522 行分别把 C 和 W 的质量槽在原位置读入局部 prior，再调用原 checked add。保留 coherent、原屏障和原失败路径。两个变换的源行、前后原件、canonical SHA 及缓存 ABI 纳入身份。

实际 RX native、ArkTS、unsigned HAP 构建 43.482 秒。静态 GLSL 与实际链接 ELF、包内 ELF 的 canonical 源字节核对通过。产物目录 `.codex/local-test/t699`，全量生成及编译证据目录 `.codex/local-test/t699-evidence`。包内 BuildId 为 `624ba44500d160259727e2b25a8422ebe5ee1884`。设备编译与后续证据由 T645 采集。
