// Isolated T250 fixture contract. No production route installs this plan.
export interface TxFaultPlan {
  fixtureId:string;buildSHA:string;backend:string;freezeArm:string;PID:number;
  rootId:number;cutId:number;edge:string;ordinal:number;
}
export function validateTxFaultPlan(p:TxFaultPlan,fixtureId:string,buildSHA:string,
  backend:string,freezeArm:string,pid:number):string {
  if(p.fixtureId!==fixtureId||p.buildSHA!==buildSHA||p.backend!==backend||
    p.freezeArm!==freezeArm||p.PID!==pid)return 'FAULT_PLAN_IDENTITY_MISMATCH';
  if(!Number.isSafeInteger(p.rootId)||p.rootId<1||
    !Number.isSafeInteger(p.cutId)||p.cutId<1||p.cutId>20||
    (p.edge!=='BEFORE'&&p.edge!=='AFTER')||
    !Number.isSafeInteger(p.ordinal)||p.ordinal<1)return 'FAULT_PLAN_RANGE';
  if(p.cutId===6&&p.edge==='BEFORE')return 'READY_REQUEST_OWNER_CORRUPTION';
  return 'NOT_IMPLEMENTED_ORIGINAL_CUT_INJECTOR';
}
