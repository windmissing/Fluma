import { P013SceneFrame, P013SceneReceipt } from '../../render/P013SceneTypes';
import { P011ParticleHotWorkspace } from '../P011ParticleHot';
import { DerivedCoordinatorV2 } from './P011DerivedTupleBarrierV2';

export interface Z08PublicationIdentity {
  kind: string;
  run: string;
  epoch: number;
  commit: number;
  sourceEpoch: number;
  ledgerKey: string;
  substep: number;
  attempt: string|null;
  committedOrdinal: number;
  seal: number;
  certificate: number;
  content: number;
}

interface Z08CrossReceipt {
  run: string;
  stateEpoch: number;
  substep: number;
  attempt: string|null;
  commit: number;
}

interface Z08FailureReceipt {
  failed: boolean;
  rolledBack: boolean;
  run: string;
  epoch: number;
  substep: number;
  attempt: string;
  cut: string;
  activeCommit: number;
}

class Z08LandStage {
  eventIndex = 0;
  ordinal = 0;
  kind = 2;
  slot = -1;
  id = 0;
  sourceId = 0;
  phase = 0;
  sourceKind = 0;
  generation = 0;
  generationAfter = 0;
  column = -1;
  targetColumn = -1;
  requested = 0;
  granted = 0;
  remainder = 0;
  ticket = 0;
  reserved = 0;
  refunded = 0;
  localBefore = 0;
  localAfter = 0;
  totalBefore = 0;
  totalAfter = 0;
  bedBefore = 0;
  bedAfter = 0;
  rowState = 0;
  readonly rowMotion = new Float64Array(5);
}

class Z08LandEvidence {
  readonly event: Z08LandStage;
  readonly rowF64Hex: string[];
  readonly finalF64Hex: string[];
  readonly finalState: number;
  readonly retired: boolean;
  readonly finalGeneration: number;
  readonly finalSlotId: number;
  readonly finalSlotState: number;
  readonly bedFinal: number;
  readonly verified: boolean;
  constructor(row: Z08LandStage, owner: DerivedCoordinatorV2) {
    // Freeze scalar event fields and original bit patterns before scratch reuse.
    this.event = new Z08LandStage();
    const e = this.event;
    e.eventIndex=row.eventIndex;e.ordinal=row.ordinal;e.slot=row.slot;e.id=row.id;
    e.sourceId=row.sourceId;e.phase=row.phase;e.sourceKind=row.sourceKind;
    e.generation=row.generation;e.generationAfter=row.generationAfter;
    e.column=row.column;e.targetColumn=row.targetColumn;
    e.requested=row.requested;e.granted=row.granted;e.remainder=row.remainder;
    e.ticket=row.ticket;e.reserved=row.reserved;e.refunded=row.refunded;
    e.localBefore=row.localBefore;e.localAfter=row.localAfter;
    e.totalBefore=row.totalBefore;e.totalAfter=row.totalAfter;
    e.bedBefore=row.bedBefore;e.bedAfter=row.bedAfter;e.rowState=row.rowState;
    e.rowMotion.set(row.rowMotion);
    this.rowF64Hex = f64Hex(row.rowMotion);
    const p = owner.canonical.pool, slot = row.slot;
    this.finalSlotId = p.pId[slot];
    this.finalSlotState = p.pState[slot];
    this.finalGeneration = owner.canonical.generations[slot];
    this.retired = row.rowState===0 && row.remainder===0 && row.granted>0 &&
      row.generationAfter===row.generation+1 && (this.finalSlotId!==row.id || this.finalSlotState===0);
    const live = this.finalSlotId===row.id && this.finalGeneration===row.generationAfter &&
      p.pSourceId[slot]===row.sourceId && p.pPhase[slot]===row.phase && p.pSource[slot]===row.sourceKind;
    this.finalState = this.retired ? 0 : live ? p.pState[slot] : -1;
    this.finalF64Hex = live ? f64Hex(new Float64Array([p.pX[slot],p.pY[slot],p.pVx[slot],p.pVy[slot],p.pMass[slot]])) : [];
    this.verified = this.retired || live && row.remainder>0 && p.pMass[slot]===row.remainder &&
      this.finalState===2 && Math.floor(p.pX[slot])===row.column;
    this.bedFinal = owner.domain.field.surface[row.column];
  }
}

function f64Hex(words: Float64Array): string[] {
  const bytes = new DataView(new ArrayBuffer(8));
  const out: string[] = [];
  for (let i=0;i<words.length;i++) {
    bytes.setFloat64(0,words[i],false);
    out.push(bytes.getUint32(0,false).toString(16).padStart(8,'0')+
      bytes.getUint32(4,false).toString(16).padStart(8,'0'));
  }
  return out;
}

