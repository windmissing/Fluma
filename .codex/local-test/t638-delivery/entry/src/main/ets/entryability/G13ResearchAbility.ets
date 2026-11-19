import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import common from '@ohos.app.ability.common';
import Want from '@ohos.app.ability.Want';
import native from 'libgrs_g13.so';
interface G13AbilityStatus { scope: Object; }
// Used only by tools/g13/prepare_build.cjs. The existing EntryAbility/default
// routes and old native libraries are not changed or loaded by this artifact.
export default class G13ResearchAbility extends UIAbility {
  onCreate(want: Want): void {
    AppStorage.setOrCreate('t630probeFlag', `${want?.parameters?.t630probe}` === 'true');
    AppStorage.setOrCreate('t654probeFlag', `${want?.parameters?.t654probe}` === 'true');
    AppStorage.setOrCreate('t654Attempt', `${want?.parameters?.t654attempt ?? '110'}`);
  }
  onWindowStageCreate(stage: window.WindowStage): void {
    stage.loadContent(AppStorage.get<boolean>('t654probeFlag') === true ? 'pages/T654Probe' : AppStorage.get<boolean>('t630probeFlag') === true ? 'pages/T630Probe' : 'pages/GpuResidentG13', (error) => {
      if (error.code !== 0) console.error('G13_PAGE_LOAD_FAILED:' + String(error.code));
    });
  }
  private revoke(): void {
    try { const status = JSON.parse(native.g13Status()) as G13AbilityStatus; native.g13Stop(JSON.stringify(status.scope)); }
    catch (error) { console.error('G13_LIFECYCLE_REVOKE_FAILED:' + String(error)); }
  }
  onBackground(): void { this.revoke(); }
  onWindowStageDestroy(): void { this.revoke(); }
  onDestroy(): void { this.revoke(); }
}
