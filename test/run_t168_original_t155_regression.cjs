'use strict';
// Replay the original 16 tester-authored assertions unchanged. Only archive I/O
// paths are redirected; this developer replay is not a new independent verdict.
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const repo=path.resolve(__dirname,'..'),file=path.join(__dirname,'p155_adapter_counterexamples.cjs');
const source=fs.readFileSync(file,'utf8');
const replacements=[
 ["require('../test/reports/t168/gate-summary.json')","require('../test/reports/t168-rev2/gate-summary.json')"],
 ["fs.mkdirSync('test/reports/t155'","fs.mkdirSync('test/reports/t168-rev2'"],
 ["'test/reports/t155/adapter_counterexamples.json'","'test/reports/t168-rev2/original-t155-regression.json'"],
 ["revision:'T155_ADAPTER_COUNTEREXAMPLES_V1'","revision:'T168_DEVELOPER_REPLAY_ORIGINAL_T155_ASSERTIONS'"],
 ["probe:'T168 6c0c654'","probe:'T168_CONTEXT_V2_WORKTREE_DEVELOPER_REPLAY_NOT_INDEPENDENT_VERDICT'"],
];
let text=source;for(const [before,after]of replacements){if(text.split(before).length!==2)throw Error('ARCHIVE_ADAPTER_ANCHOR_CHANGED');text=text.replace(before,after);}
const m=new Module(file,module);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(text,file);
