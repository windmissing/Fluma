# T372 单沙窗口冷读回交付

探针只在 `test/t372/generate.cjs` 生成的隔离 V2 单沙包中启用。工作区生产入口和单沙 shader 无源码改动。

## T373 取证

1. 启动隔离包，构造要取证的单沙画面。可用「锁定当前样本」暂停物理与触摸准入，后续绘制仍发出新 frame serial 和 token。
2. 选「中性」或「纹理」。中性仅将四个光学 gain 置零，纹理恢复生产 gain。每次切换均正常更新 config digest、revision 和资产 generation，保留生产 palette 校验。
3. 输入预注册 ROI。格式为 `x,y,w,h; x,y,w,h`，使用 EGL 原生像素、左下原点。最多三个矩形，每个最多 64×64。选取床、AIR/CONTACT_WAIT、背景或前景边界时，以当前样本可见支持为准。越界记作 `CAPTURE_FAILED / INVALID_ROI`。
4. 点击「预约一次」，再点「导出读回」。应用 files 目录生成 `.json`、`.sha256` 和可用的 `-candidate.rgba`、`-back.rgba`。两个原始文件分别按 ROI 顺序紧密连接，每个 ROI 行从下到上，manifest 记 stride、偏移和 SHA。保存失败可从同一份冷收据重试。
5. `CAPTURED` 要求两个读点完成、read/pack 状态恢复、swap 与 unbind 成功，并且 CURRENT 的 actual token 等于 requested token。失败样本可留读到的原始 bytes，按 manifest 的失败状态单独分析。包身份用签名回执 SHA 核对，设备身份由执行记录登记。

该包读回实际单沙候选目标及窗口默认 back，保留中性业务色块、非零 gains 和现有半透明前景。独立线性目标与灰阶控制沿 T370 的 host 校准路径对照。窗口截图、系统合成和设备颜色链判定交 T373。单次同步开销在 `captureMs`，性能统计排除捕获及相邻帧。

## 开发自验

- Host GLES pbuffer 检查 ROI 边界、98,304 B 上限、正常绘制零额外读回、预约帧六次读回、候选 sRGB 与默认 GL_BACK 读点、read selector/READ_FRAMEBUFFER/四项 PACK/PBO 恢复、无效目标隔离、同 token 和注入 swap 失败后的 LastGood。
- Node 冷档自测检查真实写盘字节、非零视图偏移、短写、SHA sidecar、失败档、hash 不符及缺失/错 token 的拒绝。平台文件与哈希接口用 Node 适配器执行。
- T364 原 ETS 回归复跑通过。Host 的窗口替身仅为 pbuffer，真实 swap/unbind 及系统呈现由 T373 取证。

Hvigor 隔离包构建、签名与验签均成功。签包位于 .codex/local-test/t257-sign/T372/T372-signed.hap，SHA256 98339521d6eea6ab58fe6799167d6ca7c78ba350af3243665d94b91359066992。
