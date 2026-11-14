const fs=require('node:fs'),Module=require('node:module');
let source=fs.readFileSync('test/t485/build_host.cjs','utf8');
const patch=`
source=source.replace('assert(argc==4);Context context;', 'assert(argc==4);grs::resource501::Ledger ledger;Context context;grs::resource501::Bind(&ledger,1);');
source=source.replace(' Store().Drain();', \
' GLuint observedBuffer=0;glGenBuffers(1,&observedBuffer);glBindBuffer(GL_ARRAY_BUFFER,observedBuffer);glBufferData(GL_ARRAY_BUFFER,4,nullptr,GL_STATIC_DRAW);\\n'+
' const auto before=ledger.Copy().liveBytes;glBufferData(GL_ARRAY_BUFFER,-1,nullptr,GL_STATIC_DRAW);assert(ledger.Copy().liveBytes==before);assert(glGetError()==GL_INVALID_VALUE);assert(glGetError()==GL_NO_ERROR);\\n'+
' glBindBuffer(GLenum(0xffff),0);glBufferData(GL_ARRAY_BUFFER,8,nullptr,GL_STATIC_DRAW);assert(glGetError()==GL_INVALID_ENUM);assert(glGetError()==GL_NO_ERROR);glDeleteBuffers(1,&observedBuffer);\\n'+
' auto resource=ledger.Copy();assert(resource.liveBytes==0&&resource.retiringBytes==0&&resource.incomplete==0&&resource.allocFailures==1);std::ofstream(out+"/resource-end.json")<<resource.Json();grs::resource501::Bind(nullptr,0);Store().Drain();');
`;
source=source.replace("if(process.argv[5]==='faults')source=",patch+"\nif(process.argv[5]==='faults')source=");
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
