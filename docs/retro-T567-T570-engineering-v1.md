# T567 / T570 工程交付复盘

2026-11-17，architect，不使用真机，不执行产品/GL测试。当前两卡工程/首次归因范围完成；没有实现G12或宣称READY/HOLD/水耦合PASS。

T567消费T559正式方案/20 fixtures，交独立profile、MaterialConfig128/GPUConfig192、state/WORK/派生几何/水origin/H/J/input/event64 usedMask/完整source16、dry2404、wet6204调度。四档资源与设计公式一致。补齐一次宽RNE的Cface Q16转换、J的Q22 y4精确接触、原touch cap×4而非改raw单位、dry cache明确N/A不加湿域重叠门、wet upward touch只消费一次等工程接缝。

跨两个pass的P复合kick若只保全程old/new pW，会与其它包重放重叠。本稿分别记录drag/pressure各自水动量和中间速度，F/S也分端点，整pending phase完成后seal，按实际子顺序比较；没有靠增加未计费事件绕过2K+8N+4Kw+32上界。

区分设计FULL_CORE与完整六trial原件FULL_TRIALS，后者每次预约另有6GPU arena及每冷槽6arena，最大已知并存约1.486GB（driver/FBO/host等仍排除）。这个数字是成本，不是设备可分配证明；6204 dispatch与大量31轮扫描也没有实时保证。M0仅基础，M1全部32pass才是最小完整水平湿域；wet倾斜/重叠P/旧G11湿支持不扩域。

T570推翻IME设施不可弹的旧全称推论，核prep-74未确认固定键盘，且serial并非逐frame增长。更关键是prep-40/41在失效前native run真实改变、permit始终null，不能把这次合法hard失效当成blur-only冲突。源码非READY changed仅清稳定候选，capture.IDLE invalidate直接返回；保已有正确门。确认后STALE的叙述缺同attempt首因原件，交窄53248B payload见证与精确tester补证，不先改renderer重建或延长3000ms。

本角色有限检查 `analysis-T567-T570-engineering-checks-v1.py` 通过：四档state/work/arena/cold公式、128/192配置、完整32writer和6204调度、16种新event64 masks、150个静态C8格点、114bit约分界、8份准备原件/本地生成manifest SHA及soft/hard源码分支。只是局部代数/布局/原件审计，没有CPU物理stepper、GPU测试或设备结论。

后续developer完整开发/编译/提交，不执行测试、不使用真机；实际父模块未齐报具名前置。tester/effector独立真机与效果不是开发关闭门，失败首次仍architect，闭包/能量标准反例回designer。只改T567/T570本人行状态与原卡评论，不更新其它队列/滴答同名清单。
