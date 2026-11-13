const fs=require('node:fs'),Module=require('node:module');
const route=process.argv[5]??'m2route';if(!['m1','m2route'].includes(route))throw Error('ROUTE');
if(!fs.existsSync(process.argv[2]+'/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h'))throw Error('BUILD_PROJECT_FIRST');
let source=fs.readFileSync('test/t477_fix/build_app.cjs','utf8');
source=source.replaceAll('T477_FIX_GENERATED_M2_GL_V1','T492_GENERATED_'+route.toUpperCase()+'_SOURCE_GL_V1');
source=source.replace("const route=main+'/gpu_resident/m2route';","const route=main+'/gpu_resident/"+route+"';");
source=source.replace('/DGRS_PROGRESS_V2_EXPERIMENT %INCS%', '/DGRS_PROGRESS_V2_EXPERIMENT /I'+process.argv[2]+'/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident %INCS%');
const extra=fs.readFileSync('test/t485/gl_exports.inc','utf8').split(/\r?\n/).filter(l=>l.startsWith('#define')||/glGetActiveAttrib|glGetProgramInterfaceiv|glGetProgramResourceiv|glGetAttribLocation|glGetFragDataLocation|glGetProgramResourceIndex|glIsProgram/.test(l)).join('\n');
const patch=`
fixture+='\\n'+${JSON.stringify(extra)};
${route==='m1'?"fixture=fixture.replace('w[1]=3;','w[1]=2;');":""}
fixture='#include <fstream>\\n'+fixture;
fixture=fixture.replace('std::cout<<"T477 native Draw/Swap current frames="', 'std::ofstream diagnostic("'+process.argv[3]+'/execution.json");a.diagnostics.Json(diagnostic);diagnostic.close();std::cout<<"T477 native Draw/Swap current frames="');
`;
source=source.replace("fs.writeFileSync(path.join(out,'app.cpp'),fixture);",patch+"\nfs.writeFileSync(path.join(out,'app.cpp'),fixture);");
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
