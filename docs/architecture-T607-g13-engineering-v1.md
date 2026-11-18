# T607 目标13纯流场GPU工程闭包

2026-11-18，architect，不使用真机。消费T605正式 `algorithm-G13-pure-fluid-gpu-v1.md`、manifest/16 fixtures及T613资产盘点；状态为**ENGINEERING_READY，产品NOT_IMPLEMENTED/NOT_VALIDATED**。模型固定为G13_MAC_FILLED_V1：二维全满常密度不可压牛顿MAC、守恒迎风、后退Euler黏性Chebyshev、Q32压力MG投影。不得换为旧高度扩散、旧孔隙G12或视觉流线。目标1–11和原优先级保留，10暂停；水线13→14→12，旧T583/T595/T602/T603/T604暂缓，不删除旧资产。

本稿与四张机器表共同构成T608可实施合同：

- [engineering](architecture-T607-g13-engineering-v1.json)：文件/API/输入指纹/完整原门/信用。
- [layout](architecture-T607-g13-layout-v1.json)：五profile每段精确word/byte范围、Config128/State256/Input128/Completion256/trace字典。
- [dispatch](architecture-T607-g13-dispatch-v1.json)：完整820行QUANTUM、16行SOURCE、opcode/role/J/micro/block。
- [math](architecture-T607-g13-math-v1.json)：GPU数学backend、系数位型及有界常数生成。

原算法公式/支持域/阈值不改。F05闭盒制造解的分裂压力误差尚未测，属于需独立验证的设计假设；即使Poisson残差小，物理压力门仍可能FAIL，不能豁免。所有设备资格、persistent耗时、250ms及30fps/推进比仍UNMEASURED。

## 1. 模块、构建与绑定

新增独立 `gpu_resident/g13flow/`，文件表在engineering.newFiles。contract/layout由本表单源生成；mac_access处理索引/ghost，advection/viscosity/pressure_mg/wide_q32/math_soft128/drive/ledger/trace/render_pack各有唯一权威实现。controller只管理身份/配额/事务/lease，transport独占GL与GPU提交，cold只冻结原件，scene只消费已接受场。不得在controller/native/ArkTS另写流体stepper、压力RHS、逐格边界或tracer轨迹。

新增独立`grs_g13_core`和`libgrs_g13.so`，页面GpuResidentG13默认研究OFF、显式入口；不修改grsm0/grsm1宏/旧NAPI表/旧profile或默认体验路由。最小依赖只复用program_cache、uniform_locations、资源账本/新管线FullFrameTargets、broker shared DSO及需要的纯算术；**不整体链接grs_foundation再意外拉入G11/G12 generator**。`tools/g13/prepare_build.cjs`为产品构建准备器，在隔离工程写最小native CMake及显式页面/库类型注册，不是测试脚本；canonical旧构建/旧在途树不改写。新g13flow/CMakeLists定义可独立构建的core/DSO，冷broker已有target则复用，否则只建同名shared提供者，不能为引其旧cmake顺带引入grs_foundation。编译交付为完整native/ArkTS/unsigned HAP，签名部署归tester。

`g13flow/kernels.cmake`用CMAKE_CURRENT_LIST_DIR定位自己的真实源/生成器；先完整生成头和依赖清单再编译，缺源/缺头/旧hash/重复opcode/字段覆盖失败均FATAL_ERROR。规范化LF后按依赖顺序组装GLSL310，hash最终字节，记录每个片段/JSON SHA；不能复用T613指出的错误../../../test路径或已有.cxx头。发布清单须包含实际HAP/ELF/源生成manifest、layout/math/gates/algorithm及测试开关身份。

compute默认一个4SSBO程序，以显式uOp及表中的scope/J/micro/block调度，uniform反射只在冷准备；profile offset表可用一个有界uniform数组，不能热Find失败再查询。复用Build/Reflect/Prepared与BuildGraphics；key纳入新layout/binding ABI、math/backend及最终源和实际驱动环境。使用既有ProgramArtifactStore持久化机制，缺二进制走源码编译、拒绝缓存保原错；不另造缓存系统、不将编译完成记数值PASS。新DSO的header静态store不自动成为跨DSO进程单例，host/disk及旧route重叠分别记账。

