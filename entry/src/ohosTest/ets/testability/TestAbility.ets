import UIAbility from '@ohos.app.ability.UIAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import abilityTest from '../test/List.test';

export default class TestAbility extends UIAbility {
  hilogTag: string = 'TestAbility';
  hilogDomain: number = 0x0000;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onCreate');
    abilityTest();
  }

  onDestroy(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageCreate');
    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        hilog.error(this.hilogDomain, this.hilogTag, 'Failed to load the content. Cause: %{public}s', err.message);
        return;
      }
      hilog.info(this.hilogDomain, this.hilogTag, 'Succeeded in loading the content.');
    });
  }

  onWindowStageDestroy(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageDestroy');
  }

  onForeground(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onForeground');
  }

  onBackground(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onBackground');
  }
}