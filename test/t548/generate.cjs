const path=require('node:path'),fs=require('node:fs'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);cp.execFileSync(process.execPath,[require.resolve('../t529/generate.cjs'),out],{stdio:'inherit'});
fs.writeFileSync(path.join(out,'t548-manifest.json'),JSON.stringify({revision:'T548_PARTIAL_BLOCKED_SEED_CONTRACT',product:'NOT_COMPLETE_REAL_ADAPTER_DISABLED',tests:'NOT_RUN_BY_USER_DIRECTION',device:'NOT_RUN'}));
