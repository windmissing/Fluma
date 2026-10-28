import { P051GravityInput } from '../P051GravityInputAdapter';
import { P051DerivedFacadeV2, DerivedPublishedSlotV2 } from './P051DerivedFacadeV2';
import { P011GestureRateClock } from '../P011GestureRateClock';
import { P051_DRY_REVISION } from '../P051DryParticleAdmission';
import { DERIVED_POLICY_V2 } from './P011DerivedTypesV2';
// Isolated new-run model: original V1 model/default page remains unchanged.
export class P051DerivedDemoV2 {
    private readonly owner: P051DerivedFacadeV2;
    private readonly clock = new P011GestureRateClock();
    private held = false;
    private x = 64;
    private y = 220;
    private seq = 0;
    private active = true;
    status = 'READY';
    constructor(nmax: number = 512) { this.owner = new P051DerivedFacadeV2({ isolated: true, numericPolicy: DERIVED_POLICY_V2, nmax: nmax, sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2' }); }
    start(x: number, y: number): boolean {
        if (!this.active || !Number.isFinite(x) || x < 0 || x >= 128 || !Number.isFinite(y) || y < 0 || y > 256)
            return false;
        this.x = x;
        this.y = y;
        this.held = true;
        this.clock.begin();
        return true;
    }
    move(x: number, y: number): void {
        if (!Number.isFinite(x) || x < 0 || x >= 128 || !Number.isFinite(y) || y < 0 || y > 256) {
            this.stop();
            return;
        }
        this.x = x;
        this.y = y;
    }
    stop(): void { this.held = false; this.clock.stop(); }
    setActive(active: boolean): void {
        this.active = active;
        if (!active)
            this.stop();
    }
    applyDefinedInput(input: P051GravityInput): void {
        if (!input.valid || !this.active)
            return;
        if (input.targetTiltDeg !== null)
            this.owner.setTilt(input.targetTiltDeg);
        this.owner.setShakeInput(input.shakeInput);
        if (input.source === 'stale')
            this.stop();
    }
    setTilt(degrees: number): void { this.owner.setTilt(degrees); }
    setShakeInput(value: number): void { this.owner.setShakeInput(value); }
    reset(): void { this.stop(); this.owner.reset(); this.seq = 0; this.status = 'READY'; }
    tick(ns: number, dt: number): boolean {
        if (!this.active)
            return true;
        if (this.held) {
            const seconds = this.clock.sample(ns, 'ns');
            if (seconds > 0) {
                const r = this.owner.inject({ revision: P051_DRY_REVISION, sourceId: 51, sequence: ++this.seq, inputTimeMs: ns / 1e6, x: this.x, y: this.y, seed: 51, requestedH: 8 * seconds, requestedL: 0 });
                if (!r.ok) {
                    this.stop();
                    this.status = r.derivedFault === null ? r.original?.rejectedReason ?? 'SOURCE_FAILED' : r.derivedFault.namespace + ':' + r.derivedFault.code;
                }
            }
        }
        const r = this.owner.step(dt);
        if (!r.ok) {
            this.stop();
            this.status = r.derivedFault === null ? 'ORIGINAL:' + r.originalError : r.derivedFault.namespace + ':' + r.derivedFault.code;
            return false;
        }
        return true;
    }
    createPublicationCold(): DerivedPublishedSlotV2 { return this.owner.createPublishedSlotCold(); }
    copyPublicationInto(slot: DerivedPublishedSlotV2): string { return this.owner.copyPublishedRenderStateInto(slot); }
}
