# T636：跨 DSO broker 票据与信用的可验证闭包

2026-11-19，architect，不使用真机、不运行broker/GL/构建。结论：已交最终broker的8个exported C ABI足够tester独立跨DSO harness；T627旧包实际只有5个host符号，必须分版本。无需生产只读新endpoint或可改信用开关，不要求grsm0/grsm1同时跑GL。方案机器表`architecture-T636-cross-dso-harness-v1.json`。

## 原证据能证明什么

T627 maps有grsm0、grsm1和共享broker，各4个文件映射+1个匿名bss；broker文件映射同device/inode，bss行无文件inode。说明该采样进程只观察到一套broker映射，不能把五行都叫相同inode，也不能仅此断言每route已调用API。timeline开始count34，9个分页JSON实际累计35行，manifest诚实标GREW_DURING_EXPORT；不冒稳定34行完整快照。Init/Shader/Program/Resource只证明该创建路径执行，未携ticket/Reserve/Read/Freed调用名，不证明cold slot链经过broker。保加载共享窄结论，真实交叉操作仍未测。

T617 host组件21条输出保历史PASS，但覆盖语义需收窄：R10是CHECK(true)没有状态断言；R12传routeBudget>进程cap，会先被参数门拒，未隔离累计process cap；部分route预算/two-slot/reader测试有更早门同时为false。源码实际是**最多一个builder、最多一个reader**；不同slot允许builder与reader并存，不是全局两类互斥。不能照报告词面写错新测试。

对实际ELF dynsym的额外核对：T600/T627签包内broker SHA=`c103ffeae21a40e43c4c774079b2f77203a43515de0a4deab9eeb2f6ed95e9bc`，只导出下表前5个host函数；不是当前含GPU票据的8符号版本。现成T608最终release broker SHA=`715f314d43549d4eed991922f25faa350fedcaf6ab8dab1598abc980dd278d03`已含全部8符号，路径见development.json，无须新产品开发/编译来补。不得对旧包要求不存在的GPU符号，也不得把旧maps当新DSO加载证据。

## ABI 与构建形态

直接使用cold_slot_broker.h的：

```cpp
bool GrsColdReserve(uint64_t* ticket, size_t bytes, size_t routeBudget,
                    uint32_t route, uint64_t* request, uint64_t* trace);
void GrsColdBuilderDone(uint64_t);
bool GrsColdReadBegin(uint64_t);
void GrsColdReadEnd(uint64_t);
void GrsColdFreed(uint64_t);
bool GrsFullGpuReserve(uint64_t* ticket,uint64_t context,size_t bytes);
void GrsFullGpuFreed(uint64_t);
void GrsFullGpuContextRetired(uint64_t context);
```

tester所有权：新`test/t636/{adapter_a.cpp,adapter_b.cpp,driver.cpp,CMakeLists.txt,receipt.py}`。两adapter各编为独立aarch64 DSO、各DT_NEEDED同一**原生产libgrs_cold_slots.so字节**及其匹配归档头；优先选上述已交最终8符号版本完整执行矩阵，原件SHA匹配T608 final收据。driver dlopen两者，记录dlerror、dlsym实际函数/模块归属、**本次**maps及ELF BuildId/SHA。不得把broker.cpp静态编进两个adapter、LD_PRELOAD替换、用host组件冒设备DSO。独立测试进程私有目录部署提取的同字节broker和依赖，无需GL；若用旧5符号包只测host域，GPU域NOT_AVAILABLE_IN_THIS_BUILD，不将不同build强拼。设备禁止独立exe时由tester打包同一harness到测试Ability，仍不需生产接口。

全部调用以B前缀wrapper操作A取得的票据，反向亦然；不只是两个wrapper各自Reserve。aarch64 size_t/U64遵实际头；票据与context日志十进制字符串，wrapper结果保原bool和所有输出指针前/后值，失败不假设输出被归零。新进程只作测试charges，不分配数百MiB来凑累计cap；此域是BROKER_COMPONENT_CREDIT，不宣真实payload/driver/产品route集成。

## 必须隔离原因的顺序矩阵

1. A Reserve(4096,budget8192,routeA)，B在有空slot且总量足够时Reserve拒，隔离单builder门；B BuilderDone(A票据)，B ReadBegin(A票据)成功。
2. reader A仍持有时B Reserve另route的4096成功，证明跨slot builder/reader可并存；B自身builder期ReadBegin(B票据)拒。B BuilderDone后，再ReadBegin(B票据)仍拒，才隔离全局单reader门。
3. B对reader持有A票据Freed，第三Reserve在无builder时仍拒；A ReadEnd后再次ReadBegin(A票据)成功，证A票据仍存在、ReadEnd不自动补Freed；ReadEnd，再B Freed(A票据)。第三Reserve现在成功且票据为新代。释放所有reader/builder后真实调用Freed，旧票据ReadBegin拒。不得用CHECK(true)替代保留状态证据。
4. 独立清空后，A同route4096/8192→BuilderDone；B同route4097/8192拒（只占一slot，route总量8193），4096成功等于8192。清理后换不同route，A charge=524375552−4096，budget=524375552；B charge4097拒/4096成功，隔离真实累计process cap，不用非法routeBudget遮蔽。
5. GPU-ticket分域：A opaqueContextA reserve bytes1，B第二reserve拒；错误ticket Freed、错误context Retired均不放行；B正确Freed(A票据)后B成功；B ContextRetired(本测试context)后A成功。context仅隔离组件值，**没有真实eglDestroyContext**，不冒driverfree/生产丢context证据。

每阶段用本进程已拥有票据显式清理，无ResetAll接口，无对生产进程猜票据释放；异常只收本attempt证据并退出独立进程。loader/票据调用之间wall/mono/pid-start身份完整，顺序号≤128条、每条≤512B、日志≤64KiB、manifest≤16KiB、总运行≤30s；所有外部产物有SHA，清理仅校验过的本attempt目录。各拒绝通过去掉唯一前置再成功作成对证明，不以单个false倒推拒因。

## 是否还需生产新快照

当前任务要证明跨DSO共享实际操作和额度，以上ABI已足够；**不派developer新快照**。新测如果符号未导出/目标ELF不可加载，tester保实际dlerror/NEEDED/dynsym/文件SHA，回architect定位包/ABI，现成签名部署/夹具归tester。若将来要问“生产route某次cold是否真的Reserve了哪个ticket”，那是另一个集成域，需要准确生产调用关联证据；本次harness不假充这一层，不把它加作组件或开发关闭门。driver物理free另域不误派。

T627按此独立harness续办；既有maps/host结论保留、强覆盖断言收窄。完整执行约束见`architecture-T633-T636-execution-boundaries-v1.md`，本次架构交付不冒新设备PASS。