class Z08Candidate {
  readonly publication: Z08PublicationIdentity;
  readonly cross: Z08CrossReceipt|null;
  readonly lands: Z08LandEvidence[];
  readonly dt: number;
  readonly effectiveTiltDeg: number;
  incomplete: boolean;
  presentation = 'NOT_SELECTED_FOR_DRAW';
  constructor(pub: Z08PublicationIdentity, cross: Z08CrossReceipt|null, rows: Z08LandEvidence[],
    dt: number, tilt: number, incomplete: boolean) {
    this.publication=pub;this.cross=cross;this.lands=rows;
    this.dt=dt;this.effectiveTiltDeg=tilt;this.incomplete=incomplete;
  }
}

function samePublication(a: Z08PublicationIdentity,b: Z08PublicationIdentity): boolean {
  return a.kind===b.kind && a.run===b.run && a.epoch===b.epoch && a.commit===b.commit &&
    a.sourceEpoch===b.sourceEpoch && a.ledgerKey===b.ledgerKey && a.substep===b.substep &&
    a.attempt===b.attempt && a.committedOrdinal===b.committedOrdinal && a.seal===b.seal &&
    a.certificate===b.certificate && a.content===b.content;
}

export function z08Presentation(pub: Z08PublicationIdentity, frame: P013SceneFrame|null,
  receipt: P013SceneReceipt|null, frameNs: number): string {
  if (frame===null || receipt===null) return 'DRAW_UNAVAILABLE';
  if (receipt.presentationState!=='CURRENT') return receipt.presentationState==='STALE'?'STALE':'DRAW_FAULT';
  const rendered=receipt.renderedSource;
  if (!receipt.currentFrameSucceeded || !receipt.requestedSwapSucceeded || rendered===null ||
    !frame.frameToken || frame.frameId<=0 || frame.frameTimeNs!==frameNs ||
    frame.frameToken!==receipt.requestedFrameToken || frame.frameToken!==receipt.renderedFrameToken ||
    rendered.frameToken!==frame.frameToken) return 'TOKEN_OR_SWAP_MISMATCH';
  if (frame.runId!==pub.run || frame.stateEpoch!==pub.epoch || frame.fieldEpoch!==pub.epoch || frame.physicsCommitId!==pub.commit ||
    frame.ledgerCommitKey!==pub.ledgerKey || rendered.runId!==pub.run || rendered.stateEpoch!==pub.epoch ||
    rendered.physicsCommitId!==pub.commit || rendered.ledgerCommitKey!==pub.ledgerKey ||
    rendered.surfaceEpoch!==frame.surfaceEpoch || receipt.targetSurfaceEpoch!==frame.surfaceEpoch ||
    rendered.contextEpoch!==frame.contextEpoch || receipt.targetContextEpoch!==frame.contextEpoch ||
    rendered.geometryEpoch!==frame.geometryEpoch || rendered.continuityEpoch!==frame.continuityEpoch ||
    rendered.generation!==frame.requestedGeneration || receipt.requestedGeneration!==frame.requestedGeneration ||
    receipt.renderedGeneration!==frame.requestedGeneration || receipt.actualPresentationGeneration!==frame.requestedGeneration ||
    rendered.configRevision!==frame.appearanceConfigRevision || receipt.requestedConfigRevision!==frame.appearanceConfigRevision ||
    receipt.renderedConfigRevision!==frame.appearanceConfigRevision) return 'SOURCE_IDENTITY_MISMATCH';
  return 'PRESENT_CURRENT_SWAP';
}

