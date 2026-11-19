// T681/T680: diagnostic values only. No gate/predicate/native/refresh calls.
export interface PreparePairOwner { prepareId:number;pageGeneration:number;branch:number;windowId:number;windowGeneration:number;controller:number;session:number;surface:number;context:number;run:string; }
export class PairGate {
 session:string|null=null;
 pageGeneration:number|null=null;
 prepareId:number|null=null;
 observationSerial:number|null=null;
 inputSerial:number|null=null;
 startedMonoMs:number|null=null;
 state:string|null=null;
 reason:string|null=null;
 diagnosticOverflow:boolean|null=null;
}
export class PairOwner {
 prepareId:number|null=null;
 pageGeneration:number|null=null;
 branch:number|null=null;
 windowId:number|null=null;
 windowGeneration:number|null=null;
 controller:number|null=null;
 session:number|null=null;
 surface:number|null=null;
 context:number|null=null;
 run:string|null=null;
}
export class PairSnapshot {
 windowId:number|null=null;
 windowGeneration:number|null=null;
 controller:number|null=null;
 session:number|null=null;
 surface:number|null=null;
 context:number|null=null;
 geometry:number|null=null;
 nativeGeometry:number|null=null;
 viewportWidth:number|null=null;
 viewportHeight:number|null=null;
 layoutGeneration:number|null=null;
 run:string|null=null;
 nativeReady:boolean|null=null;
}
export class PairNative {
 controller:number|null=null;
 session:number|null=null;
 surface:number|null=null;
 context:number|null=null;
 geometry:number|null=null;
 packetWidth:number|null=null;
 packetHeight:number|null=null;
 viewportWidth:number|null=null;
 viewportHeight:number|null=null;
 run:string|null=null;
}
export class PairRaster {
 observationSerial:number|null=null;
 revision:string|null=null;
 generation:number|null=null;
 geometry:number|null=null;
 rasterW:number|null=null;
 rasterH:number|null=null;
 didLayoutSerial:number|null=null;
 requestedAtLayout:number|null=null;
 layoutAcknowledged:number|null=null;
 firstDrawSerial:number|null=null;
 valid:boolean|null=null;
 reason:string|null=null;
 frameAnchorNs:string|null=null;
 native:PairNative=new PairNative();
}
export class PairInput {
 created:boolean|null=null;
 editing:boolean|null=null;
 focused:boolean|null=null;
}
export class PairRect {
 x:number|null=null;
 y:number|null=null;
 width:number|null=null;
 height:number|null=null;
}
export class PairKeyboard {
 configuration:string|null=null;
 deviceType:string|null=null;
 windowMode:number|null=null;
 avoidUnit:string|null=null;
 height:number|null=null;
 animation:boolean|null=null;
 areaSeen:boolean|null=null;
 sessionEverEdited:boolean|null=null;
 avoid:PairRect[]=[];
}
class RawPairNative {
 controller?:number;
 session?:number;
 surface?:number;
 context?:number;
 geometry?:number;
 packetWidth?:number;
 packetHeight?:number;
 viewportWidth?:number;
 viewportHeight?:number;
 run?:string;
}
class RawPairRaster {
 observationSerial?:number;
 revision?:string;
 generation?:number;
 geometry?:number;
 rasterW?:number;
 rasterH?:number;
 didLayoutSerial?:number;
 requestedAtLayout?:number;
 layoutAcknowledged?:number;
 firstDrawSerial?:number;
 valid?:boolean;
 reason?:string;
 frameAnchorNs?:string;
 native?:RawPairNative;
}
class RawPairInput {
 created?:boolean;
 editing?:boolean;
 focused?:boolean;
}
class RawPairRect {
 x?:number;
 y?:number;
 width?:number;
 height?:number;
}
class RawPairSnapshot {
 windowId?:number;
 windowGeneration?:number;
 controller?:number;
 session?:number;
 surface?:number;
 context?:number;
 geometry?:number;
 nativeGeometry?:number;
 viewportWidth?:number;
 viewportHeight?:number;
 layoutGeneration?:number;
 run?:string;
 nativeReady?:boolean;
 raster?:RawPairRaster;keyboardConfiguration?:string;deviceType?:string;windowMode?:number;avoidUnit?:string;avoid?:RawPairRect[];keyboardHeight?:number;keyboardAnimation?:boolean;
}
export class PrepareAuthorityReport {
 session?:string;prepareId?:number;pageGeneration?:number;state?:string;readinessReason?:string;observationSerial?:number;inputSerial?:number;startedMonoMs?:number;diagnosticOverflow?:boolean;
 snapshot?:RawPairSnapshot;inputs?:RawPairInput[];events?:string[];keyboardAreaSeen?:boolean;sessionEverEdited?:boolean;
}
class RawPairEvent {kind?:string;value?:string;prepareId?:number;pageGeneration?:number;serial?:number;}
class RawPairFrame {frame?:number;now?:number;reason?:string;}
export class PairLastFrame {status:string='NOT_EVALUATED';observationSerial:number|null=null;frame:number|null=null;monoMs:number|null=null;reason:string|null=null;snapshotSerialMatches:boolean|null=null;}
class PairSources {summaryBudget:string="TARGET_4096_HARD_LIMIT_IN_REPORT";serialDomains:string="GATE_SNAPSHOT_OBSERVATION_RASTER_OBSERVATION_RECORDED_FRAME_DISTINCT";gate:string='SAME_CALL_SUPER_REPORT';owner:string='SAME_CALL_OWNER_COPY';geometry:string='ARKTS_PACKET';viewport:string='NATIVE_GET_STATE';lastFrame:string='EXISTING_AUTHORITY_EVENTS';branch:string='UNKNOWN_NO_OWNER';identityJoin:string='DIAGNOSTIC_ONLY_UNKNOWN_OR_TRUNCATED_NOT_JOINABLE';}
export class PreparePairingEvidence {
 revision:string='T680_PREPARE_PAIRING_EVIDENCE_V1';status:string='COMPLETE';gate:PairGate=new PairGate();ownerSerial:number|null=null;owner:PairOwner|null=null;snapshot:PairSnapshot=new PairSnapshot();raster:PairRaster=new PairRaster();inputs:PairInput[]=[];keyboard:PairKeyboard=new PairKeyboard();lastFrame:PairLastFrame=new PairLastFrame();missingFields:string[]=[];truncatedFields:string[]=[];sources:PairSources=new PairSources();
}
export function pairingUtf8Bytes(s:string):number {let bytes=0;for(let i=0;i<s.length;i++){const c=s.charCodeAt(i);if(c<128)bytes++;else if(c<2048)bytes+=2;else if(c>=55296&&c<=56319&&i+1<s.length&&s.charCodeAt(i+1)>=56320&&s.charCodeAt(i+1)<=57343){bytes+=4;i++;}else bytes+=3;}return bytes;}
class PairFields {
 result:PreparePairingEvidence;
 constructor(result:PreparePairingEvidence){this.result=result;}
 missing(path:string):void {if(this.result.missingFields.some((p:string)=>path===p||path.startsWith(p+'.')||path.startsWith(p+'[')))return;this.result.missingFields.push(path);}
 truncated(path:string):void {if(!this.result.truncatedFields.includes(path))this.result.truncatedFields.push(path);}
 number(path:string,value:number|undefined|null,integer:boolean=true,signed:boolean=false):number|null {if(typeof value!=='number'||!Number.isFinite(value)||(integer&&!Number.isSafeInteger(value))||(!signed&&value<0)){this.missing(path);return null;}return value;}
 bit(path:string,value:boolean|undefined|null):boolean|null {if(typeof value!=='boolean'){this.missing(path);return null;}return value;}
 text(path:string,value:string|undefined|null,identity:boolean=false):string|null {if(typeof value!=='string'||(identity&&value.length===0)){this.missing(path);return null;}if(pairingUtf8Bytes(value)>128||pairingUtf8Bytes(JSON.stringify(value))>256){this.truncated(path);return null;}return value;}
}
export function buildPreparePairingEvidence(a:PrepareAuthorityReport,owner:PreparePairOwner|null,ownerSerial:number):PreparePairingEvidence {
 const e=new PreparePairingEvidence(),v=new PairFields(e);
 if(a===null||typeof a!=='object'){a=new PrepareAuthorityReport();v.missing('gate');v.missing('snapshot');v.missing('raster');v.missing('keyboard');v.missing('inputs');}
 const s=a.snapshot??new RawPairSnapshot(),r=s.raster??new RawPairRaster(),n=r.native??new RawPairNative();
 if(a.snapshot===undefined||a.snapshot===null){v.missing('snapshot');v.missing('keyboard');}if(s.raster===undefined||s.raster===null)v.missing('raster');if(r.native===undefined||r.native===null)v.missing('raster.native');
 e.gate.session=v.text('gate.session',a.session,true);
 e.gate.pageGeneration=v.number('gate.pageGeneration',a.pageGeneration,true,false);
 e.gate.prepareId=v.number('gate.prepareId',a.prepareId,true,false);
 e.gate.observationSerial=v.number('gate.observationSerial',a.observationSerial,true,false);
 e.gate.inputSerial=v.number('gate.inputSerial',a.inputSerial,true,false);
 e.gate.startedMonoMs=v.number('gate.startedMonoMs',a.startedMonoMs,false,false);
 e.gate.state=v.text('gate.state',a.state,false);
 e.gate.reason=v.text('gate.reason',a.readinessReason,false);
 e.gate.diagnosticOverflow=v.bit('gate.diagnosticOverflow',a.diagnosticOverflow);
 e.ownerSerial=v.number('ownerSerial',ownerSerial,true,false);
 if(owner!==null){e.owner=new PairOwner();e.sources.branch="OWNER_COPY";
 e.owner.prepareId=v.number('owner.prepareId',owner.prepareId,true,false);
 e.owner.pageGeneration=v.number('owner.pageGeneration',owner.pageGeneration,true,false);
 e.owner.branch=v.number('owner.branch',owner.branch,true,false);
 e.owner.windowId=v.number('owner.windowId',owner.windowId,true,false);
 e.owner.windowGeneration=v.number('owner.windowGeneration',owner.windowGeneration,true,false);
 e.owner.controller=v.number('owner.controller',owner.controller,true,false);
 e.owner.session=v.number('owner.session',owner.session,true,false);
 e.owner.surface=v.number('owner.surface',owner.surface,true,false);
 e.owner.context=v.number('owner.context',owner.context,true,false);
 e.owner.run=v.text('owner.run',owner.run,true);
 }
 e.snapshot.windowId=v.number('snapshot.windowId',s.windowId,true,false);
 e.snapshot.windowGeneration=v.number('snapshot.windowGeneration',s.windowGeneration,true,false);
 e.snapshot.controller=v.number('snapshot.controller',s.controller,true,false);
 e.snapshot.session=v.number('snapshot.session',s.session,true,false);
 e.snapshot.surface=v.number('snapshot.surface',s.surface,true,false);
 e.snapshot.context=v.number('snapshot.context',s.context,true,false);
 e.snapshot.geometry=v.number('snapshot.geometry',s.geometry,true,false);
 e.snapshot.nativeGeometry=v.number('snapshot.nativeGeometry',s.nativeGeometry,true,false);
 e.snapshot.viewportWidth=v.number('snapshot.viewportWidth',s.viewportWidth,true,false);
 e.snapshot.viewportHeight=v.number('snapshot.viewportHeight',s.viewportHeight,true,false);
 e.snapshot.layoutGeneration=v.number('snapshot.layoutGeneration',s.layoutGeneration,true,false);
 e.snapshot.run=v.text('snapshot.run',s.run,true);
 e.snapshot.nativeReady=v.bit('snapshot.nativeReady',s.nativeReady);
 e.raster.observationSerial=v.number('raster.observationSerial',r.observationSerial);
 e.raster.revision=v.text('raster.revision',r.revision,false);
 e.raster.generation=v.number('raster.generation',r.generation,true,false);
 e.raster.geometry=v.number('raster.geometry',r.geometry,true,false);
 e.raster.rasterW=v.number('raster.rasterW',r.rasterW,true,false);
 e.raster.rasterH=v.number('raster.rasterH',r.rasterH,true,false);
 e.raster.didLayoutSerial=v.number('raster.didLayoutSerial',r.didLayoutSerial,true,false);
 e.raster.requestedAtLayout=v.number('raster.requestedAtLayout',r.requestedAtLayout,true,false);
 e.raster.layoutAcknowledged=v.number('raster.layoutAcknowledged',r.layoutAcknowledged,true,false);
 e.raster.firstDrawSerial=v.number('raster.firstDrawSerial',r.firstDrawSerial,true,false);
 e.raster.valid=v.bit('raster.valid',r.valid);
 e.raster.reason=v.text('raster.reason',r.reason,false);
 if(r.frameAnchorNs==='NOT_OBSERVED')v.missing('raster.frameAnchorNs');else e.raster.frameAnchorNs=v.text('raster.frameAnchorNs',r.frameAnchorNs);
 e.raster.native.controller=v.number('raster.native.controller',n.controller,true,false);
 e.raster.native.session=v.number('raster.native.session',n.session,true,false);
 e.raster.native.surface=v.number('raster.native.surface',n.surface,true,false);
 e.raster.native.context=v.number('raster.native.context',n.context,true,false);
 e.raster.native.geometry=v.number('raster.native.geometry',n.geometry,true,false);
 e.raster.native.packetWidth=v.number('raster.native.packetWidth',n.packetWidth,true,false);
 e.raster.native.packetHeight=v.number('raster.native.packetHeight',n.packetHeight,true,false);
 e.raster.native.viewportWidth=v.number('raster.native.viewportWidth',n.viewportWidth,true,false);
 e.raster.native.viewportHeight=v.number('raster.native.viewportHeight',n.viewportHeight,true,false);
 e.raster.native.run=v.text('raster.native.run',n.run,true);
 if(!Array.isArray(a.inputs)||a.inputs.length!==3)v.missing("inputs");
 for(let i=0;i<3;i++){const item=Array.isArray(a.inputs)?a.inputs[i]:undefined;const raw=item??new RawPairInput(),copy=new PairInput();if(item===undefined||item===null)v.missing("inputs["+i+"]");
 copy.created=v.bit("inputs["+i+"].created",raw.created);
 copy.editing=v.bit("inputs["+i+"].editing",raw.editing);
 copy.focused=v.bit("inputs["+i+"].focused",raw.focused);
 e.inputs.push(copy);}
 e.keyboard.configuration=v.text('keyboard.configuration',s.keyboardConfiguration,false);
 e.keyboard.deviceType=v.text('keyboard.deviceType',s.deviceType,false);
 e.keyboard.windowMode=v.number('keyboard.windowMode',s.windowMode,true,false);
 e.keyboard.avoidUnit=v.text('keyboard.avoidUnit',s.avoidUnit,false);
 e.keyboard.height=v.number('keyboard.height',s.keyboardHeight,false,false);
 e.keyboard.animation=v.bit('keyboard.animation',s.keyboardAnimation);
 e.keyboard.areaSeen=v.bit('keyboard.areaSeen',a.keyboardAreaSeen);
 e.keyboard.sessionEverEdited=v.bit('keyboard.sessionEverEdited',a.sessionEverEdited);
 if(!Array.isArray(s.avoid)||s.avoid.length!==4)v.missing("keyboard.avoid");
 for(let i=0;i<4;i++){const item=Array.isArray(s.avoid)?s.avoid[i]:undefined,raw=item??new RawPairRect(),copy=new PairRect();if(item===undefined||item===null)v.missing("keyboard.avoid["+i+"]");
 copy.x=v.number("keyboard.avoid["+i+"].x",raw.x,false,true);
 copy.y=v.number("keyboard.avoid["+i+"].y",raw.y,false,true);
 copy.width=v.number("keyboard.avoid["+i+"].width",raw.width,false,false);
 copy.height=v.number("keyboard.avoid["+i+"].height",raw.height,false,false);
 e.keyboard.avoid.push(copy);}
 if(!Array.isArray(a.events))v.missing('lastFrame.events');else {
  if(a.events.length>256)v.truncated('lastFrame.events.after256');
  for(let i=0;i<Math.min(256,a.events.length);i++){
   try {const event=JSON.parse(a.events[i]) as RawPairEvent;
    if(e.gate.prepareId===null||e.gate.pageGeneration===null||event===null||event.kind!=='FRAME'||event.prepareId!==e.gate.prepareId||event.pageGeneration!==e.gate.pageGeneration)continue;
    e.lastFrame=new PairLastFrame();e.lastFrame.status='RECORDED';e.lastFrame.observationSerial=v.number('lastFrame.observationSerial',event.serial);
    e.lastFrame.snapshotSerialMatches=e.lastFrame.observationSerial===null||e.gate.observationSerial===null?null:e.lastFrame.observationSerial===e.gate.observationSerial;
    const frame=JSON.parse(event.value??'null') as RawPairFrame;
    if(frame===null){v.missing('lastFrame.value');continue;}
    e.lastFrame.frame=v.number('lastFrame.frame',frame.frame);e.lastFrame.monoMs=v.number('lastFrame.monoMs',frame.now,false);e.lastFrame.reason=v.text('lastFrame.reason',frame.reason);

   }catch(error){v.missing('lastFrame.eventParse');}
  }
 }
 e.status=e.missingFields.length||e.truncatedFields.length?'INCOMPLETE':'COMPLETE';
 if(pairingUtf8Bytes(JSON.stringify(e))>4096){compactPreparePairingEvidence(e);if(pairingUtf8Bytes(JSON.stringify(e))>4096)e.sources.summaryBudget='ABOVE_4096_REQUIRED_FIELDS_RETAINED_HARD_ENVELOPE_CHECKED';}
 return e;
}
// Exceptional envelope pressure: preserve scope/window/packet/viewport fields; explicitly mark dropped diagnostic groups.
export function compactPreparePairingEvidence(e:PreparePairingEvidence):PreparePairingEvidence {
 e.status='INCOMPLETE';
 if(e.raster.reason!==null){e.raster.reason=null;if(!e.truncatedFields.includes('raster.reason'))e.truncatedFields.push('raster.reason');}
 if(e.gate.reason!==null){e.gate.reason=null;if(!e.truncatedFields.includes('gate.reason'))e.truncatedFields.push('gate.reason');}
 if(e.lastFrame.reason!==null){e.lastFrame.reason=null;if(!e.truncatedFields.includes('lastFrame.reason'))e.truncatedFields.push('lastFrame.reason');}
 return e;
}

// Last-resort envelope shape is explicit and method-free, including unavailable groups.
export class PairMinimumEvidence {
 revision:string='T680_PREPARE_PAIRING_EVIDENCE_V1';status:string='INCOMPLETE';gate:PairGate=new PairGate();ownerSerial:number|null=null;owner:PairOwner|null=null;snapshot:PairSnapshot=new PairSnapshot();raster:null=null;inputs:PairInput[]=[];keyboard:null=null;lastFrame:PairLastFrame=new PairLastFrame();missingFields:string[]=['raster','keyboard'];truncatedFields:string[]=[];sources:PairSources=new PairSources();
}
export function minimumPreparePairingEvidence(e:PreparePairingEvidence):PairMinimumEvidence {
 const m=new PairMinimumEvidence();m.gate=e.gate;m.ownerSerial=e.ownerSerial;m.owner=e.owner;m.snapshot=e.snapshot;m.inputs=e.inputs;m.lastFrame=e.lastFrame;m.truncatedFields=e.truncatedFields.slice();m.sources=e.sources;return m;
}
