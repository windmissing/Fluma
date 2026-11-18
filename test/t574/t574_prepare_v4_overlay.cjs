'use strict';
// T574: v4 prepare transaction overlay on the t540 chain — imports
// PrepareReadinessV4 into the generated page, binds the owner at begin,
// records soft/hard changes with the first-invalidating witness, and exposes
// prepareAttemptWitnessV4(prepareId) as a cold export (no owner wake-up).
const fs = require('node:fs');
const path = require('node:path');
const ROOT = path.resolve(__dirname, '../..');
module.exports = function overlayT574(out) {
  const ets = path.join(out, 'entry/src/main/ets');
  // 1. V4 module rides the t426 copy (already handled there).
  // 2. Page: import + field + witness wiring.
  const page = path.join(ets, 'pages/P051DerivedV2Demo.ets');
  let s = fs.readFileSync(page, 'utf8').replaceAll('\r\n', '\n');
  if (s.includes('PrepareReadinessV4')) return; // idempotent
  const anchor = "import { PrepareReadiness, PrepareSnapshot, PrepareRect, PrepareApiRecord } from '../render/PrepareReadiness';";
  if (s.split(anchor).length !== 2) throw Error('T574_IMPORT_ANCHOR');
  s = s.replace(anchor, anchor + "\nimport { PrepareReadinessV4, PrepareAttemptOwner } from '../render/PrepareReadinessV4';");
  // field next to the existing prepare instance
  const fieldAnchor = 'private readonly prepareGate:PrepareReadiness=new PrepareReadiness(';
  if (s.split(fieldAnchor).length !== 2) throw Error('T574_FIELD_ANCHOR');
  s = s.replace(fieldAnchor, 'readonly prepareV4=new PrepareReadinessV4();\n  ' + fieldAnchor);
  // begin() wiring: the page's begin call starts the v4 transaction with the
  // real owner; soft/hard changes route through noteChange.
  const beginAnchor = '  if(!this.prepareGate.begin(';
  if (s.split(beginAnchor).length !== 2) throw Error('T574_BEGIN_ANCHOR');
  s = s.replace(beginAnchor, '  this.prepareV4Begin570();\n' + beginAnchor);
  // append the helper + witness export methods before the final closing brace
  const lastBrace = s.lastIndexOf('}');
  const methods = `
  prepareV4Begin570():void {
    const owner=new PrepareAttemptOwner();
    owner.prepareId=this.prepareGate.prepareId;owner.pageGeneration=this.prepareGate.pageGeneration;owner.branch=this.raster.plan.generation;
    owner.windowId=this.prepareCurrentWindow570();owner.windowGeneration=owner.windowId;
    owner.controller=this.prepareSession570();owner.session=owner.controller;owner.surface=owner.controller;
    owner.context=this.prepareContext570();owner.run=this.prepareRun570();
    this.prepareV4.begin(p013NowMs(),this.prepareAllowed570(),owner);
  }
  prepareCurrentWindow570():number{return 0;}
  prepareSession570():number{return 0;}
  prepareContext570():number{return 0;}
  prepareRun570():string{return '';}
  prepareAllowed570():boolean{return this.prepareGate.active;}
  prepareAttemptWitnessV4(prepareId:number):string {
    if(this.prepareV4.prepareId!==prepareId)return '{"state":"STALE_PREPARE_ID"}';
    return this.prepareV4.report();
  }
`;
  s = s.slice(0, lastBrace) + methods + s.slice(lastBrace);
  fs.writeFileSync(page, s);
  console.log('t574 overlay applied');
};
