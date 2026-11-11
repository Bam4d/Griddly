import { createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useRef, useState } from "react";

import { getGriddly } from "../lib/griddly";
import { BACKGROUND_TILE, SPIDERS_GDY, SPIDERS_LEVELS, SPRITES } from "../games/spiders";
import type { Griddly } from "@griddly/wasm";

/**
 * Griddly "Spiders Garden" demo: collect every gem without touching a spider. The GDY is bundled with
 * the app and loaded via `loadGDYString`; the state is drawn on a 2D canvas with the sprites the GDY
 * names (read from the WASM virtual FS), including WALL_16 auto-tiling and Z ordering.
 */

const SRC_TILE = 24; // native sprite size
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
const FALLBACK: Record<string, string> = { wall: "#6b7280", hedge: "#166534", rock: "#7c2d12", water: "#1d4ed8" };

type Obj = { name: string; location: [number, number]; variables?: Record<string, number> };
type State = { objects: Obj[] };
type Status = "loading" | "playing" | "won" | "lost" | "error";
type Hud = { gems: number; gemsTotal: number; keys: number; spiders: number };

function loadImages(g: Griddly): Promise<Record<string, HTMLImageElement>> {
  const paths = new Set<string>(BACKGROUND_TILE ? [BACKGROUND_TILE] : []);
  for (const s of Object.values(SPRITES)) s.images.forEach((p) => paths.add(p));
  const fs = (g.module as unknown as { FS: { readFile(p: string): Uint8Array } }).FS;
  return Promise.all(
    [...paths].map(
      (p) =>
        new Promise<[string, HTMLImageElement | null]>((resolve) => {
          try {
            const img = new Image();
            img.onload = () => resolve([p, img]);
            img.onerror = () => resolve([p, null]);
            img.src = URL.createObjectURL(new Blob([fs.readFile(RES + p) as BlobPart], { type: "image/png" }));
          } catch {
            resolve([p, null]);
          }
        })
    )
  ).then((pairs) => Object.fromEntries(pairs.filter((x): x is [string, HTMLImageElement] => !!x[1])));
}

