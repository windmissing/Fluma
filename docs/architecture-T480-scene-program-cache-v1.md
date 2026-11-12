# T480 · M1/M2场景program缓存覆盖工程方案

2026-11-12，architect，P5，不使用真机。按T462持久化合同和T475首版覆盖清单，扩展场景vertex/fragment program。T475已关闭的compute交付范围保持；当前T477设备缓存命中只证明compute三program，不能扩大成场景或上屏已通过。来源及实际六份shader字节hash见 [本轮证据索引](analysis-T484-T478-T480-evidence-v1.json)。

## 1. 实际覆盖对象和创建路径

`entry/src/main/cpp/gpu_resident/m1/scene.cpp` 的Scene::Ensure在当前GL context第一次有效Draw时，直接调用私有Program(vs,fs)，编译两个stage再link。T452生成器复制为m2route/scene.cpp，只改命名空间、V3 header检查和scene/root token，仍是同一三program创建链。已核当前共享源码与 `.codex/local-test/t482-final/` 的M1/M2生成scene，均没有programcache接线。

| 新role | 实际shader来源与CMake symbol | 固定接口与用途 |
|---|---|---|
| SCENE_FULL | m0_scene_full.vert → kM1FullVertex；m1/scene.frag → kM1SceneFragment | gl_VertexID全屏三角；fragment SSBO Publication binding0，color输出；背景/床/水体/水面/前景/最后呈现共用uLayer |
| SCENE_PARTICLES | m0_scene_particle.vert → kM1ParticleVertex；m0_scene_particle.frag → kM1ParticleFragment | layout(location0..4) aX/aY/aMass/aState/aRadius；实例化sand glyph，uint属性与float属性的读取方式保留 |
| SCENE_WATER | m1/water_glyph.vert → kM1WaterVertex；m1/water_glyph.frag → kM1WaterFragment | layout(location0..2) aX/aY/aMass；在途J glyph，uRadius/uMassReference及uOptics |

实际编译字节来自 `gpu_resident/CMakeLists.txt` 的M1_SCENE_HEADER装配，先规范CRLF并把#version300改成310，写生成m1_scene_sources.h；不能只hash磁盘原文件或kM1SceneSha字符串。T281脚本能给宿主同源header，但不能只认旧 `.codex/local-test/t281` 的过期产物。M1/M2各三条覆盖入口共六个调用点，GLSL完全相同且ABI一致时允许共享三份binary，不要求人为分成六份磁盘档。

现有program在Reset(true)时删除，在context变化Reset(false)时放弃旧句柄；每次新context会重编译。Ensure若部分创建失败，下一次重试还会覆盖已有非零句柄，需以创建事务处理清理，防止缓存改造放大泄漏。不改光学数学、shader字节、pass顺序、blend/viewport/FBO策略、粒子实例属性、publication lease或scene token协议。

## 2. 工厂接口和兼容策略

保留T475现有 `Build(uint32_t object,const char* compute,...)`、Key函数、kLinkConfig字节与所有compute调用的行为/收据兼容。**不能把object=3/4/5传给现有Build**：它不仅硬编码COMPUTE，还索引create468::Attempt.programs[3]；超界会破坏创建诊断。现有Pending只有一个shader log、Receipt硬编码Hash(kLinkConfig)，也不足以标识双stage。

建议在 `program_cache/program_factory.h` 新增graphics描述层，或独立 `graphics_program_factory.h`复用其公共load/store/export实现。接口为：

```cpp
struct ShaderStageSource { GLenum stage; std::string_view bytes; };
struct GraphicsProgramDesc {
  std::string_view role; // SCENE_FULL / SCENE_PARTICLES / SCENE_WATER
  std::array<ShaderStageSource,2> stages; // canonical VERTEX then FRAGMENT
  std::string_view bindingAbi; // generated interface contract, not runtime values
};
bool BuildGraphics(const GraphicsProgramDesc&, GLuint& program, GLenum& firstProductError);
bool ReflectGraphics(GLuint program, SceneUniformTable&, const GraphicsProgramDesc&, GLenum&);
GLenum GraphicsPrepared(GLuint program); // qualified only after interface/VAO checks
void ForgetGraphics(GLuint program, uint64_t contextGeneration);
```

函数全部由实际GL owner调用。descriptor须显式长度、不依赖连接字符串里的分隔符；拒重复/缺失stage、未知stage、超既有源/内存预算。成功path持久化和损坏/未知format/驱动拒绝的一次同GPU GLSL回退复用T475，不再造另一个文件缓存线程或目录。

