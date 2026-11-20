import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import native from 'libgrs_g11.so';
import { G11Status } from '../physics/GpuG11Port';
// Explicit isolated product entry only; existing default pages/parents stay unchanged.
export default class G11ResearchAbility extends UIAbility {
 onWindowStageCreate(stage: window.WindowStage): void {
  stage.loadContent('pages/GpuResidentG11', (error) => { if (error.code !== 0) console.error('G11_PAGE_LOAD_FAILED:' + String(error.code)); });
 }
 private revoke(source: string): void { try { const s = JSON.parse(native.g11Status()) as G11Status; if (s.scope !== undefined) native.g11Stop(JSON.stringify(s.scope), source); } catch (error) { console.error('G11_REVOKE_FAILED:' + String(error)); } }
 onBackground(): void { this.revoke('ABILITY_BACKGROUND'); }
 onWindowStageDestroy(): void { this.revoke('STAGE_DESTROY'); }
 onDestroy(): void { this.revoke('ABILITY_DESTROY'); }
}