const DemoPage = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef<HTMLDivElement>(null);
  const griddlyRef = useRef<Griddly | null>(null);
  const imagesRef = useRef<Record<string, HTMLImageElement>>({});
  const lastMoveRef = useRef<number>(DOWN);
  const gemsTotalRef = useRef(0);
  const [status, setStatus] = useState<Status>("loading");
  const [error, setError] = useState("");
  const [level, setLevel] = useState(0);
  const [steps, setSteps] = useState(0);
  const [hud, setHud] = useState<Hud>({ gems: 0, gemsTotal: 0, keys: 0, spiders: 0 });
  const [score, setScore] = useState({ wins: 0, losses: 0 });
  const [follow, setFollow] = useState(false);
  const followRef = useRef(follow);
  followRef.current = follow;

  const draw = useCallback(() => {
    const g = griddlyRef.current;
    const canvas = canvasRef.current;
    const frame = frameRef.current;
    if (!g || !canvas || !frame) return;
    const state = g.getState() as State;
    const W = g.width;
    const H = g.height;
    const imgs = imagesRef.current;

    // Camera: whole map scaled to fit, or a 15x11 window following the gnome.
    const gnome = state.objects.find((o) => o.name === "gnome");
    const viewW = followRef.current ? Math.min(W, 15) : W;
    const viewH = followRef.current ? Math.min(H, 11) : H;
    let camX = 0;
    let camY = 0;
    if (followRef.current && gnome) {
      camX = Math.min(Math.max(gnome.location[0] - Math.floor(viewW / 2), 0), W - viewW);
      camY = Math.min(Math.max(gnome.location[1] - Math.floor(viewH / 2), 0), H - viewH);
    }
    const maxW = frame.clientWidth;
    const maxH = Math.max(window.innerHeight * 0.62, 320);
    const tile = Math.max(8, Math.floor(Math.min(maxW / viewW, maxH / viewH, 40)));
    const dpr = window.devicePixelRatio || 1;
    canvas.width = viewW * tile * dpr;
    canvas.height = viewH * tile * dpr;
    canvas.style.width = `${viewW * tile}px`;
    canvas.style.height = `${viewH * tile}px`;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.imageSmoothingEnabled = false;

    const bg = BACKGROUND_TILE ? imgs[BACKGROUND_TILE] : undefined;
    for (let y = 0; y < viewH; y++)
      for (let x = 0; x < viewW; x++) {
        if (bg) ctx.drawImage(bg, x * tile, y * tile, tile, tile);
        else {
          ctx.fillStyle = "#2a2618";
          ctx.fillRect(x * tile, y * tile, tile, tile);
        }
      }

    const at = new Map<string, Set<string>>();
    for (const o of state.objects) {
      if (!at.has(o.name)) at.set(o.name, new Set());
      at.get(o.name)!.add(`${o.location[0]},${o.location[1]}`);
    }
    const same = (name: string, x: number, y: number) => at.get(name)?.has(`${x},${y}`) ?? false;

    const objs = [...state.objects].sort((a, b) => (SPRITES[a.name]?.z ?? 0) - (SPRITES[b.name]?.z ?? 0));
    for (const o of objs) {
      const [x, y] = o.location;
      if (x < camX || y < camY || x >= camX + viewW || y >= camY + viewH) continue;
      const px = (x - camX) * tile;
      const py = (y - camY) * tile;
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
      if (img) ctx.drawImage(img, px, py, tile, tile);
      else {
        ctx.fillStyle = FALLBACK[o.name] ?? "#a855f7";
        ctx.fillRect(px, py, tile, tile);
      }
      if (o.name === "gnome") drawArrow(ctx, px, py, tile, ARROW_ANGLE[lastMoveRef.current] ?? 180);
    }

    // soft vignette
    const vw = viewW * tile;
    const vh = viewH * tile;
    const grad = ctx.createRadialGradient(vw / 2, vh / 2, Math.min(vw, vh) * 0.35, vw / 2, vh / 2, Math.max(vw, vh) * 0.75);
    grad.addColorStop(0, "rgba(0,0,0,0)");
    grad.addColorStop(1, "rgba(0,0,0,0.45)");
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, vw, vh);

    const count = (n: string) => state.objects.filter((o) => o.name === n).length;
    setHud({
      gems: gemsTotalRef.current - count("gem"),
      gemsTotal: gemsTotalRef.current,
      keys: gnome?.variables?.keys ?? 0,
      spiders: count("spider"),
    });
  }, []);

  const start = useCallback(
    (lvl: number) => {
      const g = griddlyRef.current;
      if (!g) return;
      g.loadLevel(lvl);
      g.seed(Math.floor(Math.random() * 1e9));
      g.reset();
      gemsTotalRef.current = (g.getState() as State).objects.filter((o) => o.name === "gem").length;
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

  useEffect(() => {
    const onResize = () => draw();
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, [draw]);
  useEffect(() => draw(), [follow, draw]);

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

  const pick = useCallback(
    (l: number) => {
      setLevel(l);
      start(l);
    },
    [start]
  );
  const nextLevel = useCallback(() => pick((level + 1) % SPIDERS_LEVELS.length), [level, pick]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "r" || e.key === "R") {
        e.preventDefault();
        start(level);
        return;
      }
      if (e.key === "f" || e.key === "F") {
        e.preventDefault();
        setFollow((f) => !f);
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

  const meta = SPIDERS_LEVELS[level];

  return (
    <div className="min-h-full bg-[radial-gradient(ellipse_at_top,_#1b2a1f_0%,_#0b0f0c_55%,_#050605_100%)] text-stone-200">
      <main className="mx-auto flex max-w-6xl flex-col gap-5 px-4 py-6 sm:px-8">
        <header className="flex flex-wrap items-end justify-between gap-3">
          <div>
            <p className="font-mono text-[11px] uppercase tracking-[0.3em] text-emerald-400/70">
              Griddly · WebAssembly
            </p>
            <h1 className="bg-gradient-to-r from-emerald-200 via-lime-200 to-amber-200 bg-clip-text text-4xl font-black tracking-tight text-transparent">
              Spiders Garden
            </h1>
          </div>
          <div className="flex gap-2 font-mono text-xs">
            <Pill tone="emerald">🏆 {score.wins}</Pill>
            <Pill tone="rose">💀 {score.losses}</Pill>
          </div>
        </header>

        {/* level picker */}
        <nav className="-mx-1 flex gap-2 overflow-x-auto px-1 pb-1">
          {SPIDERS_LEVELS.map((l, i) => (
            <button
              key={i}
              onClick={() => pick(i)}
              className={`group shrink-0 rounded-xl border px-3 py-2 text-left transition ${
                i === level
                  ? "border-emerald-400/60 bg-emerald-400/10 shadow-[0_0_24px_-6px_rgba(52,211,153,0.6)]"
                  : "border-white/5 bg-white/[0.03] hover:border-white/15 hover:bg-white/[0.06]"
              }`}
            >
              <span className="block font-mono text-[10px] text-stone-500 group-hover:text-stone-400">
                LEVEL {String(i + 1).padStart(2, "0")}
              </span>
              <span className={`block text-sm font-semibold ${i === level ? "text-emerald-200" : "text-stone-300"}`}>
                {l.name}
              </span>
            </button>
          ))}
        </nav>

        <section className="grid min-w-0 gap-5 lg:grid-cols-[minmax(0,1fr)_260px]">
          {/* board */}
          <div className="relative min-w-0 rounded-2xl border border-white/10 bg-black/40 p-3 shadow-2xl shadow-black/60 ring-1 ring-emerald-500/10 backdrop-blur">
            <div ref={frameRef} className="flex min-h-[240px] w-full min-w-0 items-center justify-center overflow-hidden rounded-xl">
              <canvas ref={canvasRef} className="max-w-full rounded-lg" style={{ imageRendering: "pixelated" }} />
            </div>

            {status === "loading" && <Overlay>Waking the spiders…</Overlay>}
            {status === "error" && <Overlay tone="rose">Error: {error}</Overlay>}
            {status === "won" && (
              <Overlay>
                <p className="text-5xl">💎</p>
                <p className="mt-2 text-2xl font-bold text-emerald-200">All gems collected!</p>
                <p className="mt-1 font-mono text-xs text-stone-400">{steps} steps</p>
                <div className="mt-4 flex gap-2">
                  <Action onClick={() => start(level)}>Replay</Action>
                  <Action primary onClick={nextLevel}>
                    Next level →
                  </Action>
                </div>
              </Overlay>
            )}
            {status === "lost" && (
              <Overlay tone="rose">
                <p className="text-5xl">🕷️</p>
                <p className="mt-2 text-2xl font-bold text-rose-200">Eaten by a spider</p>
                <div className="mt-4">
                  <Action primary onClick={() => start(level)}>
                    Try again (R)
                  </Action>
                </div>
              </Overlay>
            )}
          </div>

          {/* side panel */}
          <aside className="flex flex-col gap-4">
            <div className="rounded-2xl border border-white/10 bg-white/[0.03] p-4">
              <p className="text-lg font-bold text-stone-100">{meta?.name}</p>
              <p className="mt-1 text-sm leading-snug text-stone-400">{meta?.blurb}</p>
              <div className="mt-4 grid grid-cols-2 gap-2 font-mono text-xs">
                <Stat label="Gems" value={`${hud.gems}/${hud.gemsTotal}`} tone="text-fuchsia-300" />
                <Stat label="Keys" value={hud.keys} tone="text-amber-300" />
                <Stat label="Spiders" value={hud.spiders} tone="text-rose-300" />
                <Stat label="Steps" value={steps} tone="text-sky-300" />
              </div>
            </div>

            <div className="flex flex-col items-center gap-2 rounded-2xl border border-white/10 bg-white/[0.03] p-4">
              <Pad onClick={() => doStep(UP)}>▲</Pad>
              <div className="flex gap-2">
                <Pad onClick={() => doStep(LEFT)}>◀</Pad>
                <Pad onClick={() => doStep(DOWN)}>▼</Pad>
                <Pad onClick={() => doStep(RIGHT)}>▶</Pad>
              </div>
              <div className="mt-2 flex w-full gap-2">
                <Action onClick={() => start(level)} className="flex-1">
                  ↻ Restart
                </Action>
                <Action onClick={() => setFollow((f) => !f)} className="flex-1" active={follow}>
                  {follow ? "◎ Follow" : "▦ Full map"}
                </Action>
              </div>
              <p className="mt-1 text-center font-mono text-[10px] leading-relaxed text-stone-500">
                ARROWS / WASD move · R restart · F camera
              </p>
            </div>

            <div className="rounded-2xl border border-white/10 bg-white/[0.03] p-4 text-xs leading-relaxed text-stone-400">
              Grab every <span className="text-fuchsia-300">gem</span>. <span className="text-amber-300">Keys</span>{" "}
              open doors. <span className="text-rose-300">Spiders</span> wander at random and cocoon nests hatch more.
              Runs entirely in your browser via <code className="text-emerald-300">@griddly/wasm</code>.
            </div>
          </aside>
        </section>
      </main>
    </div>
  );
};

const Pill = ({ tone, children }: { tone: "emerald" | "rose"; children: React.ReactNode }) => (
  <span
    className={`rounded-full border px-3 py-1 ${
      tone === "emerald"
        ? "border-emerald-400/30 bg-emerald-400/10 text-emerald-200"
        : "border-rose-400/30 bg-rose-400/10 text-rose-200"
    }`}
  >
    {children}
  </span>
);

const Stat = ({ label, value, tone }: { label: string; value: React.ReactNode; tone: string }) => (
  <div className="rounded-lg border border-white/5 bg-black/30 px-3 py-2">
    <p className="text-[10px] uppercase tracking-widest text-stone-500">{label}</p>
    <p className={`text-lg font-bold ${tone}`}>{value}</p>
  </div>
);

const Pad = ({ onClick, children }: { onClick: () => void; children: React.ReactNode }) => (
  <button
    onClick={onClick}
    className="flex h-12 w-12 items-center justify-center rounded-xl border border-white/10 bg-gradient-to-b from-stone-700/60 to-stone-900/80 text-stone-200 shadow-[inset_0_1px_0_rgba(255,255,255,0.08),0_4px_12px_-4px_rgba(0,0,0,0.8)] transition hover:border-emerald-400/40 hover:text-emerald-200 active:translate-y-px active:shadow-none"
  >
    {children}
  </button>
);

const Action = ({
  onClick,
  children,
  primary,
  active,
  className = "",
}: {
  onClick: () => void;
  children: React.ReactNode;
  primary?: boolean;
  active?: boolean;
  className?: string;
}) => (
  <button
    onClick={onClick}
    className={`rounded-lg px-4 py-2 text-sm font-semibold transition active:translate-y-px ${
      primary
        ? "bg-gradient-to-b from-emerald-400 to-emerald-600 text-emerald-950 shadow-[0_0_20px_-4px_rgba(52,211,153,0.7)] hover:from-emerald-300 hover:to-emerald-500"
        : active
          ? "border border-emerald-400/40 bg-emerald-400/10 text-emerald-200"
          : "border border-white/10 bg-white/[0.04] text-stone-300 hover:border-white/20 hover:bg-white/[0.08]"
    } ${className}`}
  >
    {children}
  </button>
);

const Overlay = ({ children, tone = "emerald" }: { children: React.ReactNode; tone?: "emerald" | "rose" }) => (
  <div
    className={`absolute inset-3 flex flex-col items-center justify-center rounded-xl text-center backdrop-blur-[2px] ${
      tone === "rose" ? "bg-rose-950/60" : "bg-black/60"
    }`}
  >
    {children}
  </div>
);

/** Small arrow above the gnome showing the last move direction (the sprite itself never rotates). */
function drawArrow(ctx: CanvasRenderingContext2D, px: number, py: number, tile: number, angleDeg: number) {
  const s = tile / SRC_TILE;
  ctx.save();
  ctx.translate(px + tile / 2, py + tile / 2);
  ctx.rotate((angleDeg * Math.PI) / 180);
  ctx.fillStyle = "#facc15";
  ctx.strokeStyle = "#1c1917";
  ctx.lineWidth = Math.max(1, s);
  ctx.beginPath();
  ctx.moveTo(0, -tile / 2 + 1);
  ctx.lineTo(-3.5 * s, -tile / 2 + 6 * s);
  ctx.lineTo(3.5 * s, -tile / 2 + 6 * s);
  ctx.closePath();
  ctx.stroke();
  ctx.fill();
  ctx.restore();
}

export const Route = createFileRoute("/demo")({
  component: DemoPage,
});
