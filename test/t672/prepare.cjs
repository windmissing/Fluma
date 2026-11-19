const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]);cp.execFileSync(process.execPath,['tools/g13/prepare_build.cjs',out],{stdio:'inherit'});
const file=path.join(out,'entry/src/main/ets/pages/GpuResidentG13.ets');let s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
function one(a,b){if(s.split(a).length!==2)throw Error('T672_ANCHOR '+a);s=s.replace(a,b);}
s="import { T672Collector } from '../physics/T672Collector';\n"+s;
one("  private statusTimer: number = -1;","  private statusTimer: number = -1;\n  private diagnostic: T672Collector = new T672Collector();\n  private diagnosticArmed: boolean = false;");
one('    this.ready = false;\n    const context',"    if (this.diagnosticArmed) { this.message = 'T672_ONE_CONFIGURE_ONLY'; return; }\n    this.ready = false;\n    const context");
one('    this.message = native.g13BeginSession',"    this.diagnostic.arm(context.filesDir); this.diagnosticArmed = true;\n    if (this.statusTimer >= 0) { clearInterval(this.statusTimer); this.statusTimer = -1; }\n    this.message = native.g13BeginSession");
one('    this.message = native.g13Configure(G13ResearchConfig.words(), G13ResearchConfig.seed());','    this.message = native.g13Configure(G13ResearchConfig.words(), G13ResearchConfig.seed());\n    this.diagnostic.configured(this.message);');
one("        Button('冷档').onClick(() => { this.archive(); })","        Button('冷档').onClick(() => { this.archive(); })\n        Button('完整初始化诊断').onClick(() => { try { this.message = this.diagnostic.capture(); } catch (error) { this.message = 'T672_INCOMPLETE:' + String(error); } })");
// Freeze all collector-induced lifecycle/control actions after configuration.
s=s.replace('    if (!this.ready) return;','    if (this.diagnosticArmed || !this.ready) return;');
s=s.replace('private source(opcode: number): void {','private source(opcode: number): void { if (this.diagnosticArmed) return;');
s=s.replace('private view(mode: number): void {','private view(mode: number): void { if (this.diagnosticArmed) return;');
fs.writeFileSync(file,s);fs.copyFileSync('test/t672/T672Collector.ets',path.join(out,'entry/src/main/ets/physics/T672Collector.ets'));
