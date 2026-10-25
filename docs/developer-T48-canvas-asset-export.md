# T48 Canvas真实材质字节导出

不使用真机，不启用完整P1材质。P012WaterSandRenderer新增opt-in prepareMaterialAssetExport(pxPerMaterial)，复用原buildGrainBitmapsFor/buildStratBitmapsFor；三处既有生成循环仅在putImageData前捕获同一img.data副本。没有第二套tile/noise/seed/量化公式。默认不保留额外六图副本，prepare时才打开捕获；开始重烘先清空对应三层，失败不能用历史图填缺。

exportMaterialAssets(P013PreparedMetadata)返回既有P013PreparedBundle：H/L/M固定顺序、grain786432B/strata196608B、alpha255与七版本检验。声明actual generator version及grain/strata六相固定seed、实际grainSigma/grainFeature/strength/名义lambda/像素比例的JSON参数，实际lambdaCanvas=256/ceil(256/(nominal*scale))、lambdaMaterial=lambdaCanvas/scale；paletteHash、grainHash、strataHash均为实际字节SHA256，FNV仅沿T24明示内容指纹。非签收低端palette拒绝，不自动改配方。generation/config/geometry由调用方明确提供，本卡不实现产品双代激活。

P013CanvasAssetExport内纯SHA256与Node crypto对多种padding/完整grain大小逐值一致。四项真实renderer测试用mock Canvas收集putImageData参数，导出六图与之逐字节相等、含真实非恒色纹理、确定性重复输出、缺图/版本/alpha拒绝。可复现命令：

    node test/run_t48_canvas_asset_units.cjs --export-dir .codex/local-test/t48-real-assets
    python tools/p013_prepare_material_assets.py --root .codex/local-test/t48-real-assets --metadata .codex/local-test/t48-real-assets/metadata.json --output .codex/local-test/t48-packed

实际链输出SHA256：grain cb8aa193b4c859c6f790f9703b6411fd59437ed23e91b6f0e9c32aed5f484fec；strata 97bec08be2be9391e09ef132ee35f949cfeebf84186d875603bedc503df22b8a。样本为实际算法在Node mock平台执行，不冒称设备Canvas输出。现有loader Python3/3、材料准备11/11、render82/82与ArkTS/native构建通过。

本轮真正引用T24模块后发现constructor参数属性不受ArkTS支持；已改显式字段和赋值，接口与数学均不变。以前未被产品引用的模块不能由app编译通过推定已被逐文件检查，本轮将其真实纳入编译。报告test/reports/t48-developer-selfcheck.json。
