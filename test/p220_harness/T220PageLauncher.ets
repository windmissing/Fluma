// T220 isolated clear-water B device harness launcher (test-only, follows V3 launcher structure).
// Want params: t220runId, t220scenario(matrix|land|tilt), t220k.
// No production page/entry modification: this file is copied into the isolated harness project only.
import worker from '@ohos.worker';
import hilog from '@ohos.hilog';
import common from '@ohos.app.ability.common';
import Want from '@ohos.app.ability.Want';
export class T220PageLauncher{
  private context:common.UIAbilityContext|null=null;private launch:Want|null=null;
  private physics:worker.ThreadWorker|null=null;private started=false;private shown=false;
  configure(context:common.UIAbilityContext,want:Want|null):void{this.context=context;this.launch=want;this.start();}
  pageReady():void{this.shown=true;this.start();}
  private start():void{if(!this.shown||this.context===null||this.started)return;this.started=true;this.run();}
  private async run():Promise<void>{
    try{
      if(this.context===null)throw new Error('T220_CONTEXT');
      const context=this.context,p=this.launch?.parameters;
      if(p===undefined||p['t220runId']===undefined)return; // not a T220 launch; idle page
      const runId=p['t220runId'];
      if(typeof runId!=='string'||!/^[a-zA-Z0-9_-]{1,64}$/.test(runId))throw new Error('T220_CONFIG_INVALID');
      this.physics=new worker.ThreadWorker('entry/ets/workers/T220Worker.ets');
      this.physics.onmessage=(ev:object):void=>{hilog.info(0,'T220','%{public}s',JSON.stringify({result:ev,uiReceivedUptimeMs:Date.now()}));};
      this.physics.onerror=(ev:object):void=>{hilog.error(0,'T220','WORKER_FAULT %{public}s',(ev as Error).message);};
      this.physics.postMessage({runId:runId,scenario:p['t220scenario']??'matrix',k:Number(p['t220k']??192)});
    }catch(error){hilog.error(0,'T220','FAULT %{public}s',(error as Error).message);}
  }
}
export const t220PageLauncher:T220PageLauncher=new T220PageLauncher();
