import { Link, createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { Griddly } from "@griddly/wasm";

import { Action, DifficultyBadge, Kbd, Overlay, Pad, Pill, Stat, TONE_TEXT, accentVars } from "../components/arcade";
import { gameGdyInfo, getGame } from "../games/catalog";
import type { Game, HudItem } from "../games/catalog";
import { loadGdy, withGriddly } from "../lib/griddly";
import { cachedThumb, renderThumbs } from "../lib/thumbs";
import type { ThumbTiming } from "../lib/thumbs";

/**
 * Player page for any catalog game. Everything on the board is rendered by the engine: each camera
 * is a render observer named in the GDY (whole map = global observer, follow cams = the player's
 * avatar-tracking observer); this page only blits the observer's RGBA pixels onto a canvas.
 * Win / lose comes from the GDY's own termination conditions via step()'s result.
 */

// MOVE_INPUTS mapping shared by every game: 1 = left, 2 = up, 3 = right, 4 = down.
const LEFT = 1;
const UP = 2;
const RIGHT = 3;
const DOWN = 4;
const MOVE_KEYS: Record<string, number> = {
  ArrowLeft: LEFT,
  a: LEFT,
  A: LEFT,
  ArrowUp: UP,
  w: UP,
  W: UP,
  ArrowRight: RIGHT,
  d: RIGHT,
  D: RIGHT,
  ArrowDown: DOWN,
  s: DOWN,
  S: DOWN,
};

type Obj = { name: string; variables?: Record<string, number> };
type State = { objects: Obj[] };
type StepResult = { terminated?: boolean; info?: { playerResults?: Record<string, string> }; reward?: number[] };
type Status = "loading" | "playing" | "won" | "lost" | "error";

/** Load timings of the current player page, exposed on `window.__griddlyPerf` for e2e reports. */
type Perf = { game: string; firstFrameMs?: number; thumbsTotalMs?: number; thumbs?: ThumbTiming[]; levelMs?: Record<number, number> };
let perf: Perf = { game: "" };

const keyLabel = (k: string) => (k === " " ? "Space" : k.length === 1 ? k.toUpperCase() : k);

const PlayPage = () => {
  const { gameId } = Route.useParams();
  const game = getGame(gameId);
  if (!game) return <UnknownGame id={gameId} />;
  // Keyed so switching games starts from a clean slate (level 0, default camera, fresh engine game).
  return <Player key={game.id} game={game} />;
};

const Player = ({ game }: { game: Game }) => {
  const { cameras, avatar } = useMemo(() => gameGdyInfo(game), [game]);
  useEffect(() => {
    perf = { game: game.id, levelMs: {} };
    (window as unknown as { __griddlyPerf: () => Perf }).__griddlyPerf = () => perf;
  }, [game]);
  const levels = game.levels;
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef<HTMLDivElement>(null);
  const griddlyRef = useRef<Griddly | null>(null);
  const actionsRef = useRef<string[]>([]);
  const initialCountsRef = useRef<Record<string, number>>({});
  const lastDirRef = useRef(DOWN);
  const [status, setStatus] = useState<Status>("loading");
  const [error, setError] = useState("");
  const [level, setLevel] = useState(0);
  const [steps, setSteps] = useState(0);
  const [reward, setReward] = useState(0);
  const [hud, setHud] = useState<Record<string, number>>({});
  const [score, setScore] = useState({ wins: 0, losses: 0 });
  const [solved, setSolved] = useState<Set<number>>(() => new Set());
  const [camera, setCamera] = useState(0);
  const [thumbs, setThumbs] = useState<string[]>(() => levels.map((_, i) => cachedThumb(game.id, i) ?? ""));
  const cam = cameras[camera] ?? cameras[0]!;
  const camRef = useRef(cam);
  camRef.current = cam;

  const hudKey = (h: HudItem, i: number) => `${i}:${h.label}`;

  const readHud = useCallback(
    (g: Griddly) => {
      const state = g.getState() as State;
      const count = (n?: string) => (n ? state.objects.filter((o) => o.name === n).length : 0);
      const globals = g.getGlobalVariables() as Record<string, Record<string, number> | number>;
      const next: Record<string, number> = {};
      game.hud.forEach((h, i) => {
        let v = 0;
        if (h.kind === "collected") v = (initialCountsRef.current[h.object ?? ""] ?? 0) - count(h.object);
        else if (h.kind === "remaining" || h.kind === "count") v = count(h.object);
        else if (h.kind === "var") {
          const o = state.objects.find((x) => x.name === (h.object || avatar));
          v = (h.variable && o?.variables?.[h.variable]) || 0;
        } else if (h.kind === "global") {
          const gv = globals[h.variable ?? h.object ?? ""];
          v = typeof gv === "number" ? gv : Number(gv?.["1"] ?? gv?.["0"] ?? Object.values(gv ?? {})[0] ?? 0);
        }
        next[hudKey(h, i)] = v;
      });
      setHud(next);
    },
    [game, avatar]
  );

  const draw = useCallback(async () => {
    const g = griddlyRef.current;
    const canvas = canvasRef.current;
    const frame = frameRef.current;
    if (!g || !canvas || !frame) return;
    // The engine renders the chosen camera observer; we only scale and blit its pixels.
    const { pixels, width, height } = camRef.current.follows ? await g.observePlayerImage(0) : await g.observeImage();
    if (!width || !height || griddlyRef.current !== g) return;
    const maxH = Math.max(window.innerHeight * (window.innerWidth < 1024 ? 0.55 : 0.68), 280);
    const scale = Math.min(frame.clientWidth / width, maxH / height, 3);
    canvas.width = width;
    canvas.height = height;
    canvas.style.width = `${Math.floor(width * scale)}px`;
    canvas.style.height = `${Math.floor(height * scale)}px`;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.putImageData(new ImageData(new Uint8ClampedArray(pixels.slice(0, width * height * 4)), width, height), 0, 0);
    readHud(g);
  }, [readHud]);

  /** Create the engine game for the current camera (GDY must be loaded). */
  const createGame = useCallback((g: Griddly) => {
    const c = camRef.current;
    if (c.follows) g.createGame("Vector", c.name);
    else g.createGame(c.name, "Vector");
    actionsRef.current = g.getActionNames();
  }, []);

  const start = useCallback(
    (lvl: number) => {
      const g = griddlyRef.current;
      if (!g) return;
      const t0 = performance.now();
      g.loadLevel(lvl);
      g.seed(Math.floor(Math.random() * 1e9));
      g.reset();
      const counts: Record<string, number> = {};
      for (const o of (g.getState() as State).objects) counts[o.name] = (counts[o.name] ?? 0) + 1;
      initialCountsRef.current = counts;
      lastDirRef.current = DOWN;
      setSteps(0);
      setReward(0);
      setStatus("playing");
      void draw().then(() => {
        if (perf.levelMs) perf.levelMs[lvl] = Math.round(performance.now() - t0);
      });
    },
    [draw]
  );

  // Release the engine game when leaving the page.
  useEffect(
    () => () => {
      griddlyRef.current?.unload();
      griddlyRef.current = null;
    },
    []
  );

  // (Re)create the game once per game + camera: the global observer is the whole-map camera, the
  // player observer the follow camera (both named observers in the GDY). Levels switch with
  // loadLevel + reset on the same game.
  const levelRef = useRef(level);
  levelRef.current = level;
  useEffect(() => {
    let cancelled = false;
    const t0 = performance.now();
    setStatus("loading");
    withGriddly((g) => {
      if (cancelled) return;
      griddlyRef.current = null;
      loadGdy(g, game.gdy);
      createGame(g);
      griddlyRef.current = g;
      start(levelRef.current);
      perf.firstFrameMs = Math.round(performance.now() - t0);
    }).catch((e: unknown) => {
      if (cancelled) return;
      setStatus("error");
      setError(e instanceof Error ? e.message : String(e));
    });
    return () => {
      cancelled = true;
    };
  }, [cam, game, start, createGame]);

  // Level-card thumbnails: rendered by the engine after the board is interactive, one level per
  // engine job so moves are never blocked behind them; cached per game.
  useEffect(() => {
    let cancelled = false;
    // After the first frame is on screen (requestIdleCallback is missing in Safari).
    const idle = (cb: () => void) =>
      typeof window.requestIdleCallback === "function" ? window.requestIdleCallback(cb, { timeout: 500 }) : setTimeout(cb, 50);
    idle(() => {
      if (cancelled) return;
      const t0 = performance.now();
      void renderThumbs(
        game,
        levels.map((_, i) => i),
        {
          maxWidth: 240,
          isCancelled: () => cancelled,
          onThumb: (l, url) => setThumbs((ts) => ts.map((x, i) => (i === l ? url : x))),
        }
      )
        .then((timings) => {
          if (!cancelled) {
            perf.thumbs = timings;
            perf.thumbsTotalMs = Math.round(performance.now() - t0);
          }
        })
        .catch(() => undefined);
    });
    return () => {
      cancelled = true;
    };
  }, [game, levels]);

  useEffect(() => {
    const onResize = () => void draw();
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, [draw]);

  /** Step with a named action; with several actions the engine wants [actionTypeIdx, actionId]. */
  const act = useCallback(
    (name: string, id: number) => {
      const g = griddlyRef.current;
      if (!g || status !== "playing") return;
      const names = actionsRef.current;
      const idx = names.indexOf(name);
      if (idx < 0) return;
      const r = g.step(names.length > 1 ? [idx, id] : [id]) as unknown as StepResult;
      setSteps((s) => s + 1);
      setReward((x) => x + (r?.reward?.[0] ?? 0));
      if (r?.terminated) {
        const res = r.info?.playerResults?.["1"];
        if (res === "Win") {
          setStatus("won");
          setScore((s) => ({ ...s, wins: s.wins + 1 }));
          setSolved((s) => new Set(s).add(levelRef.current));
        } else {
          setStatus("lost");
          setScore((s) => ({ ...s, losses: s.losses + 1 }));
        }
      }
      void draw();
    },
    [status, draw]
  );

  const move = useCallback(
    (dir: number) => {
      lastDirRef.current = dir;
      act(game.controls.move, dir);
    },
    [act, game]
  );

  const extra = useCallback(
    (i: number) => {
      const e = game.controls.extra[i];
      if (e) act(e.action, e.directional ? lastDirRef.current : 1);
    },
    [act, game]
  );

  const pick = useCallback(
    (l: number) => {
      setLevel(l);
      start(l);
    },
    [start]
  );
  const nextLevel = useCallback(() => pick((level + 1) % levels.length), [level, levels.length, pick]);
  const cycleCamera = useCallback(() => setCamera((c) => (c + 1) % cameras.length), [cameras.length]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.metaKey || e.ctrlKey || e.altKey) return;
      if (e.key === "r" || e.key === "R") {
        e.preventDefault();
        start(level);
        return;
      }
      if (e.key === "c" || e.key === "C") {
        e.preventDefault();
        cycleCamera();
        return;
      }
      if (e.key === "n" || e.key === "N" || e.key === "Enter") {
        if (status === "won") {
          e.preventDefault();
          nextLevel();
        } else if (status === "lost" && e.key === "Enter") {
          e.preventDefault();
          start(level);
        }
        return;
      }
      const ei = game.controls.extra.findIndex((x) => x.key.toLowerCase() === e.key.toLowerCase());
      if (ei >= 0) {
        e.preventDefault();
        extra(ei);
        return;
      }
      const d = MOVE_KEYS[e.key];
      if (d !== undefined) {
        e.preventDefault();
        move(d);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [move, extra, start, nextLevel, cycleCamera, level, status, game]);

  // Swipe to move on touch screens.
  const touch = useRef<{ x: number; y: number } | null>(null);
  const onTouchStart = (e: React.TouchEvent) => {
    const t = e.touches[0];
    if (t) touch.current = { x: t.clientX, y: t.clientY };
  };
  const onTouchEnd = (e: React.TouchEvent) => {
    const s = touch.current;
    const t = e.changedTouches[0];
    touch.current = null;
    if (!s || !t) return;
    const dx = t.clientX - s.x;
    const dy = t.clientY - s.y;
    if (Math.max(Math.abs(dx), Math.abs(dy)) < 24) return;
    move(Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? RIGHT : LEFT) : dy > 0 ? DOWN : UP);
  };

  const meta = levels[level];
  const hudValue = (h: HudItem, i: number) => {
    const v = hud[hudKey(h, i)] ?? 0;
    if (h.kind === "collected") return `${v}/${initialCountsRef.current[h.object ?? ""] ?? 0}`;
    return v;
  };

  return (
    <div
      data-status={status}
      style={accentVars(game.accent)}
      className="min-h-full bg-[radial-gradient(ellipse_at_top,_#1b2a1f_0%,_#0b0f0c_55%,_#050605_100%)] text-stone-200"
    >
      <main className="mx-auto flex max-w-6xl flex-col gap-4 px-3 py-4 sm:gap-5 sm:px-8 sm:py-6">
        <header className="flex flex-wrap items-end justify-between gap-3">
          <div className="min-w-0">
            <Link
              to="/"
              className="inline-flex items-center gap-1 font-mono text-[11px] uppercase tracking-[0.3em] text-emerald-400/70 transition hover:text-emerald-300"
              data-testid="back"
            >
              ← Library
            </Link>
            <h1 className="mt-1 flex items-center gap-3 text-3xl font-black tracking-tight sm:text-4xl">
              <span className="text-3xl sm:text-4xl" aria-hidden>
                {game.icon}
              </span>
              <span className="bg-gradient-to-r from-stone-100 via-[var(--accent)] to-amber-200 bg-clip-text text-transparent">
                {game.name}
              </span>
            </h1>
          </div>
          <div className="flex items-center gap-2 font-mono text-xs">
            <DifficultyBadge difficulty={game.difficulty} />
            <Pill tone="emerald">🏆 {score.wins}</Pill>
            <Pill tone="rose">💀 {score.losses}</Pill>
          </div>
        </header>

        {/* level picker */}
        <nav className="-mx-1 flex snap-x gap-2 overflow-x-auto px-1 pb-1" aria-label="Levels">
          {levels.map((l, i) => (
            <button
              key={i}
              onClick={() => pick(i)}
              data-level={i}
              aria-current={i === level}
              className={`group relative w-36 shrink-0 snap-start overflow-hidden rounded-xl border text-left transition sm:w-40 ${
                i === level
                  ? "border-[var(--accent-50)] bg-[var(--accent-15)] shadow-[0_0_24px_-6px_var(--accent-50)]"
                  : "border-white/5 bg-white/[0.03] hover:border-white/15 hover:bg-white/[0.06]"
              }`}
            >
              <div className="relative h-16 overflow-hidden bg-black/40">
                {thumbs[i] ? (
                  <img
                    src={thumbs[i]}
                    alt=""
                    className={`h-full w-full object-cover transition ${i === level ? "opacity-100" : "opacity-60 group-hover:opacity-90"}`}
                  />
                ) : (
                  <div className="h-full w-full animate-pulse bg-white/[0.04]" />
                )}
                {solved.has(i) && (
                  <span className="absolute top-1 right-1 rounded-full bg-emerald-500/90 px-1.5 text-[10px] font-bold text-emerald-950">
                    ✓
                  </span>
                )}
              </div>
              <div className="px-3 py-2">
                <span className="flex justify-between font-mono text-[10px] text-stone-500 group-hover:text-stone-400">
                  <span>LEVEL {String(i + 1).padStart(2, "0")}</span>
                  <span>
                    {l.width}×{l.height}
                  </span>
                </span>
                <span className={`block truncate text-sm font-semibold ${i === level ? "text-[var(--accent)]" : "text-stone-300"}`}>
                  {l.name}
                </span>
              </div>
            </button>
          ))}
        </nav>

        <section className="grid min-w-0 gap-4 sm:gap-5 lg:grid-cols-[minmax(0,1fr)_280px]">
          {/* board */}
          <div className="relative min-w-0 rounded-2xl border border-white/10 bg-black/40 p-2 shadow-2xl ring-1 shadow-black/60 ring-[var(--accent-15)] backdrop-blur sm:p-3">
            <div
              ref={frameRef}
              onTouchStart={onTouchStart}
              onTouchEnd={onTouchEnd}
              className="flex min-h-[240px] w-full min-w-0 touch-none items-center justify-center overflow-hidden rounded-xl"
            >
              <canvas
                ref={canvasRef}
                data-testid="board"
                className="max-w-full rounded-lg"
                style={{ imageRendering: "pixelated" }}
              />
            </div>
            <div className="pointer-events-none absolute top-4 left-4 flex gap-1.5 font-mono text-[10px]">
              <span className="rounded-md bg-black/60 px-2 py-0.5 text-stone-300 backdrop-blur">
                {meta?.name} · {steps} steps
              </span>
              <span className="hidden rounded-md bg-black/60 px-2 py-0.5 text-stone-400 backdrop-blur sm:inline">
                📷 {cam.label}
              </span>
            </div>

            {status === "loading" && <Overlay>Loading {game.name}…</Overlay>}
            {status === "error" && (
              <Overlay tone="rose" testId="error-overlay">
                <p className="font-bold text-rose-200">Couldn't start the game</p>
                <p className="mt-1 max-w-md font-mono text-xs text-rose-200/80">{error}</p>
              </Overlay>
            )}
            {status === "won" && (
              <Overlay testId="win-overlay">
                <p className="text-5xl">{game.icon}</p>
                <p className="mt-2 text-2xl font-bold text-emerald-200">{game.win_text}</p>
                <p className="mt-1 font-mono text-xs text-stone-400">
                  {meta?.name} · {steps} steps
                </p>
                <div className="mt-4 flex gap-2">
                  <Action onClick={() => start(level)}>Replay</Action>
                  <Action primary onClick={nextLevel}>
                    {level + 1 < levels.length ? "Next level →" : "Back to level 1 ↺"}
                  </Action>
                </div>
                <p className="mt-3 font-mono text-[10px] text-stone-500">
                  <Kbd>N</Kbd> / <Kbd>Enter</Kbd> next level
                </p>
              </Overlay>
            )}
            {status === "lost" && (
              <Overlay tone="rose" testId="lose-overlay">
                <p className="text-5xl">💀</p>
                <p className="mt-2 text-2xl font-bold text-rose-200">{game.lose_text}</p>
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
              <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">
                Level {level + 1} / {levels.length}
              </p>
              <p className="text-lg font-bold text-stone-100">{meta?.name}</p>
              {meta?.blurb && <p className="mt-1 text-sm leading-snug text-stone-400">{meta.blurb}</p>}
              <div className="mt-4 grid grid-cols-2 gap-2 font-mono text-xs" data-testid="hud">
                {game.hud.map((h, i) => (
                  <Stat key={hudKey(h, i)} label={h.label} value={hudValue(h, i)} tone={TONE_TEXT[h.tone ?? ""] ?? "text-stone-200"} />
                ))}
                <Stat label="Steps" value={steps} tone="text-sky-300" />
                {reward !== 0 && <Stat label="Reward" value={reward} tone="text-lime-300" />}
              </div>
            </div>

            {/* controls first on small screens so the d-pad sits right under the board */}
            <div className="order-first flex flex-col items-center gap-2 rounded-2xl border border-white/10 bg-white/[0.03] p-4 lg:order-none">
              <Pad label="Up" onClick={() => move(UP)}>
                ▲
              </Pad>
              <div className="flex gap-2">
                <Pad label="Left" onClick={() => move(LEFT)}>
                  ◀
                </Pad>
                <Pad label="Down" onClick={() => move(DOWN)}>
                  ▼
                </Pad>
                <Pad label="Right" onClick={() => move(RIGHT)}>
                  ▶
                </Pad>
              </div>
              {game.controls.extra.length > 0 && (
                <div className="mt-1 flex w-full flex-wrap justify-center gap-2">
                  {game.controls.extra.map((x, i) => (
                    <button
                      key={x.action}
                      onClick={() => extra(i)}
                      data-action={x.action}
                      className="flex flex-1 items-center justify-center gap-2 rounded-lg border border-[var(--accent-50)] bg-[var(--accent-15)] px-3 py-2 text-sm font-semibold text-[var(--accent)] transition hover:brightness-125 active:translate-y-px"
                    >
                      {x.label}
                      <Kbd>{keyLabel(x.key)}</Kbd>
                    </button>
                  ))}
                </div>
              )}
              <div className="mt-2 flex w-full gap-2">
                <Action onClick={() => start(level)} className="flex-1">
                  ↻ Restart
                </Action>
                <Action onClick={nextLevel} className="flex-1" title="Skip to the next level">
                  Skip ⏭
                </Action>
              </div>
              <p className="mt-1 text-center font-mono text-[10px] leading-relaxed text-stone-500">
                ARROWS / WASD move{game.controls.extra.map((x) => ` · ${keyLabel(x.key)} ${x.label.toLowerCase()}`).join("")}
                <br />R restart · C camera · swipe on mobile
              </p>
            </div>

            <div className="rounded-2xl border border-white/10 bg-white/[0.03] p-4">
              <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Camera · GDY observer</p>
              <div className="mt-2 flex flex-col gap-1.5" data-testid="cameras">
                {cameras.map((c, i) => (
                  <button
                    key={c.name}
                    onClick={() => setCamera(i)}
                    data-camera={c.name}
                    aria-pressed={i === camera}
                    className={`flex items-center justify-between rounded-lg border px-3 py-1.5 text-left text-sm transition ${
                      i === camera
                        ? "border-[var(--accent-50)] bg-[var(--accent-15)] text-[var(--accent)]"
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
              <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">About</p>
              {game.tagline && <p className="mt-1 text-sm font-semibold text-stone-200">{game.tagline}</p>}
              <p className="mt-1">{game.description}</p>
              <div className="mt-3 flex flex-wrap gap-1">
                {game.tags.map((t) => (
                  <span key={t} className="rounded-full bg-white/[0.05] px-2 py-0.5 font-mono text-[10px] text-stone-400">
                    #{t}
                  </span>
                ))}
              </div>
              {game.original && (
                <p className="mt-3 font-mono text-[10px] text-stone-500">
                  Remaster of <code className="text-stone-400">{game.original}</code>
                </p>
              )}
              <p className="mt-1 font-mono text-[10px] text-stone-500">
                Runs in your browser via <code className="text-emerald-300">@griddly/wasm</code>.
              </p>
            </div>
          </aside>
        </section>
      </main>
    </div>
  );
};

const UnknownGame = ({ id }: { id: string }) => (
  <div className="flex min-h-full flex-col items-center justify-center gap-3 bg-[#050605] p-8 text-center text-stone-300">
    <p className="text-5xl">🧩</p>
    <p className="text-xl font-bold">No game called “{id}”</p>
    <Link to="/" className="rounded-lg border border-white/10 px-4 py-2 text-sm hover:border-white/20">
      ← Back to the library
    </Link>
  </div>
);

export const Route = createFileRoute("/play/$gameId")({
  head: ({ params }) => ({ meta: [{ title: `${getGame(params.gameId)?.name ?? "Game"} · Griddly` }] }),
  component: PlayPage,
});