## 2. ABI、输入和单位

所有magic为LE32('G13'+末字母)，profile F、config C、state S、input I、trace T、tail R、render V、cold K；schema1、seal0xc0de1301。旧G9/G12字典不参与猜读。U64保持低/高U32，NAPI展示十进制字符串；F32保存原bits，压力为补码S64 Q32，SHA每4个原始digest字节组成LE32。保留字必须0、未知flag/revision拒绝。机器表完整展开128/256word，不允许私占reserved。

Config128保持T605组界：4..11网格/边界/seed/Lx/Ly/h/T0，12..17原L0/rho0/muSI binary64，18量化nu、19单位策略，20..33运动/力/示踪/门及上下壁速，34为显式pressureReferenceF32（默认0）、35和84..127零；六SHA组36..83不变。仅支持正式五网格，16²仅诊断；各网格hx=hy。L0/rho0正有限、mu非负，T0=1；nu0只诊断，正nu及所有运动/力/壁速/范围严格按T605。

L0/rho0/mu原bits与已量化nu/h同存。GPU冷准备将binary64拆mantissa/exponent，以U256缩放有理数计算mu/(rho*L0²)并RNE到binary32，核所给nu及h位型；超范围/溢出/量化不一致拒配置，不在首步暗改参数。单位归一仅标量，不新增CPU场演化。P0=rho0*(L0/T0)²，pi为动态零均值，显示总压力pi+phi−mean(phi)+显式p_ref；重力/离心势不再重复给速度加一次。

Input128原件与native规范化版本都留在4096B capsule。产品g13Offer128只接SOURCE控制；QUANTUM由native根据已授配额/已接受控制构造，诊断kind只在显式诊断入口。source仅更新g、连续运动段或力时段，不增减液体质量，零模拟时间。新运动须旧Omega0且原段结束、theta连续；force时段为quantum对齐[start,end)。原包验证其self-zero inputSHA后，native给sampleNativeNs/enqueueSerial盖实际CLOCK_MONOTONIC/队列身份并重算规范包SHA，原包不覆盖；两SHA及处理原因同时保存。

SOURCE安装时由GPU核旧段已结束且Omega精确端点0，连续theta取已接受状态/soft驱动的规范值；source声明theta0须匹配该规范F32值，不能借新命令任意跳角。尾136..145提供实际theta/Omega/alpha/box重力及motionEnded/origin给status，客户端可据此形成下一命令，不需额外热场读回。source保存原参数与核后值，不在CPU按wall time推旧段。

输入通过严格类型/N/offset/backing原件复制，沿T613有序predicate模式，但新route独立诊断槽，不拿G9枚举或旧firstFailure store凑新身份。同步复制到定长native数组，返回前不保JS指针；所有回复都是具名JSON对象，包含route/api/boot/diagGeneration/attemptId、阶段及原始NAPI状态。bridge属性表与d.ts逐项一致，不能只实现未绑定napi.inc。

## 3. 数据布局和信用

layout.profiles逐一给u/v/pi/tracer及WORK精确范围。u索引j*(Nx+1)+i；v索引Nu+j*Nx+i；cell索引j*Nx+i，行优先。三个state为CURRENT/CANDIDATE/REPLAY，WORK十face数组+四组各层Q32数组+四cell诊断+16word/partial+4096word系数控制，不额外分ghost、selected bank或热日志buffer。

CTRL共74752B：control/config4096、input capsule4096、实际/重放trace各32768、tail1024。control内部配置128/permit128/quota256/GPUglobal256/trial128/schedule64/保留64完整分段；input capsule包括原包/规范包、fixture/seed/boundary/运动及配置收据。phase不通过“借其它profile空闲尾部”取得存储。

MathQualification128、QuotaAndClock64及GpuGlobal256字典另在layout.schemas。未专门展开的冷构建/fixture/seed/边界收据块用TaggedReceiptHeader16（tag/schema/encoding/wordLength/usedBytes/flags/SHA8/保留0/seal）加有界原始UTF8 JSON payload，encoding1；超容量不截成成功，可保存完整流式外部收据并在该块用encoding2登记实际bytes/SHA/归档相对名，reader必须同时核到被引用原件才COMPLETE。solver state/WORK/CTRL原始字段仍必须真实导出，不能用这种引用或元数据替代。

