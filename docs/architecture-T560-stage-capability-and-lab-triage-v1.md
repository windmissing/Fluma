# T560：G9分stage设备资格与Lab失败首次归因

2026-11-17，architect，不使用真机，不跑GPU/NAPI/产品测试。输入T541 v2、T526 v3原报告/卡评论、现存原件、T529/T549及T538实际源码。工程结论：compute与scene资格必须分层，并以整数实例属性消除packet vertex shader的SSBO依赖；Lab队满注入后的控制排空有确定竞争窗口，须工程修复，现有FAIL不改PASS。机器表/审计见 `architecture-T560-stage-lab-manifest-v1.json` / `analysis-T560-evidence-v1.json`。

## 1. 设备证据精度与原谓词

正式 `g9d-execution-k192.jsonl`保renderer=Maleoon935F、ACTUAL_GL_EXECUTION、CREATE result4/UNSUPPORTED，无任何物理差分结果。T541报告/原卡给出vertex=0、bindings32/computeBlocks16/shared32768/lanes1024/fragment16/maxSSBO1073741824、原错误干净。现存glver/glext原件可独立核ES3.2/104扩展及bindings32/blocks16/shared32768/max1GiB；**完整g9_predicate_probe stdout没有单独保存在本地证据目录**，vertex0/lanes/fragment数值仍标REPORT_OBSERVATION，不能把probe.cpp的printf常量当实际输出。下一次最小复核保存完整stdout/stderr/exitCode和同context GL query error，不重跑长差分补这个小缺口。

canonical `g9spf/transport.cpp::Create`（共享树有他人在途修改，不回退）仍把vertex>=1、fragment>=1并入compute初始化门。T549冻结e429ea7也有同谓词；harness不经NAPI，因此此次拒绝与T546的页面解析/未知typedarray谓词不是同一证据域。K192之外的K512/FULL是共享门推断，未称实际执行过全部档。

逐shader核：pipeline.comp仅compute、4SSBO；scene.vert是无buffer fullscreen；scene.frag读State@0；packet.vert读State@0的mass/x/y2，packet.frag不读SSBO。所以原产品packet确有vertex SSBO依赖，不能原样删vertex门后宣支持；但纯compute差分不需要先满足packet或fragment绘制资格。

## 2. 冻结能力分层与零拷贝packet替代ABI

新增 `G9_STAGE_CAPABILITY_V2` 冷记录，含实际context/route/K/source/build/renderer和每个GL query的原值/有效status，分COMPUTE、SCENE_ATTR、PRESENT三域；支持域是实际probe+shader link/反射/预备结果，不是厂商名单或ES版本号。

| 域 | 必须通过的门 |
| --- | --- |
| COMPUTE_ONLY | 同owner当前ES3.1+context；4全局SSBO binding、compute stage blocks≥4、combined SSBO blocks≥4、shared≥4096、invocations≥64、workgroup sizeX≥64、countX≥1；最大SSBO≥max(state/work/arena及实际新profile单对象)；原GL错误/probe有效、实际compute compile/link/reflect/prewarm及资源门全过 |
| SCENE_ATTR_V2 | COMPUTE已准入；fragment SSBO blocks≥1/combined≥1；vertex SSBO需**0**（新shader不声明），至少3个vertex attribute、stride256可表达、实际unsigned integer instancing与VAO反射；FBO纹理尺寸/RGBA8完整性及真实新shader link/reflect/prepare，全部同context |
| PRESENT | 有真实有效XComponent/window/surface尺寸与同token/controller lease、draw/FBO/default-target/Swap前后权限和原错误，单独报告；pbuffer不冒产品呈现资格 |

增加的count/size/combined查询失败仍明确UNSUPPORTED_QUERY/具体code，不以未初始化0猜硬件限制。driver query只在冷创建/能力导出，不在每帧重复；不额外glGetError吞业务首因。COMPUTE通过只允许headless执行/取证，页面必须另过SCENE/PRESENT方可显示CURRENT；不能用分层结果伪装整产品支持。native `Transport::Create`只持compute要求，`Scene::PreparePrograms/PrepareTargets`持scene要求，bridge建物理页面必须按顺序检查两者并保实际失败域。

