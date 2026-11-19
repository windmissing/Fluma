import { WaterSandField } from '../WaterSandField';
import { defaultP011ParticleParams } from '../P011ParticleM2Field';
import { TxProfile, TxCommand, TxStepCommand, TxSourceCommand, TxQueuedSourceCommand, TxResetCommand, TxConfigureCommand, TxCaptureCommand, TxOriginalReceipt, TxLegacyBinding } from './TxTypes';
import { TxOwnerFacade, TxOwnerHandle } from './TxOwnerFacade';
import { TxWholePairAdapter } from './TxWholePairAdapter';
import { TxCapture } from './TxCapture';
export class TxFixtureDriver implements TxLegacyBinding {
    private readonly field: WaterSandField;
    readonly core: TxWholePairAdapter;
    readonly owner: TxOwnerHandle;
    private readonly facade: TxOwnerFacade;
    constructor(nmax: number, backend: string) { this.field = new WaterSandField(128, 512); const params = defaultP011ParticleParams(); params.nmax = nmax; if (!this.field.setParticleRoute(params).ok)
        throw new Error('TX_FIXTURE_COLD_PARAMS'); this.field.setNaturalObservationRouteId('dual-field-particle-v1'); this.core = this.field.attachWholeTransactionD1(backend); this.facade = new TxOwnerFacade(new TxProfile(), this, this.core.trace); this.owner = this.facade.create(); }
    run(command: TxCommand): TxOriginalReceipt { this.core.beginRoot(); try {
        return this.facade.executeSync(this.owner, command);
    }
    finally {
        this.core.endRoot();
    } }
    execute(command: TxCommand): TxOriginalReceipt { if (command instanceof TxStepCommand) {
        const r = this.field.step(command.dt);
        return new TxOriginalReceipt(r.errorCode === null ? 0 : r.errorCode, 'STEP', r);
    } if (command instanceof TxSourceCommand) {
        const r = this.field.injectGestureSand(command.request);
        return new TxOriginalReceipt(r === null || r.rejectedReason !== null ? 1 : 0, 'DIRECT_SOURCE', null, r);
    } if (command instanceof TxQueuedSourceCommand) {
        this.field.stageGestureSand(command.request);
        return new TxOriginalReceipt(0, 'QUEUED_NOT_ADMITTED');
    } if (command instanceof TxResetCommand) {
        this.field.reset(command.scene);
        return new TxOriginalReceipt(0, 'RESET');
    } if (command instanceof TxConfigureCommand) {
        this.field.configure(command.params);
        this.core.trace.add(37);
        return new TxOriginalReceipt(0, 'CONFIGURE_LEGACY_UNCERTIFIED');
    } if (command instanceof TxCaptureCommand) {
        return new TxOriginalReceipt(0, 'CAPTURE', null, null, this.field.snapshot());
    } throw new Error('TX_FIXTURE_COMMAND_NOT_SUPPORTED'); }
    capture(): TxCapture { return new TxCapture(this.core.trace, 'ORIGINAL_RECEIPT_AUTHORITY', 'NOT_ARCHIVED'); }
    archiveState(): string { const t = this.core.trace, scope = t.enter(1001); let code = 0; try {
        const text = JSON.stringify(this.field.snapshot());
        let bytes = 0;
        for (let i = 0; i < text.length; i++) {
            const c = text.charCodeAt(i);
            if (c < 128)
                bytes++;
            else if (c < 2048)
                bytes += 2;
            else if (c >= 0xd800 && c <= 0xdbff && i + 1 < text.length && text.charCodeAt(i + 1) >= 0xdc00 && text.charCodeAt(i + 1) <= 0xdfff) {
                bytes += 4;
                i++;
            }
            else
                bytes += 3;
        }
        t.add(33, bytes);
        return text;
    }
    catch (e) {
        code = -1;
        throw e as Error;
    }
    finally {
        t.leave(scope, 1001, code);
    } }
}
