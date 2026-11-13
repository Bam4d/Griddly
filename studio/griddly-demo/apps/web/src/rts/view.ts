/**
 * Viewport maths and canvas overlays for the RTS page. The board itself is the engine's render of
 * player 1's Sprite2D observer (egocentric shadows: green = yours, red = enemy); this only crops it
 * around the GDY camera (cam_x / cam_y) and draws UI marks (selection, health, orders) on top.
 */
import type { Order } from "./orders";
import type { Cell, Obj, WorldView } from "./world";

/** Pixel size of one tile in the engine's Sprite2D render (GDY `Observers.Sprite2D.TileSize`). */
export const SPRITE_TILE = 16;

export type Viewport = {
  /** Display size of a tile in CSS pixels. */
  tile: number;
  /** Top-left visible cell. */
  x0: number;
  y0: number;
  /** Visible size in cells. */
  cols: number;
  rows: number;
  /** Pixel offset that centres maps smaller than the view. */
  ox: number;
  oy: number;
};

export function viewport(cam: Cell, mapW: number, mapH: number, widthPx: number, heightPx: number, tile: number): Viewport {
  const cols = Math.max(1, Math.floor(widthPx / tile));
  const rows = Math.max(1, Math.floor(heightPx / tile));
  const clamp = (v: number, max: number) => Math.max(0, Math.min(v, Math.max(0, max)));
  const x0 = clamp(cam[0] - Math.floor(cols / 2), mapW - cols);
  const y0 = clamp(cam[1] - Math.floor(rows / 2), mapH - rows);
  const ox = Math.max(0, Math.floor((widthPx - Math.min(cols, mapW) * tile) / 2));
  const oy = Math.max(0, Math.floor((heightPx - Math.min(rows, mapH) * tile) / 2));
  return { tile, x0, y0, cols, rows, ox, oy };
}

export const cellAt = (v: Viewport, px: number, py: number): Cell => [Math.floor((px - v.ox) / v.tile) + v.x0, Math.floor((py - v.oy) / v.tile) + v.y0];

const toPx = (v: Viewport, c: Cell): [number, number] => [(c[0] - v.x0) * v.tile + v.ox, (c[1] - v.y0) * v.tile + v.oy];

export type Marks = {
  selected: Set<number>;
  hover: Cell | null;
  targeting: string | null;
  box: [number, number, number, number] | null;
  orders: Map<number, Order[]>;
  /** Cells flashed after an order (cell, colour, time issued). */
  pings: { cell: Cell; color: string; at: number }[];
  /** UI fog of war: cells player 1 sees now / has ever seen (display only; null = off). */
  fog: Fog | null;
};

export type Fog = { visible: Set<number>; explored: Set<number> };

/** Sight radius per object type (cells, Manhattan), for the display-only fog of war. */
const SIGHT: Record<string, number> = { scout: 7, tower: 6, base: 6, tank: 5, marine: 5, relay_owned: 4 };

/** Update `fog` with what player `pid`'s objects can see this tick. */
export function updateFog(world: WorldView, pid: number, fog: Fog): void {
  fog.visible.clear();
  for (const o of world.objects) {
    if (o.player !== pid) continue;
    const r = SIGHT[o.name] ?? 4;
    for (let dy = -r; dy <= r; dy++) {
      const span = r - Math.abs(dy);
      for (let dx = -span; dx <= span; dx++) {
        const c: Cell = [o.pos[0] + dx, o.pos[1] + dy];
        if (!world.inside(c)) continue;
        const k = world.key(c);
        fog.visible.add(k);
        fog.explored.add(k);
      }
    }
  }
}

export const seen = (world: WorldView, fog: Fog | null, c: Cell) => !fog || fog.visible.has(world.key(c));

