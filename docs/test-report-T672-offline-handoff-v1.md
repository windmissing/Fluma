# T672 离线设施与交接报告

2026-11-19；tester；不使用真机。构建/离线设施交付，不表示设备、物理或性能 PASS。开发编译基线 8fe7c2e65565350b2013dd466bb64dce189b2351；本卡新增工作区按随附源/对象 SHA 收据绑定，提交时一并入库。

完成真实 GpuResidentG13 / 原 G13ResearchAbility 同页 collector，保 XComponent/owner，不新增产品线程或无Surface probe。test/t672/prepare.cjs 只叠加隔离测试工程。配置前arm并保存完整初始status，一次Configure保存完整原回复；禁再次Configure/collector激活或pulse，诊断按钮最多3轮，status累计≤4。每轮控制锚前+0..3四页+控制锚后均按原调用保存；原JSON/base64不slice。单记录16KiB、目录含index≤1MiB，短写/重复文件/异常显式INCOMPLETE。

DevEco真实native+ArkTS BUILD SUCCESSFUL，最终32.008s（33任务）；test/reports/t672/app-build.txt。T672签名verifyApp PASS；源/对象/ABC/ELF/BuildId见 handoff.json 与 test/reports/t257/T672.json。实际解码/API行为未在真机运行。

离线 analyze.py --selftest：宽U64 ticket、四完整页、控制锚变化、坏base64均达到预期。analyze.py 原目录 [round] 严格解码8192B/页，输出JSONSHA/decodedSHA、原控制、decode/log段与逐Stage身份/API_RETURN/rawGL/EGL/ctrl255/ctrl7（-1保UNKNOWN）；同sampleSeq绝不冒原子版本。锚不一致返回INCONSISTENT_SNAPSHOT；相等只称ANCHORS_EQUAL_NOT_ATOMIC，productPASS始终false。

交接T654 effector：安装handoff所指T672签包，进入实际G13研究页，等XComponent形成；点一次“配置REST”（collector已在该调用前arm），失败窗口点“完整初始化诊断”，不要Stop/切页/激活/再次配置。读取filesDir/t672-<timestamp>整个目录，保每条原JSON，随后 python test/t672/analyze.py <dir> [1..3]。最多3轮且status4次限制由代码执行；API未返回/错ticket/缺原件按INCOMPLETE回architect，不拼旧进程110/209，不从status.attemptId选ticket。页面显示仅摘要，原始文件完整。无设备资格/数学/场景/物理PASS。
