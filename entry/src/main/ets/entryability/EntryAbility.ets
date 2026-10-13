import UIAbility from '@ohos.app.ability.UIAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import hilog from '@ohos.hilog';

export default class EntryAbility extends UIAbility {
  hilogTag: string = 'EntryAbility';
  hilogDomain: number = 0x0000;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onCreate');
    // 测试自动化参数（s77）：aa start --ps 传入，零 UI 依赖控制测试状态
    // 用法：aa start -b com.example.helloworld -a EntryAbility --ps testmode true --ps forceinvert true
    const params = want.parameters;
    if (params && params.testmode !== undefined) {
      AppStorage.setOrCreate('testmode', `${params.testmode}` === 'true');
      hilog.info(this.hilogDomain, this.hilogTag, 'want param testmode=%{public}s', `${params.testmode}`);
    }
    if (params && params.forceinvert !== undefined) {
      AppStorage.setOrCreate('forceinvert', `${params.forceinvert}` === 'true');
      hilog.info(this.hilogDomain, this.hilogTag, 'want param forceinvert=%{public}s', `${params.forceinvert}`);
    }
    if (params && params.rendermode !== undefined) {
      // 0=细沙 1=流体 2=混合（调试：细沙模式直接画网格，验证物理层粒子移动）
      const rm = parseInt(`${params.rendermode}`);
      if (!Number.isNaN(rm)) {
        AppStorage.setOrCreate('rendermode', rm);
        hilog.info(this.hilogDomain, this.hilogTag, 'want param rendermode=%{public}d', rm);
      }
    }
  }

  onDestroy(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageCreate');
    windowStage.loadContent('pages/SandCanvas', (err) => {
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