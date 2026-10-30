# T280 GPU M1 整步提交核交付

2026-10-30，developer，原卡 `6ae34b05e4b0f69bc5642ef9`。消费T237/T238及T279 d7821a3。不使用真机。

## 实现与接受边界

新增 `FullM1Transport`，冷建新空run，独立组合SOURCE/QUANTUM shader身份。SOURCE兼容模块仍仅接受原source-only身份；完整模块使用 `full_source_audit.glsl`，允许已有包的真实运动/部分LAND/退役，核累计源终态/sourceEvent16/来源区间/双相U64/physics=source+quantum身份，SOURCE本次写集仍由原完整spare replay证明。冷参数g/emit/settle/drag/kappa/soft/flux固定，逐quantum只允许请求角变化；值仍待定标，不宣已验证速度律。

QUANTUM候选后执行全相audit、base→独立spare复制、逐event64重放、metadata、复制未接受authority bank、pack publication、独立publication审核、completion256尾封。入口与末审核S/P/W/J/R、全相U64、有限整数域、bh=0/单H、容量、stock folds、守恒、角投影/Hq605394、300/s与28/s预算、pool/free/generation/ID/来源/保留位、settled/flight/arrival/quantum计数。replay从base消费本root流：kind/used-mask、完整原身份/旧值、按ID/edge/arrival确定序、每slot48/16上界与τ连续、冻结面重派生、T16/RNE/ceil、TURN最早时刻/最早平面tie、WAIT余时、预算/容量/guard、LAND/ARRIVE材料身份、释放/末接触及精确patch；最终所有authority/派生/header逐word等于候选，才得审核1023。字段污染/漏行/多行/错误旧值/末计数不一致fatal，source/此前quantum已接受前缀不撤销。

`SourceRootOwner`新增独立QUANTUM分支；原SOURCE接受门保留。fence前绝不切bank。尾fence后仅映射1024B，验证256长度/魔数/revision/seal/root类型、完整epoch身份/nonce/control/frame、版本与三SHA、stage255/audit1023/首错/重放哨兵、GL、commit/source/ledger不变、双相总量、settled+flight、arrival/quantum、事件计数/真实pass与上传字节、publication槽与generation、容量/guard/预算摘要。一次owner操作更新bank/header/receipt/publication lease。取消tombstone的迟到fence不映射/不接受；故障停frame后续root，必须新run。context lost全丢失，不能从摘要恢复池。

双frame arena按frame奇偶保留；同frame SOURCE与已成功QUANTUM前缀在GPU原位/拷贝保留，root ordinal全局追加，eventStart单独封。publications双槽，拒复写当前接受publication槽，另槽被lease/releasePending则背压；generation/释放fence退休后才能复用。候选失败前从不改已接受bank，未接受pub也不可Acquire。普通路径无逐包CPU推进/整pool读回；显式Freeze/Capture才冷导出authority/event。

水ID按T237正式原文：MAX分配一次后next保持MAX/exhausted=1，后续新包ID_EXHAUSTED，DIRECT_W无需包仍可用。此策略仅FullM1组合模块启用；T240 source-only已交模块的0哨兵兼容行为保持，沙ID沿T259=0哨兵。全M1新run不导入旧run。该差异已在原卡评论说明，不将source-only水ID表现当正式全M1合同。full-source拒绝回填已知量化requested到rejectedUnits，保持真实外源分母。

## 实测与复跑

PC ANGLE/SwiftShader真实GLES，四档native各 **465/0（合1860）**；包含SOURCE→QUANTUM→后续SOURCE、J→W、water MAX/耗尽/DIRECT_W、池/容量拒绝真实分母、同frame多quantum、持有lease背压/旧generation拒绝、完整publication、正常尾包逐字段篡改/长度/GL拒绝、Stop迟到保持前缀，以及14类GPU事件/候选/ledger/pub/tail污染反例，全部拒接受并保旧publication。另独立整数oracle与完整GPU replay四档各38例，合 **152/0**；原SOURCE兼容回归 **562/0**。T279 fixture接口仅host测试宏；生产无全池导入入口。测试冷等待延至180秒只供软件GPU冷JIT，不改120Hz/33.3ms/F≤4或物理dt。

| K/Kw | 稳态buffer实际payload字节 | QUANTUM dispatch/barrier |
| --- | --- | --- |
| 192/64 | 23,707,088 | 113/113 |
| 192/128 | 25,826,256 | 127/127 |
| 512/64 | 55,432,912 | 122/122 |
| 512/128 | 57,552,080 | 136/136 |

实际公式 `35+L(Kpad)+2L(Kw)`，包括候选失败恢复、spare、审核/重放、authority copy、pub审核及seal；每pass重新四binding并storage/update barrier，一个尾fence。热CPU上传逐glUniform计字节，四档1260/1372/1368/1480B（按相同档序）；真实逐pass/stage/groups/root/owner/bytes/rawGL/wall耗时有界trace（每root重置，溢出Dropped）。无GPUtimer数据，不填0冒测量。SOURCE仍8pass/9barrier/一个尾fence。buffer数含独立SOURCE资源、候选、扩展WORK/spare、双frame arena、pub/tail；不含driver/JIT/RSS、冷resize共存和T281 scene，不宣设备内存/性能收益。

复跑：`prepare_t280_commit.py`→`build_t280_replay.py`→生成build.cmd与`check_t280_replay.py K Kw`；`build_t280_full.cmd`→full.exe K Kw；`archive_t280.py`。证据 `test/reports/t280/delivery.json`、`full-*.txt`（逐pass trace）、`replay-*.txt`、`source-regression.txt`、`package.log.gz`。组合SHA `6c476dcf7597378c7d072251a15fe2cd8ab362339fdc47bff8505f17c17a96d6`，CMake与host生成字节完全相同。hvigor **BUILD SUCCESSFUL**；signed HAP SHA `29a73219ed0aa3ac7856ff0379e02fb938171d6c26b4a929b6b665a6f7857c9e`，签名/verify PASS，文件 `.codex/local-test/t257-sign/T280/T280-signed.hap`。

本卡仅完整root接受/publication lease。默认产品M1路由OFF、full profile Plan仍未开放scene配置；T281需消费新publication water20/column16与组合SHA，不套M0 reader。未用机、未做scene像素/CURRENT/收益/独立tester验收。所有渲染边界仍XComponent+shader，故障FAULT/完整旧token显式STALE，禁Canvas。
