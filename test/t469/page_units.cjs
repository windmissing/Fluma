const fs=require('node:fs'),Module=require('node:module');
const base=fs.readFileSync('test/t432/prepare_diff.cjs','utf8').replaceAll('\r\n','\n');
const prefix=base.slice(0,base.indexOf('function page()')).replace("const native = { getState: () => ({ surfaceEpoch: 3, contextEpoch: 2, widthPx: 800, heightPx: 600 }) };","let nativeState;const native={getState:()=>nativeState};");
const mod=new Module(__filename);mod.filename=__filename;mod.paths=module.paths;
mod._compile(prefix+fs.readFileSync('test/t469/page_cases.inc','utf8'),__filename);
