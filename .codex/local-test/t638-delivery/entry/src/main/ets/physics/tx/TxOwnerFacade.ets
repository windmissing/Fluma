import { TxProfile, TxCommand, TxOriginalReceipt, TxLegacyBinding } from './TxTypes';
import { TxWorkTrace } from './TxWorkTrace';
export class TxOwnerHandle {
    private constructor() { }
    static cold(): TxOwnerHandle { return new TxOwnerHandle(); }
}
// Binding executes the original synchronous call and retains its original receipt.
// The facade adds no catch-all audit, root rollback or new physical refusal.
export class TxOwnerFacade {
    private readonly handle = TxOwnerHandle.cold();
    private readonly binding: TxLegacyBinding;
    readonly profile: TxProfile;
    readonly trace: TxWorkTrace;
    constructor(profile: TxProfile, binding: TxLegacyBinding, trace: TxWorkTrace) { this.profile = profile; this.binding = binding; this.trace = trace; }
    create(): TxOwnerHandle { return this.handle; }
    executeSync(owner: TxOwnerHandle, command: TxCommand): TxOriginalReceipt { if (owner !== this.handle)
        throw new Error('TX_FOREIGN_OWNER'); const s = this.trace.enter(1000); let code = 0; try {
        const r = this.binding.execute(command);
        code = r.originalCode;
        return r;
    }
    catch (e) {
        code = -1;
        throw e as Error;
    }
    finally {
        this.trace.leave(s, 1000, code);
    } }
}
