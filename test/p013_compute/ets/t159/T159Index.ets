import { t165PageLauncher } from '../physics/T165PageLauncher';
@Entry
@Component
struct Index {
  onPageShow(): void { t165PageLauncher.pageReady(); }
  build() {
    Column() {
      Text('T159 isolated transaction test')
      Text('Physics and EGL run in the test Worker. Results are archived as JSONL.')
    }.width('100%').height('100%')
  }
}