| 网格 | state单份 | WORK | 双RenderPack | resident | ROOT / FULL冷raw |
|---|---:|---:|---:|---:|---:|
| 32² | 148736 | 162880 | 131584 | 815424 | 437760 / 880960 |
| 64² | 198144 | 597632 | 524800 | 1791616 | 536576 / 1857152 |
| 64×128 | 263936 | 1176256 | 1049088 | 3091904 | 668160 / 3157440 |
| 128² | 395264 | 2331392 | 2097664 | 5689600 | 930816 / 5755136 |

单位B，不含driver/JIT、host、cold槽、FBO/default framebuffer/compositor、文件编码、旧profile重叠。16²诊断值见机器表。分配/offset/乘法先checked size_t/U64；运行时仅选正式profile，不以实际malloc成功替容量资格。FBO双RGBA8另8*viewportW*viewportH，最大texture/renderbuffer/viewport尺寸查询。

每scope trace为4096B根/三trial摘要+7份4096B micro。micro固定128word头、16×16word压力记录、8×16word黏性记录、32×8word阶段记录、motion/energy/momentum各64word、兼容性证明32word及零尾/seal；不按迭代无限追加。记录flags分ACTIVE/CONVERGED_FROZEN/SUBMITTED_NOOP/FAILED_SEALED_PREFIX，缺失不伪造已执行。native/IO诊断不混入可重放物理字段。

host控制输入队列16条、原/规范包各512B，合计16KiB；配置/文件收据单份≤64KiB，拒绝无限字符串。cold单页raw4096+JSON16384+元数据≤32768B，文件worker流式buffer≤65536B，不在JS聚合整档；源码组装单源≤4MiB且至多两份8MiB，程序binary cache沿既有单entry32MiB/合计128MiB机制并披露实际DSO实例/临时写/pin成本。以上host/cold/cache不隐入resident数字，不能挪用旧route已占信用；超额具名拒绝，不自动涨预算。

## 4. MAC边界与真实算子接线

唯一access层分开cell与u/v索引。CLOSED：活动u为1≤i<Nx，v为1≤j<Ny；normal面+0，切向越壁取2*wall−inside，不把最近内侧面清0。pressure ghost同邻cell。TEST_PERIODIC_XY：u i<Nx、v j<Ny；TEST_PERIODIC_X_CHANNEL：u i<Nx、v 1≤j<Ny。所有邻读直接canonical wrap，右u/上v重复槽只在原ENFORCE_NORMAL_FACES中复制、不参与norm/能量/Coriolis两次。corner先处理周期轴，再normal已知值，再切向镜像；pressure只映射cell坐标。没有逐格ghost库存。

ADVECT只读ENTRY、写ADV及ADV_L；u的east/west输运速度为相邻u平均，north/south分别为两邻v平均，v按交换轴对称定义；同一dual边两侧使用同一按正轴排序的平均式和upwind选择。normal壁输运0，incoming/outgoing CFL分别核≤1/2，非有限/范围立即记录首错；不能等投影后再隐藏坏CFL。双向通量和边界项进入原动量/能量账。

BODY_KICK以本micro accepted中点alpha/f_nc写KICK，保实际前后中点功。CORIO从KICK初始化，八色按(−1,0)/(0,0)/(−1,1)/(0,1)及反序；每色pair唯一owner、同时读旧u/v再写，排除fixed-normal与alias。Omega0精确恒等。无跨group写同一面；并行pair内in-place安全，不需要第五face临时数组。

VISC_SETUP构造b及非零壁Dirichlet常数项，x0=b；XM1/X/XP1在既有三个face数组轮换，ENTRY/KICK/CORIO不覆盖。H stencil用齐次ghost，非零壁常数只进b一次。d0/nu0原样复制；否则按T605系数递推8块×16次。每块末另算b−Hx真实RMS/max，首块满足两门即冻结；以后同表提交uniform NOOP，不用递推残差或“迭代够了”当收敛。128次不达为VISCOSITY_NO_CONVERGENCE。

