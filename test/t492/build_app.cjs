const fs=require('node:fs'),Module=require('node:module');
let source=fs.readFileSync('test/t485/build_app_check.cjs','utf8');
const patch=`
source=source.replace('assert(a.AllowedLocked()&&a.recovery.phase==recovery467::Phase::READY&&a.progress.generated==0);',\
'assert(a.AllowedLocked()&&a.recovery.phase==recovery467::Phase::READY&&a.progress.generated==0);\\n'+
'  unsigned builds=0,vaos=0;for(unsigned i=0;i<a.diagnostics.stageCount;++i){const auto& s=a.diagnostics.stages[i];if(std::string(s.first.stage.data())=="SCENE_BUILD_EXIT")builds=unsigned(s.count);if(std::string(s.first.stage.data())=="SCENE_VAO_EXIT")vaos=unsigned(s.count);}assert(builds==3&&vaos==1);\\n'+
'  std::ofstream diagnostic("'+process.argv[3]+'/diagnostic-"+hostPhase+".json");a.diagnostics.Json(diagnostic);\\n');
`;
source=source.replace('const moduleFile=new Module(__filename);',patch+'\nconst moduleFile=new Module(__filename);');
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
