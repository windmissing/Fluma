export class Particle {
  x: number = 0;
  y: number = 0;
  vx: number = 0;
  vy: number = 0;
  r: number;
  mass: number = 1;
  color: string;
  phase: number = 0;

  constructor(x: number, y: number, r: number, color: string, mass: number = 1, phase: number = 0) {
    this.x = x;
    this.y = y;
    this.r = r;
    this.color = color;
    this.mass = mass;
    this.phase = phase;
  }

  integrate(dt: number, gx: number, gy: number): void {
    this.vx += gx * dt;
    this.vy += gy * dt;
    this.x += this.vx * dt;
    this.y += this.vy * dt;
  }
}