PRESSURE_RHS逐面FP32→S64 Q40 RNE，再精确差分/h；b32=RNE(−120J*div40/256)。PRESSURE_RHS_COMPAT检查|sum b|≤Nc/2 LSB，TZ均值加canonical序±1余量，使sum精确0；记录pre/post sum、修正count/max。不能用大幅减均值修非法法向流。

PRESSURE_INIT清所有层p/b/r未用域，fine b为上述真实原件。每V-cycle一个64lane group：各非粗层2Jacobi→真残差→四child RNE restriction/兼容修正→清coarse p；最粗32Jacobi并减均值；逆序prolong（各轴3/4与1/4、分母16）→2Jacobi；fine减RNE均值、真r=b−Ap与证书。Neumann/periodic/通道每层用同拓扑，h_l=2^l*h，Jacobi分母3diagA、numerator2r，S128/U128 checked，不能把coarse h漏乘。

pi限64Q32，b/r限2^56，平方和≤2^126；比较证书前先约分/缩放，任何中间溢出FAULT。GPU用lambdaLB=8/Lmax²或双周期32/Lmax²，etaB=(120J)*2^-39/h+3*2^-33。rRMS采用整数ceil-sqrt(ceil(sum r²/Nc))提供保守上界U（Q32原量），不以浮点norm放行。正式网格h^-1≥16，使etaB的Q32原量为`a+3/2`、a=15J*h^-1/16整数：压力门比较`2U+2a+3 <= 2*lambdaLB*2^16`；速度门比较`(2U+2a+3)^2 <= 4*lambdaLB*120^2*2^32`，均checked U128且最大中间量小于2^116。这样无需宽度偷偷升级或先对负余量平方。F02 tighter certificate在同整数尺度只可收紧，不能放松原门。

首个满足压力/速度证书及规范的V-cycle冻结，后续uniform NOOP；16轮仍失败分别PRESSURE_NO_CONVERGENCE/RESOLUTION_LIMIT。PROJECT先整型相邻pi差和h^-1，再按符号/幅值转FP32除2^32和120J。normal/alias定序封存后FLOW_AUDIT从实际输出FP32重新算D u和投影恒等式，绝不拿rP代替实际散度。

## 5. persistent同步、数学函数和设备资格

VISC_BLOCK/PRESSURE_V严格glDispatchCompute(1,1,1)、local_size_x64，lane跨数组stride；每轮写SSBO后memoryBarrierBuffer再barrier。错误由共享firstFailure/active统一传播，所有lane完成同一屏障序列，不能某lane提前return。跨组归约仅在独立PARTIAL→FINAL两pass用API barrier，不跨组自旋。每producer dispatch后SHADER_STORAGE屏障，copy加BUFFER_UPDATE，draw读pack/state加VERTEX_ATTRIB；fence只在root尾，map正常仅1024B尾。

数学backend固定为math表的GPU SOFT_BINARY128_CONSTANT_RANGE_REDUCTION，只服务冷解析构造、标量驱动/函数与系数证明，主速度算子仍T605 FP32、压力仍Q32。U32 limbs实现sign/exponent15/significand113，乘积U256，除法逐bit restoring、guard/round/sticky RNE；不要求native shader double。软件basic op输入先分类，0/非有限/溢出显式处理；no contraction/reassociation，零输出规范+0。

pi/ln2及512-bit 2/pi由构建期有理区间生成，JSON给RNE位型和系数；不是产品物理oracle。有限原始theta0先用整数512 reciprocal做quadrant reduction，再叠加由U64 accepted tick/480算的驱动相位，不能先把大theta/时间压成FP32。RAMP中x=2pi*t/T，theta积分用Om*T/(4pi)*(x−sin x)，小|x|<1/4用x³级数避免相消；表中sin25/cos24在[−pi/4,pi/4]、exp20在ln2归约域内求值并恢复象限/指数。制造解exp输出不可表示时具名NUMERIC_DOMAIN，不以静默underflow冒相对误差合格。正好0及端点精确分支。

