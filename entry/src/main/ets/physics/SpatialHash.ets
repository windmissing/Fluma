import { Particle } from './Particle';

export class SpatialHash {
  private cellSize: number;
  private buckets: Map<number, number[]> = new Map();
  private particles: Particle[] = [];

  constructor(cellSize: number) {
    this.cellSize = cellSize;
  }

  setCellSize(size: number): void {
    this.cellSize = size;
  }

  private cellKey(cx: number, cy: number): number {
    return (cx + 100000) * 200001 + (cy + 100000);
  }

  rebuild(particles: Particle[]): void {
    this.particles = particles;
    this.buckets.clear();
    for (let i = 0; i < particles.length; i++) {
      const p = particles[i];
      const cx = Math.floor(p.x / this.cellSize);
      const cy = Math.floor(p.y / this.cellSize);
      const key = this.cellKey(cx, cy);
      let bucket = this.buckets.get(key);
      if (!bucket) {
        bucket = [];
        this.buckets.set(key, bucket);
      }
      bucket.push(i);
    }
  }

  queryNearby(p: Particle): number[] {
    const result: number[] = [];
    const cx = Math.floor(p.x / this.cellSize);
    const cy = Math.floor(p.y / this.cellSize);
    for (let dx = -1; dx <= 1; dx++) {
      for (let dy = -1; dy <= 1; dy++) {
        const key = this.cellKey(cx + dx, cy + dy);
        const bucket = this.buckets.get(key);
        if (bucket) {
          for (let i = 0; i < bucket.length; i++) {
            result.push(bucket[i]);
          }
        }
      }
    }
    return result;
  }
}