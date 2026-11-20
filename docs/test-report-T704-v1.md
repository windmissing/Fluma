# T704 G13 精度修复离线交接

使用 T690 的原 unsigned 构建，SHA256 b764c268c12769cabdf3c55dad340fc7d5f58cd4464eb19dcea7adbea74d292e。开发全量编译 45.400s，独立重跑生产 GLSL 静态编译退出 0。包内 native BuildId 0ce39297324fe8f1a2a6a14e3090ea6f2028503e；ELF、ABC、实际 shader 字节与生产收据一致。签名 verifyApp 通过，路径、SHA、源及构建闭包见 test/reports/t704/handoff.json。

交 T654 effector。在实际 G13 XComponent 页就绪后，使用当前包一次 Configure，保状态/票据、同步返回、实际加载 ELF 身份及前后锚。沿现有 T672 collector 导出整个目录，包括 status-before、configure、三个轮次以内的 status/index/错误、前锚/四原页/后锚。status 总量≤4，原记录≤16KiB，目录≤1MiB。失败保持原同页窗口，避免再次 Configure 或切页补证。

用 python test/t704/check.py <目录> <轮次>。校验解码后每页8192B、原 JSON/原页 SHA、宽 ID、锚和 stage。相同锚只支持 ANCHORS_EQUAL_NOT_ATOMIC。CAP_COMPUTE 的资源及 VERTEX_HIGH_FLOAT_DECLARATION 分列原 GL 返回，顶点声明仅用于预筛。CAP_FP32 必须有真实同 context/owner 的 CAPABILITY、INIT_FP32、INIT_FP32_RESULT 原件、16个实际 word、两次一致、原 GL 无错；陷入错误时保 NOT_MEASURED 或 FAILED。CAP_MATH/CAP_SCENE 及可信流场由各自证据确定。

离线 11 项 capability 正反控及 T672 四页 decoder 自检通过。没有执行设备或 compute；原错误29历史票据保持单独归档，不用新包补写原同步 Configure 返回。
