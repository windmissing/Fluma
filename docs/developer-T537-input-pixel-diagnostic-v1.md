# T537 显式诊断取证交接

本目录生成/编译工程，不自动运行测试或设备命令。M1/M2：

```text
node test/t537/generate.cjs .codex/local-test/<fresh-name> m1|m2
```

在实际页面进入显式“诊断三色图”：先停源、退休旧root/lease并切新scope，仍使用同一XComponent、scene FBO及最终present shader。它不是物理失败回退，不增加accepted/source/physics commit。诊断时隐藏物理控制、暂停传感输入，返回新空场才重新开启物理研究。原物理允许谓词/源事务不改。

页面和native都标 `DIAGNOSTIC_NOT_PHYSICS`。Native独立pulse保双时域33,333,333ns GAP，Stop/window/epoch/背景撤权仍有效。单次诊断帧持有2s，不续期、不自动重复Swap；到期导出为HOLD_EXPIRED。几何回调撤销旧hold。点“导出诊断帧”保留 `T537_PIXEL_CAPTURE_ENVELOPE_V1` 原件及真实文件hash/字节数。

## 三域同帧证据

1. `pixel` 原件含token/epoch/native created/holdUntil/sample与Swap起止ns、实际EGL尺寸、三个bottom-left像素ROI、FBO和默认目标送显前RGBA8、GL/EGL原错、program/FBO/source texture/sampler/viewport/scissor/blend。
2. `geometryRaw` 是同步读取的实际 render/touch ComponentInfo（含local/window/screen/矩阵）及layout serial，不用截图反推映射；`scope` 绑定进程/route/source/控制身份。必须核两节点原框与native实际尺寸同代、独立屏幕位置；缺项/变代/不一致不得比较像素。
3. tester在**同一hold**使用已有 `test/t406/capture.py` 包裹其无损系统PNG命令，保存原调用、exitCode/stdout/stderr、完整PNG SHA和PYTHON_MONOTONIC_NS起止：

```text
python test/t406/capture.py <png-path> <capture-call-receipt.json> <tester-screenshot-command...>
```

以上只是命令接口，不会由生成器执行。Python monotonic和设备STEADY_NS不是同一域，必须消费既有独立时钟绑定及其误差区间；PNG调用区间在该误差下完全落在native hold内才具配对资格。没有绑定时明确NOT_MEASURED，不能用Date.now、文件mtime或两个裸ns相减补造。

系统ROI由已测screen origin、实际native尺寸和记录的GL bottom-left ROI转换，禁止逐点平移拟合/换邻点/取整消除0.2px。保系统PNG原件，逐域列事实：FBO错→scene/资源；FBO对默认目标错→present；二者对且真实Swap成功、同hold系统错→才缩到Surface/layer/窗口合成。诊断图通过亦不表示真实沙水呈现、SOURCE消费或性能通过。

## 输入阶段证据

正常物理模式从同框独立透明touch节点收事件，render节点禁止抢ArkUI命中；面板固定300vp滚动区，不随展开挤掉render。页面事件入口先记type/changedTouches数、原timestamp、local/window/screen坐标、node/bounds/layout；空changedTouches也记。控制器记实际ready/revision/permission/epoch/word合法性、sourceOpen/边界和仅作显示的state。native独立保存action API到达及128B LE命令/回执。

`executionDiagnosticsV1` 在原总2MiB writer中追加 `inputStages:T537_INPUT_STAGE_V1`，与原1,404,072B Capture独立；32×512B槽，额外定容计数/来源header。每scope首Down/Move/Up/Cancel/谓词/回执/Offer受保护，额外记录满则dropped增加；新epoch/run/session退休旧输入槽，不让旧Move耗尽新scope的首条记录。旧Capture的首因/首root不受影响。仅一份锁内一致copy，超UTF8信用走原完整INCOMPLETE/fallback，不抬2MiB/4MiB限额。

没有运行上述取证，不以编译、日志计数或UI声明给产品PASS。
