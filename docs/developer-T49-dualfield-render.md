# T49 新管线双场渲染开发交付

不使用真机。复用T20/T37已入库e9e2be4：SandCanvas生产路线p011particle=1绑定粒子物理门后，发布field+pool同physicsCommitId；P013RenderController.submitDualField→同一次native DrawDualFieldFrame上传bed/particle两个pass→一次swap。生产为XComponent.TEXTURE，既有Canvas只负责背景/前景；成功GLES帧不再画Canvas沙或B tracer。失败回退不当作新管线成功。独立T51 EmptySandDemo入口将消费同接口，不经过G0色块。

沉积高度场shader只读row0；活动state1/state2来自真实池并携带phase/ID，shader无积分。LAND由物理写床及释放粒子，下一发布包同时反映两者。测试真实空场0粒/0床→注入0.1真实粒→成功子步落床→同一packet无粒且有床，质量闭合；孔洞/state2/拒绝回滚/容量/原子包检查沿用并复测。

本卡最小修复：Float64坐标在转换Float32后可能等于128（原值127.99999999），原native会拒绝；adapter现在预检实际上传坐标域，同时拒绝超uint32的ID，旧包不被部分写入；不夹紧、不伪造ID或丢粒。既有shader/颜色/物理规则未改。

验证：dualfield11/11、P0 20/20、composition6/6、render82/82、tracer9/9，真实native bridge/lifecycle host通过；ArkTS及arm64 native编译通过。原始逐例test/reports/t49-developer-selfcheck.json与t49-host.log。T52负责设备落沙累积/state2及LAND可见性；本卡未操作设备，不把离线packet结果当屏幕PASS。
