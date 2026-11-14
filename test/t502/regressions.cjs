const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');const out=process.argv[2];if(!out||fs.existsSync(out))throw Error('FRESH_OUTPUT');fs.mkdirSync(out,{recursive:true});
for(const task of ['t475','t476']){
 const file=task==='t475'?'test/t475/store_test.cpp':'test/t476/store_diff.cpp';
 const script=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',`cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DNOMINMAX /Ientry/src/main/cpp/gpu_resident/program_cache ${file} /Fe${out}/${task}.exe /Fo${out}/`,'if errorlevel 1 exit /b 1',out.replaceAll('/','\\')+'\\'+task+'.exe '+out+'/data-'+task,'exit /b %errorlevel%'].join('\r\n');fs.writeFileSync(out+'/'+task+'.cmd',script);const result=cp.execFileSync('cmd.exe',['/c',path.join(out,task+'.cmd')],{encoding:'utf8',timeout:300000});fs.writeFileSync(out+'/'+task+'.txt',result);process.stdout.write(result);
}
