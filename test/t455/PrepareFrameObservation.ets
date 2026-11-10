// Cold evidence for one PREPARING transaction. Each stage has a fixed
// first/last pair and count; no frame-time values are used as elapsed time.
export class PrepareFramePoint {
  seq: number = 0;
  appMs: number = 0;
  frameNs: number = 0;
  reason: string = '';
}
export class PrepareFrameStage {
  name: string;
  count: number = 0;
  first: PrepareFramePoint = new PrepareFramePoint();
  last: PrepareFramePoint = new PrepareFramePoint();
  constructor(name: string) { this.name = name; }
}
export class PrepareFrameObservation {
  readonly revision: string = 'T449_FRAME_OBSERVATION_V2';
  readonly prepareId: number;
  readonly pageGeneration: number;
  readonly windowId: number;
  readonly windowGeneration: number;
  readonly branch: number;
  readonly stages: PrepareFrameStage[] = [new PrepareFrameStage('POST'), new PrepareFrameStage('FRAME_ENTER'),
    new PrepareFrameStage('LAYOUT_DONE'), new PrepareFrameStage('DRAW_ENTER'), new PrepareFrameStage('SAMPLE'),
    new PrepareFrameStage('STALE_DROP')];
  layoutGeneration: number = 0;
  private serial: number = 0;
  private lastFrameNs: number = 0;
  private ticketNs: number = 0;
  private anchor: string = '';
  private pendingLayout: boolean = true;

  constructor(id: number, page: number, windowId: number, windowGeneration: number, branch: number) {
    this.prepareId = id; this.pageGeneration = page; this.windowId = windowId;
    this.windowGeneration = windowGeneration; this.branch = branch;
  }

  private note(stage: number, ms: number, frame: number, reason: string = ''): void {
    const point = new PrepareFramePoint();
    point.seq = ++this.serial; point.appMs = ms; point.frameNs = frame; point.reason = reason;
    const item = this.stages[stage];
    if (item.count === 0) item.first = point;
    item.last = point;
    ++item.count;
  }

  post(ms: number): void { this.note(0, ms, 0); }
  drop(ms: number, ns: number, reason: string): void { this.note(5, ms, ns, reason); }

  layoutChanged(): void {
    this.pendingLayout = true;
    this.ticketNs = 0;
  }

  didLayout(ms: number, anchor: string): void {
    if (anchor.length === 0) { this.drop(ms, this.ticketNs, 'ACTION_ANCHOR_MISSING'); return; }
    if (this.pendingLayout || anchor !== this.anchor) ++this.layoutGeneration;
    this.pendingLayout = false;
    this.anchor = anchor;
    this.note(2, ms, this.ticketNs);
  }

  frame(ms: number, ns: number): void {
    if (this.ticketNs !== 0) this.drop(ms, this.ticketNs, 'DRAW_NOT_DELIVERED');
    this.ticketNs = 0;
    if (!Number.isSafeInteger(ns) || ns <= this.lastFrameNs) { this.drop(ms, ns, 'FRAME_NOT_INCREASING'); return; }
    this.lastFrameNs = ns;
    this.ticketNs = ns;
    this.note(1, ms, ns);
  }

  matchesLayout(anchor: string): boolean {
    return !this.pendingLayout && this.layoutGeneration !== 0 && anchor.length > 0 && anchor === this.anchor;
  }

  draw(ms: number, anchor: string): number {
    const ns = this.ticketNs;
    this.ticketNs = 0;
    this.note(3, ms, ns);
    if (ns === 0) { this.drop(ms, ns, 'NO_FRAME_TICKET'); return 0; }
    if (!this.matchesLayout(anchor)) {
      this.drop(ms, ns, 'NO_CURRENT_LAYOUT'); return 0;
    }
    return ns;
  }

  sample(ms: number, ns: number): void { this.note(4, ms, ns); }

  timeoutReason(lastPredicate: string): string {
    if (this.stages[0].count === 0) return 'WAIT_POST';
    if (this.stages[1].count === 0) return 'WAIT_FRAME_ENTER';
    if (this.pendingLayout || this.layoutGeneration === 0) return 'WAIT_LAYOUT_DONE';
    if (this.stages[4].count === 0) return 'WAIT_DRAW_PAIR';
    return lastPredicate.length > 0 ? lastPredicate : 'WAIT_STABLE_FRAME';
  }
}
