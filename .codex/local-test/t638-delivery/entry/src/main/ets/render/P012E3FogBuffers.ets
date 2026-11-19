// P-012 E3 I2 光学缓冲（architecture-P012.md §12.1/12.2；algorithm-E3 §3.1）
// 问题 ID：P-012；方案版本：p012-e3-grid-removal-v1（第一批 I2）
// 职责：water-fog / interface 两个独立光学缓冲——Float32 预乘内部计算 +
//   Uint8ClampedArray 直通上传（un-premultiply 层），OffscreenCanvas+
//   transferToImageBitmap 缓存，九参 drawImage 双线性缩放上屏。
// 红线（designer §3.1 / §12.1）：
//   · ImageData.data 按 Canvas 规范是直通（非预乘）——预乘值直接写入会被
//     二次预乘产生暗边；上传前强制 un-premultiply：
//     R = a>0 ? round(rgbaR/a) : 0（a=0 时 RGB 置 0，防 NaN/残留）。
//   · 内部预乘保证双线性插值对半透明边缘正确（直通 RGB 在 alpha 梯度区
//     插值会拖色）。
//   · 宽 256|384 构造定档，运行期不切换（设计稿红线）。
//   · 静止态全命中 → 零烘焙零上传（dirty 缓存复用）。

/** I2 单个雾缓冲（water-fog 或 interface） */
export class P012E3FogBuffer {
  readonly width: number;                       // 256 | 384
  readonly height: number;                      // round(width * boxH / boxW)
  /** 预乘内部缓冲（fogR*a, fogG*a, fogB*a, a），Float32，width*height*4 */
  private readonly premul: Float32Array;
  /** 上传直通缓冲（fogR, fogG, fogB, a）——由 ImageData.data 持有 */
  private readonly upload: Uint8ClampedArray;
  /** 本缓冲对应的 OffscreenCanvas 位图（putImageData+transfer 产物） */
  private bitmap: ImageBitmap | null = null;
  /** 内容脏位：true=需重烘焙；烘焙后置 false */
  dirty: boolean = true;
  /** 已烘焙内容对应的平流相位（aPhaseX 快照） */
  sampleU: number = 0;
  /** 上传缓冲构造（Node 通道无 ImageData 时仍可写读 premul 供单测） */
  private readonly uploadOwner: boolean;

  constructor(width: number, boxW: number, boxH: number) {
    this.width = width;
    this.height = Math.max(1, Math.round(width * boxH / boxW));
    this.premul = new Float32Array(width * this.height * 4);
    // 上传缓冲：独立 Uint8ClampedArray（不经 ImageData 构造借 .data——
    // ArkTS 对 ImageData(data) 构造/借出 data 的语义与标准 Canvas 存在
    // 差异，真机实测出现 RGB 被压暗；独立数组 + transferBitmap 的
    // createImageData 逐字节拷贝链路行为与 B 档颗粒位图一致）。
    // uploadOwner=false 时表示通道可用（有 OffscreenCanvas 可 transfer）。
    this.upload = new Uint8ClampedArray(width * this.height * 4);
    let hasChannel = false;
    try {
      hasChannel = typeof OffscreenCanvas !== 'undefined' && OffscreenCanvas !== null;
    } catch (e) {
      hasChannel = false;
    }
    this.uploadOwner = !hasChannel;              // true=Node 无通道（单测直读）
  }

  /** 预乘写入（烘焙循环逐样本）：alpha∈[0,1]，fog RGB 0-255 */
  setPremul(u: number, v: number, fogR: number, fogG: number, fogB: number,
    alpha: number): void {
    const idx = (v * this.width + u) * 4;
    const a = Math.max(0, Math.min(1, alpha));
    this.premul[idx] = fogR * a;
    this.premul[idx + 1] = fogG * a;
    this.premul[idx + 2] = fogB * a;
    this.premul[idx + 3] = a;
  }