graphics新key域为 `T480_GRAPHICS_KEY_V1`，按长度前缀编码每个(stage enum,length,实际source bytes)，再编码真实linkConfig、bindingAbi和T475同一真实environment指纹；role写收据，只有影响行为/ABI时才进key。linkConfig明确 `VERTEX+FRAGMENT|SEPARABLE=0|GLSL_EXPLICIT_ATTRIBUTES|NO_PRELINK_ATTRIBUTE_OR_OUTPUT_CALLS`，以及按实际源推导的接口版本。当前没有glBindAttribLocation、transform feedback或多render-target绑定调用，**不为缓存擅加这些调用**；未来新增时须编码实际name/location/顺序和配置，改key。

环境指纹仍包含真实驱动四字符串、context/config/version和扩展。package SHA/run/session/geometry/viewport/optics uniform值不入key，只是来源/运行状态；binding ABI包括三program各自attribute位置/类型、SSBO binding及期望的uniform接口。layout或publication读取ABI改变须失效。kM1SceneSha仍按原完整生成header产生，继续scene token绑定，不用缓存key替换业务shader/scene hash。

存储沿 `gpu-program-cache/v1` 和 `T475B001` 的144B header+payload，compute旧key与旧binary无需迁移或清空。store只验证key/bytes/hash，graphics隔离靠新key域，因此旧compute不可能误装graphics；旧有效compute缓存应继续命中。新增graphics收据显式revision=`T480_SCENE_PROGRAM_ARTIFACT_V1`、programKind=GRAPHICS、role、两个stage hash/bytes/各自compile日志和linkConfigSHA，现有compute收据仍写T475原字段。若抽公共Receipt，要把linkConfig放进record，默认值保持原compute，不可硬编码成新graphics导致旧收据来源失真。

每graphics对象两个stage各最多1536B原log、program link最多1024B，总4096B；三role原log总12288B，超界明确reported/copied/truncated与queryError。沿既有有界JSONL、条数32、总payload128MiB、单binary32MiB、文件原子写/pin/LRU/队列限额。六调用点不等于六倍预算，不新增无限内存shader日志。创建stage删除前采真实日志，compile失败不写binary；前一个role成功可独立持久化，不因另一个失败清掉有效产物。

## 3. 场景冷准备、反射和生命周期

新增 `Scene::PreparePrograms(contextGeneration)` 及幂等Ensure适配。主路径在实际GL owner完成makeCurrent/transport Create、尚未发布READY时执行三program构建、接口反射与VAO对象创建，避免首次持publication lease和root许可时触发几十秒冷编译。每role前后核candidate epoch/window/context仍有效，失效停止后续准备并清理候选；GL编译不可抢占时返回后再核，不能在控制锁内编译或阻塞UI。冷阶段不Draw、不Swap、不读/接受publication，不设置opticsBound、lastGood/lastRenderFrame、quota/permit或capture READY。

App只有当前candidate三个program及接口完整才可继续原READY提交；失败发布有明确SCENE_PROGRAM_CREATE/role/stage的FAULT，不伪报CURRENT。V1/V2控制session恢复仍按已交T467合同，不能为准备延长旧permit或制造pulse；新冷阶段的host/GL真实时长单列，READY之前不追补物理quota。该接线在M1和m2route生成应用同时实施，不能只改共享scene而让发布包仍走旧副本。

Ensure保留作为Draw端强制校验：当前contextGeneration匹配、三role句柄及反射表/VAO有效，才可运行；非App离线host可以显式先PreparePrograms。未准备的首Draw若仍需调用幂等Ensure，必须标准备时长，权限/lease失败按原门拒，不宣称热路径全部预备完成。

reflection至少核下列接口，source和binary load走同一检查：

- FULL：uResolution(vec2)、uBox(vec4)、uMirror(bool)、uColumn(uint)、uLayer(int)、uFrame(sampler2D)、uOptics(vec4 array)及fragment Publication SSBO binding0。uOptics反射数组长度须覆盖实际使用的最大索引。
- PARTICLES：uResolution/uBox/uMirror、aX/aY float location0/1、aMass uint location2、aRadius float location4。aState声明location3但当前vertex未使用，优化后可能不active，不能因此报坏binary；若active则位置/类型必须正确。
- WATER：uResolution/uBox/uMirror/uRadius/uMassReference/uOptics、aX/aY float0/1、aMass uint2。三个program各自唯一color输出位置0，active varyings匹配由真实link保证；检查结果落冷收据。

反射后缓存**当前context内**的uniform location/type表，Geometry/Draw按该表上传原有值，避免每帧重复glGetUniformLocation。所有uniform在binary load后重新初始化；每次Draw仍更新frame/viewport/box/mirror/uColumn/optics/radius/massReference，以及FULL呈现层uFrame=1、各层uLayer。SSBO binding0、texture unit1、VAO、属性stride/offset/divisor和整数属性API均按原流程重新设置，不能加载旧uniform/VAO状态。禁止为了“warmup”运行compute的PrepareProgram(0组dispatch)在graphics program上；graphics资格来自link+接口+VAO创建无GL错，首个真实绘制另记DRAW证据。

