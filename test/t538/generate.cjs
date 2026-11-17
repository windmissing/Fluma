const path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]),route=process.argv[3];
if(!['m1','m2'].includes(route))throw Error('ROUTE_REQUIRED');
cp.execFileSync(process.execPath,[require.resolve('../t537/generate.cjs'),out,route],{stdio:'inherit'});
cp.execFileSync('python',['-X','utf8',path.join(__dirname,'overlay.py'),out,route],{stdio:'inherit'});
