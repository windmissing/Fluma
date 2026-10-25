// P-013 P0 v1: independent, read-only DISPLAY_4X3_V1 adapter.
import { WaterSandSnapshot } from '../physics/WaterSandField';
import { P013FrameInput, P013LightParams } from './P013RenderTypes';
export class P013FrameAdapter {
  readonly data: Float32Array = new Float32Array(512);
  private profiles: Float64Array[] = [new Float64Array(128), new Float64Array(128),
    new Float64Array(128), new Float64Array(128)];
  private scratch: Float64Array = new Float64Array(128);
  pack(s: WaterSandSnapshot): boolean {
    if (s.cols !== 128 || !Number.isFinite(s.capacity) || s.capacity <= 0 ||
      s.sandTop.length !== 128 || s.waterTop.length !== 128 || s.bh.length !== 128 || s.bl.length !== 128) {
      return false;
    }
    for (let c = 0; c < 128; c++) {
      const top = s.sandTop[c];
      const water = s.waterTop[c];
      const bottom = s.bh[c];
      const mix = bottom + s.bl[c];
      if (!Number.isFinite(top) || !Number.isFinite(water) || !Number.isFinite(bottom) ||
        !Number.isFinite(mix) || bottom < 0 || bottom > mix || mix > top || top > water || water > s.capacity) {
        return false;
      }
      this.profiles[0][c] = top;
      this.profiles[1][c] = water;
      this.profiles[2][c] = bottom;
      this.profiles[3][c] = mix;
    }
    // Same pass ordering, edge replication and Float64 scratch->set as Canvas.
    for (let pass = 0; pass < 4; pass++) {
      for (let k = 0; k < 4; k++) {
        const src = this.profiles[k];
        for (let c = 0; c < 128; c++) {
          this.scratch[c] = (src[Math.max(0, c - 1)] + src[c] + src[Math.min(127, c + 1)]) / 3;
        }
        src.set(this.scratch);
      }
    }
    // Validate float32 conversion before committing the reusable output packet.
    for (let c = 0; c < 128; c++) {
      const top = Math.fround(this.profiles[0][c]);
      const water = Math.fround(this.profiles[1][c]);
      const bottom = Math.fround(this.profiles[2][c]);
      const mix = Math.fround(this.profiles[3][c]);
      if (!Number.isFinite(top) || !Number.isFinite(water) || bottom < 0 ||
        bottom > mix || mix > top || top > water || water > s.capacity) {
        return false;
      }
    }
    for (let c = 0; c < 128; c++) {
      for (let k = 0; k < 4; k++) {
        this.data[c * 4 + k] = this.profiles[k][c];
      }
    }
    return true;
  }
}
/** Static CPU golden reference in native top-left pixels, not screen colorimetry. */
export function p013Reference(data: Float32Array, f: P013FrameInput,
  l: P013LightParams, x: number, y: number): number | null {
  let qx = x - f.boxXPx;
  let qy = y - f.boxYPx;
  if (!f.diagnosticEnabled || qx < 0 || qy < 0 || qx >= f.boxWidthPx || qy >= f.boxHeightPx) {
    return null;
  }
  if (f.visualInverted) {
    qx = f.boxWidthPx - qx;
    qy = f.boxHeightPx - qy;
  }
  const sample = (px: number): number => {
    const r = Math.max(0, Math.min(127, px / (f.boxWidthPx / 128)));
    const i = Math.min(Math.floor(r), 126);
    return data[i * 4] * (1 - (r - i)) + data[(i + 1) * 4] * (r - i);
  };
  const top = sample(qx);
  const h = (1 - qy / f.boxHeightPx) * f.capacity;
  if (h < 0 || h >= top) {
    return null;
  }
  const dx = f.boxWidthPx / 128;
  const pb = (sample(qx + dx) - sample(qx - dx)) * (f.boxHeightPx / f.capacity) / (2 * dx);
  const norm = Math.sqrt(l.lightX * l.lightX + l.lightY * l.lightY + l.lightZ * l.lightZ);
  const dot = (-pb * l.lightX + l.lightZ) / Math.sqrt(1 + pb * pb) / norm;
  const face = Math.max(l.lmin, Math.min(l.lmax, l.la + l.ld * Math.max(0, dot)));
  const z = (top - h) * f.boxHeightPx / f.capacity;
  return 1 + Math.exp(-z / (l.ellLightMaterial * f.boxHeightPx / f.capacity)) * (face - l.lref);
}
