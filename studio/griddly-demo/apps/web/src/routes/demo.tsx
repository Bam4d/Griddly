import { createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useRef, useState } from "react";

import { getGriddly } from "../lib/griddly";
import { BACKGROUND_TILE, SPIDERS_GDY, SPIDERS_LEVELS, SPRITES } from "../games/spiders";
import type { Griddly } from "@griddly/wasm";

/**
 * Griddly "Spiders Garden" demo: guide the gnome to the gem without touching a spider. The GDY is
 * bundled with the app and loaded via `loadGDYString`. The game state is drawn on a 2D canvas with
 * the sprites the GDY names, read straight from the WASM virtual FS (incl. WALL_16 auto-tiling).
 */

const TILE = 32;
const RES = "/resources/images/";

// Default (absolute) move mapping from the GDY: 1 = left, 2 = up, 3 = right, 4 = down.
const LEFT = 1;
const UP = 2;
const RIGHT = 3;
const DOWN = 4;
const KEYS: Record<string, number> = {
  ArrowLeft: LEFT,
  a: LEFT,
  ArrowUp: UP,
  w: UP,
  ArrowRight: RIGHT,
  d: RIGHT,
  ArrowDown: DOWN,
  s: DOWN,
};
const ARROW_ANGLE: Record<number, number> = { [UP]: 0, [RIGHT]: 90, [DOWN]: 180, [LEFT]: 270 };

const FALLBACK: Record<string, string> = {
  wall: "#6b7280",
  hedge: "#166534",
  water: "#1d4ed8",
  spider: "#ef4444",
  gnome: "#3b82f6",
  gem: "#22c55e",
};

type Obj = { name: string; location: [number, number]; zidx: number };
type State = { objects: Obj[] };
type Status = "loading" | "playing" | "won" | "lost" | "error";

function loadImages(g: Griddly): Promise<Record<string, HTMLImageElement>> {
  const paths = new Set<string>(BACKGROUND_TILE ? [BACKGROUND_TILE] : []);
  for (const s of Object.values(SPRITES)) s.images.forEach((p) => paths.add(p));
  const fs = (g.module as unknown as { FS: { readFile(p: string): Uint8Array } }).FS;
  return Promise.all(
    [...paths].map(
      (p) =>
        new Promise<[string, HTMLImageElement | null]>((resolve) => {
          try {
            const bytes = fs.readFile(RES + p);
            const img = new Image();
            img.onload = () => resolve([p, img]);
            img.onerror = () => resolve([p, null]);
            img.src = URL.createObjectURL(new Blob([bytes as BlobPart], { type: "image/png" }));
          } catch {
            resolve([p, null]);
          }
        })
    )
  ).then((pairs) => Object.fromEntries(pairs.filter((x): x is [string, HTMLImageElement] => !!x[1])));
}

