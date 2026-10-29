# T240 source记录与尾包投影（未开放接受门）

2026-10-29，不使用真机。新增7-pass source continuation：GPU sourceEvent16按接受前缀写request/commit/kind/column/admitted/ID边界；publication按M1 water20/column16写GPU数据、完整身份/版本/hash及seal；completion256包含双相总账、settled/flight、source/dedup前后prefix、nonce/frame/control/三bank/pub generation/三项SHA/尾seal。

PC SwiftShader13/0，新增真实sourceEvent16、水publication J与completion256的身份/总账/序号检查；保留此前源/拒绝/重复/篡改回归。ES310编译与native/ArkTS/PackageHap BUILD SUCCESSFUL。证据`test/reports/t240/source-host-v3.txt`、`source-shader.json`、`seal-package.log.gz`。

**仍部分交付，看评论，不满足本卡source闭环终点。**目前auditFlag仅267（stock/ledger/capacity/replay）而非产品要求1023；ID/source/budget/guard/publication全族审核需补全。publication目前只对water state16做写后核对，不声称完整独立pub审核。缺M1 native transport/owner接受门、真实尾fence/1024B校验与lease交接，不能将生成的尾包直接作为ACK。对外Plan仍NOT_IMPLEMENTED，M0不可消费revision2，未启用M1路线。T16与水scene仍另待接力，不作为本卡门；当前留下的是本卡尚未完成的开发，不借设备或新设计前置解释。
