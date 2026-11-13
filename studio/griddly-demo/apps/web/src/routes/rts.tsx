import { Link, createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { Griddly } from "@griddly/wasm";
import { parse } from "yaml";

import { Action, Kbd, Overlay } from "../components/arcade";
import { loadGdy, withGriddly } from "../lib/griddly";
import { STRATEGIES } from "../rts/ai";
import { RTS_ACTIONS, RTS_GDY, RTS_LEVELS, chordToActionId, commandKey, keyToActionId } from "../rts/gdy";
import { Match } from "../rts/match";
import type { Outcome } from "../rts/match";
import { describeOrder, formation } from "../rts/orders";
import type { Order } from "../rts/orders";
import { cellAt, drawBoard, drawMinimap, seen, updateFog, viewport } from "../rts/view";
import type { Fog, Marks, Viewport } from "../rts/view";
import { ARMY, COSTS, DISPLAY, TECHS, UNITS, manhattan } from "../rts/world";
import type { Cell, Dir, Obj, WorldView } from "../rts/world";

/**
 * Griddly RTS: Frontiers, playable. Player 1 is you (mouse + keyboard), player 2 the scripted AI
 * (src/rts/ai.ts, a port of resources/games/RTS/fow_policy.py). Every rule lives in the GDY; the
 * page only turns clicks into the GDY's external actions (one per unit per tick), reads the engine's
 * available actions for whatever you select, and draws the engine's renders: player 1's Sprite2D
 * observer (cropped around the GDY camera, cam_x / cam_y) and the global Block2D mini-map.
 */

type Setup = { level: number; enemy: string; spectate: boolean; fog: boolean; speed: number; key: number };
type LogLine = { tick: number; text: string; tone: "good" | "bad" | "info" };

const TRAIN_ACTIONS: Record<string, string> = {
  build_worker: "worker",
  build_scout: "scout",
  build_combat: "combat",
  build_marine: "marine",
  build_medic: "medic",
  build_tank: "tank",
};
const SITE_ACTIONS = ["build_barracks", "build_lab", "build_tower", "build_wall"];
const TARGET_ACTIONS = ["move", "attack", "shoot", "gather", "heal", "repair", ...SITE_ACTIONS];
const ICONS: Record<string, string> = {
  move: "➜",
  attack: "⚔",
  shoot: "🎯",
  gather: "⛏",
  heal: "✚",
  repair: "🔧",
  build_barracks: "🏛",
  build_lab: "⚗",
  build_tower: "🗼",
  build_wall: "▦",
  build_worker: "👷",
  build_scout: "🛵",
  build_combat: "🗡",
  build_marine: "🔫",
  build_medic: "⛑",
  build_tank: "🛡",
  research: "🔬",
};
const ZOOMS = [16, 20, 24, 32, 40, 48, 64];
const SPEEDS = [1, 2, 4, 6, 8, 12];
const ARROWS: Record<number, string> = { 1: "←", 2: "↑", 3: "→", 4: "↓" };

const label = (action: string) => {
  if (TRAIN_ACTIONS[action]) return `Train ${DISPLAY[TRAIN_ACTIONS[action]]}`;
  if (action.startsWith("build_")) return `Build ${action.slice(6)}`;
  return action[0]!.toUpperCase() + action.slice(1);
};
const costOf = (action: string) => COSTS[TRAIN_ACTIONS[action] ?? action.replace("build_", "")];

// ------------------------------------------------------------------------------------------ menu

const PREVIEW_COLORS: Record<string, string> = {
  ".": "#20261d",
  "?": "#20261d",
  M: "#34d399",
  G: "#c084fc",
  T: "#14532d",
  W: "#6b6560",
  "~": "#1e40af",
  "=": "#92400e",
  R: "#e5e5e5",
};

function levelPreview(index: number, scale = 3): string {
  if (typeof document === "undefined") return "";
  const doc = parse(RTS_GDY) as { Environment?: { Levels?: string[] } };
  const text = doc.Environment?.Levels?.[index];
  if (!text) return "";
  const rows = text.trim().split("\n").map((r) => r.trim().split(/\s+/));
  const h = rows.length;
  const w = rows.reduce((m, r) => Math.max(m, r.length), 0);
  const c = document.createElement("canvas");
  c.width = w * scale;
  c.height = h * scale;
  const ctx = c.getContext("2d")!;
  rows.forEach((row, y) =>
    row.forEach((tok, x) => {
      const owner = tok.length > 1 ? tok[1] : "";
      ctx.fillStyle = owner === "1" ? "#4ade80" : owner === "2" ? "#f87171" : (PREVIEW_COLORS[tok[0] ?? "."] ?? "#20261d");
      const s = owner ? scale + 2 : scale;
      ctx.fillRect(x * scale - (s - scale) / 2, y * scale - (s - scale) / 2, s, s);
    })
  );
  return c.toDataURL();
}

const Menu = ({ initial, onStart }: { initial: Setup; onStart: (s: Setup) => void }) => {
  const [s, setS] = useState(initial);
  const [previews, setPreviews] = useState<string[]>([]);
  useEffect(() => setPreviews(RTS_LEVELS.map((_, i) => levelPreview(i))), []);
  return (
    <div className="mx-auto flex max-w-6xl flex-col gap-6 px-4 py-8 sm:px-8" data-testid="rts-menu">
      <header>
        <Link to="/" className="font-mono text-[11px] uppercase tracking-[0.3em] text-emerald-400/70 hover:text-emerald-300">
          ← Library
        </Link>
        <h1 className="mt-1 bg-gradient-to-r from-emerald-200 via-lime-200 to-amber-200 bg-clip-text text-4xl font-black tracking-tight text-transparent sm:text-5xl">
          Griddly RTS: Frontiers
        </h1>
        <p className="mt-2 max-w-3xl text-sm leading-relaxed text-stone-400">
          Mine minerals, gems and wood, capture relays for income, research a tech tree and build an army to destroy the enemy
          base. Every rule is GDY running in <code className="text-emerald-300">@griddly/wasm</code>; you command units with the
          mouse, a scripted AI commands the other side, and both players act in the same tick.
        </p>
      </header>

      <section>
        <p className="mb-2 font-mono text-[10px] uppercase tracking-widest text-stone-500">Map</p>
        <div className="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-4">
          {RTS_LEVELS.map((l, i) => (
            <button
              key={l.name}
              data-level={i}
              onClick={() => setS({ ...s, level: i })}
              aria-pressed={s.level === i}
              className={`overflow-hidden rounded-xl border text-left transition ${
                s.level === i ? "border-emerald-400/60 bg-emerald-400/10 shadow-[0_0_24px_-8px_rgba(52,211,153,.8)]" : "border-white/10 bg-white/[0.03] hover:border-white/20"
              }`}
            >
              <div className="flex h-28 items-center justify-center bg-black/50 p-2">
                {previews[i] ? <img src={previews[i]} alt="" className="max-h-full max-w-full [image-rendering:pixelated]" /> : null}
              </div>
              <div className="px-3 py-2">
                <p className="flex justify-between text-sm font-bold text-stone-100">
                  {l.name}
                  <span className="font-mono text-[10px] font-normal text-stone-500">
                    {l.width}×{l.height}
                  </span>
                </p>
                <p className="mt-0.5 text-xs leading-snug text-stone-400">{l.blurb}</p>
              </div>
            </button>
          ))}
        </div>
      </section>

      <section className="grid gap-4 sm:grid-cols-3">
        <div className="rounded-xl border border-white/10 bg-white/[0.03] p-4">
          <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Enemy AI</p>
          <div className="mt-2 flex flex-col gap-1.5">
            {Object.values(STRATEGIES).map((st) => (
              <button
                key={st.name}
                data-strategy={st.name}
                onClick={() => setS({ ...s, enemy: st.name })}
                aria-pressed={s.enemy === st.name}
                className={`rounded-lg border px-3 py-1.5 text-left text-sm ${
                  s.enemy === st.name ? "border-rose-400/50 bg-rose-400/10 text-rose-200" : "border-white/5 bg-black/20 text-stone-300 hover:border-white/15"
                }`}
              >
                <span className="font-semibold">{st.name}</span>
                <span className="ml-2 text-xs text-stone-500">{st.army.map(([u]) => DISPLAY[u]).join(" · ")}</span>
              </button>
            ))}
          </div>
        </div>
        <div className="rounded-xl border border-white/10 bg-white/[0.03] p-4">
          <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Mode</p>
          <div className="mt-2 flex flex-col gap-1.5 text-sm">
            <label className="flex items-center gap-2">
              <input type="radio" checked={!s.spectate} onChange={() => setS({ ...s, spectate: false, fog: true })} /> Play vs AI
            </label>
            <label className="flex items-center gap-2">
              <input type="radio" checked={s.spectate} onChange={() => setS({ ...s, spectate: true, fog: false })} data-testid="spectate" /> Watch AI vs AI
            </label>
            <label className="mt-2 flex items-center gap-2">
              <input type="checkbox" checked={s.fog} onChange={(e) => setS({ ...s, fog: e.target.checked })} /> Fog of war
            </label>
          </div>
        </div>
        <div className="flex flex-col justify-between gap-3 rounded-xl border border-white/10 bg-white/[0.03] p-4">
          <p className="text-xs leading-relaxed text-stone-400">
            <b className="text-stone-200">Left-click</b> select · <b className="text-stone-200">drag</b> box-select ·{" "}
            <b className="text-stone-200">right-click</b> move / attack / harvest / capture · <Kbd>Shift</Kbd> queue orders ·{" "}
            <Kbd>←↑→↓</Kbd> camera · <Kbd>Space</Kbd> pause · hotkeys come from the GDY KeyBindings.
          </p>
          <Action primary onClick={() => onStart({ ...s, key: s.key + 1 })}>
            ▶ Start {RTS_LEVELS[s.level]?.name}
          </Action>
        </div>
      </section>
    </div>
  );
};

// ------------------------------------------------------------------------------------------ game

type Images = { board: HTMLCanvasElement | null; mini: HTMLCanvasElement | null };

function blit(target: HTMLCanvasElement | null, img: { pixels: Uint8Array; width: number; height: number }): HTMLCanvasElement {
  const c = target ?? document.createElement("canvas");
  if (c.width !== img.width || c.height !== img.height) {
    c.width = img.width;
    c.height = img.height;
  }
  c.getContext("2d")!.putImageData(new ImageData(new Uint8ClampedArray(img.pixels.slice(0, img.width * img.height * 4)), img.width, img.height), 0, 0);
  return c;
}

const GameScreen = ({ setup, onExit, onRestart }: { setup: Setup; onExit: () => void; onRestart: () => void }) => {
  const matchRef = useRef<Match | null>(null);
  const engineRef = useRef<Griddly | null>(null);
  const imagesRef = useRef<Images>({ board: null, mini: null });
  const boardRef = useRef<HTMLCanvasElement>(null);
  const miniRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const busyRef = useRef(false);
  const fogRef = useRef<Fog | null>(setup.fog ? { visible: new Set(), explored: new Set() } : null);
  const marksRef = useRef<Marks>({ selected: new Set(), hover: null, targeting: null, box: null, orders: new Map(), pings: [], fog: fogRef.current });
  const dragRef = useRef<{ x: number; y: number } | null>(null);
  const vpRef = useRef<Viewport | null>(null);
  const prevRef = useRef<{ ids: Map<number, string>; enemy: Map<number, string>; tech: Record<string, number>; baseHp: number; alarm: number } | null>(null);

  const [status, setStatus] = useState<"loading" | "error" | "ready">("loading");
  const [error, setError] = useState("");
  const [outcome, setOutcome] = useState<Outcome>("playing");
  const [paused, setPaused] = useState(false);
  const [speed, setSpeed] = useState(setup.speed);
  const [zoom, setZoom] = useState(32);
  const [fogOn, setFogOn] = useState(setup.fog);
  const [autopilot, setAutopilot] = useState(setup.spectate);
  const [selected, setSelected] = useState<number[]>([]);
  const [inspect, setInspect] = useState<number | null>(null);
  const [targeting, setTargeting] = useState<string | null>(null);
  const [chord, setChord] = useState<{ action: string; prefix: string } | null>(null);
  const [log, setLog] = useState<LogLine[]>([]);
  const [, setVersion] = useState(0);
  const bump = useCallback(() => setVersion((v) => v + 1), []);

  const level = RTS_LEVELS[setup.level];

  // ------------------------------------------------------------------ engine setup / teardown
  const render = useCallback(async () => {
    const g = engineRef.current;
    const m = matchRef.current;
    if (!g || !m) return;
    const [board, mini] = await Promise.all([g.observePlayerImage(0), g.observeImage()]);
    if (engineRef.current !== g) return;
    imagesRef.current = { board: blit(imagesRef.current.board, board), mini: blit(imagesRef.current.mini, mini) };
    if (fogRef.current) updateFog(m.world, 1, fogRef.current);
  }, []);

  useEffect(() => {
    let cancelled = false;
    withGriddly(async (g) => {
      if (cancelled) return;
      loadGdy(g, RTS_GDY);
      // global observer = Block2D mini-map; each player's observer = the Sprite2D board (egocentric shadows)
      g.createGame("Block2D", "Sprite2D");
      const m = new Match(g, {
        level: setup.level,
        seed: setup.key,
        enemy: STRATEGIES[setup.enemy],
        autopilot: setup.spectate ? STRATEGIES.GUNLINE : null,
      });
      engineRef.current = g;
      matchRef.current = m;
      prevRef.current = snapshot(m.world, null);
      await render();
      if (cancelled) return;
      setStatus("ready");
      bump();
    }).catch((e: unknown) => {
      if (cancelled) return;
      setStatus("error");
      setError(e instanceof Error ? e.message : String(e));
    });
    return () => {
      cancelled = true;
      engineRef.current?.unload();
      engineRef.current = null;
      matchRef.current = null;
    };
  }, [setup, render, bump]);

  // ------------------------------------------------------------------ game loop
  const pushLog = useCallback((lines: LogLine[]) => {
    if (lines.length) setLog((l) => [...lines.reverse(), ...l].slice(0, 40));
  }, []);

  const tickOnce = useCallback(async () => {
    const m = matchRef.current;
    if (!m || busyRef.current || m.outcome !== "playing") return;
    busyRef.current = true;
    try {
      m.tick();
      const prev = prevRef.current;
      const next = snapshot(m.world, prev);
      pushLog(events(m.world, prev, next));
      prevRef.current = next;
      await render();
      setSelected((sel) => {
        const alive = sel.filter((id) => m.world.byId.has(id));
        return alive.length === sel.length ? sel : alive;
      });
      if (m.outcome !== "playing") setOutcome(m.outcome);
      bump();
    } finally {
      busyRef.current = false;
    }
  }, [render, bump, pushLog]);

  useEffect(() => {
    if (status !== "ready" || paused || outcome !== "playing") return;
    const id = setInterval(() => void tickOnce(), 1000 / speed);
    return () => clearInterval(id);
  }, [status, paused, outcome, speed, tickOnce]);

  useEffect(() => {
    matchRef.current?.setAutopilot(autopilot ? STRATEGIES.GUNLINE! : null);
  }, [autopilot, status]);

  useEffect(() => {
    fogRef.current = fogOn ? (fogRef.current ?? { visible: new Set(), explored: new Set() }) : null;
    if (fogRef.current && matchRef.current) updateFog(matchRef.current.world, 1, fogRef.current);
    marksRef.current.fog = fogRef.current;
  }, [fogOn]);

  // ------------------------------------------------------------------ drawing (every animation frame)
  useEffect(() => {
    let raf = 0;
    const draw = () => {
      raf = requestAnimationFrame(draw);
      const m = matchRef.current;
      const canvas = boardRef.current;
      const wrap = wrapRef.current;
      if (!m || !canvas || !wrap) return;
      const dpr = window.devicePixelRatio || 1;
      const w = wrap.clientWidth;
      const h = wrap.clientHeight;
      if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
        canvas.width = Math.round(w * dpr);
        canvas.height = Math.round(h * dpr);
        canvas.style.width = `${w}px`;
        canvas.style.height = `${h}px`;
      }
      const ctx = canvas.getContext("2d");
      if (!ctx) return;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      const v = viewport(m.world.camera(1), m.world.width, m.world.height, w, h, zoomRef.current);
      vpRef.current = v;
      const marks = marksRef.current;
      marks.orders = m.orders.queues;
      const now = performance.now();
      marks.pings = marks.pings.filter((p) => now - p.at < 700);
      drawBoard(ctx, imagesRef.current.board, m.world, v, marks, now);
      const mini = miniRef.current;
      const mctx = mini?.getContext("2d");
      if (mini && mctx) {
        const units = [...marks.selected].map((id) => m.world.byId.get(id)).filter((o): o is Obj => !!o);
        drawMinimap(mctx, imagesRef.current.mini, m.world, v, units, fogRef.current);
      }
    };
    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
  }, []);
  const zoomRef = useRef(zoom);
  zoomRef.current = zoom;
  useEffect(() => {
    marksRef.current.selected = new Set(selected);
  }, [selected]);
  useEffect(() => {
    marksRef.current.targeting = targeting;
  }, [targeting]);

  // ------------------------------------------------------------------ orders
  const m = matchRef.current;
  const world = m?.world ?? null;
  const selObjs = useMemo(
    () => (world ? selected.map((id) => world.byId.get(id)).filter((o): o is Obj => !!o) : []),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [selected, world]
  );
  const primary = selObjs[0] ?? null;
  const available = useMemo(() => (m && primary ? m.availableAt(primary.pos) : {}), [m, primary, world]);

  const ping = (cell: Cell, color: string) => marksRef.current.pings.push({ cell, color, at: performance.now() });

  /** Units of the selection that can do `action` (by the engine's available action names). */
  const capable = useCallback(
    (action: string) => (m ? selObjs.filter((o) => action in m.availableAt(o.pos)) : []),
    [m, selObjs]
  );

  const issueTargeted = useCallback(
    (action: string, cell: Cell, append: boolean) => {
      if (!m) return;
      const w = m.world;
      const target = w.get(cell);
      const units = capable(action);
      if (units.length === 0) return;
      if (SITE_ACTIONS.includes(action)) {
        const builder = units.reduce((a, b) => (manhattan(a.pos, cell) <= manhattan(b.pos, cell) ? a : b));
        m.orders.set(builder.id, { kind: "build", building: action.slice(6), site: cell }, append);
        ping(cell, "rgba(251,191,36,.9)");
        return;
      }
      const enemy = target && target.player === 2 ? target : undefined;
      const spots = formation(w, cell, units.length, new Set(units.map((u) => u.id)));
      units.forEach((u, i) => {
        let order: Order | null = null;
        if ((action === "attack" || action === "shoot") && enemy) order = { kind: "attack", target: enemy.id, last: enemy.pos };
        else if (action === "gather" && target) order = m.orders.smartOrder(w, u, cell);
        else if ((action === "heal" || action === "repair") && target && target.player === 1) order = { kind: "assist", action, target: target.id };
        else order = { kind: "move", to: spots[i] ?? cell };
        if (order) m.orders.set(u.id, order, append);
      });
      ping(cell, enemy ? "rgba(248,113,113,.9)" : "rgba(134,239,172,.9)");
    },
    [m, capable]
  );

  const issueSmart = useCallback(
    (cell: Cell, append: boolean) => {
      if (!m) return;
      const w = m.world;
      const movers = selObjs.filter((o) => UNITS.includes(o.name));
      const spots = formation(w, cell, movers.length, new Set(movers.map((u) => u.id)));
      let i = 0;
      for (const u of selObjs) {
        if (u.name === "base" || u.name === "barracks") {
          m.rallies.set(u.id, cell);
          continue;
        }
        if (!UNITS.includes(u.name)) continue;
        let order = m.orders.smartOrder(w, u, cell);
        if (order?.kind === "move") order = { kind: "move", to: spots[i] ?? cell };
        i++;
        if (order) m.orders.set(u.id, order, append);
      }
      const t = w.get(cell);
      ping(cell, t && t.player === 2 ? "rgba(248,113,113,.9)" : "rgba(134,239,172,.9)");
      bump();
    },
    [m, selObjs, bump]
  );

  const train = useCallback(
    (action: string, times = 1) => {
      if (!m) return;
      const unit = TRAIN_ACTIONS[action];
      if (!unit) return;
      const producers = capable(action);
      // spread across producers, shortest queue first
      for (let n = 0; n < times; n++) {
        const p = producers.reduce<Obj | null>((a, b) => (!a || (m.orders.queues.get(b.id)?.length ?? 0) < (m.orders.queues.get(a.id)?.length ?? 0) ? b : a), null);
        if (p) m.orders.set(p.id, { kind: "train", unit }, true);
      }
      bump();
    },
    [m, capable, bump]
  );

  const research = useCallback(
    (tech: number) => {
      if (!m) return;
      const lab = m.world.of("lab", 1).find((l) => !l.vars.busy && !m.orders.current(l.id));
      if (lab) m.orders.set(lab.id, { kind: "research", tech });
      bump();
    },
    [m, bump]
  );

  const direct = useCallback(
    (action: string, id: number) => {
      if (!m) return;
      for (const u of capable(action)) m.orders.set(u.id, { kind: "direct", action, id });
      bump();
    },
    [m, capable, bump]
  );

  const pressAction = useCallback(
    (action: string) => {
      if (TRAIN_ACTIONS[action]) train(action);
      else if (action === "research") setChord({ action, prefix: commandKey(RTS_ACTIONS.research) ?? "r" });
      else if (TARGET_ACTIONS.includes(action)) {
        setTargeting(action);
        const prefix = commandKey(RTS_ACTIONS[action]);
        setChord(prefix ? { action, prefix } : null);
      }
    },
    [train]
  );

  // ------------------------------------------------------------------ mouse
  const cellFromEvent = (e: React.MouseEvent): Cell | null => {
    const v = vpRef.current;
    const c = boardRef.current;
    if (!v || !c) return null;
    const r = c.getBoundingClientRect();
    return cellAt(v, e.clientX - r.left, e.clientY - r.top);
  };

  const selectAt = (cell: Cell, add: boolean, sameType: boolean) => {
    if (!m) return;
    const o = m.world.get(cell);
    if (!o || o.player !== 1) {
      setInspect(o && (o.player !== 2 || seen(m.world, fogRef.current, o.pos)) ? o.id : null);
      if (!add) setSelected([]);
      return;
    }
    setInspect(null);
    if (sameType && vpRef.current) {
      const v = vpRef.current;
      const ids = m.world.objects
        .filter((x) => x.player === 1 && x.name === o.name && x.pos[0] >= v.x0 && x.pos[1] >= v.y0 && x.pos[0] < v.x0 + v.cols && x.pos[1] < v.y0 + v.rows)
        .map((x) => x.id);
      setSelected(ids);
      return;
    }
    setSelected((s) => (add ? (s.includes(o.id) ? s.filter((x) => x !== o.id) : [...s, o.id]) : [o.id]));
  };

  const onMouseDown = (e: React.MouseEvent) => {
    if (e.button !== 0) return;
    const r = boardRef.current!.getBoundingClientRect();
    dragRef.current = { x: e.clientX - r.left, y: e.clientY - r.top };
  };
  const onMouseMove = (e: React.MouseEvent) => {
    marksRef.current.hover = cellFromEvent(e);
    const d = dragRef.current;
    if (d) {
      const r = boardRef.current!.getBoundingClientRect();
      const x = e.clientX - r.left;
      const y = e.clientY - r.top;
      marksRef.current.box = Math.abs(x - d.x) + Math.abs(y - d.y) > 6 ? [d.x, d.y, x, y] : null;
    }
  };
  const onMouseUp = (e: React.MouseEvent) => {
    if (e.button !== 0 || !m) return;
    const box = marksRef.current.box;
    dragRef.current = null;
    marksRef.current.box = null;
    const v = vpRef.current;
    if (box && v) {
      const a = cellAt(v, Math.min(box[0], box[2]), Math.min(box[1], box[3]));
      const b = cellAt(v, Math.max(box[0], box[2]), Math.max(box[1], box[3]));
      const inBox = m.world.objects.filter((o) => o.player === 1 && o.pos[0] >= a[0] && o.pos[1] >= a[1] && o.pos[0] <= b[0] && o.pos[1] <= b[1]);
      const units = inBox.filter((o) => UNITS.includes(o.name));
      const ids = (units.length ? units : inBox).map((o) => o.id);
      setSelected((s) => (e.shiftKey ? [...new Set([...s, ...ids])] : ids));
      setInspect(null);
      return;
    }
    const cell = cellFromEvent(e);
    if (!cell) return;
    if (targeting) {
      issueTargeted(targeting, cell, e.shiftKey);
      if (!e.shiftKey) {
        setTargeting(null);
        setChord(null);
      }
      bump();
      return;
    }
    selectAt(cell, e.shiftKey, e.detail >= 2);
  };
  const onContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    const cell = cellFromEvent(e);
    if (!cell) return;
    if (targeting) {
      setTargeting(null);
      setChord(null);
      return;
    }
    issueSmart(cell, e.shiftKey);
  };
  const onWheel = (e: React.WheelEvent) => {
    const i = ZOOMS.indexOf(zoom);
    const next = ZOOMS[Math.max(0, Math.min(ZOOMS.length - 1, i + (e.deltaY < 0 ? 1 : -1)))]!;
    if (next !== zoom) setZoom(next);
  };
  const onMinimap = (e: React.MouseEvent) => {
    if (!m) return;
    const c = miniRef.current!;
    const r = c.getBoundingClientRect();
    const cell: Cell = [Math.floor(((e.clientX - r.left) / r.width) * m.world.width), Math.floor(((e.clientY - r.top) / r.height) * m.world.height)];
    if (e.type === "contextmenu") {
      e.preventDefault();
      issueSmart(cell, e.shiftKey);
      return;
    }
    if (e.type === "mousedown" ? e.button !== 0 : !(e.buttons & 1)) return;
    m.panCameraTo(cell);
    bump();
  };

  // ------------------------------------------------------------------ keyboard (bindings from the GDY)
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const mm = matchRef.current;
      if (!mm || e.metaKey || e.ctrlKey || e.altKey || (e.target as HTMLElement | null)?.tagName === "INPUT") return;
      const cam = keyToActionId(RTS_ACTIONS.move_camera, e.key);
      if (cam !== null) {
        e.preventDefault();
        mm.panCamera([cam as Dir]);
        bump();
        return;
      }
      if (e.key === " ") {
        e.preventDefault();
        setPaused((p) => !p);
        return;
      }
      if (e.key === "Escape") {
        setTargeting(null);
        setChord(null);
        if (!targeting && !chord) setSelected([]);
        return;
      }
      if (e.key === "+" || e.key === "=") return setSpeed((s) => SPEEDS[Math.min(SPEEDS.length - 1, SPEEDS.indexOf(s) + 1)] ?? s);
      if (e.key === "-") return setSpeed((s) => SPEEDS[Math.max(0, SPEEDS.indexOf(s) - 1)] ?? s);
      if (e.key === "Home" || e.key === "Tab") {
        // jump the camera to the selection (or the base)
        e.preventDefault();
        const focus = primary ?? mm.world.of("base", 1)[0];
        if (focus) mm.panCameraTo(focus.pos);
        bump();
        return;
      }
      const k = e.key.toLowerCase();
      if (chord) {
        const id = chordToActionId(RTS_ACTIONS[chord.action], chord.prefix, k);
        if (id !== null) {
          e.preventDefault();
          if (chord.action === "research") research(id);
          else direct(chord.action, id);
          setChord(null);
          setTargeting(null);
          return;
        }
      }
      if (!primary) return;
      const names = Object.keys(available);
      const byCommand = names.find((n) => commandKey(RTS_ACTIONS[n]) === k);
      if (byCommand) {
        e.preventDefault();
        pressAction(byCommand);
        return;
      }
      for (const n of names) {
        const id = keyToActionId(RTS_ACTIONS[n], k);
        if (id !== null) {
          e.preventDefault();
          direct(n, id);
          return;
        }
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [available, chord, direct, pressAction, primary, research, targeting, bump]);

  // ------------------------------------------------------------------ view
  if (status === "error") {
    return (
      <div className="p-8 text-rose-300">
        Engine error: {error}{" "}
        <button className="underline" onClick={onExit}>
          back
        </button>
      </div>
    );
  }

  const res = world?.resources(1) ?? 0;
  const mine = world ? world.objects.filter((o) => o.player === 1) : [];
  const count = (pred: (o: Obj) => boolean) => mine.filter(pred).length;
  const enemyCaption = m?.lastAi.caption ?? [];
  const inspected = inspect !== null ? world?.byId.get(inspect) : undefined;

  return (
    <div className="flex h-full min-h-0 flex-col" data-testid="rts-game" data-outcome={outcome} data-tick={world?.tick ?? 0}>
      {/* top bar */}
      <header className="flex flex-wrap items-center gap-x-4 gap-y-2 border-b border-white/10 bg-black/40 px-3 py-2 text-sm">
        <button onClick={onExit} className="font-mono text-[11px] uppercase tracking-[0.2em] text-emerald-400/70 hover:text-emerald-300">
          ← Maps
        </button>
        <span className="font-bold text-stone-100">{level?.name}</span>
        <span className="font-mono text-xs text-stone-500" data-testid="tick">
          tick {world?.tick ?? 0}
        </span>
        <span className="rounded-md bg-emerald-400/10 px-2 py-0.5 font-mono text-emerald-200" data-testid="resources" title="resources">
          ◆ {res}
        </span>
        <span className="font-mono text-xs text-stone-300" title="workers / army / relays">
          👷 {count((o) => o.name === "worker")} · ⚔ {count((o) => ARMY.includes(o.name))} · ⚡ {count((o) => o.name === "relay_owned")}
        </span>
        <div className="ml-auto flex flex-wrap items-center gap-2">
          <Action onClick={() => setPaused((p) => !p)} active={paused}>
            {paused ? "▶ Resume" : "❚❚ Pause"}
          </Action>
          <label className="flex items-center gap-1 font-mono text-xs text-stone-400">
            speed
            <select value={speed} onChange={(e) => setSpeed(Number(e.target.value))} className="rounded border border-white/10 bg-black/40 px-1 py-0.5 text-stone-200">
              {SPEEDS.map((s) => (
                <option key={s} value={s}>
                  {s}/s
                </option>
              ))}
            </select>
          </label>
          <Action onClick={() => setFogOn((f) => !f)} active={fogOn}>
            Fog
          </Action>
          <Action onClick={() => setAutopilot((a) => !a)} active={autopilot} title="Let the AI command your side">
            Autopilot
          </Action>
          <Action onClick={onRestart}>↻ Restart</Action>
        </div>
      </header>

      <div className="flex min-h-0 flex-1 flex-col lg:flex-row">
        {/* board */}
        <div ref={wrapRef} className="relative min-h-[420px] flex-1 overflow-hidden bg-black" onWheel={onWheel}>
          <canvas
            ref={boardRef}
            data-testid="board"
            className={`block ${targeting ? "cursor-crosshair" : "cursor-default"}`}
            onMouseDown={onMouseDown}
            onMouseMove={onMouseMove}
            onMouseUp={onMouseUp}
            onMouseLeave={() => {
              marksRef.current.hover = null;
              dragRef.current = null;
              marksRef.current.box = null;
            }}
            onContextMenu={onContextMenu}
          />
          {status === "loading" && <Overlay>Loading the engine…</Overlay>}
          {targeting && (
            <div className="pointer-events-none absolute top-2 left-1/2 -translate-x-1/2 rounded-full border border-amber-400/40 bg-black/70 px-3 py-1 font-mono text-xs text-amber-200">
              {label(targeting)}: click a target{chord ? ` or press ${chord.prefix.toUpperCase()}+W/A/S/D` : ""} · Esc cancels
            </div>
          )}
          {paused && outcome === "playing" && status === "ready" && (
            <div className="pointer-events-none absolute top-2 right-2 rounded bg-black/70 px-2 py-1 font-mono text-xs text-amber-200">PAUSED</div>
          )}
          {outcome !== "playing" && (
            <Overlay tone={outcome === "won" ? "emerald" : "rose"} testId="result">
              <p className="text-4xl font-black">{outcome === "won" ? "Victory" : outcome === "lost" ? "Defeat" : "Draw"}</p>
              <p className="mt-1 font-mono text-xs text-stone-300">after {world?.tick} ticks</p>
              <div className="mt-4 flex gap-2">
                <Action primary onClick={onRestart}>
                  ↻ Play again
                </Action>
                <Action onClick={onExit}>Maps</Action>
              </div>
            </Overlay>
          )}
        </div>

        {/* sidebar */}
        <aside className="flex w-full shrink-0 flex-col gap-3 overflow-y-auto border-l border-white/10 bg-black/30 p-3 lg:w-80">
          <canvas
            ref={miniRef}
            width={288}
            height={Math.round((288 * (world?.height ?? 1)) / (world?.width ?? 1))}
            data-testid="minimap"
            className="w-full cursor-pointer rounded border border-white/10 [image-rendering:pixelated]"
            onMouseDown={onMinimap}
            onMouseMove={onMinimap}
            onContextMenu={onMinimap}
          />

          <SelectionPanel
            world={world}
            match={m}
            sel={selObjs}
            inspected={inspected}
            available={available}
            status={m?.unitStatus}
            onAction={pressAction}
            onResearch={research}
            targeting={targeting}
            chord={chord}
          />

          <TechPanel world={world} onResearch={research} hasLab={count((o) => o.name === "lab") > 0} />

          <div className="rounded-lg border border-white/10 bg-white/[0.03] p-2">
            <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Events</p>
            <ul className="mt-1 max-h-32 overflow-y-auto font-mono text-[11px] leading-relaxed" data-testid="log">
              {log.slice(0, 12).map((l, i) => (
                <li key={`${l.tick}-${i}`} className={l.tone === "good" ? "text-emerald-300" : l.tone === "bad" ? "text-rose-300" : "text-stone-400"}>
                  <span className="text-stone-600">{l.tick}</span> {l.text}
                </li>
              ))}
            </ul>
          </div>

          <details className="rounded-lg border border-white/10 bg-white/[0.03] p-2">
            <summary className="cursor-pointer font-mono text-[10px] uppercase tracking-widest text-stone-500">Enemy AI ({setup.enemy}) · debug</summary>
            <pre className="mt-1 whitespace-pre-wrap font-mono text-[10px] leading-relaxed text-rose-200/80">{enemyCaption.join("\n")}</pre>
            {autopilot && m?.autopilot && <pre className="mt-1 whitespace-pre-wrap font-mono text-[10px] text-emerald-200/80">autopilot: {m.autopilot.phase}</pre>}
          </details>
        </aside>
      </div>
    </div>
  );
};

