# T270 CW_C_DIAG_V1 逐笔源/库存/层采样

2026-10-29，developer，不使用真机。隔离工程`.codex/local-test/t270-v4`，不改生产C物理/光学、默认页面或M0/M1平台公共源码。

默认关闭诊断；冷启want `cwdiag=true`可开。预分配256×64 Float64行，逐笔select/stop reason/new DOWN/首次与有效tick/clock原raw-unit-domain/window、enqueue ack、batch processed、terminal query及旧key只读query；完整key原数值与runId分别保存，无hash身份替代。terminal保留kind/disposition/status/admitted/rejected/transactionId/kindSuccessSeq/source/physics commit、W全局/触点列前后、I_water/计数/首错。切源时只在诊断开启且有旧请求时query旧key，不重enqueue；新DOWN与两tick原语义保持。定容满保留首批记录、dropped/incomplete明确，热记录不JSON/全profile打印。

`cwdiagFrame=N`选至多N..N+2三帧，slot封存同token header/ledger/optics/profile和context/surface/viewport/mirror、requested/rendered/lastGood token与首失败stage。按列正W数/总W与ledger[11]、slot.verify及采样CPU成本一起保存；不拿新canonical冒旧帧。选定帧由native记录真实packet的W总和、ledgerW与source/physics commit，供slot/native核对。

native只在原Layer成功后，于层2（粒子后）、3（水体后）、4（水面后）采同candidate FBO只读ROI；冷arm最多三帧，每ROI≤16×16，页面默认4×4，位置want `cwdiagX/Y`（物理GL左下像素坐标）。先核actual FBO、PACK buffer/对齐/rowlength/skip及ROI范围，异常记录raw/status，不改原Present/Swap返回或层序。目标格式实际为GL_SRGB8_ALPHA8，导出明确SRGB编码RGBA8，非线性读值不能直接当线性alpha。采样ms单列；真实GL错误归原layer检查优先，诊断自身错误单列，不回写物理或渲染判据。

页面“导出诊断”需提供want `cwdiagSha`为该固定签包64位小写SHA；导出携revision/包SHA、原行词典、完整run/key/token/profile/ROI，冷分段hilog有part序号和END总长度。设备若丢chunk或未出现END，标INCOMPLETE，不宣完整证据。未启用时不自动arm或做readback。原C可由该隔离包独立启动，白名单只ClearWaterDemo，背景前景仍全XComponent+shader；FAULT/STALE合同不变，无Canvas回退。

离线ETS26/0：水正量终态、容量终态拒绝、旧key重复查询零副作用、切源需新DOWN/两tick、诊断开关W/I/commit/feedback等价、成功源后失败step保W前缀、环满显式丢失、slot封存与native回执同token身份。native mock9/0核默认无GL调用、最多3×3层、ROI FBO拒绝和token JSON转义；这不是设备ROI执行。最终native/ArkTS/PackageHap与签包verify-app/profile见附带证据。

交付证据`test/reports/t270/{offline.json,native.txt,manifest.json,delivery.json,v4-package.log.gz}`及`test/reports/t257/T270-v4.json`。T271须在该固定签包新run选水后重新DOWN、两有效tick，沿正终态→W→slot/native→同token层间ROI→真实swap/截图取证。当前未做真机冷启动、ROI或水可见性判定，不把编译或mock通过冒设备ACK/PASS；公式/参数未调。

native/ArkTS/PackageHap BUILD SUCCESSFUL，verify-app/profile PASS。签包路径`D:\hmprojects\HelloWorld\.codex\local-test\t257-sign\T270-v4\T270-v4-signed.hap`，SHA256 `599d03e6fc98278a14a1c006187791d8ebd116070f0c134117c114d6b14d793f`。测试want的cwdiagSha应使用该值。
