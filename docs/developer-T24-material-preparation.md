# T24 P1 离线准备交接

2026-10-25；PREPARED_NOT_DEVICE_VALIDATED；不使用真机，productEnabled=false，G1仍关闭。依据P1 rev2§4/§5、T23裁决§4，复用P0显示profile适配与T09参考，不重复T20/T34/profile probe。

## 准备产物

- render/preparation/P013MaterialPreparation.ets：七版本身份/六图metadata、精确786432+196608字节/alpha255拒绝、拥有独立纹理字节、确定性内容指纹、正mod texel坐标；Rmat增量→统一Lmat→一次ToneMap的CPU参考。指纹为FNV内容校验，不能冒充SHA256；metadata的生产hash由文件加载脚手架核验。
- P013MaterialDataPreparation.ets：隔离layout2 1536float装配；row0复用DISPLAY_4X3_V1，坡度从Float64四趟显示profile以固定37tap σ6/R18卷积、中心差分后量化Float32；fH来自同snapshot，warp/spark由调用方提供已有生成器同源表，不重造seed/噪声。保留通道为0，非法数据拒绝并保留上一发布buffer。
- p1_material_functions.glsl：独立准备函数库，Q/EOTF/最后负裁/共同Lmat/ToneMap；未加入CMake、native或页面调用，未宣称GPU编译/性能通过。
- tools/p013_prepare_material_assets.py：明确映射grainH/L/M与strataH/L/M原始RGBA文件，固定顺序拼接、精确尺寸/alpha/版本与SHA256检查，缺图/损坏/越界路径拒绝；全部验证完才写prepared输出。用途是离线真实导出字节的装配，不伪造生产资产。

## 验证与缺口

Node11/11：固定schema/缺图/版本/alpha、Q表/C3、neutral与OFF、共同受光顺序、texel边界、layout2及flat/ramp/rawImpulse已发布pb32逐值一致。Python资产3/3；既有T09 CPU executor58/58 MATCH。ArkTS与native编译通过，报告test/reports/t24-developer-selfcheck.json。

现有Canvas生成器仍在P012WaterSandRenderer，grainTile/stratNoise/warp与putImageData前字节未抽出公共导出接口。后续G3必须导出同一生成结果，不能复制本卡的测试恒色图当真实资产。metadata归调用方配置，装配对象持有引用；后续产品原子配置必须实现只读配置快照/活待双代，不得把本准备对象当已实现的生产生命周期。

待T42有效draw/swap/透明/sRGB证据与designer裁决后另派G2/G3：完整stage/pump/activate API、GPU纹理上传/六图消费、同FrameToken激活、Canvas fallback及GPU/CPU逐fixture对照。测试样本为明确合成数据；不存在产品启用、材质标定或G1通过声明。本卡不解锁产品启用，仅交接可复用准备模块。
