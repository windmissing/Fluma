// T630 tester probe page (test facility only; not a product page).
// Sends same-provider typed arrays of different element kinds/sizes through
// the REAL native.m3Configure / native.g9ResearchConfig64 bindings and shows
// the raw reply plus the diagnostic elementKind/elementLength, to measure the
// actual napi_get_typedarray_info length unit (ELEMENTS vs BYTES) on device.
import native from 'libgrsm1.so';

@Entry
@Component
struct T630Probe {
  @State log: string = 'ready';

  private call(name: string, fn: () => string): void {
    try {
      const reply: string = fn();
      this.log = `${this.log}\n[${name}] ${reply.slice(0, 1400)}`;
    } catch (e) {
      this.log = `${this.log}\n[${name}] THREW:${String(e)}`;
    }
  }

  build() {
    Column({ space: 8 }) {
      Text('T630 typed length-unit probe').fontSize(18)
      Scroll() { Text(this.log).fontSize(11).width('96%') }.height('58%')
      Button('Uint8Array(64) -> m3Configure').onClick((): void => {
        this.call('u8(64)', (): string => native.m3Configure(new Uint8Array(64)));
      })
      Button('Uint16Array(32) -> m3Configure').onClick((): void => {
        this.call('u16(32)', (): string => native.m3Configure(new Uint16Array(32)));
      })
      Button('Uint32Array(16) -> m3Configure').onClick((): void => {
        this.call('u32(16)', (): string => native.m3Configure(new Uint32Array([4, 15, 192, 64, 7680, 8388608, 1048576, 65536, 1048576, 0, 0, 120, 524288, 1048576, 1, 0])));
      })
      Button('Uint32Array(64) -> m3Configure').onClick((): void => {
        this.call('u32(64)', (): string => native.m3Configure(new Uint32Array(64)));
      })
      Button('Float32Array(16) -> m3Configure').onClick((): void => {
        this.call('f32(16)', (): string => native.m3Configure(new Float32Array(16)));
      })
      Button('Uint32Array(1) -> g9ResearchConfig64').onClick((): void => {
        this.call('g9 u32(1)', (): string => native.g9ResearchConfig64(new Uint32Array([2])));
      })
      Button('Uint32Array(4) -> g9ResearchConfig64').onClick((): void => {
        this.call('g9 u32(4)', (): string => native.g9ResearchConfig64(new Uint32Array(4)));
      })
      Button('Float32Array(1) -> g9ResearchConfig64').onClick((): void => {
        this.call('g9 f32(1)', (): string => native.g9ResearchConfig64(new Float32Array(1)));
      })
    }.width('100%').padding(8)
  }
}