这给出了固定常数、阶数和实现策略，**不把尚未实现的shader宣成已满足误差门**。Taylor截断上界分别2^-102/2^-97/2^-96，实际GPU软件运算/归约/转换还须独立核sin/cos≤2^-21、exp≤2^-20；输入函数误差和物理场误差分别记录。固定空间正弦可在冷GPU生成一个h相关quarter-wave表，最多129F32，驻原4096word系数区，按镜像索引重用；不加root dispatch或CPU逐格函数上传。

具体资格层为CAP_COMPUTE（4bindings、64lane、shared≤4096、最大单块/组数/对齐）、CAP_FP32（声明精度/有限性/次正规及重放行为）、CAP_MATH（同程序代函数/构造证据）、CAP_SCENE（attribute/texture/FBO）、PRESENT实际Swap。小probe只能作实测样本，不能替全域函数误差分析或独立F05。编译/ComputeReady/SceneReady/Present分开，不因vertexSSBO0拒可行compute，不因compute通过宣可见；缺资格保持UNKNOWN/UNSUPPORTED，不换CPU/Canvas。

GLSL精度和屏障依据 [Khronos GLSL ES3.10规范](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf)，本项目的soft数学精度、求解正确性及耗时仍由上述合同和独立证据判定。

## 6. 完整提交顺序和失败原子性

dispatch表逐行权威：root前5pass ROOT_INIT/INPUT_GATE/BASE_AUDIT/WINDOW_SNAPSHOT/ACTUAL_BEGIN；ACTUAL J1/J2/J4各4trial初始化后依次1/2/4个micro；ROOT_REPLAY_BEGIN封实际scope摘要并从同BASE重新开始；REPLAY重复3trial；最后6pass SCOPE_COMPARE/FINAL_FIELD_AUDIT/TIME_PREPARE/PACK_VIEW/PACK_AUDIT/COMPLETION_SEAL。合计14*56+24+12=820，不加隐藏authority rerun。56为32固定+8黏性block+16V-cycle；内部工作扫数8L+28及128次黏性迭代另计，不能以dispatch数量冒工作量。SCOPE_COMPARE时binding0/1绑定CANDIDATE/REPLAY；PACK_VIEW/AUDIT改绑0为只读候选场、1为候选RenderPack，2/3仍WORK/CTRL，绝不同时索要第五binding。

trial初始化固定TRIAL_GATE/RESET_FROM_BASE/INIT/ENTRY_AUDIT；失败J的candidate、外功credit、窗口暂账、tracer和源状态全部恢复同BASE。首个全部micro成功trial唯一权威；后续trial/已收敛迭代仍按表提交但所有lane uniform NOOP，保持真实计数和未执行标记。重放必须再现拒绝前缀/J/首错和成功完整字段。CANDIDATE与REPLAY的整个state逐word比较，不能只比hash；micro trace除scope标识外的数值、驱动、阶段、失败与sealed前缀逐word对比，native/GPU计时等非确定观测另存不参与物理等价。未用记录先零，不给后继空结果覆盖首错。

SOURCE按表16pass：输入/BASE/去重→实际copy/install/audit→重放copy/install/audit→比较/控制准备/零时间审计→PACK/AUDIT/尾seal。重复source native返回ALREADY_CONSUMED或GPU按原seq验证，不能二次推进控制。仅正式completion成功后pop FIFO；被control generation撤销的旧队头给STALE_RETIRED回执，不冒source consumed。源/quantum共享完整permit键，不混run/page/geometry/permission。

每micro按T605原方程登记真实operator冲量、墙反作用、外功/耗散；ledger先实际操作数积分，再核DeltaP−sumJ与能量式，不能用残差回填墙力。固定120个**成功quantum**窗口，Escale=max(窗口初K,累计abs外功,Area*2^-20)，试探/重放/失败不刷新额度，不用后续耗散补先前超额。正功额度1，负功不退款；nu0、无功/rest、壁功及phi牵引分域。

GPU TIME_PREPARE仅在私有候选写proposed commit/acceptedQuantum+1及tracer年龄+4tick总量，每micro为4/J；SOURCE年龄/时间均不增。首err原子锁定，pending body/seal分离，尾seal最后写。tail completedNativeNs字段在无GPU原生时钟时为0/NOT_MEASURED，真实Poll/fence时间在native外部收据，不伪填成shader完成时间。

