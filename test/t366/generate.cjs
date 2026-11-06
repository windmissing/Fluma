const fs=require('fs'),path=require('path'),cp=require('child_process');
const out=path.resolve(process.argv[2]);
cp.execFileSync(process.execPath,[require.resolve('../t351/generate.cjs'),out],{stdio:'inherit'});
const page=path.join(out,'entry/src/main/ets/pages/P051DerivedV2Demo.ets');
const source=fs.readFileSync(page,'utf8');
fs.writeFileSync(page,source.replace('this.model.budget.enabled=true;','this.model.budget.enabled=true;this.model.configureParticleFineReceipt(true);').replace('this.model.budget.exportCold()','JSON.stringify({budget:JSON.parse(this.model.budget.exportCold()),hotApply:JSON.parse(this.model.exportParticleFineReceiptCold())})'));