// ------------------------------------------------------------------------------------------ panels

const SelectionPanel = ({
  world,
  match,
  sel,
  inspected,
  available,
  status,
  onAction,
  onResearch,
  targeting,
  chord,
}: {
  world: WorldView | null;
  match: Match | null;
  sel: Obj[];
  inspected: Obj | undefined;
  available: Record<string, number[]>;
  status: Map<number, string> | undefined;
  onAction: (a: string) => void;
  onResearch: (t: number) => void;
  targeting: string | null;
  chord: { action: string; prefix: string } | null;
}) => {
  if (!world || !match) return null;
  const primary = sel[0];
  if (!primary) {
    if (inspected) {
      return (
        <div className="rounded-lg border border-white/10 bg-white/[0.03] p-2 text-sm" data-testid="inspect">
          <p className="font-bold text-stone-100">
            {inspected.player === 2 ? "Enemy " : ""}
            {DISPLAY[inspected.name] ?? inspected.name}
          </p>
          <Vars o={inspected} />
        </div>
      );
    }
    return (
      <div className="rounded-lg border border-white/10 bg-white/[0.03] p-2 text-xs leading-relaxed text-stone-500" data-testid="selection">
        Click a unit or building to select it (drag for a box, double-click for all of a type). The engine reports what it can do.
      </div>
    );
  }
  const counts = new Map<string, number>();
  for (const o of sel) counts.set(o.name, (counts.get(o.name) ?? 0) + 1);
  const queue = match.orders.queues.get(primary.id) ?? [];
  const actions = Object.entries(available).filter(([n]) => n !== "research");
  const res = world.resources(1);
  const inputs = (n: string) => RTS_ACTIONS[n]?.inputs ?? [];
  return (
    <div className="rounded-lg border border-emerald-400/20 bg-emerald-400/[0.04] p-2 text-sm" data-testid="selection">
      <p className="font-bold text-stone-100">
        {sel.length === 1 ? DISPLAY[primary.name] : [...counts].map(([n, c]) => `${c} ${DISPLAY[n]}`).join(", ")}
        <span className="ml-2 font-mono text-[10px] font-normal text-stone-500">
          @{primary.pos[0]},{primary.pos[1]}
        </span>
      </p>
      {sel.length === 1 && <Vars o={primary} />}
      {sel.length === 1 && (
        <p className="mt-1 font-mono text-[11px] text-stone-400">
          {status?.get(primary.id) ?? (queue.length ? "" : "idle")}
          {queue.length > 0 && ` · ${queue.map(describeOrder).join(" → ")}`}
        </p>
      )}
      <div className="mt-2 grid grid-cols-2 gap-1.5" data-testid="actions">
        {actions.map(([name, ids]) => {
          const key = commandKey(RTS_ACTIONS[name]);
          const plain = inputs(name).filter((i) => i.key && !i.key.includes("+")).map((i) => i.key!.toUpperCase());
          const cost = costOf(name);
          const short = cost !== undefined && res < cost;
          const ready = ids.length > 0;
          const disabled = TRAIN_ACTIONS[name] ? !ready && !short : false;
          return (
            <button
              key={name}
              data-action={name}
              data-ids={ids.join(",")}
              disabled={disabled}
              onClick={() => onAction(name)}
              title={inputs(name)[0]?.description}
              className={`flex flex-col items-start rounded-md border px-2 py-1 text-left transition disabled:opacity-35 ${
                targeting === name ? "border-amber-400/70 bg-amber-400/15" : ready ? "border-white/15 bg-black/30 hover:border-emerald-400/50" : "border-white/5 bg-black/20 text-stone-400 hover:border-white/20"
              }`}
            >
              <span className="flex w-full items-center justify-between gap-1 text-xs font-semibold">
                <span>
                  {ICONS[name] ?? "•"} {label(name)}
                </span>
                {key ? <Kbd>{key.toUpperCase()}</Kbd> : plain.length ? <Kbd>{plain.join("")}</Kbd> : null}
              </span>
              <span className="font-mono text-[10px] text-stone-500">
                {cost !== undefined ? `◆${cost} ` : ""}
                {ready ? ids.map((i) => ARROWS[i] ?? i).join(" ") : short ? "need resources" : "no target adjacent"}
              </span>
            </button>
          );
        })}
      </div>
      {"research" in available && (
        <div className="mt-2" data-testid="research">
          <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">
            Research {chord?.action === "research" ? "· press 1-6" : <Kbd>R</Kbd>}
          </p>
          <div className="mt-1 flex flex-col gap-1">
            {inputs("research").map((i) => {
              const ok = available.research!.includes(i.id);
              return (
                <button
                  key={i.id}
                  disabled={!ok}
                  onClick={() => onResearch(i.id)}
                  className="rounded border border-white/10 bg-black/30 px-2 py-1 text-left text-[11px] leading-snug text-stone-300 enabled:hover:border-sky-400/50 disabled:opacity-35"
                >
                  <Kbd>{i.id}</Kbd> {i.description}
                </button>
              );
            })}
          </div>
        </div>
      )}
      <p className="mt-2 font-mono text-[10px] leading-snug text-stone-600" data-testid="engine-report">
        engine · getAvailableActionIds({primary.pos[0]},{primary.pos[1]}):{" "}
        {Object.entries(available)
          .map(([n, ids]) => `${n}[${ids.join(",")}]`)
          .join(" ")}
      </p>
      {(primary.name === "base" || primary.name === "barracks") && (
        <p className="mt-1 text-[11px] text-stone-500">Right-click the map to set a rally point{match.rallies.has(primary.id) ? " (set)" : ""}.</p>
      )}
    </div>
  );
};