requiredAuditMask用layout.auditBits具名，actualMask只由真实对应审计完成置位，未知高位拒绝。QUANTUM要求除SOURCE_CONTROL/SOURCE_ZERO_TIME外的全部原门，ZERO_REST位表示条件已检查（非rest明确不适用）；SOURCE要求BASE/范围/normal/几何/输入hash/控制/零时间/原件重放与pack一致，不能借未跑的黏性/压力bit冒通过。缺required位为MISSING_REQUIRED_AUDIT。GPU候选header的proposed commit/数值有效状态不等于已被native发布，不增加一次热header改写。

native只在唯一尾fence信号后map1024B，核seal、完整身份/nonce/plan/BASE+proposed commit/hash/审计mask/结果和fresh deadline，再原子换CURRENT+pack generation、消费源和配额；GPU私有proposed不是已发布。NUMERIC_BLOCKED/CFL/solver未收敛丢候选和时间、保原CURRENT及失败前缀；重放不等/非有限/溢出/坏身份FAULT。任何取消/晚归来/超时不发布，不把工作完成当物理接受。

## 7. native时钟、批次、租借与恢复

Controller新控制schema，复用V3的配额恒等/permit思想而非其旧二进制字段。CLOCK_MONOTONIC是native根期限/receipt真源；每条UI pulse保session/sequence/raw uiNs/native receive，不能从UI数值拟合epoch或用呈现token授权clock。运动只按已接受物理tick，不需要依赖T599的真实拨动proof链。渲染可30fps节流，UI freshness pulse可继续每vsync；120Hz配额不是120fps。若实际callback/nativeGap超33333333ns仍CLOCK_GAP，不能为30fps改旧门。

每grant最多4Q，source+4Q最多3296次提交、五尾fence/5120B。单Pump最多10顶层op或8ms CPU提交墙钟，continuation保相同nonce/BASE，根绝对250ms不可重置。persistent内部host无法打断，返回后必须重新核全部permit和deadline；单dispatch最坏耗时及活跃/NOOP成本由设备测试报告，不承诺820次一定实时。

单GL owner管理创建/提交/冷捕获/scene/退役；锁内仅固定元数据copy，GL/IO/等待和大hash移锁外、owner回到提交点再核permit。禁止App mutex内再次锁同一mutex的claim observer。GPU fence零等待轮询，pin/lease/retiring真实占账至GPU退休，不能逻辑取消即复用bank。

只有3state/2pack：场景租CURRENT及同代pack，候选/重放bank必须无未退休读者，旧frame draw fence未退则WOULD_BLOCK，不能临时分第四bank/第三pack。冷冻结也pin它们与WORK/CTRL；context loss不能跨context删除旧GLuint或继续用缓存对象。Stop先撤资格、清未提交配额/源并登记retired，再owner drain；旧GPU晚完成只退役，不返accepted时间。

viewport resize或同context surface切换先撤旧scene/root许可，drain后建立新scene geometry及新permission。BASE旧field token仍作为不可变前态；新根必须同时记录accepted BASE identity和当前root/scene identity，仅在run/context/config/网格未变且native具名VIEW_REBIND许可下允许旧surface/geometry作为BASE，ROOT_INIT把新身份写私有header，后续新接受token才可呈现current。未闭合期间只stale旧有效帧。context重建必须新run/合法同版cold seed，不拿上一context GPU名恢复。

## 8. RenderPack、scene与呈现身份

双pack每份64+16Nc words。PACK_VIEW在GPU从同候选取cell-center平均u/v、pi整数转float、phi/固定规范总压力、curl/实际div、固定范围等；16word字典见layout，residual是诊断原量而非“实际压力误差”。pack64word头用render magic/schema、root field token（run/context/surface/geometry/config/commit/nonce）、网格/边界、固定色标和valid seal。PACK_AUDIT核同field token、有限性/范围/容量/错误flags，不能用checksum代替字段资格。

scene采用GPU instanced attributes：每cell4组uvec4/stride64/divisor1，tracer2组uvec4/stride32直接来自已租state；坐标角点/全屏三角由gl_VertexID生成，不CPU逐格建顶点。不要求vertex/fragment SSBO，最多6次draw（背景/场图/箭头/tracer/壁/整帧合成），与compute dispatch分账。颜色范围显式固定，不每帧自动放大误差；箭头和tracer只画真实已接受速度。

