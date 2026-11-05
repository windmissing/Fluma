const fs=require('fs'),cp=require('child_process'),path=require('path');const root='.codex/local-test/t351-baseline';
if(!fs.existsSync(root+'/entry/src/main/ets')){fs.mkdirSync(root,{recursive:true});const tar=path.resolve('.codex/local-test/t351-baseline.tar');cp.execFileSync('git',['archive','--format=tar','--output='+tar,'5970fbd','entry/src/main/ets']);cp.execFileSync('tar',['-xf',tar,'-C',root]);}
module.exports=path.resolve(root+'/entry/src/main/ets');
