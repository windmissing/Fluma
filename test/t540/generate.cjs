const path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t516/generate.cjs'),out],{stdio:'inherit'});
cp.execFileSync('python',['-X','utf8',path.join(__dirname,'overlay.py'),out],{stdio:'inherit'});
require('../t574/t574_prepare_v4_overlay.cjs')(out);
