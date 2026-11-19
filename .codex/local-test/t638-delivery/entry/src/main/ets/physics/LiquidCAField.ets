import { LiquidCAConfig, LiquidCAEventPayload, LiquidCAFramePayload, LiquidCAMetricPayload,
  LiquidCAObserver, LiquidInputSource, LiquidRenderSnapshot } from './LiquidCATypes';

const EMPTY = 0;
const SAND = 1;
const HEAVY = 0;
const LIGHT = 1;
const PHYSICS_DT_SECONDS = 1 / 120;

export class LiquidCAField {
  private config: LiquidCAConfig;
  private cells: Uint8Array;
  private phase: Uint8Array;
  private shade: Uint8Array;
  private activeBits: Uint8Array;
  private heavyCount: number = 0;
  private lightCount: number = 0;
  private initialHeavyCount: number = 0;
  private initialLightCount: number = 0;
  private gravityDown: boolean = true;
  private shakeEnergy: number = 0;
  private targetShakeEnergy: number = 0;
  private accumulatorSeconds: number = 0;
  private idleSubsteps: number = 0;
  private dormant: boolean = false;
  private rngState: number = 1;
  private simulationTimeSeconds: number = 0;
  private eventSequence: number = 0;
  private frameSequence: number = 0;
  private lastFrameTimeMs: number = 0;
  private lastPhysicsTimeMs: number = 0;
  private observer: LiquidCAObserver | null = null;
  private metricCache: LiquidCAMetricPayload | null = null;
  private frameCache: LiquidCAFramePayload | null = null;
  private fatalFinite: boolean = false;
  private changedCellsSinceMetric: number = 0;

  constructor(config: LiquidCAConfig) {
    this.config = config;
    if (!Number.isInteger(config.cols) || !Number.isInteger(config.rows) ||
      config.cols < 2 || config.rows < 2 || config.cols * config.rows > config.maxCells) {
      this.cells = new Uint8Array(0);
      this.phase = new Uint8Array(0);
      this.shade = new Uint8Array(0);
      this.activeBits = new Uint8Array(0);
      return;
    }
    const cellCount = config.cols * config.rows;
    this.cells = new Uint8Array(cellCount);
    this.phase = new Uint8Array(cellCount);
    this.shade = new Uint8Array(cellCount);
    this.activeBits = new Uint8Array((cellCount + 7) >> 3);
    this.resetState();
  }

  initialize(eventTimeMs: number = 0): LiquidCAEventPayload {
    if (this.cells.length === 0) {
      return this.emitEvent('model_error', eventTimeMs, LiquidInputSource.INTERNAL, 'GRID_TOO_LARGE', true);
    }
    this.resetState();
    return this.emitEvent('init', eventTimeMs, LiquidInputSource.INTERNAL, '', true);
  }

  private resetState(): void {
    this.cells.fill(EMPTY);
    this.phase.fill(HEAVY);
    this.activeBits.fill(0);
    this.shakeEnergy = 0;
    this.targetShakeEnergy = 0;
    this.accumulatorSeconds = 0;
    this.idleSubsteps = 0;
    this.dormant = false;
    this.simulationTimeSeconds = 0;
    this.eventSequence = 0;
    this.frameSequence = 0;
    this.lastFrameTimeMs = 0;
    this.lastPhysicsTimeMs = 0;
    this.fatalFinite = false;
    this.changedCellsSinceMetric = 0;
    this.rngState = (this.config.seed >>> 0) || 1;
    const lightStart = Math.floor(this.config.rows * 0.25);
    const interfaceRow = Math.floor(this.config.rows * 0.5);
    const liquidEnd = Math.floor(this.config.rows * 0.85);
    for (let row = lightStart; row < liquidEnd; row++) {
      const rowPhase = row < interfaceRow ? LIGHT : HEAVY;
      for (let col = 0; col < this.config.cols; col++) {
        const index = row * this.config.cols + col;
        this.cells[index] = SAND;
        this.phase[index] = rowPhase;
        this.shade[index] = this.nextRandom() & 7;
        this.markActive(index);
      }
    }
    this.countPhases();
    this.initialHeavyCount = this.heavyCount;
    this.initialLightCount = this.lightCount;
  }

