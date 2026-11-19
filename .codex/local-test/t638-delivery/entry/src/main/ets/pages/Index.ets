import router from '@ohos.router';
@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Column({space:16}) {
      Button('单沙＋明水 C V1').onClick(()=>{router.pushUrl({url:'pages/ClearWaterDemo'});})
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize(50)
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
    }
    }
    .height('100%')
    .width('100%')
  }
}