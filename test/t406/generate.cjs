const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t394/generate.cjs'),out],{stdio:'inherit'});
const ets=path.join(out,'entry/src/main/ets'),cpp=path.join(out,'entry/src/main/cpp/p013renderer');
function edit(root,name,old,value){
 const file=path.join(root,name),text=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 if(text.split(old).length!==2)throw Error('T406_ANCHOR '+name+' '+old.slice(0,100));
 fs.writeFileSync(file,text.replace(old,value));
}
const shader=fs.readFileSync(path.join(__dirname,'calibration.glsl'),'utf8');
const shaderSha=crypto.createHash('sha256').update(shader).digest('hex');
const vertex='#version 300 es\nvoid main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0.,1.);}';
edit(cpp,'window_probe.h',' double ms=0;',' bool calibration=false;\n double ms=0;');
edit(cpp,'single_sand_napi.inc','  windowProbe.candidateNs=',
 '  windowProbe.calibration=false;\n  windowProbe.candidateNs=');
edit(cpp,'single_sand_napi.inc','  windowProbe.packet=p;\n  windowDecode=T385DecodeRoi',
 '  windowProbe.packet=p;\n  napi_value flag=nullptr;\n  if(napi_get_named_property(e,a[0],"calibration",&flag)==napi_ok)napi_get_value_bool(e,flag,&windowProbe.calibration);\n  windowDecode=T385DecodeRoi');
edit(cpp,'single_sand_pass.cpp',' if(probe){probe->config=v;probe->Capture(targets.fbos[targets.candidate]);}',`
 if(probe&&probe->calibration){
  static constexpr const char* vertex=R"CAL(${vertex})CAL";
  static constexpr const char* fragment=R"CAL(${shader})CAL";
  const auto vs=Compile(GL_VERTEX_SHADER,vertex),fs=Compile(GL_FRAGMENT_SHADER,fragment);
  const auto calibrationProgram=glCreateProgram();
  glAttachShader(calibrationProgram,vs);glAttachShader(calibrationProgram,fs);glLinkProgram(calibrationProgram);
  GLint linked=0;glGetProgramiv(calibrationProgram,GL_LINK_STATUS,&linked);
  glDeleteShader(vs);glDeleteShader(fs);
  if(!linked){glDeleteProgram(calibrationProgram);out.stage="CALIBRATION_SHADER";return fail();}
  glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glUseProgram(calibrationProgram);
  glViewport(0,0,p.width,p.height);glDisable(GL_BLEND);glDisable(GL_DITHER);
  glDrawArrays(GL_TRIANGLES,0,3);glDeleteProgram(calibrationProgram);
  glUseProgram(program);glBindFramebuffer(GL_FRAMEBUFFER,0);
  glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);
  glUniform1i(loc("layer"),4);glDrawArrays(GL_TRIANGLES,0,3);
 }
 if(probe){probe->config=v;probe->Capture(targets.fbos[targets.candidate]);}`);
// The ordinary material packet and shader remain authoritative outside
// the explicitly reserved calibration draw.
edit(cpp,'window_probe_napi.inc',' Put(e,o,"captureStatus",',
 ' Put(e,o,"calibration",probe.calibration);\n Put(e,o,"calibrationShaderSha256","'+shaderSha+'");\n Put(e,o,"captureStatus",');
edit(ets,'render/P013SingleSandTypes.ets','  probeRequestId:number=0;','  calibration:boolean=false;\n  probeRequestId:number=0;');
edit(ets,'render/P013SingleSandController.ets','  readonly t394=new T394Phase();',`  private t406Calibration=false;
  private t406PlanRoi:Int32Array|null=null;
  private t406PlanId='';
  private t406CalibrationId='';
  setT406Plan(text:string):string {
    const fields=text.split('|');
    if(fields.length!==3||!/^[a-f0-9]{64}$/.test(fields[0])||!/^[a-f0-9]{64}$/.test(fields[1]))return 'INVALID_FROZEN_PLAN';
    const roi=parseWindowRoi(fields[2]);
    if(roi===null||roi.length!==12||this.probeBusy())return 'INVALID_FROZEN_ROIS';
    this.t406PlanRoi=roi;this.t406CalibrationId=fields[0];this.t406PlanId=fields[1];return 'FROZEN_PLAN_READY';
  }
  setT406Calibration(enabled:boolean):void {this.t406Calibration=enabled;}
  readonly t394=new T394Phase();`);