  reset(seed: number, eventTimeMs: number): LiquidCAEventPayload {
    this.config.seed = Math.max(0, Math.floor(seed));
    const event = this.initialize();
    event.type = 'reset';
    event.eventTimeMs = eventTimeMs;
    return event;
  }

  setShake(intensity: number, eventTimeMs: number, source: LiquidInputSource): LiquidCAEventPayload {
    const value = Number.isFinite(intensity) ? Math.max(0, Math.min(1, intensity)) : 0;
    this.targetShakeEnergy = value;
    this.idleSubsteps = 0;
    this.dormant = false;
    const event = this.emitEvent('shake', eventTimeMs, source, '', true);
    event.shakeIntensity = value;
    return event;
  }

  invert180(enabled: boolean, eventTimeMs: number, source: LiquidInputSource): LiquidCAEventPayload {
    this.gravityDown = !enabled;
    this.idleSubsteps = 0;
    this.dormant = false;
    const event = this.emitEvent('invert', eventTimeMs, source, '', true);
    event.forceInvert = enabled;
    return event;
  }

  stepFrame(realDtSeconds: number, frameTimeMs: number = 0, sequence?: number): LiquidCAFramePayload {
    if (this.frameCache === null) {
      this.frameCache = {
        sequence: 0, caseId: '', frameTimeMs: 0, physicsTimeMs: 0, snapshotTimeMs: 0,
        substeps: 0, clippedTimeMs: 0, valid: false
      };
    }
    const frame = this.frameCache;
    frame.frameTimeMs = Math.max(0, frameTimeMs);
    frame.clippedTimeMs = 0;
    frame.valid = !this.fatalFinite;
    this.accumulatorSeconds += Math.max(0, Math.min(1 / 60, realDtSeconds));
    let substeps = 0;
    while (!this.fatalFinite && this.accumulatorSeconds >= PHYSICS_DT_SECONDS && substeps < 2) {
      this.physicsSubstep();
      this.accumulatorSeconds -= PHYSICS_DT_SECONDS;
      substeps++;
    }
    if (substeps === 2 && this.accumulatorSeconds >= PHYSICS_DT_SECONDS) {
      frame.clippedTimeMs = this.accumulatorSeconds * 1000;
      this.accumulatorSeconds = PHYSICS_DT_SECONDS;
    }
    this.lastPhysicsTimeMs = 0;
    this.frameSequence++;
    frame.sequence = sequence === undefined ? this.frameSequence : sequence;
    frame.substeps = substeps;
    frame.physicsTimeMs = 0;
    frame.snapshotTimeMs = 0;
    if (this.observer !== null) {
      this.observer.onFrame(frame);
    }
    return frame;
  }

  metrics(): LiquidCAMetricPayload {
    if (this.metricCache === null) {
      this.metricCache = {
        sequence: 0, metricTimeMs: 0, heavyCount: 0, lightCount: 0, emptyCount: 0,
        initialHeavyCount: 0, initialLightCount: 0, heavyDrift: 0, lightDrift: 0,
        heavyCentroidX: 0, heavyCentroidY: 0, lightCentroidX: 0, lightCentroidY: 0,
        interfaceRowsPx: [], interfaceRangeCells: 0, mixedBandShare: 0,
        changedCells: 0, lightPixelArea: 0, heavyPixelArea: 0,
        simulationTime: 0, dormant: false, finite: false, gravityInverted: false
      };
    }
    const metric = this.metricCache;
    metric.sequence = this.frameSequence;
    metric.metricTimeMs = 0;
    metric.heavyCount = this.heavyCount;
    metric.lightCount = this.lightCount;
    metric.emptyCount = this.cells.length - this.heavyCount - this.lightCount;
    metric.initialHeavyCount = this.initialHeavyCount;
    metric.initialLightCount = this.initialLightCount;
    metric.heavyDrift = Math.abs(this.heavyCount - this.initialHeavyCount);
    metric.lightDrift = Math.abs(this.lightCount - this.initialLightCount);
    this.centroids(metric);
    this.interfaceRows(metric);
    metric.mixedBandShare = this.mixedShare();
    metric.changedCells = this.changedCellsSinceMetric;
    metric.lightPixelArea = this.lightCount * this.config.cellPx * this.config.cellPx;
    metric.heavyPixelArea = this.heavyCount * this.config.cellPx * this.config.cellPx;
    this.changedCellsSinceMetric = 0;
    metric.simulationTime = this.simulationTimeSeconds;
    metric.dormant = this.dormant;
    metric.gravityInverted = !this.gravityDown;
    metric.finite = Number.isFinite(metric.heavyCentroidX) && Number.isFinite(metric.heavyCentroidY) &&
      Number.isFinite(metric.lightCentroidX) && Number.isFinite(metric.lightCentroidY) &&
      Number.isFinite(metric.interfaceRangeCells) && Number.isFinite(metric.mixedBandShare);
    if (!metric.finite) {
      this.fatalFinite = true;
      const event = this.emitEvent('model_error', 0, LiquidInputSource.INTERNAL, 'NON_FINITE_METRIC', true);
      event.errorCode = 'NON_FINITE_METRIC';
    }
    return metric;
  }

