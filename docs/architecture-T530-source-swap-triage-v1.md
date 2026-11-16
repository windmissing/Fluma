# T530 源输入、root成本与Swap后黑区首次归因 v1

2026-11-16，architect，P5，不使用真机。读取T510两run报告及全部现存原件、T505/T490/T410/T437/T461和实际T492增强delivery。结论：**root超期确有原件，但不是所有run首根都失败；源“非CURRENT结构封锁”和“已定位合成器故障”均证据不足**。本卡完成首次归因与工程修订/精确取证合同，产品修复和像素判定另执行。指纹及整数时间复算见 `analysis-T530-evidence-v1.json`。

## 1. 分scope重算，纠正报告推论

实际包801c2d63…、ELF32947a02…按签包及包内字节核，增强helper与原卡相符。execution-t510/2/3都来自PID54243，不能将它们当PID31918的run2执行档。execution4-t510.json实际是153B shell `cat ... No such file`，不是JSON诊断；run2只有boot/UI截图，执行根数据不全。run2 boot的attempt4/recovery实际为epoch4/run2/session2/READY，同时boot dropped54、recovery incomplete/dropped45044，不能把“未导出execution”补成该run始终已撤权。first/last/count是跨epoch累计，4603次不等于有4603份可逐帧关联屏幕的完整原件。

| 实际scope | 原件重算结论 |
|---|---|
| PID54243 epoch3/run1/session1，root1 kind2 | started257397081421250，deadline+250ms；13批共127槽，末Flush退出+223.781562ms；timeout note+250.865312ms，FIRST_REVOKE+250.879062ms。首根未接受成立，不能把250ms检测时间称GPU执行时长。 |
| 同PID epoch4/run2/session2，root1 | +160.538594ms接受，+189.185625ms进Swap，+199.538907ms返回，+199.771094ms post-permit current=1/swap=1，原GL/EGL0。**此根在250ms内成功**；随后root2才超期。 |
| 同PID epoch8/run6/session6 | execution2/3累计ROOT_ACCEPT与EGL_SWAP_RETURN均5821，末物理4564；后有独立pulse GAP33,413,750ns，超过33,333,333ns仅80,417ns。不能回写成root1永不运行或均为同一GPU超时。 |

报告首Swap的差值 `260765521189375−260765510836093=10,353,282ns`，即10.353282ms，**不是103.5ms**。code0且rawEgl0及post-permit支持首条真实成功Swap；其它累计次数仍只由summary保first/last/firstError，未逐帧覆盖。旧T448的2455ms不恢复使用。

首失败root13次Flush中最大一批100..110耗86.528281ms，所有Flush和219.136405ms；大量成本在真实GL调用/提交链，不能靠延长业务期限解决，也不能从这些wall时间区分JIT/driver等待/GPU完成时间。未被完整保留的后根不套首根分布。

## 2. 源链归因：现有源码已经把显示标签和输入许可分开

实际 `GpuM1DemoController.allowed()` 检查UI revision、ready、permissionState=READY、epoch=readyEpoch和字段合法性，**不检查s.state==='CURRENT'**；native AllowedLocked同样检查ready/epoch/active/window/permission，不检查显示state。页面的CURRENT判断只用于STALE文本。GPU_WORK_PENDING能发生在资源许可仍READY的合法忙态，不能直接推导不能投沙。

`down()` 无论PAGE_INACTIVE/NOT_READY/OUTSIDE_BOX还是成功，均应调用POINTER_DOWN_READY_COORDINATES诊断及recordSourceAttempt，然后才return；页面touch直接遍历changedTouches调用down，没有CURRENT条件。现存三个execution的actions各32、actionDropped0、packetDropped0、packets0，解码仅SOURCE_SELECT和RESET各阶段，**没有一次down拒绝原件**。因此目前最多证明未到native Offer，不能证明down因NOT_READY被调用并拒绝；也不能从物理81→559推进推导触摸可达。

可能切点包括SURFACE原生手势与ArkUI回调分发、覆盖/面板布局、changedTouches为空、controller未收到Down、诊断入口返回拒绝或跨进程缺档。没有原触摸命令和同scope回执，不裁定其中一个。T510已证reset/export真实命中不重测；面板展开挤走XComponent是独立布局可达性问题，不把已收起后的未知再解释成误点。真正GAP/timeout后ready撤销必须继续拒输入，不能删除该门。