TRACER_ADVECT对本micro ENTRY与投影后MAC线性时间插值，RK2，同壁ghost/canonical周期拓扑；TRACER_CHECK发现闭域越界标VISUAL_DOMAIN_ERROR，不clamp掩盖；只有周期诊断wrap。无质量/无沙ID，SOURCE/失败/重放不对外增龄，U32 age溢出具名拒绝。

通过FullFrameTargets完整GPU帧后同lease核资格，再合成/Swap；成功后才换lastGood及immutable PresentedMapping（field commit/pack generation/geometry/uBox/viewport/成功Swap frame）。g13ViewMode只换显式mode/固定色标及viewGeneration，原field token不变，完整scene token另含viewPolicySHA；同完整token重复图必须相同。每次getter不递增generation，不能拼旧token+新scope。Swap成功与真实display present验证分域；故障FAULT或显式stale上一有效新管线帧。所有背景/前景/壁/数据提示均XComponent+shader，禁止Canvas/软件GL回退。

ViewPolicy64见layout，明确每种色标min/max和大小参数、hash、自零规则及保留字；要求有限且min<max，初始速度0..2、动态压力−64..64、涡量−256..256、总压力p_ref±128，饱和色必须有固定图例，不能据当前场自动缩放。字体/状态提示可用shader内有界procedural glyph，不能隐加未记账图集。30fps成本同时分列新QUANTUM场帧、SOURCE控制帧、重复帧及STALE帧，不能刷source或重绘旧token凑物理推进。

## 9. 冷证据、reader与可执行测试入口

冷schema G13独立，prefix65536B，ROOT目录PREFIX/BASE/CANDIDATE/CTRL，FULL再含REPLAY/WORK/双pack；各段完整原字节与SHA，固定段号不按执行临时删段。prefix前256word header，后32×32word descriptor；其后config128、原/规范输入capsule1024、能力/编译构建/数学/scene/quota/permit收据，剩余0。descriptor32固定kind/schema/index/flags/bank/stride、offset/bytes U64、有效planned/sealed行、commit/nonce、SHA8及seal，不套旧V5 reader。

prefix自身SHA沿显式自零8word规则（header[248..255]），prefix descriptor的rawSHA8固定0，由header的canonicalPrefixSHA授权；其它段全部hash完整原始字节。原始prefix SHA及payload/page/file SHA放外部最后manifest，不能自引用。header/descriptor未知字段拒绝，offset/大小/页序/行对齐/全长hash/零尾checked；源码/布局/数学/门/配置/seed/输入/fixture/设备和根scope全部绑定。FORENSIC字段不可冒可恢复CURRENT，封存前缀与未进入/NOOP分开；私有PROJECT scratch更不能作生产CURRENT。

使用现有shared broker route13（新route号）、route raw总预算11510272B，两槽/单builder/单reader/TTL60s不续。先native/GPU pin联合预约，再owner root-idle安全点冻结，临时阻止新claim，映射实际bank/CTRL/WORK/pack到已预约caller-owned span；不分额外GPU FULL bank，不热每帧读场。copy全部完成后释放GPU pin恢复调度，native不可变raw留给reader；逻辑Trim/TTL不退仍有reader/DETACHED/GPU退休信用。期间配额/暂停/超载照新控制账登记，不追补失去fresh的时间。

页≤1024U32且UTF8≤16384，目录/尾不足按真实bytes；`pageMax=1+ceil(rawBytes/4096)+32`，归档UTF8≤(pageMax+1)*16384，按worker流写/末manifest/hash/原子rename完成才ARCHIVED。GL/map/unmap/上下文/IO/限额分别保首错，页必须返回真实words数组，不能像旧G11/G12只回words数量。out-of-order/重复/越界/过期保精确结果，不悄悄换last档。