  copyRenderState(target: LiquidRenderSnapshot | null): boolean {
    if (target === null || this.cells.length === 0) {
      return false;
    }
    target.cols = this.config.cols;
    target.rows = this.config.rows;
    target.cellPx = this.config.cellPx;
    target.gravityInverted = !this.gravityDown;
    target.cells.set(this.cells);
    target.phase.set(this.phase);
    target.shade.set(this.shade);
    return true;
  }

  setObserver(observer: LiquidCAObserver | null): void {
    this.observer = observer;
  }

  get gridCellCount(): number {
    return this.cells.length;
  }

  get isDormant(): boolean {
    return this.dormant;
  }

  get isGravityDown(): boolean {
    return this.gravityDown;
  }

  private physicsSubstep(): void {
    const dt = PHYSICS_DT_SECONDS;
    if (this.targetShakeEnergy > 0) {
      const rate = 8 * dt;
      this.shakeEnergy += (this.targetShakeEnergy - this.shakeEnergy) * Math.min(1, rate);
    } else {
      this.shakeEnergy *= Math.exp(-this.config.shakeDecayRate * dt);
    }
    if (this.shakeEnergy < 0.001) {
      this.shakeEnergy = 0;
    }
    let changed = 0;
    if (!this.dormant && this.shakeEnergy > 0.001) {
      changed += this.stir();
    }
    changed += this.settleAndLevel();
    if (changed === 0 && this.shakeEnergy <= 0.001) {
      this.idleSubsteps++;
      if (this.idleSubsteps >= 12) {
        this.dormant = true;
      }
    } else {
      this.idleSubsteps = 0;
      this.dormant = false;
    }
    this.changedCellsSinceMetric += changed * 2;
    this.simulationTimeSeconds += dt;
  }

  private settleAndLevel(): number {
    let changed = 0;
    const cols = this.config.cols;
    const rows = this.config.rows;
    const uprightScale = this.shakeEnergy > 0.0001 ? 0.25 : 0.15;
    const settleProbability = this.config.pSettle * (this.gravityDown ? uprightScale : 0.5);
    for (let pass = 0; pass < 2; pass++) {
      for (let row = rows - 1; row >= 0; row--) {
        const screenRow = this.gravityDown ? row : rows - 1 - row;
        for (let col = 0; col < cols; col++) {
          const index = screenRow * cols + col;
          if (this.cells[index] !== SAND) {
            continue;
          }
          const belowRow = this.gravityDown ? screenRow + 1 : screenRow - 1;
          if (belowRow < 0 || belowRow >= rows) {
            continue;
          }
          const below = belowRow * cols + col;
          if (this.cells[below] === EMPTY) {
            this.moveOccupancy(index, below);
            changed++;
            continue;
          }
          if (this.phase[index] === HEAVY && this.phase[below] === LIGHT) {
            const aboveRow = this.gravityDown ? screenRow - 1 : screenRow + 1;
            if (aboveRow >= 0 && aboveRow < rows) {
              const above = aboveRow * cols + col;
              if (this.cells[above] === SAND && this.phase[above] === HEAVY &&
                this.randomBelow(settleProbability)) {
                this.swapPhase(index, below);
                changed++;
              }
            }
          }
        }
      }
    }
    if (changed === 0) {
      changed += this.levelInterfaces();
    }
    return changed;
  }

