import native from 'libgrsm1.so';
import { common } from '@kit.AbilityKit';
import { GpuInputAttempt } from '../physics/GpuInputAttempt';
@Entry
@Component
struct T669Probe {
  @State message: string = 'T669 same-provider input observations; not a product PASS';
  private call(route: string, api: string, words: Uint32Array): void {
    const context = getContext(this) as common.UIAbilityContext;
    this.message = GpuInputAttempt.call(route, api, words, 'T669_TYPED_NEGATIVE_V1', context.filesDir,
      (value: Uint32Array): string => route === 'M3A' ? native.m3Configure(value) : native.g9ResearchConfig64(value),
      (state: string): void => { this.message = state; });
  }
  build() {
    Column() {
      Text('T669 拒因与同attempt原件').fontSize(18)
      Button('M3 错长度').onClick(() => { this.call('M3A', 'm3Configure', new Uint32Array(64)); })
      Button('M3 正长度坏版本').onClick(() => { this.call('M3A', 'm3Configure', new Uint32Array(16)); })
      Button('M3 非零offset').onClick(() => { this.call('M3A', 'm3Configure', new Uint32Array(new ArrayBuffer(68), 4, 16)); })
      Button('G9 错长度').onClick(() => { this.call('G9', 'g9ResearchConfig64', new Uint32Array(1)); })
      Button('G9 坏正文').onClick(() => { this.call('G9', 'g9ResearchConfig64', new Uint32Array(64)); })
      Text(this.message).maxLines(6)
    }
  }
}
