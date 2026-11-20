import native from 'libgrs_g13vk_boot.so';

@Entry
@Component
struct G13VulkanBoot {
  @State private result: string = '';
  private timer: number = -1;

  onPageShow(): void {
    this.result = native.g13VkBootStatus();
    this.timer = setInterval(() => { this.result = native.g13VkBootStatus(); }, 250);
  }

  onPageHide(): void {
    clearInterval(this.timer);
    this.timer = -1;
    native.g13VkBootStop();
  }

  build() {
    Column() {
      Text('G13 Vulkan BOOT · FP32 资格').fontSize(20)
      XComponent({ id: 'g13-vulkan-boot', type: XComponentType.SURFACE, libraryname: 'grs_g13vk_boot' })
        .height('20%').width('100%')
      Row() {
        Button('运行资格探针').onClick(() => { this.result = native.g13VkBootStart(); })
        Button('停止').onClick(() => { this.result = native.g13VkBootStop(); })
      }
      Scroll() {
        Text(this.result).fontSize(12).copyOption(CopyOptions.LocalDevice)
      }.layoutWeight(1).width('100%')
    }.width('100%').height('100%').padding(12)
  }
}
