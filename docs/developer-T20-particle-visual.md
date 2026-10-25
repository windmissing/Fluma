# T20 开发交付（2026-10-25）

实现Canvas首选路径：P012ParticleLayer只读已提交M1位置/相别/质量/状态/ID，不另积分、不创造age、不回写物理。物理copyParticleRenderSnapshot输出定容复用数组，physicsCommitId/particleRevision在初始化、准入、成功子步/倒立发布时推进；与copyStateTo床层身份一致，异号跳过整层。共享结构在common中，physics不import render。

页面按M2真实active快照切换；C active无论count=0或视觉ablation关闭均不feed/draw B tracer，切入清B视觉点；inactive保留B。真实粒子在材质后、高光/雾/容器边框前绘制；FLYING使用当帧水/沙/雾采样，CONTACT_WAIT不以飞行龄期淡出。沿用已有2px/相色候选，不作为定标。真实LAND全额后不再有sprite，同提交床层仍显示质量累积。C active时即使外观tier=OFF也使用基础E层床面及粒子；canonical旧臂保持原强制路径。粒子层显隐不改物理route。没有增加页面猜测route/want武装物理的入口，仍由M2既有显式接口武装，默认关闭；GLES迁移等待T37契约。

验证：test/run_p012_particle_visual_units.cjs 8/8，含真实投影复用/防回写、位置/WAIT、重复帧无积分、失配/ablation、逐帧遮蔽、OFF、容量反例、平床真实LAND与床层同commit；M2既有27/27通过。hvigor default@CompileArkTS成功，零错误。日志.codex/local-test/t20-build.txt。真实粒子帧时/观感与容量定标交T30/T31，未占设备，未删除B类（仅active分支退出）。当前SandCanvas既有P013桥页面接线原先未入库，本次作为同页集成基线带入；hello/其他任务暂存不带入。
