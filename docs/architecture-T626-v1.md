# T626：真实事件时钟资格的可行路线

> 2026-11-19后续：T637独立接收时间位移候选的完整工程合同见 `architecture-T641-v1.md`。该域仅用native receipt作资格/年龄，固定Dref作位移映射；不宣手速/sample age，不改变下述旧REAL_TOUCH路线A的未资格结论。

2026-11-19；architect；不使用真机。结论：现有商业设备仍INPUT_CLOCK_UNVERIFIED；完整厂商源/符号/部署映射没有可用交付物，不能让T616无输入重复取证。架构裁定已完成；外部平台输入未齐是REAL_TOUCH资格的真实前置，不是本卡未完成。

## 当前证据和实际可得范围

消费T589/T599、test-report-T615-platform-clock-v1.md与test/reports/t615/platform-evidence.json。SDK26 common.d.ts的ns since system start声明有实际文件SHA；厂商 MLN-AL00 7.1.0.566 的producer/resampleChain/deployedBinaryMap三项均null。SDK安装目录只能提供公开头/接口合同，不是已部署系统源码/符号包。本地HAP/包内ELF SHA不证明本进程实际加载，也不证明系统事件producer对应源。

报告把T593 nativeNs写约1.667e14与T510混用；本批执行snapshot实际约2.029e14，且uptime是另时刻采集，没有同步配对。量级一致仍不能识别MONOTONIC/BOOTTIME、休眠语义、重采样和单位转换链。经验offset/多点拟合/SDK一句话不可当proof。

本轮查公开华为文档和上游入口，未找到**该精确厂商build**对应源/符号和部署清单；这是本轮未找到，不是“厂商绝不提供”的断言。[OpenHarmony多模输入仓库入口](https://gitee.com/openharmony/multimodalinput_input) 本轮读取返回HTTP405，因此未核其内容，只列后续检索线索；即使取得上游源也不能证明商用build部署映射。没有把不可读资源记为平台证据已交。

## 路线A：保持当前REAL_TOUCH合同，平台角色提供可信bundle

一次性输入清单：精确OS build/设备型号→MMI事件取时producer源commit/文件行和编译宏→原始clock id、单位、epoch/休眠语义→重采样/转换/ArkUI timestamp全链源/符号→各已部署系统模块BuildId/源清单匹配证明→本应用native CLOCK_MONOTONIC实现/转换源身份→实际加载应用ELF对应证明。时间来源不一定是pressedTime，不能预设μs→ns就是全链；必须跟实际timestamp producer。

接手责任：平台/系统输入方取得厂商支持包或系统构建符号/映射；tester只采可读实际模块/进程身份及真实事件冷原件；developer消费可信bundle实现LoadQualifiedPlatformClockProof，不承担索取厂商私有资料。bundle需可追溯授权来源、原件hash/BuildId和明确转换公式，验证绑定scope/generation/OS build/provider SHA；代码里deploymentChainVerified=true本身不构成证据，不能由ArkTS或经验探针自设。证明失配/未知继续UNVERIFIED并禁止REAL_TOUCH资格。

当前没有该bundle或已证可下载地址，因此不安排T616空转。下一次只在平台输入实际到齐或厂商明确提供途径后执行准确资格验证；不反复比较uptime延长等待。

## 路线B：可控系统构建资格

若团队已有可刷写且兼容XComponent/GPU的OpenHarmony设备/构建环境，可选择固定源commit、编译宏、符号/模块BuildId、部署镜像SHA的自建系统，跟踪真实事件链并独立资格；这有可审计资料取得路径，但当前未提供相应设备/镜像，不能宣称已可执行。它产生新platformProfile，不给当前商用7.1.0.566背书。设备/维护成本与平台选择交manager裁决，不擅自新增大系统项目，也不恢复CPU。

## 路线C：修改模型的时间语义须designer设计

若产品不愿承担平台证明输入，交designer明确新profile：采用native receipt monotonic时间而非event sample时间，规定事件排队/合并/重采样/丢弃、速度/能量和dt/age语义与可测误差上限、暂停/恢复/多点处理及输入身份。接收时间只证明native接收时刻，不能冒原采样时刻，也不使旧REAL_TOUCH proof自动PASS。该路线改变算法合同，manager决定是否采用；architect不能自行降低门或沿旧名称启用。

已交研究诊断、SYNTHETIC_TEST及接口/安全/非真实手势研究可继续按原域工作，不以商业clock缺口阻塞所有GPU研发；输出必须标domain，不宣真实触摸动能正确。三条路线没有选择前，本卡交付是明确的外部输入/角色/可行替代裁定，不伪产品PASS。

后续执行prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。具体阻断写未完成、原错/复现/证据/尝试、不能继续原因、所需平台输入/接手角色。设计/开发不使用真机；tester取证另标真机串行，开发仅开发编译。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。
