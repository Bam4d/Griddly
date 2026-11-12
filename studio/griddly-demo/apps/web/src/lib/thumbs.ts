// Level thumbnails rendered by the engine (whole-map Sprite2D observer), downscaled to data URLs
// and cached for the page's lifetime.
//
// Thumbnails use their own short-lived engine game (straight from the embind API), separate from
// the `Griddly` wrapper's game that the player page plays. So rendering them never disturbs a game
// in progress, and the player page can render them after its board is already interactive. One
// game renders every level of a GDY (loadLevel + reset).
import type { Griddly } from "@griddly/wasm";

import type { Game } from "../games/catalog";
import { withGriddly } from "./griddly";

const cache = new Map<string, string>();
const key = (id: string, level: number) => `${id}:${level}`;

export const cachedThumb = (id: string, level: number): string | undefined => cache.get(key(id, level));

function toDataUrl(pixels: Uint8Array, width: number, height: number, maxWidth: number): string {
  const src = document.createElement("canvas");
  src.width = width;
  src.height = height;
  src.getContext("2d")!.putImageData(new ImageData(new Uint8ClampedArray(pixels.slice(0, width * height * 4)), width, height), 0, 0);
  const scale = Math.min(1, maxWidth / width);
  const out = document.createElement("canvas");
  out.width = Math.max(1, Math.round(width * scale));
  out.height = Math.max(1, Math.round(height * scale));
  const ctx = out.getContext("2d")!;
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(src, 0, 0, out.width, out.height);
  return out.toDataURL("image/webp", 0.85);
}

/** Embind throws raw exception pointers; turn them into readable errors. */
const engineError = (g: Griddly, e: unknown) =>
  typeof e === "number" ? new Error(g.griddlyjs.getExceptionMessage(e)) : e;

const shapeOf = (shape: unknown): number[] => {
  if (Array.isArray(shape)) return shape as number[];
  const v = shape as { size?: () => number; get?: (i: number) => number } | undefined;
  return v?.size && v.get ? Array.from({ length: v.size() }, (_, i) => v.get!(i)) : [];
};

export type ThumbTiming = { level: number; ms: number };

/**
 * Render (and cache) thumbnails of `levels` of a game. Each level is its own job on the shared engine
 * queue, so a player's moves interleave with thumbnail rendering instead of waiting for all of it.
 * `onThumb` is called as each one is ready. Returns per-level render timings (cached = not listed).
 */
export async function renderThumbs(
  game: Game,
  levels: number[],
  {
    maxWidth = 360,
    onThumb,
    isCancelled = () => false,
  }: { maxWidth?: number; onThumb?: (level: number, url: string) => void; isCancelled?: () => boolean } = {}
): Promise<ThumbTiming[]> {
  const todo = levels.filter((l) => !cache.has(key(game.id, l)));
  if (!todo.length || isCancelled()) return [];
  type Handles = { gdy: ReturnType<Griddly["griddlyjs"]["loadString"]>; game: ReturnType<ReturnType<Griddly["griddlyjs"]["loadString"]>["createGame"]> };
  let h: Handles | null = null;
  const timings: ThumbTiming[] = [];
  try {
    for (const l of todo) {
      if (isCancelled()) break;
      await withGriddly(async (g) => {
        try {
          if (!h) {
            const gdy = g.griddlyjs.loadString(game.gdy);
            const tg = gdy.createGame("Sprite2D");
            for (let p = 0; p < gdy.getPlayerCount(); p++) tg.registerPlayer(`thumb ${p}`, "Vector");
            tg.init();
            h = { gdy, game: tg };
          }
          const t0 = performance.now();
          h.game.loadLevel(l);
          h.game.seedRandomGenerator(1);
          h.game.reset();
          const pixels = await Promise.resolve(h.game.observeImage());
          const [, width = 0, height = 0] = shapeOf(h.game.getGlobalObservationDescription().Shape);
          if (width && height) {
            const url = toDataUrl(pixels, width, height, maxWidth);
            cache.set(key(game.id, l), url);
            timings.push({ level: l, ms: Math.round(performance.now() - t0) });
            onThumb?.(l, url);
          }
        } catch (e) {
          throw engineError(g, e);
        }
      });
    }
  } finally {
    const done = h as Handles | null;
    if (done) {
      await withGriddly(() => {
        done.game.release();
        done.game.delete();
        done.gdy.delete();
      });
    }
  }
  return timings;
}

/** Thumbnail of one level (used by the library cards). */
export async function thumb(game: Game, level = 0, isCancelled: () => boolean = () => false): Promise<string | undefined> {
  const hit = cache.get(key(game.id, level));
  if (hit) return hit;
  await renderThumbs(game, [level], { isCancelled });
  return cache.get(key(game.id, level));
}
