import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import { t165PageLauncher } from '../physics/T165PageLauncher';
export default class EntryAbility extends UIAbility {
  private launch: Want | null = null;
  onCreate(want: Want): void { this.launch=want; }
  async onWindowStageCreate(stage: window.WindowStage): Promise<void> {
    try {
      await stage.loadContent('pages/Index');
      t165PageLauncher.configure(this.context,this.launch);
    } catch(error) { hilog.error(0,'T159','LOAD_CONTENT_FAULT %{public}s',(error as Error).message); }
  }
}
