# T257 隔离包签名交接

2026-10-29，developer，不使用真机。按T253复用hap-sign-tool本地授权流程，未修改root/隔离工程签名配置、密码、keystore、设备保护或旧包。工具与原包/新包SHA、签名操作退出码、验证结果见test/reports/t257/；私钥与密码未入日志/仓库。

T248现有unsigned先签，T246新2166eba对应unsigned随后签；两包verify-profile/verify-app均通过，原ZIP所有payload entry hash保持，证书/原包输入SHA均核对。包在.codex/local-test/t257-sign/T248/T248-signed.hap和T246/T246-signed.hap，签成不等于已安装或功能通过。设备仍NOT_RUN。

发现原配置appCertFile首张证书对后续issuer验签失败（工具11013002），但keystore导出的证书与该首张一致；profile内development-certificate则能由同一原issuer链验签，且profile证书/keystore的SPKI SHA完全一致。未更换密钥、生成新证书或改原材料；在临时输出副本中用**现有已验证profile的证书＋原issuer/root链**组装公开证书链。Java X509逐级签名/有效期及最终root自签验证通过，再用原keystore/alias/password签名并verify-app；验证包内profile与同bundle身份、公钥一致。两张证书各自指纹与临时链SHA记录在逐包JSON；不以证书serial差异误判公钥不对应，也不跳过证书链验签。

签名脚本test/t257/sign.py只在内存读取配置；不打印命令参数或工具原始输出，失败信息替换material值，记录退出码/输出摘要。生成的profile/证书/密钥导出仅位于本机未入库目录，交付JSON不含debug设备列表或密码。旧失败JSON留作诊断。

T250新包在后续原卡实现到件后继续逐SHA签，不让其尚未生成阻住以上两包交付；本报告首批交接暂为部分，后续补T250。新代码为签名工具与公开证书校验器，无ArkTS/native业务改动；所签T248/T246对应真实编译PackageHap证据已核。
