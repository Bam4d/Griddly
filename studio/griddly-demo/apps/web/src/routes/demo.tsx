import { createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useRef, useState } from "react";

import { getGriddly } from "../lib/griddly";
import { CAMERAS, SPIDERS_GDY, SPIDERS_LEVELS } from "../games/spiders";
import type { Griddly } from "@griddly/wasm";

/**
 * Griddly "Spiders Garden" demo: collect every gem without touching a spider. The GDY is bundled with
 * the app and loaded via `loadGDYString`. Everything you see is rendered by the engine: each camera is
 * a SPRITE_2D observer defined in the GDY (whole map = global observer, follow cams = the player's
 * avatar-tracking observer). The page only blits the observer's RGBA pixels onto a canvas.
 */

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

type Obj = { name: string; variables?: Record<string, number> };
type State = { objects: Obj[] };
type Status = "loading" | "playing" | "won" | "lost" | "error";
type Hud = { gems: number; gemsTotal: number; keys: number; spiders: number };

const DemoPage = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef<HTMLDivElement>(null);
  const griddlyRef = useRef<Griddly | null>(null);
  const gemsTotalRef = useRef(0);
  const [status, setStatus] = useState<Status>("loading");
  const [error, setError] = useState("");
  const [level, setLevel] = useState(0);
  const [steps, setSteps] = useState(0);
  const [hud, setHud] = useState<Hud>({ gems: 0, gemsTotal: 0, keys: 0, spiders: 0 });
  const [score, setScore] = useState({ wins: 0, losses: 0 });
  const [camera, setCamera] = useState(0);
  const cam = CAMERAS[camera] ?? CAMERAS[0]!;
  const camRef = useRef(cam);
  camRef.current = cam;

  const draw = useCallback(async () => {
    const g = griddlyRef.current;
    const canvas = canvasRef.current;
    const frame = frameRef.current;
    if (!g || !canvas || !frame) return;

    // The engine renders the chosen camera observer; we only scale and blit its pixels.
    const { pixels, width, height } = camRef.current.follows ? await g.observePlayerImage(0) : await g.observeImage();
    if (!width || !height) return;
    const scale = Math.max(
      1,
      Math.min(frame.clientWidth / width, Math.max(window.innerHeight * 0.62, 320) / height, 2.5)
    );
    canvas.width = width;
    canvas.height = height;
    canvas.style.width = `${Math.floor(width * scale)}px`;
    canvas.style.height = `${Math.floor(height * scale)}px`;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.putImageData(new ImageData(new Uint8ClampedArray(pixels.slice(0, width * height * 4)), width, height), 0, 0);

    const state = g.getState() as State;
    const count = (n: string) => state.objects.filter((o) => o.name === n).length;
    const gnome = state.objects.find((o) => o.name === "gnome");
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
      setSteps(0);
      setStatus("playing");
      void draw();
    },
    [draw]
  );

  // (Re)create the game whenever the camera changes: the global observer is the whole-map camera,
  // and the player observer is the follow camera. Both are named observers in the GDY.
  const levelRef = useRef(level);
  levelRef.current = level;
  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const g = await getGriddly();
        if (cancelled) return;
        g.unload();
        griddlyRef.current = g;
        g.loadGDYString(SPIDERS_GDY);
        if (cam.follows) g.createGame("Vector", cam.name);
        else g.createGame(cam.name, "Vector");
        start(levelRef.current);
      } catch (e) {
        setStatus("error");
        setError(e instanceof Error ? e.message : String(e));
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [cam, start]);

  useEffect(
    () => () => {
      griddlyRef.current?.unload();
      griddlyRef.current = null;
    },
    []
  );

  useEffect(() => {
    const onResize = () => void draw();
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, [draw]);

  const doStep = useCallback(
    (action: number) => {
      const g = griddlyRef.current;
      if (!g || status !== "playing") return;
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
      void draw();
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
      if (e.key === "c" || e.key === "C") {
        e.preventDefault();
        setCamera((c) => (c + 1) % CAMERAS.length);
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
              <Action onClick={() => start(level)} className="mt-2 w-full">
                ↻ Restart
              </Action>
              <p className="mt-1 text-center font-mono text-[10px] leading-relaxed text-stone-500">
                ARROWS / WASD move · R restart · C camera
              </p>
            </div>

            <div className="rounded-2xl border border-white/10 bg-white/[0.03] p-4">
              <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Camera · GDY observer</p>
              <div className="mt-2 flex flex-col gap-1.5">
                {CAMERAS.map((c, i) => (
                  <button
                    key={c.name}
                    onClick={() => setCamera(i)}
                    className={`flex items-center justify-between rounded-lg border px-3 py-1.5 text-left text-sm transition ${
                      i === camera
                        ? "border-emerald-400/50 bg-emerald-400/10 text-emerald-200"
                        : "border-white/5 bg-black/20 text-stone-300 hover:border-white/15"
                    }`}
                  >
                    <span>{c.label}</span>
                    <code className="text-[10px] text-stone-500">{c.name}</code>
                  </button>
                ))}
              </div>
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

export const Route = createFileRoute("/demo")({
  component: DemoPage,
});