edit(ets,'render/P013SingleSandController.ets','WindowProbeRequest, cancelledWindowProbe',
 'WindowProbeRequest, cancelledWindowProbe, parseWindowRoi');
edit(ets,'render/P013SingleSandController.ets','this.t394Roi=t394DynamicRoi(p);',
 'this.t394Roi=this.t406PlanRoi===null?null:new Int32Array(this.t406PlanRoi);');
edit(ets,'render/SingleSandWindowCold.ets','export interface WindowProbeEvidence {',
 'export interface WindowProbeEvidence {\n  calibrationId?:string;roiPlanId?:string;');
edit(ets,'render/SingleSandWindowCold.ets','interface WindowArchive {',
 'interface WindowArchive {\n  calibrationId?:string;roiPlanId?:string;');
edit(ets,'render/SingleSandWindowCold.ets','phase:evidence.phase};',
 'phase:evidence.phase,calibrationId:evidence.calibrationId,roiPlanId:evidence.roiPlanId};');
edit(ets,'render/P013SingleSandController.ets','    const evidence=this.probeEvidence;\n    let phaseStatus=',
 '    const evidence=this.probeEvidence;\n    if(evidence!==null){evidence.calibrationId=this.t406CalibrationId;evidence.roiPlanId=this.t406PlanId;}\n    let phaseStatus=');
edit(ets,'render/P013SingleSandController.ets','      prep.packet.probeRequestId=request.id;',
 '      prep.packet.calibration=this.t406Calibration;\n      prep.packet.probeRequestId=request.id;');
edit(ets,'render/P013SingleSandController.ets','        prep.packet.probeRequestId=0;',
 '        prep.packet.calibration=false;\n        prep.packet.probeRequestId=0;');
edit(ets,'pages/P051DerivedV2Demo.ets'," private t394Arm:string='';",` @State private t406Geometry:string='';
 private t406GlobalX:number=0;private t406GlobalY:number=0;
 private t394Arm:string='';`);
edit(ets,'pages/P051DerivedV2Demo.ets',"  this.t394Arm=arm;",
 "  this.singleSand.setT406Calibration(false);\n  this.t394Arm=arm;");
edit(ets,'pages/P051DerivedV2Demo.ets',"    Row({space:4}){\n      Button('厚床三区')",`    Row({space:4}){
      Button('独立标定图').onClick(()=>{
        if(this.singleSand.probeBusy()||this.evidence!==null)return;
        this.t394Phase('CALIBRATION',true);
        this.singleSand.setT406Calibration(true);
        this.probeStatus='先导出阶段获取实际窗口尺寸，再填三处标定 ROI';
      })
      Button('厚床三区')`);
edit(ets,'pages/P051DerivedV2Demo.ets',"    Text(this.probeStatus).fontSize(12)",`    Text(this.t406Geometry).fontSize(8).maxLines(1)
    TextInput({placeholder:'校准ID|计划ID|三组原生 ROI'}).onChange((text:string)=>{this.probeStatus=this.singleSand.setT406Plan(text);})
    Text(this.probeStatus).fontSize(12)`);
edit(ets,'pages/P051DerivedV2Demo.ets','const w=Number(area.width),h=Number(area.height);',
 `this.t406GlobalX=Number(area.globalPosition.x);this.t406GlobalY=Number(area.globalPosition.y);
      this.t406Geometry=JSON.stringify({revision:'T406_GEOMETRY_V1',globalX:this.t406GlobalX,globalY:this.t406GlobalY,
        widthVp:Number(area.width),heightVp:Number(area.height),vp2px:this.getUIContext().vp2px(1),geometry:this.t394GeometryEpoch});
      const w=Number(area.width),h=Number(area.height);`);
edit(ets,'pages/P051DerivedV2Demo.ets','const text=JSON.stringify({revision:', 'const text=JSON.stringify({geometry:this.t406Geometry,revision:');
edit(ets,'pages/P051DerivedV2Demo.ets',"Button('预约一次').onClick(()=>{",
 "Button('预约一次').onClick(()=>{this.t394Phase(this.t394Arm==='STATIC'?'STATIC_'+this.t394Optics:'CALIBRATED_ROI',true);");
fs.writeFileSync(path.join(out,'t406-manifest.json'),JSON.stringify({revision:'T406_CALIBRATION_V1',shaderSha256:shaderSha},null,2));
