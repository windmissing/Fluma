import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import native from 'libgrs_g13vk_boot.so';

export default class G13VulkanBootAbility extends UIAbility {
  onWindowStageCreate(stage: window.WindowStage): void {
    stage.loadContent('pages/G13VulkanBoot', (error) => {
      if (error.code !== 0) console.error('G13_VK_BOOT_PAGE_LOAD_FAILED:' + String(error.code));
    });
  }

  onBackground(): void { native.g13VkBootStop(); }
  onWindowStageDestroy(): void { native.g13VkBootStop(); }
  onDestroy(): void { native.g13VkBootStop(); }
}