export function drawBoard(ctx: CanvasRenderingContext2D, board: HTMLCanvasElement | null, world: WorldView, v: Viewport, marks: Marks, now: number): void {
  const { width, height } = ctx.canvas;
  ctx.fillStyle = "#0b0d0a";
  ctx.fillRect(0, 0, width, height);
  if (!board) return;
  const cols = Math.min(v.cols, world.width - v.x0);
  const rows = Math.min(v.rows, world.height - v.y0);
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(board, v.x0 * SPRITE_TILE, v.y0 * SPRITE_TILE, cols * SPRITE_TILE, rows * SPRITE_TILE, v.ox, v.oy, cols * v.tile, rows * v.tile);

  const visible = (c: Cell) => c[0] >= v.x0 && c[1] >= v.y0 && c[0] < v.x0 + cols && c[1] < v.y0 + rows;
  const t = v.tile;

  if (marks.fog) {
    for (let y = v.y0; y < v.y0 + rows; y++) {
      for (let x = v.x0; x < v.x0 + cols; x++) {
        const k = world.key([x, y]);
        if (marks.fog.visible.has(k)) continue;
        ctx.fillStyle = marks.fog.explored.has(k) ? "rgba(4,6,4,.72)" : "#040604";
        const [px, py] = toPx(v, [x, y]);
        ctx.fillRect(px, py, t, t);
      }
    }
  }

  // order paths of selected units
  ctx.lineWidth = Math.max(1, t / 16);
  for (const id of marks.selected) {
    const u = world.byId.get(id);
    const q = marks.orders.get(id);
    if (!u || !q?.length) continue;
    let from = u.pos;
    ctx.setLineDash([t / 6, t / 6]);
    for (const o of q) {
      const to = orderTarget(world, o);
      if (!to) continue;
      const [ax, ay] = toPx(v, from);
      const [bx, by] = toPx(v, to);
      ctx.strokeStyle = o.kind === "attack" ? "rgba(248,113,113,.8)" : o.kind === "build" ? "rgba(251,191,36,.85)" : "rgba(134,239,172,.75)";
      ctx.beginPath();
      ctx.moveTo(ax + t / 2, ay + t / 2);
      ctx.lineTo(bx + t / 2, by + t / 2);
      ctx.stroke();
      if (o.kind === "build") {
        ctx.setLineDash([]);
        ctx.strokeRect(bx + 2, by + 2, t - 4, t - 4);
        ctx.setLineDash([t / 6, t / 6]);
      }
      from = to;
    }
    ctx.setLineDash([]);
  }

  for (const o of world.objects) {
    if (!visible(o.pos) || !seen(world, marks.fog, o.pos)) continue;
    const [x, y] = toPx(v, o.pos);
    if (marks.selected.has(o.id)) {
      ctx.strokeStyle = "rgba(134,239,172,.95)";
      ctx.lineWidth = Math.max(1.5, t / 14);
      ctx.strokeRect(x + 1, y + 1, t - 2, t - 2);
    }
    const hp = o.vars.health;
    const max = o.vars.max_health;
    if (hp !== undefined && max && (hp < max || marks.selected.has(o.id))) {
      const w = t - 6;
      ctx.fillStyle = "rgba(0,0,0,.65)";
      ctx.fillRect(x + 3, y + 1, w, Math.max(3, t / 10));
      const f = Math.max(0, hp / max);
      ctx.fillStyle = f > 0.6 ? "#4ade80" : f > 0.3 ? "#facc15" : "#f87171";
      ctx.fillRect(x + 3, y + 1, w * f, Math.max(3, t / 10));
    }
    if (o.name === "worker" && o.vars.carrying) {
      ctx.fillStyle = o.vars.carrying === 3 ? "#c084fc" : o.vars.carrying === 2 ? "#a3e635" : "#34d399";
      ctx.beginPath();
      ctx.arc(x + t - t / 5, y + t - t / 5, Math.max(2, t / 9), 0, Math.PI * 2);
      ctx.fill();
    }
    if (o.name === "lab" && o.vars.busy) {
      ctx.fillStyle = "rgba(125,211,252,.9)";
      ctx.font = `${Math.max(9, t / 3)}px monospace`;
      ctx.fillText("…", x + t / 2 - 3, y + t / 3);
    }
  }

  // pings
  for (const p of marks.pings) {
    const age = (now - p.at) / 600;
    if (age > 1 || !visible(p.cell)) continue;
    const [x, y] = toPx(v, p.cell);
    ctx.strokeStyle = p.color;
    ctx.globalAlpha = 1 - age;
    ctx.lineWidth = 2;
    const r = (t / 2) * (0.4 + age);
    ctx.beginPath();
    ctx.arc(x + t / 2, y + t / 2, r, 0, Math.PI * 2);
    ctx.stroke();
    ctx.globalAlpha = 1;
  }

  if (marks.hover && visible(marks.hover)) {
    const [x, y] = toPx(v, marks.hover);
    ctx.strokeStyle = marks.targeting ? "rgba(251,191,36,.95)" : "rgba(255,255,255,.35)";
    ctx.lineWidth = 1.5;
    ctx.strokeRect(x + 0.5, y + 0.5, t - 1, t - 1);
  }
  if (marks.box) {
    const [ax, ay, bx, by] = marks.box;
    ctx.strokeStyle = "rgba(134,239,172,.9)";
    ctx.fillStyle = "rgba(134,239,172,.1)";
    ctx.lineWidth = 1;
    ctx.fillRect(Math.min(ax, bx), Math.min(ay, by), Math.abs(bx - ax), Math.abs(by - ay));
    ctx.strokeRect(Math.min(ax, bx), Math.min(ay, by), Math.abs(bx - ax), Math.abs(by - ay));
  }
}

export function orderTarget(world: WorldView, o: Order): Cell | null {
  switch (o.kind) {
    case "move":
      return o.to;
    case "attack":
      return world.byId.get(o.target)?.pos ?? o.last;
    case "harvest":
      return o.at;
    case "build":
      return o.site;
    case "assist":
      return world.byId.get(o.target)?.pos ?? null;
    case "capture":
      return o.relay;
    default:
      return null;
  }
}

/** Minimap: the engine's global Block2D render, plus the camera rectangle. */
export function drawMinimap(ctx: CanvasRenderingContext2D, mini: HTMLCanvasElement | null, world: WorldView, v: Viewport, units: Obj[], fog: Fog | null): void {
  const { width, height } = ctx.canvas;
  ctx.fillStyle = "#0b0d0a";
  ctx.fillRect(0, 0, width, height);
  if (!mini) return;
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(mini, 0, 0, width, height);
  const sx = width / world.width;
  const sy = height / world.height;
  if (fog) {
    for (let y = 0; y < world.height; y++) {
      for (let x = 0; x < world.width; x++) {
        const k = world.key([x, y]);
        if (fog.visible.has(k)) continue;
        ctx.fillStyle = fog.explored.has(k) ? "rgba(4,6,4,.6)" : "rgba(4,6,4,.92)";
        ctx.fillRect(Math.floor(x * sx), Math.floor(y * sy), Math.ceil(sx), Math.ceil(sy));
      }
    }
  }
  // selected units blink on the minimap
  ctx.fillStyle = "#fef08a";
  for (const u of units) ctx.fillRect(u.pos[0] * sx - 1, u.pos[1] * sy - 1, Math.max(3, sx + 2), Math.max(3, sy + 2));
  ctx.strokeStyle = "rgba(255,255,255,.9)";
  ctx.lineWidth = 1.5;
  ctx.strokeRect(v.x0 * sx + 0.5, v.y0 * sy + 0.5, Math.min(v.cols, world.width) * sx - 1, Math.min(v.rows, world.height) * sy - 1);
}