engineering.API是真实新NAPI绑定清单，BeginSession/SetActive接页面/Ability生命周期，onLoad注册XComponent native callbacks；配置可先返回QUEUED，真实context/初态/资格齐才Ready。除页面操作外，独立产品C++入口 `Configure(config,seed,identity) / Offer(input,receipt) / BeginRoot(permit) / PumpBounded(now) / Poll(Completion256) / AcquireScene / ReleaseScene / CaptureSpan` 供tester用已有EGL设施创建COMPUTE_ONLY执行器。headless记录COMPUTE_ONLY_NATIVE_CONTROL与真实native定时pulse/permission、同250ms/GAP；不伪造UI事件或呈现，不能拿该吞吐冒带scene30fps。tester维护CLI/夹具和签名部署，无需developer写测试。正常配置REST/GPU解析seed或一次性已审核同版冷态；首次GPU完整初态审计才CURRENT。一次性fixture原始u/v/pi/tracer或PROJECT scratch可冷上传到既有bank，不形成CPU时间步/逐帧field驱动。

PRIVATE_PROJECT诊断保独立mode：允许非div-free的uStar只进scratch，不作生产CURRENT；复用真实压力核，非适用NS阶段按固定表uniform NOOP，结果DIAGNOSTIC_COMPLETE、accepted时间0、publicationEligible0。F03/J1/2/4时间收敛诊断可显式forcedJ，早J为NOT_REQUESTED而非假失败，仍固定提交820；生产forcedJ必须0。负控仅单独GRS_G13_TEST_HOOKS构建/身份可用，不能把注入实现开在产品route。

| 例子 | 真实执行与必须取回的原件 |
|---|---|
| F01/F09/F10 | REST/gravity-only/corotating/periodic种子，实际场bits+pi/phi+canonical alias；无流不等失败。 |
| F02 | PRIVATE_PROJECT冷scratch→真实MG/PROJECT核，独立连续梯度输入；压力/规范/投影和tight证书，诊断不发布。 |
| F03/F06/F07 | Fourier/Couette/Poiseuille正式边界/nu与一次性解析seed；独立闭式/有界级数比速度/壁剪切/压力/能量；不读产品结果造期望。 |
| F04/F05 | TG/闭盒MMS完整NS，原始forcing参数与逐点采样、accepted区间、真实压力/速度、残差和空间阶全部核；F05未测风险不豁免。 |
| F08/F12 | 实际RAMP/力时段/制动，真实theta/Omega/alpha与算子外功/耗散；不是仅播放动画。 |
| F11/F13/F14 | 显式坏边界/兼容性/符号/alias/solver/cancel/late/精度/资格，原firstError和sealed前缀；失败无CURRENT。 |
| F15/F16 | 同accepted lease冷场/pack/重复token/resize与真正新帧；root分位、推进比、活跃/NOOP、GPU块耗时/提交/退休/内存分账。 |

所有runtime及解析/时空/能量/成本门完整保存在engineering.gates，独立验算从原场/原始输入得到，不能只复述tail绿灯。GPU分段计时优先真实timer-query扩展及disjoint标记；不可用标NOT_MEASURED，由tester选已有有界设施精确补证，不把host提交墙钟冒GPU耗时，不为插桩改变生产dispatch/期限后宣原成本通过。

## 10. 接力与本轮检查

T608完整实现上述实际kernel/owner/唯一bridge/scene/cold并编译，不能仅交schema/helper或把未实现包装READY；不等待旧T596/T599，也不改旧1–11。T606独立基准可继续；T609/T610在实现齐后沿已有构建签名/部署/执行设施做独立定量及观测，不增加developer测试门。T611须等实际目标13可信门成立，不能以工程或测试报告交付替代产品PASS。

本轮脚本只生成/核五profile字节区间、word覆盖、820/16/3296顺序计数、已有四档资源等式、有理常数区间及本地文档链接，不导入designer checker、不演化流体、不编译/跑产品测试或使用真机；收据见 [有限工程检查](analysis-T607-engineering-evidence-v1.json)。并存旧线产出及其失败不改写。

所有执行prompt明确：不允许修改`D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态例外）或滴答同名清单；要么完整完成当前卡约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止，局部受阻先做其它可做部分。阻断原卡列全部未完/原错复现/证据/尝试/不能继续原因/所需输入前置及接手角色，成功结束待验收，失败/关键缺证看评论。CPU停投；developer仅开发编译，已有签名部署执行采证归tester/effector，manager登记/推进不增验收门。