const Vars = ({ o }: { o: Obj }) => {
  const v = o.vars;
  const parts: string[] = [];
  if (v.max_health) parts.push(`hp ${v.health}/${v.max_health}`);
  if (v.damage) parts.push(`dmg ${v.damage}`);
  if (v.resources !== undefined) parts.push(`${v.resources} left`);
  if (v.carrying) parts.push(["", "carrying ore", "carrying wood", "carrying gems"][v.carrying] ?? "carrying");
  if (o.name === "lab" && v.busy) parts.push("researching…");
  if (o.name === "tank" && v.cooldown) parts.push("reloading");
  return <p className="font-mono text-[11px] text-stone-400">{parts.join(" · ")}</p>;
};

const TechPanel = ({ world, onResearch, hasLab }: { world: WorldView | null; onResearch: (t: number) => void; hasLab: boolean }) => {
  if (!world) return null;
  const res = world.resources(1);
  const busy = world.of("lab", 1).every((l) => l.vars.busy);
  return (
    <div className="rounded-lg border border-white/10 bg-white/[0.03] p-2" data-testid="tech">
      <p className="font-mono text-[10px] uppercase tracking-widest text-stone-500">Tech tree {hasLab ? "" : "· build a lab"}</p>
      <div className="mt-1 grid grid-cols-3 gap-1">
        {Object.entries(TECHS).map(([id, t]) => {
          const state = world.tech(t.variable, 1);
          const locked = t.needs !== null && world.tech(t.needs, 1) !== 2;
          const can = hasLab && !busy && state === 0 && !locked && res >= t.cost;
          return (
            <button
              key={id}
              disabled={!can}
              onClick={() => onResearch(Number(id))}
              title={RTS_ACTIONS.research?.inputs.find((i) => i.id === Number(id))?.description}
              className={`rounded border px-1 py-1 text-center text-[11px] leading-tight transition ${
                state === 2
                  ? "border-sky-400/50 bg-sky-400/15 text-sky-200"
                  : state === 1
                    ? "animate-pulse border-amber-400/50 bg-amber-400/10 text-amber-200"
                    : can
                      ? "border-white/15 bg-black/30 text-stone-200 hover:border-sky-400/50"
                      : "border-white/5 bg-black/20 text-stone-500"
              }`}
            >
              {t.name}
              <span className="block font-mono text-[9px] opacity-70">{state === 2 ? "done" : state === 1 ? "…" : locked ? "locked" : `◆${t.cost}`}</span>
            </button>
          );
        })}
      </div>
    </div>
  );
};