新packet顶点ABI `G9_PACKET_ATTR_U32_V2`：沿原state里的packet64原件，直接将**同一被lease pin住的CURRENT buffer**绑定为GL_ARRAY_BUFFER，不复制、不CPU map、不生成新物理缓存、不新增compute。`glVertexAttribIPointer`三路均size1/typeGL_UNSIGNED_INT/stride256/divisor1：location0 mass偏移packetByteStart+0、location1 xQ20偏移+4、location2 y2Q21偏移+8。K192/K512当前packetByteStart均280576；由对应profile机器表计算，不能把70144word常量当所有未来profile真值。实例i的原始读址=base+256i+fieldOffset，最后实例每属性末端≤stateBytes，checked pointer/stride/count乘加后才下发。

packet.vert只声明layout(location0/1/2) in uint，不再有State SSBO；gl_VertexID仍生成原6顶点quad，mass0移出视口/fragment discard、x/y/半径/颜色/裁剪数学保持。glVertexAttribPointer浮点隐式转换不等价，必须IPointer；不能CPU拆包再给VBO。packet shader缓存ABI与sourceSHA显式换代，旧binary不复用。反射核三attribute location/type/size，VAO重新绑定当前lease buffer且divisor1，无关array disable，fullscreen VAO分离，不让旧divisor污染fullscreen。

compute写→scene读使用 `GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT`，在实际提交/读前同GL owner下发；不靠glFinish代替consumer barrier。State fragment读用前者，packet实例读用后者。scene fence/ReleaseScene和controller原“不覆盖未退休lease/frame”的规则保留，当前bank在GPU attribute fetch完成前不得重写/释放。context换代删除/失效VAO和反射表，晚返回不publish。此方案新增payload0、新dispatch0、热读回0/复制0，仅新VAO状态/反射及能力记录固定对象单列；不改变物理state/event/输入/守恒或250ms/GAP。

Khronos依据：[integer vertex attribute说明](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3.1/glVertexAttribPointer.xml)、[实例divisor](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3.0/glVertexAttribDivisor.xml)、[consumer memory barrier](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3.1/glMemoryBarrier.xml)。这些支持工程接口/同步选择，不是设备已通过新shader的证明。

T554 g9dm及T563组合视觉应消费同分层接口，按各自work/arena/scene属性实际需求核准；T563不再新增vertex SSBO。T549原冻结R1 baseline不改，另生成明确 `T560_STAGE_CAPABILITY_V2` harness/源码差异与EXE收据（仅资格和scene变化，物理shader保持原父版本）；T541用新版本续办，不让新HEAD覆盖旧证据或旧oracle期望。

## 3. CACHE_QUEUE_FULL：注入与二次Drain竞争分开

原 `lab-cache-fault.json` state=FINISHED仅表示worker已结束；records确有CACHE_QUEUE/CACHE_QUEUE_FULL(a128)随后FAIL/CACHE_QUEUE_FULL，无END，不等case完成。文件中实际FILE/FILE_SHA256对为ENOSPC、TMP、RECOVER、SCAN共**4对**，PIN与SCAN_RESTORE是阶段记录，不是“六个都各有原件”——保持报告历史，补证表按实际列出。

源码Cache先用blocking job确认worker被屏障卡住，再塞128个job，第129个Enqueue必触发容量拒绝，局部catch记录CACHE_QUEUE，这是**预期注入**。但release/notify后立即store.Drain，Drain自己也Enqueue一个promise标记；若worker尚未pop出任何一项，此时jobs.size仍128，Drain的Enqueue再次抛同一CACHE_QUEUE_FULL，外层Run catch记录FAIL。原件顺序与这个确定竞争窗口一致；没有栈/子阶段标记，**不把具体第二次抛点冒独立运行栈实证**，但源码已足以要求消除该窗口。不能把整个FAIL解释成预期QUEUE测试结束，也不能扩大队列/删门或把Drain异常一律吞掉。

