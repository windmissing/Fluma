 private armCapture():void {
  if(this.captureCold.busy||this.captureExporting)return;
  let scope:CaptureScope|null=null;
  if(this.launchDeclaration.reason==='SHA_DECLARED'){
   try{scope=JSON.parse(m1.status()) as CaptureScope;}catch(e){scope=null;}
  }
  const attempt=prepareCaptureArm(this.launchDeclaration,scope,this.active,this.frame.session);
  this.lastLaunchAttempt=attempt;
  const config=attempt.config;
  if(config===null){this.captureText=attempt.validation+' '+attempt.invalidField;return;}
  const reply=JSON.parse(m1.beginEvidenceCapture(JSON.stringify(config))) as LaunchCaptureReply;
  attempt.requestId=reply.requestId??'';
  attempt.nativeReason=reply.reason??reply.state;
  attempt.invalidField=reply.invalidField??'';
  if(reply.state==='RESERVED'&&reply.requestId!==undefined){
   this.captureRequestId=reply.requestId;this.captureSha=config.packageSha256;
   this.pendingLaunchAttempt=attempt;
  }
  this.updateCapture(reply);
 }
 private async exportLaunchAttempt():Promise<void> {
  if(this.lastLaunchAttempt===null)return;
  try {
   const context=getContext(this) as common.UIAbilityContext;
   const files=new DiagnosticColdFiles(context.filesDir);
   await files.save('gpu-capture-launch-last.json',JSON.stringify(this.lastLaunchAttempt));
   this.coldStatus='LAUNCH_ATTEMPT_EXPORTED';
  }catch(e){this.coldStatus='INCOMPLETE '+String(e);}
 }
