import { router } from '@kit.ArkUI';

@Entry @Component
struct Index {
  @State private message: string = '';

  build() {
    Column({ space: 16 }) {
      Text('流沙画').fontSize(24)
      Button('进入 GPU 沙水体验').onClick(() => {
        router.pushUrl({ url: 'pages/GpuSandWaterExperience' }).catch(() => {
          this.message = 'GPU_EXPERIENCE_ROUTE_FAILED';
        });
      })
      Text(this.message).fontSize(12)
    }.width('100%').height('100%').justifyContent(FlexAlign.Center)
  }
}