  private levelInterfaces(): number {
    let changed = 0;
    const cols = this.config.cols;
    for (let col = 0; col + 1 < cols; col++) {
      const left = this.heavyInterfaceRow(col);
      const right = this.heavyInterfaceRow(col + 1);
      if (left < 0 || right < 0 || Math.abs(left - right) < 2) {
        continue;
      }
      const highCol = left < right ? col : col + 1;
      const lowCol = left < right ? col + 1 : col;
      if (this.transferInterfaceCell(highCol, lowCol)) {
        changed++;
      }
    }
    return changed;
  }

  private heavyInterfaceRow(col: number): number {
    const cols = this.config.cols;
    const rows = this.config.rows;
    if (this.gravityDown) {
      for (let row = 0; row < rows; row++) {
        const index = row * cols + col;
        if (this.cells[index] === SAND && this.phase[index] === HEAVY) {
          return row;
        }
      }
    } else {
      for (let row = rows - 1; row >= 0; row--) {
        const index = row * cols + col;
        if (this.cells[index] === SAND && this.phase[index] === HEAVY) {
          return row;
        }
      }
    }
    return -1;
  }

  private transferInterfaceCell(fromCol: number, toCol: number): boolean {
    const cols = this.config.cols;
    const fromRow = this.heavyInterfaceRow(fromCol);
    const toRow = this.heavyInterfaceRow(toCol);
    if (fromRow < 0 || toRow < 0) {
      return false;
    }
    const from = fromRow * cols + fromCol;
    const to = toRow * cols + toCol;
    if (this.cells[to] !== SAND || this.phase[from] !== HEAVY || this.phase[to] === HEAVY) {
      return false;
    }
    this.phase[from] = LIGHT;
    this.phase[to] = HEAVY;
    this.markNeighborhood(from);
    this.markNeighborhood(to);
    return true;
  }

