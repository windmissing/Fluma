import { P011GestureSandRequest, WaterSandStepResult, WaterSandParams, WaterSandScene, WaterSandSnapshot } from '../WaterSandField';
import { P011InjectionReceipt } from '../../render/P012SandTracer';
// D1 synchronous compatibility types. No owner admission/async/rejection policy.
export class TxProfile {
    readonly revision = 'TX_EXACT_STRUCTURE_V1';
    readonly storage = 'WHOLE_BANK';
    readonly viewSlots = 8;
    readonly depthMax = 8;
    readonly numericAudit = 'ORIGINAL_SLOT_FOLD_ALWAYS';
}
export class TxCommand {
    readonly kind: string;
    constructor(kind: string) { this.kind = kind; }
}
export class TxStepCommand extends TxCommand {
    readonly dt: number;
    constructor(dt: number) { super('STEP'); this.dt = dt; }
}
export class TxSourceCommand extends TxCommand {
    readonly request: P011GestureSandRequest;
    constructor(request: P011GestureSandRequest) { super('DIRECT_SOURCE'); this.request = request; }
}
export class TxQueuedSourceCommand extends TxCommand {
    readonly request: P011GestureSandRequest;
    constructor(request: P011GestureSandRequest) { super('QUEUED_SOURCE'); this.request = request; }
}
export class TxResetCommand extends TxCommand {
    readonly scene: WaterSandScene | undefined;
    constructor(scene?: WaterSandScene) { super('RESET'); this.scene = scene; }
}
export class TxConfigureCommand extends TxCommand {
    readonly params: WaterSandParams;
    constructor(params: WaterSandParams) { super('CONFIGURE'); this.params = params; }
}
export class TxCaptureCommand extends TxCommand {
    constructor() { super('CAPTURE'); }
}
export class TxOriginalReceipt {
    readonly originalCode: number;
    readonly originalStage: string;
    readonly step: WaterSandStepResult | null;
    readonly source: P011InjectionReceipt | null;
    readonly snapshot: WaterSandSnapshot | null;
    constructor(code: number, stage: string, step: WaterSandStepResult | null = null, source: P011InjectionReceipt | null = null, snapshot: WaterSandSnapshot | null = null) { this.originalCode = code; this.originalStage = stage; this.step = step; this.source = source; this.snapshot = snapshot; }
}
export interface TxLegacyBinding {
    execute(command: TxCommand): TxOriginalReceipt;
}