// One explicit reservation. No recording or publication work outside an armed callback.
export class P011Z08Archive {
  private static readonly MAX_IDS=8;
  private static readonly MAX_ROWS=16;
  private static readonly MAX_FRAMES=2400;
  private readonly staged: Z08LandStage[]=[];
  readonly ids: Uint32Array;
  private count=0;
  private incomplete=false;
  private frameIncomplete=false;
  private frames=0;
  private candidates: Z08Candidate[]=[];
  private frameOpen=false;
  private output='';
  readonly waitForLand: boolean;
  constructor(ids: number[], waitForLand: boolean) {
    if (ids.length===0 || ids.length>P011Z08Archive.MAX_IDS) throw new Error('Z08_IDS');
    for (let i=0;i<ids.length;i++) {
      if (!Number.isSafeInteger(ids[i]) || ids[i]<1 || ids[i]>0xffffffff || ids.indexOf(ids[i])!==i)
        throw new Error('Z08_IDS');
    }
    this.ids=new Uint32Array(ids);this.waitForLand=waitForLand;
    for (let i=0;i<P011Z08Archive.MAX_ROWS;i++) this.staged.push(new Z08LandStage());
  }
  beginFrame(): void {
    if (this.output.length!==0 || this.frameOpen) return;
    this.frames++;this.frameOpen=true;this.candidates.length=0;this.frameIncomplete=false;this.beginStep();
  }
  beginStep(): void { this.count=0;this.incomplete=false; }
  markIncomplete(): void { this.frameIncomplete=true; }
  selects(id: number): boolean { return this.frameOpen && this.ids.indexOf(id)>=0; }
  stage(work: P011ParticleHotWorkspace, eventIndex: number, generation: number, ticket: number,
    reserved: number, refunded: number, bedBefore: number, bedAfter: number,
    localBefore: number, localAfter: number, totalBefore: number, totalAfter: number): void {
    const e=work.events, slot=work.crossEditedSlot, owner=work.derivedV2;
    if (this.count===this.staged.length || owner===null || e.kind[eventIndex]!==2 || slot<0 ||
      slot>=work.nmax || work.patchBefore.pId[slot]!==e.particleId[eventIndex]) {
      this.incomplete=true;
      return;
    }
    const row=this.staged[this.count++], after=work.patchAfter;
    row.eventIndex=eventIndex;row.ordinal=e.requestOrdinal[eventIndex];row.slot=slot;row.id=e.particleId[eventIndex];
    row.sourceId=e.sourceId[eventIndex];row.phase=e.phase[eventIndex];row.sourceKind=e.sourceKind[eventIndex];
    row.generation=generation;row.generationAfter=owner.canonical.generations[slot];
    row.column=e.column[eventIndex];row.targetColumn=e.targetColumn[eventIndex];
    row.requested=e.requested[eventIndex];row.granted=e.granted[eventIndex];row.remainder=e.remainder[eventIndex];
    row.ticket=ticket;row.reserved=reserved;row.refunded=refunded;
    row.bedBefore=bedBefore;row.bedAfter=bedAfter;
    row.localBefore=localBefore;row.localAfter=localAfter;row.totalBefore=totalBefore;row.totalAfter=totalAfter;
    row.rowState=after.pState[slot];
    row.rowMotion[0]=after.pX[slot];row.rowMotion[1]=after.pY[slot];
    row.rowMotion[2]=after.pVx[slot];row.rowMotion[3]=after.pVy[slot];row.rowMotion[4]=after.pMass[slot];
    if (row.rowState!==0 && row.id!==after.pId[slot]) this.incomplete=true;
  }
  commit(pub: Z08PublicationIdentity|null, owner: DerivedCoordinatorV2, crossText: string, dt: number, tilt: number): void {
    if (!this.frameOpen) return;
    if (pub===null || this.candidates.length===2) { this.markIncomplete();return; }
    const cross=JSON.parse(crossText) as Z08CrossReceipt;
    const matched=cross.run===pub.run && cross.stateEpoch===pub.epoch && cross.commit===pub.commit &&
      cross.substep===pub.substep && cross.attempt===pub.attempt;
    const lands: Z08LandEvidence[]=[];
    let incomplete=this.incomplete||!matched;
    for (let i=0;i<this.count;i++) {
      const evidence=new Z08LandEvidence(this.staged[i],owner);
      if (!evidence.verified || lands.some((prior: Z08LandEvidence)=>prior.event.id===evidence.event.id))
        incomplete=true;
      lands.push(evidence);
    }
    this.candidates.push(new Z08Candidate(pub,matched?cross:null,lands,dt,tilt,incomplete));
  }
  finishFrame(pub: Z08PublicationIdentity|null, frame: P013SceneFrame|null, receipt: P013SceneReceipt|null,
    frameNs: number, pageFrame: number, failureText: string): void {
    if (!this.frameOpen) return;
    this.frameOpen=false;
    const failure=failureText.length===0?null:JSON.parse(failureText) as Z08FailureReceipt;
    const last=this.candidates.length===0?null:this.candidates[this.candidates.length-1];
    const selected=pub!==null && last!==null && samePublication(last.publication,pub) ? last : null;
    let presentation='PUBLICATION_NOT_FROM_CALLBACK';
    if (selected!==null && pub!==null) {
      presentation=z08Presentation(pub,frame,receipt,frameNs);
      selected.presentation=presentation;
    }
    const positive=this.candidates.some((candidate: Z08Candidate)=>candidate.lands.some(
      (land: Z08LandEvidence)=>land.event.granted>0));
    const fault=failure!==null && failure.failed;
    const aborted=failureText.length>0;
    if (this.waitForLand && !positive && !aborted && this.frames<P011Z08Archive.MAX_FRAMES) return;
    const incomplete=this.frameIncomplete || this.incomplete || this.candidates.some((c: Z08Candidate)=>c.incomplete);
    this.output=JSON.stringify({
      revision:'T392_Z08_ARCHIVE_V1', ids:Array.from(this.ids), waitForLand:this.waitForLand,
      callbacks:this.frames, frameNs:frameNs, pageFrame:pageFrame, candidates:this.candidates,
      selectedPublication:pub, presentation:presentation, renderReceipt:receipt,
      nativeFrame:frame===null?0:frame.frameId, systemPresentVerified:receipt?.displayPresentVerified??false,
      failure:fault?failure:null, incomplete:incomplete || this.waitForLand && !positive && !fault,
      captureStatus:fault?'PHYSICS_FAILURE':aborted?'STEP_FAILED_WITHOUT_CROSS_RECEIPT':this.waitForLand&&!positive?'LAND_TIMEOUT':
        incomplete?'INCOMPLETE':selected===null?'PUBLICATION_MISSING':'CAPTURED'
    });
  }
  exportCold(): string { return this.output; }
}
