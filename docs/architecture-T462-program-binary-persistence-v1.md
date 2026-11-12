# T462补充 · GPU编译产物持久化与复用

2026-11-10，architect。用户追问“为什么一直不安排编译结果持久化”后补齐工程遗漏。不使用真机。本稿补充T451/T462的启动成本治理，包含可直接派发的开发及独立验证prompt，不更改正在执行的T468诊断卡关闭条件。

## 1. 遗漏与范围

此前T462只要求编译/链接日志落盘，没有安排成功program的binary持久化。当前仓库检索没有glGetProgramBinary/glProgramBinary接线；m1及T452生成m2route均在Create时编译SOURCE、中间QUANTUM及最终QUANTUM，Release/新context后重新创建program。不能依赖驱动可能存在的隐式缓存作为应用已经实现的能力。这是架构方案漏项，应作为GPU P5后续实施，不再等待下一次失败才提出。

本稿要求**成功链接产物自动持久化，下次同环境先加载**。同时持久化每次编译/加载的结果与原始日志，不等用户点导出。缓存降低重复编译成本；不能修复首次编译失败，不能证明30–32ms运行期Flush消失，也不能替代T468/T472定位错误6。不得以缓存命中掩盖已变更shader、旧layout或设备能力不足。

## 2. 唯一program创建入口

M1与独立M2生成链共同使用受版本控制的ProgramFactory/ProgramArtifactStore，实际SOURCE、中间QUANTUM、最终QUANTUM分别标识，不只给诊断页另加一条缓存旁路。场景shader使用同一工厂接入，不能把compute缓存宣称为全部渲染缓存完成；交付清单逐program列出覆盖与未接入项。首版优先完整接通这三个compute program，场景接入可独立派卡，不阻塞首版交付。

工厂的优先序为：同context有效GLuint引用复用（存在时）→进程内不可变binary字节→本应用私有目录持久binary→原GLSL编译链接。当前reset会销毁context，首版不得为保GLuint重写恢复/lease生命周期；跨context只能加载binary，不能复用GL对象名。program引用按context/share-group实际代和owner管理，最终删除由所属GL owner执行，禁止在UI/文件线程调用GL。

每个program保持原GLSL精确字节、阶段组合、link前属性/输出绑定、separable等配置，以及布局/绑定ABI。key为这些内容和真实驱动环境指纹的规范化SHA256，包含schema版本、GL_VENDOR/RENDERER/VERSION/SHADING_LANGUAGE_VERSION、实际context版本/配置与相关扩展能力；runtime支持的binaryFormat清单另存并核对。K/Kw/宏/常量只在改变实际代码或链接配置时改变key，uniform-only值不制造重复缓存。package签名SHA和epoch/run/session是来源收据，不直接作为缓存key；否则每次重签或新run都会无效。

缓存只存program可执行产物及校验元数据，不存沙水库存、SSBO、FBO、uniform location、permit、quota、token或接受状态。不同run的初始数据、绑定与uniform仍全部按原路径初始化；相同shader在新签包中可以复用，但layout/ABI/key或环境变化必须失效。

## 3. GL加载、导出与失效

1. 在真实GL owner/context查询GL_NUM_PROGRAM_BINARY_FORMATS及GL_PROGRAM_BINARY_FORMATS。无可用格式、查询失败或不支持时记BINARY_UNAVAILABLE并走原GLSL路径，不假称命中。GLSL能力门保持。
2. 磁盘数据先检查schema/key/环境、format、bytes和payload SHA256，有界读取。新program调用glProgramBinary后必须检查GL_LINK_STATUS、原始GL错误及program log；失败立即丢弃该对象并使该条cache失效。本次候选最多一次源码编译回退，不反复load/recompile循环。fallback仍为相同GPU GLSL，不允许软件GL/Canvas回退。
3. 命中后重新反射uniform locations，恢复全部uniform和SSBO等绑定，并完成现有prepare/warmup。program载入成功不等于transport READY；原能力、资源分配、身份、恢复事务、GAP和250ms边界保持。加载/反射/warmup的耗时分别记录，禁止用命中位代替真实时长。
4. 源码分支在link前设置GL_PROGRAM_BINARY_RETRIEVABLE_HINT=GL_TRUE；link成功且原reflection/prepare检查成立后，在GL owner读取GL_PROGRAM_BINARY_LENGTH并glGetProgramBinary取得实际length/format/bytes。每个成功program分别导出，后一个program失败不能丢掉前一个成功产物。失败program只写诊断，不写成功binary或永久负缓存。
5. 导出失败、长度零、格式不可用或超界记CACHE_NOT_STORED及原始原因，不改变原来可运行的program。glGetError具有消费性，原产品错误仍送入原判定，缓存查询错误单列；不能读走原错让失败变成功。无界GL调用只能记录enter/in-call，不能强杀线程后复用context。