工程修订只作用隔离lab/test能力：128容量和生产非阻塞Enqueue/Drain接口语义不变。队满注入用具名EXPECT_QUEUE_REJECT步骤核首次队满、accepted128、queueRejected增量1、原始reason；unexpected接受或其它异常直接CASE_FAILED。为lab加入 `DrainForTest(deadline,cancel)`（只在GRS_PROGRAM_CACHE_TEST_FAULTS目标启用）：短锁等待队列有空位、在同一store mutex内原子插入fence标记，不能先看size再解锁调用Enqueue产生新TOCTOU；worker每次pop通知spaceCv。deadline固定5s从调用起、cancel/stop具名终态，不轮询无限sleep；入队后等该标记执行，不保store mutex等future。其fence覆盖调用前已提交job，完成后再Diagnostics502存队列边界原件；所有错误仍上报，不当生产成功。

测试屏障RAII保证任何异常/取消释放blocking job，store在barrier/wake销毁前drain/join；不能异常绕过release导致析构等引用失效对象。成功记录QUEUE_REJECT_EXPECTED→QUEUE_DRAINED→恢复后可再Put/Drain→32MiB/128MiB pin/oversize→END；后续旧未到步骤均继续，不因一次修复只完成队列就停。

Lab顶层V2拆lifecycle=RUNNING/FINISHED与outcome=COMPLETED_FIXTURE/CANCELLED/CASE_FAILED/INCOMPLETE，保firstFailure(stage/reason/原错误)，END与CASE_FAILED互斥；V1 FINISHED没有outcome时必须读records，不回写旧原件为PASS。SaveBytes/Storage必须返回I/O结果；open/write/fflush/fsync/close/manifest失败记录具体阶段并使outcome失败，FILE_SHA只在完整write+sync+close成功后当候选记录。当前SaveBytes对内存源算hash，不独立证明落盘文件，V2需read-back/传输验证；不能只因a=expectedBytes/b1便宣布取回完整。

此为工程分流，不是manager对测试FAIL的阈值重判。T526 v3 FAIL保留、物理资格仍未证；新版本独立tester复跑剩余队列/恢复和压力，BUSY_FAIL原卡可独立继续。

## 4. 私域原件：有界同进程读取通道

hdc不能直接读app filesDir，不用chmod世界可读、root或猜public路径。新增lab独立库冷接口 `resourceLabFilesBeginV2(jobId) / resourceLabFilesIndexV2(exportId,page) / resourceLabFilePageV2(exportId,fileId,offset) / resourceLabFilesEndV2(exportId)`，ID/U64十进制字符串，typed JSON普通错误也完整；不走生产App/trace许可，仍FIXTURE_SYNTHETIC。Start新job在export pin期间LAB_EXPORT_BUSY；只允许FINISHED会话，失败会话的已落完整原件也可取，不伪造不存在文件。

只取本job **SaveBytes显式登记**的具名根目录文件，最多32项，每文件≤2MiB，累计≤64MiB（已有原件磁盘信用，不再复制一份RAM）；不递归导gpu-program-cache目录的128MiB binary，不接受任意路径/..、symlink、调用方filesDir或旧job目录。需要cache文件身份时独立增版allowlist，不能私扩导出范围。Snapshot catalog保存实际fstat设备/inode/长度/mtime与已完成写入身份；openat根fd+O_NOFOLLOW，读取前后核同一文件，变化返回FILE_CHANGED/INCOMPLETE，不能静默跟新文件。

单reader、无builder，begin固定TTL60s不续，4096B原始chunk→base64≤5464B、整reply UTF8≤8192B；index每页≤8项/8192B。Native只保一个4096B原始块+8192B writer+32×256B catalog=20480B及固定控制/hash态；不全文件读入JS数组。page含job/export/fileId/offset/nBytes/totalBytes/eof、chunkSHA和实际原字节base64；chunk先hash再传，collector逐页base64 decode/hash/write，offset必须连续，逐文件fsync/close后和begin/final声明的原文件SHA比对。结束/TTL/取消归还fd/pin；scope/job变化先拒，无权限越目录。

应用内collector通过系统文档保存picker/用户选择目录或平台分享交出实际归档（每文件原件+传输manifest，逐页写临时文件、完整校验后rename）；可由具备目标app上下文的测试ability直接落获授权导出URI。只是新协议开发并未证明picker/hdc在此设备成功，tester必须保原API/URI/权限/结果和传输前后SHA，必要的用户保存交互由effector执行。未得到原字节仍NOT_MEASURED，不能仅保存Begin/FILE_SHA manifest宣独立核验。