## 3. 工程修订A：稳定输入面与有界动作身份

developer在实际M1/M2生成页面接独立同框ArkUI touch Stack：render SURFACE与输入节点分别有稳定id，输入节点同尺寸/同窗口位置，在render之上，避免依赖SURFACE是否转发ArkUI触摸。不得给simulation画Canvas，不增加物理源入口旁路。诊断面板移入固定区域/折叠页，不再用会挤掉render的layoutWeight作为唯一空间保证；若几何确实变化，走既有generation/取消/重新DOWN，不偷保旧坐标。

页面handler入口先固定计数和首条Down/Move/Up/Cancel摘要（含changedTouches个数、设备event时间原值、local/window/screen坐标域、node/bounds/layout revision、pageSession/epoch）；再交controller。controller记录精确allowed各子谓词/拒因与sourceOpen/boundary，而非只报STALE文本；native记录action API是否ACTION_RECORDED及Offer raw128B/原hash/回执。观测计数/首因采用独立定容32条新输入阶段槽、每条≤512B，总16KiB，溢出明确dropped；不扩既有1,404,072B Capture或挤占其首根/首因，新增小ledger与其快照分别记host账，只保一个读快照。新schema按T503真实UTF8信用流式导出，仍总diagnostic≤2MiB/timeline+diagnostic≤4MiB；超界INCOMPLETE保首因，不默抬预算。不无限加日志或写热文件，不用Date.now比较native时长。

保持现有source协议、16队列、有效gravity/页面pulse、新意图/边界、不积旧时间、UP/Cancel关闭及必须重新DOWN。单元必须同时覆盖 `state=STALE/GPU_WORK_PENDING,ready=true,permission=READY` 下仍可down与offer；`state=CURRENT,ready=false`反而拒绝。不把pixel验证当输入许可，也不因允许忙态入队就提前接受SOURCE或改变Run消费次序。不得再“修复”一个不存在的CURRENT谓词。

## 4. 工程修订B：整条GPU执行链的成本方案

当前M1 K192/Kw128固定root表127次dispatch（35固定槽＋sort256的36＋两次sort128各28），保0..28实际stage及全部audit/replay/promotion/pub/seal。单组串行水/运动/bed/replay为有界全池工作，索引sort为O(Klog²K+Kwlog²Kw)，轨迹/碰撞/重放的有界扫描按实际循环另记，不把127×64lane当并行加速证明。SOURCE仍保既有完整8阶段和4096条来源/dedup前缀、全池credit/ID/事务及重放成本，不以空Q成功推断非空SOURCE或M2 pair搜索已满足预算。已有root1耗时记录显示“继续加预热1个pass/延长等待”不能作为根治合同。

先做**静态stage族特化＋完整只读冷预备**，不改物理数学、顺序或审核。建议8个Q程序：COPY{0,20,22,28}、PREFLIGHT{1,24}、WATER{2,3,4}、INDEX_SORT{6,7,8,9,16,17}、TRANSPORT{5,10,11,15,18}、BED{12,13,14}、AUDIT_REPLAY{19,21,23}、PUBLICATION{25,26,27}。compile-time group把不属于该族的入口/不可达依赖裁掉，不能只在原巨大运行时if外加uniform。所有29 stage各归一族，原uniform顺序、buffers、barrier、deterministic error和先成功前缀原样；不对物理索引/库存作host计算。更长串行闭包若需改算法，带实际stage成本回designer，不能以删replay/空场跳审核替代。

cache按实际特化源码/ABI/组别/layout/环境构新key，原artifact不冒同key；每活跃route8Q+SOURCE+3scene≤12角色，保32条/128MiB预算、历史profile允许正常LRU，不承诺四profile全驻缓存。反射每组实际需要的uniform集合，原无关uniform不可强制active，也不忽略该组必需项。生成器/native/GLSL/reader manifest逐TU同源，M1/M2实际路由分开适配，不能把m3a15程序按字符串强植老M1。

