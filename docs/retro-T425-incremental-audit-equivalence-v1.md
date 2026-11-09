# T425 增量审计证明复盘

2026-11-09，designer，CPU参考P4。不使用真机。主稿 `algorithm-T425-incremental-audit-equivalence-v1.md`。

首轮核对 T420/C、T423 实际局部 undo 和 T424 基线。A/B 已减少部分整池复制，公共验证与原数值 fold 仍独立存在。当前可用的性能记录没有同时匹配全部N/L/R，R记账也有缺口，因此本稿只证明操作与工作界，不能外推33.3ms。

比较了四条路线。仅凭contentSerial复用成本低，但未跟踪别名写会漏错。用exact70最终舍入替换所有浮点fold会改变操作图。缓存任意子树sum不能恢复原左结合序列。最终选择分域成功证明和原序后缀重放，异常在原cut走完整验证。

proof更新根据实际tuple及其前后原字节，独立于canonical派生数组。每桶70limb增量闭合可归纳证明；溢出/负数和中间cut的失效仍须逐prefix保留。结构验证需要覆盖所有slot字段，mass-only revision不涵盖vy/y故障。free栈业务顺序、缓存±0和未用槽尾字节各有不同读者，不能用一个“物理量没变”证书归并。

原首错和冻结字节决定采用成功命中。新检查发现某个域疑似非法只令该域uncertified，原函数按原次序确定code/cut/ordinal。支持的干场profile以冷全验为基线，未知input/writer/callback/native边界仍执行原路。T423 before物化和source/子步成功前缀必须纳入新增proof的mark/rollback。

原序fold最坏仍线性。后缀重放只复用已证明不变的prefix，不能借固定N或正常小样本宣称O(logN)。所有发布仍有实际copy/物化成本。typed payload只是新reference单view模板，多view及rollback工作区另账。下一版由architect明确符号级读写闭包，developer实施C1/C2可证部分，tester独立逐cut对拍；若出现真正的数值或首错反例，再回本角色修订。
