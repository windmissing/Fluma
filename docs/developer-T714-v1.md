# T714 四臂首帧与配置停点

核对了 T711 八臂原 unsigned HAP、签名回执及签包。签前后的原包条目逐字节一致，包内 BuildId 对齐 T701。历史设备样本缺同进程 loaded ELF 回执，包内身份和进程身份分别登记。[原件核对](../test/reports/t714/attribution.json)

## M1

T711 的 THREE_OFF 和 TWO_ON 显示 CLOCK_V1、STATE_LOST、run 0:0、物理提交 0:0。T701 M1 四臂的实际 app_bridge 编译命令缺少 `GRS_PROGRESS_V2_EXPERIMENT`。T593 的 T597-fixed 使用 CLOCK_V2 并观察到已发布 run 与物理提交，因此只能作为独立参考。T711 原件缺 EGL、传输创建及 READY 请求的完整返回，无法将未发布 run 归到 250 ms root 期限。

交 T711 使用原 THREE_OFF 与新 THREE_OFF 冷启对照。新包固定现有 CLOCK_V2 和同一套原生期限，进入原 M1 XComponent 页，保 T701 原 shader、资源和 cache OFF。用“导出首帧原件”保留当前 arm、原 status、owner 初始化进入/原返回与候选 run。每包同设备同档位只启一次，记录包及 loaded ELF、首次非零 run、READY 或实际拒因，再核首个内容帧。该包没有移植 T597 的整套恢复设施。[M1 构建回执](../test/reports/t714/compile-m1.json)

## M2

T711 THREE_OFF 原生回执为 boot 1、epoch 3、object 0。API 6 的 `glLinkProgram` ENTER 无 EXIT，开放 65.411456875 秒，返回与 GL 错误均未观测，日志尚未查询。SOURCE 共 85,839 字节和 1,698 行，SHA-256 为 `b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1`，与 T701 包内源和 T591 SOURCE 相同。T590 的开放调用在 object 2。第 1728 行超出该 SOURCE，更无证据将此开放调用归到 RX/DM 的编译报错。

TWO_ON 的错误页面留作未开测记录。T701 的实际 M2 工件默认进入 Index，已有真实 M2V3 按钮。旧设备布局显示 SandCanvas，缺 loaded ELF，无法确定当时装载的入口版本。新 TWO_ON 直接进入原 M2 V3 XComponent 页，保 T663 原生游标、原 shader、资源和 cache ON。交 T711 冷启一次，保 T714_CONTROL、loaded ELF、SOURCE 进入/退出和三个日志。开放调用保采样截尾，保留原程序与阈值。[M2 构建回执](../test/reports/t714/compile-m2.json)

## G13

T654 v3 复用的是旧 T672 四页。T672 与 T704 的包内 ABC 相同，native BuildId 分别为 `693660849353acc6c3e1f678df902de944f952b9` 和 `0ce39297324fe8f1a2a6a14e3090ea6f2028503e`，页面外观无法鉴别。旧 `status-before.json` 是 UNCONFIGURED。随后 `r1-status.json` 对票据 216 明确记录 owner `g13Configure REJECTED error=29`、`CAPABILITY:EXIT`、`rawGL=1280`、`floatPrecision=0`。这与 T686 修复的旧 `GL_COMPUTE_SHADER` 精度查询一致。目录缺同步 `configure.json`，亦无 T704 loaded ELF 或 T704 配置结果，CAP_FP32 保持未测。

交 T654 直接复用 T704 签包 `6cb12f2e…`，同进程核 loaded BuildId 及 ELF，原 G13 页单次 Configure 后取同步回包、ticket、owner 状态和完整同页目录。同步准入与 owner 结果分别判读，旧票据 216 只作历史对照。T704 已带原 collector，本卡沿用现成产物。

## 开发产物

生成命令为 `node tools/t701/prepare.cjs M1 .codex/local-test/t714/M1 --first-frame-control` 和对应的 M2 命令。两包的原生、ArkTS、unsigned HAP 构建均退出 0。实际包内入口字节码、ELF BuildId、SOURCE/FINAL GLSL、资源尺寸均已核对。开发未签包、安装、运行产品或使用真机。
