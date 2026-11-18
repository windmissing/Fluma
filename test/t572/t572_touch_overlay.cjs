'use strict';
// T572: inject the G9 real-touch adapter endpoints + PresentedMapping +
// cold-V2 records into a t529-chain delivery tree. Idempotent; all anchors are
// unique strings introduced by the T529/T550 overlays.
const fs = require('node:fs');
const path = require('node:path');
module.exports = function injectTouchAdapter(main) {
  const cpp = path.join(main, 'cpp');
  const bridge = path.join(cpp, 'gpu_resident/app_bridge.cpp');
  let s = fs.readFileSync(bridge, 'utf8').replaceAll('\r\n', '\n');
  if (s.includes('g9TouchBegin')) return; // idempotent
  const inc = fs.readFileSync(path.join(__dirname, 't572_touch_bridge.inc'), 'utf8').replaceAll('\r\n', '\n');
  const wrap = t => '\n#ifdef GRS_APP_M1\n' + t + '\n#endif\n';
  // 1. include the T548 adapter header (namespace candidate548) + the V2 records
  const anchorInclude = '#include "g9spf/controller.h"';
  if (s.split(anchorInclude).length !== 2) throw Error('T572_INCLUDE_ANCHOR');
  s = s.replace(anchorInclude, '#include "g9spf/touch_adapter.h"\n' + anchorInclude);
  // 1b. app field for the last presented scene token + capture after Draw.
  const anchorFields='std::shared_ptr<grs::g9spf::Controller> g9Controller;';
  if (s.split(anchorFields).length !== 2) throw Error('T572_FIELDS_ANCHOR');
  s = s.replace(anchorFields, anchorFields + 'std::string g9PresentedToken;');
  const anchorDraw='g9LastDisplay=generation;g9LastDraw=now();publish("{\\"state\\":\\"G9_CURRENT\\"';
  const drawAt=s.indexOf('g9LastDisplay=generation;');
  if (drawAt < 0) throw Error('T572_DRAW_ANCHOR');
  const lineEnd=s.indexOf('\n',drawAt);
  s=s.slice(0,lineEnd)+' {std::lock_guard<std::mutex> lock(mutex);g9PresentedToken=token;}'+s.slice(lineEnd);
  // 2. the endpoint bodies + state, right before napi_value init(
  const anchorInit = 'napi_value init(napi_env e,napi_value out){';
  if (s.split(anchorInit).length !== 2) throw Error('T572_INIT_ANCHOR');
  s = s.replace(anchorInit, wrap(inc) + '\n' + anchorInit);
  // 3. register the five new endpoints in the property table next to g9OfferModel
  const anchorProp = '{"g9OfferModel",nullptr,g9OfferModel,nullptr,nullptr,nullptr,napi_default,nullptr},';
  if (s.split(anchorProp).length !== 2) throw Error('T572_PROP_ANCHOR');
  const newProps = [
    '{"g9TouchBegin",nullptr,g9TouchBegin,nullptr,nullptr,nullptr,napi_default,nullptr},',
    '{"g9TouchUpdate",nullptr,g9TouchUpdate,nullptr,nullptr,nullptr,napi_default,nullptr},',
    '{"g9TouchEnd",nullptr,g9TouchEnd,nullptr,nullptr,nullptr,napi_default,nullptr},',
    '{"g9TouchCancel",nullptr,g9TouchCancel,nullptr,nullptr,nullptr,napi_default,nullptr},',
    '{"g9PresentedMappingV2",nullptr,g9PresentedMappingV2,nullptr,nullptr,nullptr,napi_default,nullptr},',
    '{"g9ColdRecordsV2",nullptr,g9ColdRecordsV2,nullptr,nullptr,nullptr,napi_default,nullptr},',
  ].join('\n');
  s = s.replace(anchorProp, anchorProp + '\n' + newProps);
  fs.writeFileSync(bridge, s);
  // 4. d.ts declarations
  const types = path.join(cpp, 'types/libgrsm1/index.d.ts');
  fs.appendFileSync(types, [
    'export const g9TouchBegin:(sample:number,receipt:number,pointer:number,px:number,py:number,changed:number,touches:number,generation:number)=>string;',
    'export const g9TouchUpdate:(sample:number,receipt:number,pointer:number,px:number,py:number,changed:number,touches:number)=>string;',
    'export const g9TouchEnd:(sample:number,receipt:number,pointer:number,px:number,py:number,changed:number,touches:number)=>string;',
    'export const g9TouchCancel:()=>string;',
    'export const g9PresentedMappingV2:(token:string)=>string;',
    'export const g9ColdRecordsV2:()=>string;',
    '',
  ].join('\n'));
};
