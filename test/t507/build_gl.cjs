const fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const project=process.argv[2],out=process.argv[3];
const main=project+'/entry/src/main/cpp';
// Reuse actual SOURCE/J→W/Draw/Swap fixture, with a test-only owner pointer.
let source=fs.readFileSync('test/t492/build_source_gl.cjs','utf8');
const injection=`
fs.mkdirSync(out,{recursive:true});
let app=fs.readFileSync(main+'/gpu_resident/app_bridge.cpp','utf8');
const traceProbe=fs.readFileSync('test/t507/gl_trace.inc','utf8');
const probeBegin=traceProbe.indexOf('  auto& gpu=*static_cast<AppTransport*>');
const probeEnd=traceProbe.indexOf('  cold503::SmallDiagnostic summary;',probeBegin);
const spanProbe=traceProbe.slice(probeBegin,probeEnd).replace('  auto& gpu=*static_cast<AppTransport*>(observedTransport507);','').replaceAll('a.K','K').replaceAll('directory+','std::string(\"'+out+'\")+');
app=app.replace('void ServiceColdTrace(AppTransport& gpu,uint64_t seen,void* held){','void ServiceColdTrace(AppTransport& gpu,uint64_t seen,void* held){observedTransport507=&gpu;');
app=app.replace('    const auto restore=gpu.Freeze(false);','    if(code==TransportCode::OK&&gpu.LastCompletion()[27]==2){'+spanProbe+'}\\n    const auto restore=gpu.Freeze(false);');
fs.writeFileSync(out+'/app_bridge.cpp',app);
fixture='static void* observedTransport507=nullptr;extern int mapFault507,unmapFault507;\\n'+fixture;
let probe=fs.readFileSync('test/t507/gl_trace.inc','utf8');probe=probe.slice(0,probe.indexOf('  auto& gpu=*static_cast<AppTransport*>'))+probe.slice(probe.indexOf('  cold503::SmallDiagnostic summary;'));
fixture=fixture.replace('void Wake(App& a,BridgeStage stage){',probe+'\\nvoid OriginalWake(App& a,BridgeStage stage){');
fixture=fixture.replace('case 4: if(a.sourceCommit==2){','case 4: if(a.sourceCommit==2&&TraceCheckpoint507(a,0)){');
fixture=fixture.replace('case 5: if(a.ui.settled.lo==admittedWater&&a.ui.flight.lo==0&&a.renderCadence.newPhysicalFrames>0)', 'case 5: if(a.ui.settled.lo==admittedWater&&a.ui.flight.lo==0&&a.renderCadence.newPhysicalFrames>0&&TraceCheckpoint507(a,1))');
fixture=fixture.replace('int main(){','void Wake(App& a,BridgeStage stage){OriginalWake(a,stage);SaveTrace507(a,"'+out+'");}\\nint main(){');
`;
// This patch must run in the innermost builder after it assembled the fixture.
source=source.replace('const moduleFile=new Module(__filename);', 'source=source.replace("fs.writeFileSync(path.join(out,\'app.cpp\'),fixture);",'+JSON.stringify(injection)+'+"\\nfs.writeFileSync(path.join(out,\'app.cpp\'),fixture);");\nconst moduleFile=new Module(__filename);');
const forward=fs.readFileSync('.codex/local-test/t236-source-host/gl_forward.cpp','utf8')
 .replace('return fn(target,offset,length,access);','if(mapFault507){mapFault507=0;target=GLenum(0xffff);}return fn(target,offset,length,access);')
 .replace('assert(fn);return fn(target);}\n', 'assert(fn);return fn(target);}\n');
const unmapMarker='extern "C" GLboolean GL_APIENTRY glUnmapBuffer';
const unmapAt=forward.indexOf(unmapMarker);if(unmapAt<0)throw Error('UNMAP_FORWARD');
const fixedForward=forward.slice(0,unmapAt)+forward.slice(unmapAt).replace('return fn(target);','auto result=fn(target);if(unmapFault507){unmapFault507=0;return GL_FALSE;}return result;');
// Only the map/unmap forwarders use these parameter names; preserve all others.
const temp=out+'-gl-forward.cpp';fs.writeFileSync(temp,'int mapFault507=0,unmapFault507=0;\n'+fixedForward);
source=source.replace('const moduleFile=new Module(__filename);','source=source.replace("...vars.map(line=>line.replace(","...vars.map(line=>line.replaceAll(\\\".codex/local-test/t236-source-host/gl_forward.cpp\\\",'+JSON.stringify(temp).replaceAll('"','\\\"')+').replace(");\nconst moduleFile=new Module(__filename);');
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
