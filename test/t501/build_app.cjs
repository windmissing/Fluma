const fs=require('node:fs'),Module=require('node:module');
let source=fs.readFileSync('test/t492/build_app.cjs','utf8');
const snippet='a.diagnostics.Json(diagnostic);std::ofstream resourceFile("'+process.argv[3]+'/ledger-"+hostPhase+".json");auto ledger=a.resources->Copy();resourceFile<<ledger.Json();unsigned buffers=0,programs=0;for(const auto& r:ledger.objects)if(r.state==grs::resource501::State::Live){if(r.kind==grs::resource501::Kind::Buffer)++buffers;if(r.kind==grs::resource501::Kind::Program)++programs;}assert(buffers=='+(process.argv[4]==='m1'?17:20)+'&&programs==5&&ledger.incomplete==0);';
const at=source.lastIndexOf('const moduleFile=new Module(__filename);');source=source.slice(0,at)+'source=source.replace(\'a.diagnostics.Json(diagnostic);\','+JSON.stringify(snippet)+');\n'+source.slice(at);
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
