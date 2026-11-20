'use strict';

// Isolated T714 comparison. Keep the T701 shader and creation arms intact.
function one(body, anchor, replacement) {
  if (body.split(anchor).length !== 2) throw Error('T714_PARENT_DRIFT ' + anchor);
  return body.replace(anchor, replacement);
}

function applyFirstFrameControl(edit, m2) {
  const page = m2 ? 'GpuSandWaterExperience' : 'GpuResidentM1';
  edit('ets/entryability/EntryAbility.ets', body => {
    const routeStart = '    const probePage: string = ';
    const start = body.indexOf(routeStart);
    const end = body.indexOf('    windowStage.loadContent(probePage,', start);
    if (start < 0 || end < 0) throw Error('T714_ROUTE_DRIFT');
    body = one(body, body.slice(start, end),
      `    console.info('T714_CONTROL ' + grsm1.fullComputeVariant());
    const probePage: string = 'pages/${page}';
`);
    if (!m2) {
      const active = body.split('\n').find(line => line.startsWith("    AppStorage.setOrCreate('gpuResidentM1',"));
      if (!active) throw Error('T714_M1_ACTIVATION_DRIFT');
      body = one(body, active, "    AppStorage.setOrCreate('gpuResidentM1', true);");
    }
    return body;
  });
  if (m2) return;

  edit('cpp/CMakeLists.txt', body =>
    body + '\n# T714 M1 first-frame comparator only.\ntarget_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)\n');
  edit('cpp/gpu_resident/app_bridge.cpp', body => {
    body = one(body, ' bool progressV2=false,nextProgressV2=false,v2ResourcesReady=false;',
      ' bool progressV2=true,v2ResourcesReady=false;');
    body = one(body, '    progressV2=nextProgressV2;', '    progressV2=true;');
    body = one(body, ' auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);a.nextProgressV2=enabled;',
      ' if(!enabled)return text(e,"T714_CLOCK_FIXED_V2");');
    body = one(body, 'struct App : BridgeState {', `struct App : BridgeState {
 struct FirstFrameInit {
  uint64_t epoch=0, begin=0, end=0, candidateRun=0, window=0, ownerTid=0;
  const char* stage="NOT_ENTERED";
  int rawCode=-1;
 } firstFrameInit;
 void ObserveFirstFrameInit(uint64_t e,const char* stage,int code=-1,uint64_t run=0,void* window=nullptr){
  std::lock_guard<std::mutex> lock(mutex);
  auto& row=firstFrameInit;
  if(window)row={e,Stamp(),0,0,reinterpret_cast<uintptr_t>(window),BridgeTid(),stage,-1};
  if(row.epoch!=e)return;
  row.stage=stage;row.rawCode=code;
  if(run)row.candidateRun=run;
  if(code!=-1)row.end=Stamp();
 }
 void FirstFrameInitJsonLocked(std::ostringstream& o)const{
  const auto& row=firstFrameInit;
  o<<",\\"firstFrameInit\\":{\\"epoch\\":\\""<<row.epoch<<"\\",\\"candidateRun\\":\\""<<row.candidateRun
   <<"\\",\\"window\\":\\""<<row.window<<"\\",\\"ownerTid\\":\\""<<row.ownerTid
   <<"\\",\\"beginNs\\":\\""<<row.begin<<"\\",\\"endNs\\":\\""<<row.end
   <<"\\",\\"stage\\":\\""<<row.stage<<"\\",\\"rawCode\\":"<<row.rawCode<<"}";
 }`);
    body = one(body, '    Mark(BridgeStage::InitBegin,seen);',
      '    ObserveFirstFrameInit(seen,"EGL_ENTER",-1,0,held);\n    Mark(BridgeStage::InitBegin,seen);');
    const eglFailure = body.split('\n').find(line => line.includes('if(!good){publish(') && line.includes('eglGetError()'));
    if (!eglFailure) throw Error('T714_EGL_RETURN_DRIFT');
    body = one(body, eglFailure, eglFailure
      .replace('if(!good){publish(', 'if(!good){const auto raw=eglGetError();ObserveFirstFrameInit(seen,"EGL_RETURN",raw);publish(')
      .replace('std::to_string(eglGetError())','std::to_string(raw)'));
    const create = '    auto code=gpu->Create(capacity,waterCapacity,id,m1::QuantumParameters{},16u*1048576u,cold);';
    body = one(body, create,
      '    ObserveFirstFrameInit(seen,"TRANSPORT_CREATE_ENTER",-1,runId);\n' +
      create + '\n    ObserveFirstFrameInit(seen,"TRANSPORT_CREATE_RETURN",int(code),runId);');
    return one(body, 'a.UiJsonLocked(o);a.ProgressJsonLocked(o);',
      'a.UiJsonLocked(o);a.ProgressJsonLocked(o);a.FirstFrameInitJsonLocked(o);');
  });
  edit('ets/pages/GpuResidentM1.ets', body => {
    body = one(body, '@State private progressV2:boolean=false;', '');
    const marker = "Button(this.progressV2?'下轮进度时钟：V2':'下轮进度时钟：V1')";
    const begin = body.indexOf(marker);
    const end = body.indexOf("Button('导出原生时间线')", begin);
    if (begin < 0 || end < 0) throw Error('T714_CLOCK_PICKER_DRIFT');
    body = body.slice(0,begin) + body.slice(end);
    body = one(body, ' private readonly cold:GpuBridgeTimelineCold=new GpuBridgeTimelineCold();',
      ` private async exportFirstFrameControl():Promise<void> {
  try {
   const context=getContext(this) as common.UIAbilityContext;
   const evidence=new DiagnosticColdFiles(context.filesDir);
   const contents:string='{"arm":'+m1.fullComputeVariant()+',"status":'+m1.status()+'}';
   const name:string='t714-first-frame-'+String(Date.now())+'.json';
   await evidence.save(name,contents);this.coldStatus=name;
  } catch(error) {this.coldStatus=String(error);}
 }
 private readonly cold:GpuBridgeTimelineCold=new GpuBridgeTimelineCold();`);
    return one(body, "  Text('沙水 DEMO V4 · GPU M1').fontSize(18)",
      "  Text('沙水 DEMO V4 · GPU M1').fontSize(18)\n" +
      "  Button('导出首帧原件').onClick(()=>this.exportFirstFrameControl());\n" +
      "  Text(this.coldStatus).fontSize(10).maxLines(2)");
  });
}

module.exports = { applyFirstFrameControl };
