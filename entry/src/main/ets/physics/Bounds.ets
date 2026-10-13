import { Particle } from './Particle';

export class Bounds {
  static reflect(p: Particle, w: number, h: number, e: number): void {
    if (p.x - p.r < 0) {
      p.x = p.r;
      p.vx = -p.vx * e;
    }
    if (p.x + p.r > w) {
      p.x = w - p.r;
      p.vx = -p.vx * e;
    }
    if (p.y - p.r < 0) {
      p.y = p.r;
      p.vy = -p.vy * e;
    }
    if (p.y + p.r > h) {
      p.y = h - p.r;
      p.vy = -p.vy * e;
    }
    if (Math.abs(p.vx) < 1) p.vx = 0;
    if (Math.abs(p.vy) < 1) p.vy = 0;
  }
}