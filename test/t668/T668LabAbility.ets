import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
export default class T668LabAbility extends UIAbility {
  onWindowStageCreate(stage: window.WindowStage): void { stage.loadContent('pages/GpuResourceLab'); }
}
