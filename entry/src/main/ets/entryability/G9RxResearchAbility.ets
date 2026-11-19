import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import native from 'libgrs_g9rx.so';
export default class G9RxResearchAbility extends UIAbility {
  onWindowStageCreate(stage: window.WindowStage): void { stage.loadContent('pages/GpuResidentG9Rx'); }
  private stop(): void { try { native.g9RxStop(native.g9RxSession()); } catch (error) { console.error('GNR1_STOP_FAILED:' + String(error)); } }
  onBackground(): void { this.stop(); }
  onWindowStageDestroy(): void { this.stop(); }
  onDestroy(): void { this.stop(); }
}
