// T209 / T203 + T196 V1. Layout constants are the frozen engineering manifest.
import { ClearWaterStatus, ClearWaterAllocation } from './P011ClearWaterTypes';
export const CW_B_COUNTERS: string[]=["frameEnqueueCalls", "frameSourceBoundaries", "frameSourceProcessed", "frameSourceAccepted", "frameSourceRejected", "frameSourceBirths", "frameSubstepsAttempted", "frameSubstepsCommitted", "frameSubstepsFailed", "frameCandidates", "stepEntryAudits", "stepWorkAudits", "stepReplayAudits", "stepTrajectoryVisits", "stepFinalFoldVisits", "stepReplayFinalFoldVisits", "waterEdgeVisits", "bedEdgeVisits", "moveRows", "mediumRows", "crossRows", "contactRows", "landRows", "bedRows", "waterRows", "lineageRows", "massEncodeCalls", "massAddCalls", "massSubCalls", "massCompareCalls", "wholePoolCopies", "copyBytes", "replayRows", "activeUnlinks", "wholeCacheRebuilds", "noEffectCount", "capacityBlockedCount", "budgetBlockedCount", "highBedBlockedCount", "wholeIndexRebuilds", "mergeCalls", "lambdaAttempts", "sourceEnqueueScanRows", "sourceDedupScanRows", "sourceCopyBytes", "coldExportBytes", "firstFailureStage", "firstFailureOrdinal"];
export const CW_B_ARENA_HEADER: string[]=["arenaSerial", "eventCount", "lineageCount", "physicsCommit", "sourceCommit", "runGeneration", "configRev", "inputGeneration", "geometryEpoch", "stepSerial", "attemptSerial", "acceptedAngleDeg", "dt", "clockWindowId", "ownerId", "eventLimitPerParticle", "eventCapacity", "lineageCapacity", "cols", "particleCapacity", "waterEdgesVisited", "bedEdgesVisited", "entryParticleCount", "trajectoryParticleVisits", "geometryFoldVisits", "replayRows", "auditCalls", "copyBytes", "maxResidual", "allEdgesWithinH", "noEffectCount", "capacityBlockedCount", "budgetBlockedCount", "highBedBlockedCount", "sourceBoundaryCountInFrame", "sourceProcessedInFrame", "sourceBirthsInFrame", "frameSubsteps", "candidateCount", "implementationVersionNumber", "entryAuditMs", "copyMs", "accBuildMs", "waterMs", "particleMs", "bedMs", "finalFoldMs", "workAuditMs", "replayMs", "replayAuditMs", "commitMs", "wholeSubstepMs", "frameSourceBoundaryMs", "frameEnqueueMs", "frameColdExportMs", "clockStatus", "reserved56", "reserved57", "reserved58", "reserved59", "reserved60", "reserved61", "reserved62", "reserved63"];
export const CW_B_STATE_HEADER: string[]=["stepSerial", "acceptedAngleDeg", "acceptedDt", "lastBPhysicsCommit", "sourceCommitAtLastB", "sealedArenaSerial", "eventCount", "lineageCount", "runGeneration", "configRev", "inputGenerationAtLastB", "geometryEpochAtLastB", "maxResidual", "allEdgesWithinH", "lastSuccessfulAttemptSerial", "reserved0"];
export const CW_B_BUDGET_SCALARS: string[]=["dt", "totalLimit", "workTotalRemaining", "workTotalUsed", "replayTotalRemaining", "replayTotalUsed", "angleDeg", "angleRad", "tanAngle", "sinAngle", "cosAngle", "maxResidual", "allEdgesWithinH", "patchRequested", "patchGranted", "patchReason", "guardResidual0", "guardResidual1", "guardResidual2", "guardInfluence0", "guardInfluence1", "guardInfluence2", "guardEdge0", "guardEdge1", "guardEdge2", "guardBound0", "guardBound1", "guardBound2", "reserved28", "reserved29", "reserved30", "reserved31"];
export const CW_B_SUMMARY: string[]=["id", "slot", "origin", "state", "waitReason", "column", "mass", "x", "y", "vx", "vy", "processedTime", "remainingTime", "eventStartOrdinal", "eventCount", "flags"];
export enum BReason { NONE=0,INVALID_CONFIG=1,INVALID_IDENTITY=2,INPUT_INVALID=3,STORAGE_OR_ALIAS=4,UNSUPPORTED_FEATURE=5,ORIGIN_UNRESOLVED=6,TOKEN_EXHAUSTED=7,MASS_ENCODING_DOMAIN=8,MASS_ACCUM_OVERFLOW=9,MASS_ACCUM_UNDERFLOW=10,EVENT_LIMIT=11,EVENT_ARENA_FULL=12,LINEAGE_ARENA_FULL=13,STEP_TIME_NO_PROGRESS=14,LOCAL_NUMERIC_INVALID=15,WORK_AUDIT_FAILED=16,FINAL_GUARD_FAILED=17,REPLAY_OLD_BITS_MISMATCH=18,REPLAY_RESULT_MISMATCH=19,COMMIT_STORAGE_INVALID=20,NUMERIC_NO_EFFECT=21,CAPACITY_NO_GRANT=22,BUDGET_NO_GRANT=23,BUDGET_QUANTUM_BLOCKED=24,HIGH_BED_WAIT=25 }
export enum BKind {MOVE=1,MEDIUM=2,CROSS=3,CONTACT=4,LAND=5,BED=6,WATER=7}
export const CW_B_USED_MASKS:number[]=[0,962206891647,962206891647,1097498361855,962206891647,1097490497535,1097497839591,964085940711];
export interface ClearWaterBConfig {
 revision:string;configRev:number;stepRevision:string;eventRevision:string;lineageRevision:string;priorityRevision:string;contactRevision:string;accRevision:string;
 gModel:number;dragAir:number;maxAvalancheFlux:number;e:number;eventCapacity:number;lineageCapacity:number;calibrationStatus:string;
}
export function cwBConfigValid(c:ClearWaterBConfig,k:number):boolean {
 return c.revision==='CW_B_CONFIG_V1'&&Number.isSafeInteger(c.configRev)&&c.configRev>0&&c.stepRevision==='CW_B_STEP_V1'&&c.eventRevision==='CW_B_EVENT40_V1'&&
 c.lineageRevision==='CW_B_LINEAGE3_V1'&&c.priorityRevision==='CW_B_SERIAL_PRIORITY_V1'&&c.contactRevision==='CW_POINT_CONTACT_V1'&&c.accRevision==='CW_MASS_ACCUM70_V1'&&
 Number.isFinite(c.gModel)&&c.gModel>=0&&Number.isFinite(c.dragAir)&&c.dragAir>=0&&Number.isFinite(c.maxAvalancheFlux)&&c.maxAvalancheFlux>=0&&
 Number.isSafeInteger(c.e)&&c.e>=2&&Number.isSafeInteger(c.eventCapacity)&&c.eventCapacity>=c.e*k+254&&Number.isSafeInteger(c.lineageCapacity)&&c.lineageCapacity>=2*c.eventCapacity&&
 Number.isSafeInteger(c.eventCapacity*40)&&Number.isSafeInteger(c.lineageCapacity*3)&&c.calibrationStatus==='PENDING_CALIBRATION';
}
export class ClearWaterBAttachResult {status:ClearWaterStatus=ClearWaterStatus.OK;reason:BReason=BReason.NONE;payloadBytes:number=0;}
export class ClearWaterBEvidenceBuffers {
 expectedOwnerId:number=0;expectedRunGeneration:number=0;expectedArenaSerial:number=0;
 header:Float64Array;events:Float64Array;lineage:Float64Array;summary:Float64Array;
 currentPhysicsCommit:number=0;currentSourceCommit:number=0;
 constructor(e:number,l:number,k:number){this.header=new Float64Array(64);this.events=new Float64Array(e*40);this.lineage=new Float64Array(l*3);this.summary=new Float64Array(k*16);}
}
export class ClearWaterBFrameCounters {
 values:Float64Array=new Float64Array(48);scope:string='PARTIAL_SCOPE';frameId:number=0;N:number=128;K:number=0;F:number=0;e:number=0;Q:number=0;B:number=0;L:number=0;V:number=1;
}
export function bFlags(os:number,ns:number,ow:number,nw:number,om:number,nm:number,reason:number):number{return os+ns*4+ow*16+nw*128+om*1024+nm*4096+reason*16384;}
export function bField(flags:number,shift:number,width:number):number{return Math.floor(flags/Math.pow(2,shift))%Math.pow(2,width);}
export function bAllocation(name:string,type:string,length:number,bytes:number):ClearWaterAllocation{return {name:name,type:type,length:length,byteLength:bytes,lifetime:'COLD_RUN_FIXED'};}