以上API语义依据Khronos官方参考：[GetProgramBinary](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3/glGetProgramBinary.xml)、[ProgramBinary](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3/glProgramBinary.xml)、[ProgramParameteri](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3/glProgramParameteri.xml)。binary可能因硬件/软件变化被拒，load会重置uniform；因此环境key和成功后的重新初始化都必须落实。这是规范支持的工程路线，当前真机是否有可导出格式及实际收益仍未测。

## 4. 自动落盘与证据

目录使用应用私有filesDir下gpu-program-cache/v1，页面提供所属应用可信路径，不能使用包资源目录或任意外部路径。文件线程执行有界读取与写入，GL owner只处理GL和不可变字节交接，不在控制锁中做磁盘I/O。写入临时文件→完整数据/元数据→平台同步与同目录原子替换；崩溃/磁盘满/部分写入不得破坏既有有效条目。相同key并发写串行，读取只见完整版本。启动清理未完成临时文件，不遍历或删除缓存目录以外路径。

首版上限每binary32MiB、总量128MiB、最多32条，LRU在文件线程淘汰，正在读取/写入的条目有pin；超限或I/O失败按缓存不可用记录并保留正常源码路径。后续调整预算须有实际binary大小证据，不能无界缓存。进程内binary同样计量并有界。每个key的不可用状态在一次启动内去重，禁止每帧重复扫描、落盘或尝试导出。

每次compile/load结果自动生成有界持久收据，字段至少为programRole/key、shader/layout/linkConfig哈希、环境指纹、package来源、cacheRoute、MISS/REJECT/COMPILE/LINK/REFLECT/EXPORT/WRITE_DONE阶段、GL原错与日志、reported/copied/truncated长度、binary bytes/format/SHA、各阶段起止native stamp。进程退出前写入可能尚未完成，必须区分EXPORTED_IN_MEMORY、WRITE_PENDING、PERSISTED和WRITE_FAILED；只有实际持久化成功才报PERSISTED。结果日志复用T462的有界原文规范，不能等用户点按钮才保存。

## 5. 可派开发prompt

P5，developer，不使用真机。按本稿实现ProgramFactory/ProgramArtifactStore，先完整覆盖M1与T452/T468生成M2的SOURCE/中间QUANTUM/最终QUANTUM实际链；复用T468诊断helper，不回退在途修改。成功binary自动原子落盘，下次启动优先load，环境/代码失配和损坏失效后一次源码回退，自动保存成功失败诊断收据。不得只交日志或仅内存缓存就声称持久化完成。记录场景shader尚未覆盖的实际范围，不扩大首版任务。

必要非机检查使用实际工厂和文件helper：缺档→源码成功→PERSISTED→新context binary加载、同key重签命中、代码/宏/link/驱动指纹改变失效、未知format/坏hash/截断文件/GL_LINK_STATUS=false回退、无binary格式、导出空/超限、磁盘满/中断写后旧档完整、并发key与有界LRU、uniform反射重建、旧context GLuint不能跨代。允许既有离线GL和注入存储/GL失败，但必须区分真实API执行与桩。完成实际生成应用ArkTS/native编译、签包、收据和真机复跑步骤后交付，不把独立真机验证加入开发关闭门。

要么完成本卡当前范围及角色必要检查，要么确有具体不能继续的问题才停止，局部受阻先完成其它可做项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停止投入，现有oracle仅复用。所有前景背景仅XComponent+shader，禁止Canvas/软件GL上屏回退；失败FAULT或标STALE完整旧有效帧。成功回原卡且本人行结束待验收；失败/关键缺证原卡写未完项、复现原错、证据、尝试、不能自行继续原因、所需前置和接手角色，本人行看评论。

## 6. 独立验证与排期接续

manager据本稿新建P5缓存实施卡及其独立验证卡；T468继续完成既有诊断范围，不因本补充新增关闭门。缓存实施与恢复接线各保独立职责，合并生成基线时显式核来源，不把已交T467回退。

tester真机卡必须带“真机”标签且同设备串行。新包安装后的首次冷创建证明compile/export/PERSISTED；保留应用数据，强停并重新启动证明同keyload命中、源码compile计数为0，分别记录load/反射/warmup/READY/首root/Draw耗时。新空场/重建context再测，不能只测同进程同context复用。清数据/卸载会清私有缓存，应单列冷缓存场景，不能混作失效缺陷。用受控环境指纹/代码更改、坏档负控证明拒绝并正确重编译，原root事务和设备能力门不变。

收益必须是实际冷/热启动时长对照和compile/load计数，不能仅以落盘文件存在或软件GL通过推真机加速。若设备格式数为0或实际load仍慢，带真实能力/时长回architect；不得因此省略失败日志或放宽控制时钟。失败首次归因仍回architect，manager不重跑测试。

## 2026-11-12 T480 场景覆盖工程接续

T475已实现M1/M2三compute program持久化，场景vertex/fragment首版未覆盖。新增 [T480工程稿](architecture-T480-scene-program-cache-v1.md) 明确三graphics role、双stage工厂、旧compute key与T475B001兼容、冷准备/反射/代际清理和同帧像素回归；不扩大T475关闭范围，不把compute真机暖load推为场景已通过。