创建事务使用局部句柄，全部成功才提交到Scene；失败对本次已创建对象逐一Forget和glDelete，释放VAO，保持Scene无半初始化状态。context仍current时Reset(true)先Forget对应代再delete；旧context已销毁时Reset(false)只清其受代际约束的工厂bookkeeping，不在新context对同数值GLuint调用delete。Factory记录须验证contextGeneration/owner，防止跨context句柄值复用误删新记录；不能只拿GLuint作旧代清理凭据。实际lease/FBO retirement和场景上一有效token不因cache失效放宽。

## 4. 故障和呈现边界

磁盘miss/坏hash/过期key/驱动拒绝，丢该条后一次原GLSL编译；缓存导出或I/O失败保已合法program并记CACHE_NOT_STORED，不改业务成功条件。source compile/link/reflection失败则整个新Scene不可用，保原错误、role和stage；FBO/pass/Swap仍执行原权限与同帧token核验，不把program加载成功当上屏成功。旧有效帧若沿原机制保留必须显示STALE及旧token，否则FAULT；所有背景/前景继续XComponent+shader。

factory读取glGetError的消费必须保留原产品firstError，缓存查询/日志错误单列。缓存路径失败不允许CPU/Canvas/软件GL产品上屏；离线ANGLE仅作实现测试设施。设备格式数0可走原GPU GLSL并记不支持缓存，不冒称场景暖启动命中。

## 5. 可派developer任务书

P5，developer，不使用真机。实现本稿graphics descriptor/key/双stage工厂、独立收据、Scene三role与当前M1/M2生成链覆盖、READY前冷准备、反射位置表及context清理。优先受版本控制的program_cache公共helper、m1/scene.{h,cpp}和独立新生成overlay；新生成基线消费T475/T482 coherent修复和T483来源修正，保留后续已提交恢复/字典改动，不复制旧中间目录冒最终来源。交付逐role/route创建入口、stage实际字节hash、ABI/key/包ELF清单。旧T475关闭范围不扩大。

必要非机检查：双stage冷compile→PERSISTED→新进程binary load且两个compile计数0；VS/FS单独变字节、属性/输出link配置、ABI和环境变更失效；未知format/坏档/驱动拒绝只源码回退一次；stage编译失败与link接口不匹配失败原log；graphics load后location/uniform/SSBO/VAO重建；aState优化不active正确处理；三role中途失败清理；context代切换/同GLuint数值复用/旧回调不污染；compute原key及既有binary仍命中。不要只测文件Put/Get或自写理想shader就声称实际scene覆盖。

使用真实生成的M1与M2 Scene在同一已冻结publication/root/optics、frame/box/dpi/mirror下比较缓存禁用/源码冷建/暖load的每层FBO与最终FBO原像素及readback hash，正常冷/暖均CURRENT且同业务token（程序GLuint/cache身份不进token）。至少覆盖空场背景前景、床+飞沙、W水面+J在途水、镜像、viewport变化；复用T281实际层捕获hook和既有图像预期，不能把旧图像PASS当新binary路径回归。新run/帧递增受合同约束，配对在等价独立run执行并只规范化显式允许的运行身份，光学参数不变。

对准入前context失效、坏publication、旧generation、Draw失败/Swap失败/迟到permit，核禁止新CURRENT及lease正确归还，不能用缓存修复掩盖原控制阻断。必要编译、非机检查和签包通过即交付；设备验证另卡，不作开发关闭门。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它可做项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投。全部前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE完整旧有效帧。成功原卡回执并本人行结束待验收；失败/关键缺证写未完项、复现原错、证据、已试办法、为何不能自行继续、所需前置与接手角色，本人行看评论。

## 6. 独立接续与交付范围

tester离线卡消费实际新工厂、生成Scene和cold/warm各层原件，独立确认真实三graphics role+旧compute兼容及原同帧语义。真机卡带“真机”标签，同设备串行，先核包/ELF与生成shader SHA，首次冷建保数据强停暖启，逐role核PERSISTED/load/compile=0、阶段时长，再核READY/首root/完整Draw/Swap。T477的三compute暖load可按原件复用，不等于场景已测；若原有STALE_PAGE_SESSION/permit→Swap阻断仍在，场景缓存收据与上屏失败分别报告，失败先回architect，manager不重跑。

本轮完成实际创建点、stage/绑定ABI、工厂限制、key与存储兼容、生命周期及回归范围工程设计，未执行GL或设备测试。没有把T475成果改记为场景已覆盖，也不把新增缓存当作T477上屏阻断的已证根治。