// ------------------------------------------------------------------------------------------ events

type Snap = { ids: Map<number, string>; enemy: Map<number, string>; tech: Record<string, number>; baseHp: number; alarm: number };

function snapshot(world: WorldView, prev: Snap | null): Snap {
  const ids = new Map<number, string>();
  const enemy = new Map<number, string>();
  for (const o of world.objects) {
    if (o.player === 1) ids.set(o.id, o.name);
    else if (o.player === 2) enemy.set(o.id, o.name);
  }
  const tech: Record<string, number> = {};
  for (const t of Object.values(TECHS)) tech[t.variable] = world.tech(t.variable, 1);
  return { ids, enemy, tech, baseHp: world.of("base", 1)[0]?.vars.health ?? 0, alarm: prev?.alarm ?? -100 };
}

function events(world: WorldView, prev: Snap | null, next: Snap): LogLine[] {
  if (!prev) return [];
  const out: LogLine[] = [];
  const tick = world.tick;
  for (const [id, name] of next.ids) {
    if (prev.ids.has(id)) continue;
    if (name === "relay_owned") out.push({ tick, text: "Relay captured: +1 income every 6 ticks", tone: "good" });
    else out.push({ tick, text: `${DISPLAY[name] ?? name} ${UNITS.includes(name) ? "ready" : "built"}`, tone: "good" });
  }
  for (const [id, name] of prev.ids) {
    if (!next.ids.has(id)) out.push({ tick, text: `${DISPLAY[name] ?? name} lost`, tone: "bad" });
  }
  for (const [id, name] of prev.enemy) {
    if (next.enemy.has(id)) continue;
    out.push({ tick, text: `enemy ${DISPLAY[name] ?? name} destroyed`, tone: "info" });
  }
  for (const t of Object.values(TECHS)) {
    if (prev.tech[t.variable] !== 2 && next.tech[t.variable] === 2) out.push({ tick, text: `Research complete: ${t.name}`, tone: "good" });
  }
  if (next.baseHp < prev.baseHp && tick - prev.alarm > 30) {
    out.push({ tick, text: "Your base is under attack!", tone: "bad" });
    next.alarm = tick;
  }
  return out;
}

// ------------------------------------------------------------------------------------------ route

const DEFAULT_SETUP: Setup = { level: 0, enemy: "ARMOUR", spectate: false, fog: true, speed: 4, key: 0 };

const RtsPage = () => {
  const [setup, setSetup] = useState<Setup | null>(null);
  const [last, setLast] = useState<Setup>(DEFAULT_SETUP);
  return (
    <div className="h-screen bg-[radial-gradient(ellipse_at_top,_#1b2a1f_0%,_#0b0f0c_55%,_#050605_100%)] text-stone-200">
      {setup ? (
        <GameScreen
          key={setup.key}
          setup={setup}
          onExit={() => setSetup(null)}
          onRestart={() => setSetup({ ...setup, key: setup.key + 1 })}
        />
      ) : (
        <div className="h-full overflow-y-auto">
          <Menu
            initial={last}
            onStart={(s) => {
              setLast(s);
              setSetup(s);
            }}
          />
        </div>
      )}
    </div>
  );
};

export const Route = createFileRoute("/rts")({
  head: () => ({ meta: [{ title: "Griddly RTS: Frontiers" }] }),
  component: RtsPage,
});
