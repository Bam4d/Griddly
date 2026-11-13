/**
 * Puts the engine's renders on the page. Everything about the game picture comes from the GDY's
 * observers, rendered in WASM: the camera window (PlayerView / CameraView: cam_x / cam_y, fog of war,
 * health bars, ownership shadows, workers' loads, labs researching) and the mini-map (MiniMap /
 * GlobalMap: fog, camera outline). This file only scales those pixels to the canvas and draws the
 * pointer interaction on top: the selection outline, hovered cell, drag box and order lines.
 */
import type { Order } from "./orders";
import type { Cell, WorldView } from "./world";

/** Engine observer names (GriddlyRTS-FoW.yaml). */
export const OBSERVERS = {
  view: "PlayerView",
  viewNoFog: "CameraView",
  minimap: "MiniMap",
  minimapNoFog: "GlobalMap",
} as const;

/** Where the engine's camera window sits on the board canvas. */
export type Viewport = {
  /** Display size of a cell in CSS pixels. */
  tile: number;
  /** Top-left cell of the engine's view (its camera origin). */
  x0: number;
  y0: number;
  /** View size in cells. */
  cols: number;
  rows: number;
  /** Pixel offset that centres the view on the canvas. */
  ox: number;
  oy: number;
};

/** Fit the engine's view (cols x rows cells, origin from the engine) into a widthPx x heightPx canvas. */
export function viewport(origin: Cell, cols: number, rows: number, widthPx: number, heightPx: number): Viewport {
  const tile = Math.max(1, Math.min(widthPx / cols, heightPx / rows));
  return {
    tile,
    x0: origin[0],
    y0: origin[1],
    cols,
    rows,
    ox: Math.floor((widthPx - cols * tile) / 2),
    oy: Math.floor((heightPx - rows * tile) / 2),
  };
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
};

/** RGBA pixels from the engine into a reusable canvas. */
export function blit(target: HTMLCanvasElement | null, img: { pixels: Uint8Array; width: number; height: number }): HTMLCanvasElement {
  const c = target ?? document.createElement("canvas");
  if (c.width !== img.width || c.height !== img.height) {
    c.width = img.width;
    c.height = img.height;
  }
  // copy: the pixels are a view into WASM memory that the next render reuses
  const data = new Uint8ClampedArray(img.pixels.slice(0, img.width * img.height * 4));
  c.getContext("2d")!.putImageData(new ImageData(data, img.width, img.height), 0, 0);
  return c;
}

/** The engine's camera view, scaled, plus the pointer marks. */
export function drawBoard(ctx: CanvasRenderingContext2D, board: HTMLCanvasElement | null, world: WorldView, v: Viewport, marks: Marks, now: number): void {
  const { width, height } = ctx.canvas;
  ctx.fillStyle = "#0b0d0a";
  ctx.fillRect(0, 0, width, height);
  if (!board) return;
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(board, v.ox, v.oy, v.cols * v.tile, v.rows * v.tile);

  const visible = (c: Cell) => c[0] >= v.x0 && c[1] >= v.y0 && c[0] < v.x0 + v.cols && c[1] < v.y0 + v.rows;
  const t = v.tile;

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

  // selection outlines (your own units only, so this never reveals anything)
  ctx.strokeStyle = "rgba(134,239,172,.95)";
  ctx.lineWidth = Math.max(1.5, t / 14);
  for (const id of marks.selected) {
    const o = world.byId.get(id);
    if (!o || !visible(o.pos)) continue;
    const [x, y] = toPx(v, o.pos);
    ctx.strokeRect(x + 1, y + 1, t - 2, t - 2);
  }

  for (const p of marks.pings) {
    const age = (now - p.at) / 600;
    if (age > 1 || !visible(p.cell)) continue;
    const [x, y] = toPx(v, p.cell);
    ctx.strokeStyle = p.color;
    ctx.globalAlpha = 1 - age;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(x + t / 2, y + t / 2, (t / 2) * (0.4 + age), 0, Math.PI * 2);
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

/** The engine's mini-map, scaled to the canvas. */
export function drawMinimap(ctx: CanvasRenderingContext2D, mini: HTMLCanvasElement | null): void {
  const { width, height } = ctx.canvas;
  ctx.fillStyle = "#0b0d0a";
  ctx.fillRect(0, 0, width, height);
  if (!mini) return;
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(mini, 0, 0, width, height);
}

/** Engine fog of war state of a cell for the viewing player (2 = visible now); everything is visible without fog. */
export const isVisible = (world: WorldView, fog: Uint8Array | null, c: Cell) => !fog || (world.inside(c) && fog[world.key(c)] === 2);