const DemoPage = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const griddlyRef = useRef<Griddly | null>(null);
  const imagesRef = useRef<Record<string, HTMLImageElement>>({});
  const lastMoveRef = useRef<number>(DOWN);
  const [status, setStatus] = useState<Status>("loading");
  const [error, setError] = useState("");
  const [level, setLevel] = useState(0);
  const [steps, setSteps] = useState(0);
  const [score, setScore] = useState({ wins: 0, losses: 0 });

  const draw = useCallback(() => {
    const g = griddlyRef.current;
    const canvas = canvasRef.current;
    if (!g || !canvas) return;
    const state = g.getState() as State;
    const W = g.width;
    const H = g.height;
    canvas.width = W * TILE;
    canvas.height = H * TILE;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.imageSmoothingEnabled = false;
    const imgs = imagesRef.current;

    const bg = BACKGROUND_TILE ? imgs[BACKGROUND_TILE] : undefined;
    for (let y = 0; y < H; y++)
      for (let x = 0; x < W; x++) {
        if (bg) ctx.drawImage(bg, x * TILE, y * TILE, TILE, TILE);
        else {
          ctx.fillStyle = "#2a2618";
          ctx.fillRect(x * TILE, y * TILE, TILE, TILE);
        }
      }

    // Occupancy per object type, for WALL_16 / WALL_2 tiling (same object name connects).
    const at = new Map<string, Set<string>>();
    for (const o of state.objects) {
      if (!at.has(o.name)) at.set(o.name, new Set());
      at.get(o.name)!.add(`${o.location[0]},${o.location[1]}`);
    }
    const same = (name: string, x: number, y: number) => at.get(name)?.has(`${x},${y}`) ?? false;

    const objs = [...state.objects].sort((a, b) => (SPRITES[a.name]?.z ?? 0) - (SPRITES[b.name]?.z ?? 0));
    for (const o of objs) {
      const [x, y] = o.location;
      const px = x * TILE;
      const py = y * TILE;
      const def = SPRITES[o.name];
      let path = def?.images[0];
      if (def?.tiling === "WALL_16") {
        // Bit order matches Griddly's SpriteObserver: left=1, right=2, up=4, down=8
        const idx =
          (same(o.name, x - 1, y) ? 1 : 0) |
          (same(o.name, x + 1, y) ? 2 : 0) |
          (same(o.name, x, y - 1) ? 4 : 0) |
          (same(o.name, x, y + 1) ? 8 : 0);
        path = def.images[idx];
      } else if (def?.tiling === "WALL_2") {
        path = def.images[same(o.name, x, y + 1) ? 1 : 0];
      }
      const img = path ? imgs[path] : undefined;
      if (img) ctx.drawImage(img, px, py, TILE, TILE);
      else {
        ctx.fillStyle = FALLBACK[o.name] ?? "#a855f7";
        ctx.fillRect(px + 2, py + 2, TILE - 4, TILE - 4);
      }
      if (o.name === "gnome") drawArrow(ctx, px, py, ARROW_ANGLE[lastMoveRef.current] ?? 180);
    }
  }, []);

  const start = useCallback(
    (lvl: number) => {
      const g = griddlyRef.current;
      if (!g) return;
      g.loadLevel(lvl);
      g.seed(Math.floor(Math.random() * 1e9));
      g.reset();
      lastMoveRef.current = DOWN;
      setSteps(0);
      setStatus("playing");
      draw();
    },
    [draw]
  );

  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const g = await getGriddly();
        if (cancelled) return;
        griddlyRef.current = g;
        g.loadGDYString(SPIDERS_GDY);
        g.createGame("Vector");
        imagesRef.current = await loadImages(g);
        start(0);
      } catch (e) {
        setStatus("error");
        setError(e instanceof Error ? e.message : String(e));
      }
    })();
    return () => {
      cancelled = true;
      griddlyRef.current?.unload();
      griddlyRef.current = null;
    };
  }, [start]);

  const doStep = useCallback(
    (action: number) => {
      const g = griddlyRef.current;
      if (!g || status !== "playing") return;
      lastMoveRef.current = action;
      g.step([action]);
      setSteps((s) => s + 1);
      const st = g.getState() as State;
      const has = (n: string) => st.objects.some((o) => o.name === n);
      if (!has("gnome")) {
        setStatus("lost");
        setScore((s) => ({ ...s, losses: s.losses + 1 }));
      } else if (!has("gem")) {
        setStatus("won");
        setScore((s) => ({ ...s, wins: s.wins + 1 }));
      }
      draw();
    },
    [status, draw]
  );

  const nextLevel = useCallback(() => {
    const next = (level + 1) % SPIDERS_LEVELS.length;
    setLevel(next);
    start(next);
  }, [level, start]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.target instanceof HTMLSelectElement) return;
      if (e.key === "r" || e.key === "R") {
        e.preventDefault();
        start(level);
        return;
      }
      if ((e.key === "n" || e.key === "N" || e.key === "Enter") && status === "won") {
        e.preventDefault();
        nextLevel();
        return;
      }
      const a = KEYS[e.key];
      if (a !== undefined) {
        e.preventDefault();
        doStep(a);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [doStep, start, nextLevel, level, status]);

  return (
    <main className="mx-auto flex min-h-full max-w-4xl flex-col gap-4 p-8">
      <header className="flex flex-col gap-1">
        <p className="text-sm text-neutral-500">Griddly WASM demo</p>
        <h1 className="text-3xl font-semibold">Spiders Garden 🕷️</h1>
        <p className="text-sm text-neutral-400">
          Guide the gnome to the gem without touching a spider. Spiders wander at random; water, hedges, bushes, trees
          and tombstones block the way. Running fully in your browser via <code>@griddly/wasm</code>.
        </p>
      </header>

      <div className="flex flex-wrap items-center gap-3 text-sm">
        <label className="flex items-center gap-2">
          Level
          <select
            value={level}
            onChange={(e) => {
              const l = Number(e.target.value);
              setLevel(l);
              start(l);
              e.currentTarget.blur();
            }}
            className="rounded bg-neutral-800 px-2 py-1"
          >
            {SPIDERS_LEVELS.map((name, i) => (
              <option key={i} value={i}>
                {i + 1}. {name}
              </option>
            ))}
          </select>
        </label>
        <span className="ml-auto">steps: {steps}</span>
        <span className="text-green-500">wins: {score.wins}</span>
        <span className="text-red-500">deaths: {score.losses}</span>
      </div>

      <p className="text-sm">
        {status === "loading" && <span className="text-amber-500">Loading WASM engine…</span>}
        {status === "error" && <span className="text-red-500">Error: {error}</span>}
        {status === "playing" && <span className="text-neutral-400">Arrow keys / WASD to move · R restart</span>}
        {status === "won" && (
          <span className="text-green-500">💎 Got the gem! Press N for the next level, R to replay.</span>
        )}
        {status === "lost" && <span className="text-red-500">🕷️ Eaten by a spider. Press R to try again.</span>}
      </p>

      <div className="flex justify-center overflow-auto">
        <canvas
          ref={canvasRef}
          className="rounded-lg border border-neutral-700 bg-neutral-900"
          style={{ imageRendering: "pixelated", maxWidth: "100%" }}
        />
      </div>

      <div className="flex flex-col items-center gap-1">
        <Btn onClick={() => doStep(UP)}>↑</Btn>
        <div className="flex gap-1">
          <Btn onClick={() => doStep(LEFT)}>←</Btn>
          <Btn onClick={() => doStep(DOWN)}>↓</Btn>
          <Btn onClick={() => doStep(RIGHT)}>→</Btn>
        </div>
        <div className="mt-2 flex gap-2">
          <button onClick={() => start(level)} className="rounded bg-blue-700 px-3 py-1 text-sm hover:bg-blue-600">
            Restart (R)
          </button>
          {status === "won" && (
            <button onClick={nextLevel} className="rounded bg-green-700 px-3 py-1 text-sm hover:bg-green-600">
              Next level (N)
            </button>
          )}
        </div>
      </div>
    </main>
  );
};

const Btn = ({ onClick, children }: { onClick: () => void; children: React.ReactNode }) => (
  <button onClick={onClick} className="w-10 rounded bg-neutral-800 py-1 text-sm hover:bg-neutral-700">
    {children}
  </button>
);

/** Small arrow above the gnome showing the last move direction (the sprite itself never rotates). */
function drawArrow(ctx: CanvasRenderingContext2D, px: number, py: number, angleDeg: number) {
  ctx.save();
  ctx.translate(px + TILE / 2, py + TILE / 2);
  ctx.rotate((angleDeg * Math.PI) / 180);
  ctx.fillStyle = "#facc15";
  ctx.strokeStyle = "#1c1917";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, -TILE / 2 + 1);
  ctx.lineTo(-4, -TILE / 2 + 7);
  ctx.lineTo(4, -TILE / 2 + 7);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  ctx.restore();
}

export const Route = createFileRoute("/demo")({
  component: DemoPage,
});