  /**
   * un-premultiply → 直通上传缓冲（ImageData 格式）。
   * R = a>0 ? round(rgbaR/a) : 0；a=0 时 RGB 置 0（防 NaN/残留）。
   * 返回上传字节数（静止零上传断言用）。
   */
  flushToUpload(): number {
    const n = this.width * this.height;
    for (let i = 0; i < n; i++) {
      const p = i * 4;
      const a = this.premul[p + 3];
      if (a <= 0) {
        this.upload[p] = 0;
        this.upload[p + 1] = 0;
        this.upload[p + 2] = 0;
        this.upload[p + 3] = 0;
      } else if (a >= 1) {
        this.upload[p] = Math.round(this.premul[p]);
        this.upload[p + 1] = Math.round(this.premul[p + 1]);
        this.upload[p + 2] = Math.round(this.premul[p + 2]);
        this.upload[p + 3] = 255;
      } else {
        const inv = 1 / a;
        this.upload[p] = Math.round(this.premul[p] * inv);
        this.upload[p + 1] = Math.round(this.premul[p + 1] * inv);
        this.upload[p + 2] = Math.round(this.premul[p + 2] * inv);
        this.upload[p + 3] = Math.round(a * 255);
      }
    }
    return n * 4;
  }

  /** 直通读回（单测/暗边检查）：返回 [R,G,B,A] 0-255 */
  readUpload(u: number, v: number): number[] {
    const p = (v * this.width + u) * 4;
    return [this.upload[p], this.upload[p + 1],
      this.upload[p + 2], this.upload[p + 3]];
  }

  /** premul alpha 直读（降级提交路径用：I2 计算面 + Path 上屏） */
  readPremulAlpha(u: number, v: number): number {
    return this.premul[(v * this.width + u) * 4 + 3];
  }

  get bufferWidth(): number {
    return this.width;
  }

  get bufferHeight(): number {
    return this.height;
  }

  /**
   * 上传到 OffscreenCanvas → ImageBitmap（putImageData+transfer）。
   * 返回位图（null=通道不可用：Node 无 OffscreenCanvas）。
   * 完全同构 makeGrainBitmap（B 档真机已验证链路）：createImageData →
   * **直接写 img.data**（不经过任何中间数组/跨帧引用；ArkTS 对
   * ImageData(data) 构造与借出 data 的语义均与标准 Canvas 存在差异，
   * 真机实测独立数组拷贝链产生全白位图）→ putImageData → transfer。
   */
  transferBitmap(): ImageBitmap | null {
    if (this.uploadOwner) {
      return null;
    }
    try {
      if (typeof OffscreenCanvas === 'undefined' || OffscreenCanvas === null) {
        return null;
      }
      const off = new OffscreenCanvas(this.width, this.height);
      const octx = off.getContext('2d');
      if (octx === null || octx === undefined) {
        return null;
      }
      const img = octx.createImageData(this.width, this.height);
      const dst = img.data;
      const n = this.width * this.height;
      for (let i = 0; i < n; i++) {
        const p = i * 4;
        const a = this.premul[p + 3];
        if (a <= 0) {
          dst[p] = 0; dst[p + 1] = 0; dst[p + 2] = 0; dst[p + 3] = 0;
        } else {
          const inv = 1 / a;
          // un-premultiply → 直通（R=round(rgbaR/a)；a=1 区直接舍入）
          dst[p] = a >= 1 ? Math.round(this.premul[p]) :
            Math.round(this.premul[p] * inv);
          dst[p + 1] = a >= 1 ? Math.round(this.premul[p + 1]) :
            Math.round(this.premul[p + 1] * inv);
          dst[p + 2] = a >= 1 ? Math.round(this.premul[p + 2]) :
            Math.round(this.premul[p + 2] * inv);
          dst[p + 3] = a >= 1 ? 255 : Math.round(a * 255);
        }
      }
      octx.putImageData(img, 0, 0);
      this.bitmap = off.transferToImageBitmap();
      return this.bitmap;
    } catch (e) {
      return null;
    }
  }

  /** 当前缓存位图（dirty=false 时有效） */
  get cachedBitmap(): ImageBitmap | null {
    return this.bitmap;
  }
}