  private stir(): number {
    let changed = 0;
    const cols = this.config.cols;
    const rows = this.config.rows;
    const probability = this.config.qMixBase * this.shakeEnergy;
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < cols; col++) {
        const index = row * cols + col;
        if (this.cells[index] !== SAND || !this.randomBelow(probability)) {
          continue;
        }
        const opposite = this.phase[index] === HEAVY ? LIGHT : HEAVY;
        const neighbor = this.firstNeighborWithPhase(col, row, opposite);
        if (neighbor >= 0) {
          this.swapPhase(index, neighbor);
          changed++;
        }
      }
    }
    return changed;
  }

  private firstNeighborWithPhase(col: number, row: number, wanted: number): number {
    const cols = this.config.cols;
    const rows = this.config.rows;
    const offsets: number[][] = [[0, 1], [0, -1], [1, 0], [-1, 0], [1, 1], [1, -1], [-1, 1], [-1, -1]];
    for (let i = 0; i < offsets.length; i++) {
      const nextCol = col + offsets[i][0];
      const nextRow = row + offsets[i][1];
      if (nextCol < 0 || nextCol >= cols || nextRow < 0 || nextRow >= rows) {
        continue;
      }
      const index = nextRow * cols + nextCol;
      if (this.cells[index] === SAND && this.phase[index] === wanted) {
        return index;
      }
    }
    return -1;
  }

  private moveOccupancy(from: number, to: number): void {
    this.cells[to] = SAND;
    this.phase[to] = this.phase[from];
    this.shade[to] = this.shade[from];
    this.cells[from] = EMPTY;
    this.phase[from] = HEAVY;
    this.markNeighborhood(from);
    this.markNeighborhood(to);
  }

  private swapPhase(a: number, b: number): void {
    const phase = this.phase[a];
    this.phase[a] = this.phase[b];
    this.phase[b] = phase;
    this.markNeighborhood(a);
    this.markNeighborhood(b);
  }

  private markActive(index: number): void {
    if (index < 0 || index >= this.activeBits.length * 8) {
      return;
    }
    this.activeBits[index >> 3] |= 1 << (index & 7);
  }

  private markNeighborhood(index: number): void {
    const cols = this.config.cols;
    const rows = this.config.rows;
    const col = index % cols;
    const row = Math.floor(index / cols);
    for (let rowOffset = -1; rowOffset <= 1; rowOffset++) {
      for (let colOffset = -1; colOffset <= 1; colOffset++) {
        const nextRow = row + rowOffset;
        const nextCol = col + colOffset;
        if (nextRow >= 0 && nextRow < rows && nextCol >= 0 && nextCol < cols) {
          this.markActive(nextRow * cols + nextCol);
        }
      }
    }
  }

  private countPhases(): void {
    this.heavyCount = 0;
    this.lightCount = 0;
    for (let i = 0; i < this.cells.length; i++) {
      if (this.cells[i] === SAND) {
        if (this.phase[i] === HEAVY) {
          this.heavyCount++;
        } else {
          this.lightCount++;
        }
      }
    }
  }

  private centroids(metric: LiquidCAMetricPayload): void {
    let heavyX = 0;
    let heavyY = 0;
    let lightX = 0;
    let lightY = 0;
    const cols = this.config.cols;
    for (let i = 0; i < this.cells.length; i++) {
      if (this.cells[i] !== SAND) {
        continue;
      }
      const x = (i % cols) + 0.5;
      const y = Math.floor(i / cols) + 0.5;
      if (this.phase[i] === HEAVY) {
        heavyX += x;
        heavyY += y;
      } else {
        lightX += x;
        lightY += y;
      }
    }
    metric.heavyCentroidX = this.heavyCount > 0 ? heavyX / this.heavyCount : 0;
    metric.heavyCentroidY = this.heavyCount > 0 ? heavyY / this.heavyCount : 0;
    metric.lightCentroidX = this.lightCount > 0 ? lightX / this.lightCount : 0;
    metric.lightCentroidY = this.lightCount > 0 ? lightY / this.lightCount : 0;
  }

  private interfaceRows(metric: LiquidCAMetricPayload): void {
    const rows: number[] = [];
    const cols = this.config.cols;
    const ignore = Math.max(1, Math.floor(cols * 0.1));
    let min = Number.MAX_SAFE_INTEGER;
    let max = Number.MIN_SAFE_INTEGER;
    for (let col = ignore; col < cols - ignore; col++) {
      const row = this.heavyInterfaceRow(col);
      if (row < 0) {
        continue;
      }
      const value = row * this.config.cellPx;
      rows.push(value);
      min = Math.min(min, value);
      max = Math.max(max, value);
    }
    metric.interfaceRowsPx = rows;
    metric.interfaceRangeCells = rows.length === 0 ? 0 : (max - min) / this.config.cellPx;
  }

  private mixedShare(): number {
    const cols = this.config.cols;
    const rows = this.config.rows;
    let mixed = 0;
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < cols; col++) {
        const index = row * cols + col;
        if (this.cells[index] !== SAND) {
          continue;
        }
        const opposite = this.phase[index] === HEAVY ? LIGHT : HEAVY;
        if (this.firstNeighborWithPhase(col, row, opposite) >= 0) {
          mixed++;
        }
      }
    }
    const total = this.heavyCount + this.lightCount;
    return total > 0 ? mixed / total : 0;
  }

  private nextRandom(): number {
    let state = this.rngState;
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    this.rngState = state >>> 0;
    return this.rngState;
  }

  private randomBelow(probability: number): boolean {
    return (this.nextRandom() / 4294967296) < probability;
  }

  private emitEvent(type: string, eventTimeMs: number, source: LiquidInputSource,
                    errorCode: string, boundary: boolean): LiquidCAEventPayload {
    this.eventSequence++;
    const event: LiquidCAEventPayload = {
      type: type,
      sequence: this.eventSequence,
      eventTimeMs: eventTimeMs,
      boundary: boundary,
      source: source,
      forceInvert: !this.gravityDown,
      shakeIntensity: this.targetShakeEnergy,
      errorCode: errorCode,
      cols: this.config.cols,
      rows: this.config.rows,
      cellPx: this.config.cellPx,
      initialHeavy: this.initialHeavyCount,
      initialLight: this.initialLightCount,
      seed: this.config.seed,
      scaled: false
    };
    if (this.observer !== null) {
      this.observer.onEvent(event);
    }
    return event;
  }
}