相关V1 `resourceLabExportV1`只是Poll别名，**没有文件取回能力**；新接口显式V2，保旧记录可读、不改老方法使调用方误解。业务资源/诊断2MiB/总4MiB限额不抬，新增lab文件通道单列20480B内存、60s、64MiB允许磁盘总量。writer/索引越限完整INCOMPLETE而非截JSON。原门正确工作不等完整取证链通过。

## 5. 接力及检查

developer prompt：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿/机器表完成能力分层、packet整数属性/反射/barrier/lease、新版本harness及同源生成；隔离lab队满排空/RAII/终态和有界私域文件通道，更新声明收据。明确文件归属，保共享在途修改，不回退别人的transport。仅开发/编译/提交、不执行测试，独立tester不作为开发关闭前置。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它可做项。阻断原卡写全部未完/原始错误/复现/证据/尝试/不能继续原因/输入前置与接手角色，失败本人行看评论、成功结束待验收。

tester在新包先保同context完整probe输出、compute-only与scene分层拒因/反射/真实属性读值/错误和物理差分，再验证新CURRENT同token；头less通过不冒可见渲染。Lab按真实case/outcome、队满一次拒绝与排空恢复、原文件传输核验、取消/TTL/短读/变更/私域路径负控分别报告；首次失败仍architect分流。所有产品前景背景新XComponent+shader，失败FAULT或标明STALE上一有效完整帧，禁Canvas/软件GL回退；CPU停投、默认OFF、250ms/GAP33333333ns、120Hz配额/30fps显示不变，不新增manager验收门。

实施接缝：`g9spf/transport.{h,cpp}`新增 `Status QueryComputeCaps(ProfileRequirements, StageCapabilityV2*)`，只在owner冷阶段查询；`scene.{h,cpp}`新增 `Status PrepareAttributeScene(const StageCapabilityV2&, SceneCapabilityV2*)`，更新 `packet.vert`、generated shader/cache ABI与独立VAO；新增 `stage_capability.{h,cpp}`/reader字典保每项{pname,value,queryStatus,required,domain}和build/context身份。桥接按compute→scene→present错误域保留原code，实际调用方不能因“compute支持”把scene失败吞掉。

`program_cache/artifact_store.h`仅test-fault目标加 `DrainResult DrainForTest(steady_clock::time_point deadline,const CancelToken&)`，结果DRAINED/TIMEOUT/CANCELLED/STOPPED，唤醒条件和fence插入同锁；worker pop/stop均notify，锁外等待fence。`resource_lab/lab.h`及test/t538 adapter新增lab_v2 export模块与NDK桥接，所有文件I/O在lab worker，页面只是单页传输/状态展示，禁止UI线程等future/读整档。生产Cache/Broker不依赖lab模块。

FilesV2回包共同字段{revision,jobId,exportId,outcome,reason}；Begin加{expiresAtMonotonicNs,fileCount,totalBytes}，Index加{page,entries:[fileId,name,bytes,sha256],done}，Page加{fileId,offset,nBytes,totalBytes,eof,chunkSHA256,base64}，End加{released}。U64一律十进制字符串，fileId/exportId不接受路径。超期/错job/错顺序分别EXPIRED/STALE_JOB/INVALID_OFFSET，未完整写入文件不进入allowlist；失败返回也必须是完整有界JSON。Begin的文件SHA在worker流式重读计算；Index返回该固定snapshot SHA，Page EOF后再核原身份及全文件SHA，collector逐文件对照真实取回原件。固定32项catalog只存fileId/有界名(≤96 UTF8B)/stat/size/SHA等≤256B/项，超名或条目数拒，不能借字符串堆扩容绕过20480B payload。TTL/END/异常取消均由RAII释放fd/worker pin。

本轮 `analysis-T560-evidence-v1.json`保历史原件SHA、4对真实FILE/FILE_SHA、分层真值表、队列两种调度窗口和4096B/base64/索引容量核算。只检查源码与有限布局关系，不执行lab/GL/真机，不改T526原FAIL或T541资格未证。
