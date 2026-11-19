'use strict';
// T648 supersedes the incorrect T555 construction; T549 frozen inputs remain untouched.
const cp=require('node:child_process'),path=require('node:path');
cp.execFileSync('python',[path.resolve(__dirname,'../tools/g9dm/generate_assets.py'),'--canonical'],{stdio:'inherit'});