冷预备在READY前，用私有fixture将全部组/关键资源绑定/实际最大排序规模与replay访问域执行一遍，核private candidate/current只读、无Run配额/源commit/Swap。每组binary-load后仍须反射/prepare，收据compile/load/prepare分别保存；不能从磁盘命中推断驱动已完成所有延迟编译。沿既有cold-prefix500ms政策和owner撤权，超时明确COLD_PREPARE_FAILED，不把root期限挪到预备里“已通过”；若完整预备成本无法落现政策，先回architect/manager裁决新独立cold profile，不私调预算。

热态保持最多10 op/8ms batch检查、单尾fence/1024B map、绝对250ms和双GAP，原GL调用无法抢占时返回后立即按旧deadline拒，不能断言batch必≤8ms。用原有stage ENTER/EXIT的actual wall/counters按scope汇总，不无限新增同质日志。比较同来源冷/暖、空场/非空SOURCE＋Q、四profile和长序列；原资源/program高水位由T531入口记，不以某一个暖空根≤250ms冒全部PASS。

渲染按既有30fps节奏选可呈现的已接受publication，不要求120fps。每次Draw仍核同代lease、完整token和原Swap permit期限，不能给过期root自动续许可。输入依据控制资格独立，不能等待UI偶然刷到CURRENT那一瞬间。对照特化前后相同原始输入和不可变成功前缀，保守恒/数值/错误/退出全等，才讨论收益。tester真机性能另卡，开发编译及必要非机自测可交，不把真机通过作developer关卡。

## 5. 黑区定位：一次分层证据，不先宣合成器故障

Swap成功不证明默认framebuffer内容非黑、最终viewport/scissor/blend/sampler/程序状态正确，也不证明Surface layer可见。截图没有与某个renderedToken/hold逐帧配对；原件虽给首根Draw/Swap闭环，尚无同帧FBO/默认目标/系统像素三域证据。因此保 `PIXEL_CAUSE_UNRESOLVED`，不直接派合成器缺陷，也不默认GL失败。

最小新增是显式research诊断场景，**不是失败回退**：由同一XComponent/EGL owner/scene-FBO/最终present shader送三块已知颜色/坐标图，醒目标记DIAGNOSTIC_NOT_PHYSICS，独立render token，不要求CURRENT物理成功、不伪增commit/accepted。先安全退休物理root/lease再切诊断route，新scope与真实source场景隔离。诊断期间页面pulse/撤权仍真运行，不绕过GAP/窗口失效。

冻结诊断帧及actual node/window/screen/viewport/scissor、FBO/source texture/program/sampler/blend/alpha/Swap原EGL结果和时间，hold窗口内只冷读取三个小ROI（FBO和默认目标送显前），同时用已有capture.py取得带原调用时钟收据的系统PNG。位置从独立geometry测量，不逐ROI平移拟合、不用截图反推映射。native readback/Png采集失败明确NOT_MEASURED，frame token/hold覆盖不齐拒判。诊断小图不能宣真实沙水呈现通过。

判读依次为：FBO错误→scene/输入资源；FBO正确、默认目标错误→最终present状态/采样；二者正确且Swap成功、同hold系统ROI错误→才把范围缩到Surface/layer/窗口合成并要对应layer树/事务时刻。跨scope、黑空场或没hold不能越级。真实SOURCE仍用A的完整down→offer→SOURCE receipt→后继Q→publication链，不以诊断图替代。

## 6. 接力与真实未测域

manager可分派developer两项独立范围：稳定触摸/固定布局＋分层诊断，和完整8族特化/预备/预算接线；后者不得只交一族/一次日志就停。tester在新来源上接真实输入链与一次分层像素/根成本，保原报告事实，修正“非CURRENT封锁/合成器已定因/103.5ms”措辞；不重跑已证reset/export。T410/T437负控/收益仍需真实非空SOURCE前置，T448仍需有效内容，不因本归因交付自动判功能通过。T461保独立M2创建/控制scope，不能用本M1资料定其同因。

本卡首次归因和可实施工程合同完成；产品根治效果/最终像素归因尚待上述独立执行，精确前置已列，不笼统归GPU超时。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退；FAULT或标STALE完整上一有效帧。
