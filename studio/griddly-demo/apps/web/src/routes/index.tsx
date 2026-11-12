import { Link, createFileRoute, useNavigate } from "@tanstack/react-router";
import { useEffect, useMemo, useRef, useState } from "react";

import { ALL_TAGS, GAMES, searchScore } from "../games/catalog";
import type { Game } from "../games/catalog";
import { cachedThumb, thumb } from "../lib/thumbs";
import { DifficultyBadge, accentVars } from "../components/arcade";

/**
 * Games library: every bundled Griddly game (the remastered single-player games + Spiders Garden),
 * with search, tag / difficulty filters and sorting. Cards open `/play/$gameId`.
 */

type Sort = "featured" | "name" | "difficulty" | "levels";
const SORTS: { id: Sort; label: string }[] = [
  { id: "featured", label: "Featured" },
  { id: "name", label: "A–Z" },
  { id: "difficulty", label: "Difficulty" },
  { id: "levels", label: "Most levels" },
];
const DIFF_ORDER = { easy: 0, medium: 1, hard: 2 } as const;
const DIFFS = ["all", "easy", "medium", "hard"] as const;
type DiffFilter = (typeof DIFFS)[number];

const Library = () => {
  const navigate = useNavigate();
  const searchRef = useRef<HTMLInputElement>(null);
  const [query, setQuery] = useState("");
  const [tags, setTags] = useState<string[]>([]);
  const [diff, setDiff] = useState<DiffFilter>("all");
  const [sort, setSort] = useState<Sort>("featured");

  const results = useMemo(() => {
    const scored = GAMES.map((g) => ({ g, s: searchScore(g, query) })).filter(
      ({ g, s }) => s > 0 && (diff === "all" || g.difficulty === diff) && tags.every((t) => g.tags.includes(t))
    );
    const by: Record<Sort, (a: Game, b: Game) => number> = {
      featured: (a, b) => a.rank - b.rank,
      name: (a, b) => a.name.localeCompare(b.name),
      difficulty: (a, b) => DIFF_ORDER[a.difficulty] - DIFF_ORDER[b.difficulty] || a.name.localeCompare(b.name),
      levels: (a, b) => b.levels.length - a.levels.length || a.name.localeCompare(b.name),
    };
    return scored
      .sort((a, b) => (query.trim() && sort === "featured" ? b.s - a.s : 0) || by[sort](a.g, b.g))
      .map(({ g }) => g);
  }, [query, tags, diff, sort]);

  const first = results[0];
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const typing = document.activeElement instanceof HTMLInputElement || document.activeElement instanceof HTMLSelectElement;
      if (e.key === "/" && !typing) {
        e.preventDefault();
        searchRef.current?.focus();
        searchRef.current?.select();
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  const toggleTag = (t: string) => setTags((ts) => (ts.includes(t) ? ts.filter((x) => x !== t) : [...ts, t]));
  const filtered = !!query || tags.length > 0 || diff !== "all";
  const totalLevels = GAMES.reduce((n, g) => n + g.levels.length, 0);

  return (
    <div className="min-h-full bg-[radial-gradient(ellipse_at_top,_#1b2a1f_0%,_#0b0f0c_55%,_#050605_100%)] text-stone-200">
      <main className="mx-auto flex max-w-6xl flex-col gap-6 px-4 py-8 sm:px-8">
        {/* hero */}
        <header className="relative overflow-hidden rounded-3xl border border-white/10 bg-black/30 px-6 py-8 shadow-2xl shadow-black/50 sm:px-10 sm:py-10">
          <div
            aria-hidden
            className="pointer-events-none absolute inset-0 opacity-[0.07]"
            style={{
              backgroundImage:
                "linear-gradient(rgba(255,255,255,.6) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.6) 1px, transparent 1px)",
              backgroundSize: "24px 24px",
            }}
          />
          <div aria-hidden className="pointer-events-none absolute -top-24 -right-16 h-64 w-64 rounded-full bg-emerald-500/20 blur-3xl" />
          <div aria-hidden className="pointer-events-none absolute -bottom-28 left-10 h-56 w-56 rounded-full bg-amber-400/10 blur-3xl" />
          <p className="relative font-mono text-[11px] uppercase tracking-[0.3em] text-emerald-400/70">
            Griddly · WebAssembly arcade
          </p>
          <h1 className="relative mt-1 bg-gradient-to-r from-emerald-200 via-lime-200 to-amber-200 bg-clip-text text-4xl font-black tracking-tight text-transparent sm:text-5xl">
            Grid Games Library
          </h1>
          <p className="relative mt-3 max-w-2xl text-sm leading-relaxed text-stone-400 sm:text-base">
            Classic Griddly research games, remastered with bigger themed worlds. Every frame is rendered by the
            Griddly engine running in your browser via <code className="text-emerald-300">@griddly/wasm</code>.
          </p>
          <div className="relative mt-5 flex flex-wrap gap-2 font-mono text-xs">
            <span className="rounded-full border border-emerald-400/30 bg-emerald-400/10 px-3 py-1 text-emerald-200">
              🎮 {GAMES.length} games
            </span>
            <span className="rounded-full border border-amber-400/30 bg-amber-400/10 px-3 py-1 text-amber-200">
              🗺️ {totalLevels} levels
            </span>
          </div>
        </header>

        {/* search + filters */}
        <section className="flex flex-col gap-3">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-center">
            <label className="relative flex-1">
              <span className="sr-only">Search games</span>
              <span aria-hidden className="pointer-events-none absolute top-1/2 left-4 -translate-y-1/2 text-stone-500">
                ⌕
              </span>
              <input
                ref={searchRef}
                type="search"
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === "Enter" && first) {
                    e.preventDefault();
                    void navigate({ to: "/play/$gameId", params: { gameId: first.id } });
                  } else if (e.key === "Escape") {
                    setQuery("");
                    e.currentTarget.blur();
                  }
                }}
                placeholder="Search games, tags, mechanics…"
                aria-label="Search games"
                className="w-full rounded-2xl border border-white/10 bg-black/40 py-3 pr-14 pl-10 text-sm text-stone-100 placeholder-stone-500 shadow-inner shadow-black/40 outline-none transition focus:border-emerald-400/50 focus:ring-2 focus:ring-emerald-400/20"
              />
              <kbd className="pointer-events-none absolute top-1/2 right-3 hidden -translate-y-1/2 rounded-md border border-white/10 bg-white/5 px-2 py-0.5 font-mono text-[10px] text-stone-400 sm:block">
                /
              </kbd>
            </label>
            <div className="flex gap-2">
              <div role="group" aria-label="Difficulty" className="flex rounded-xl border border-white/10 bg-black/30 p-1">
                {DIFFS.map((d) => (
                  <button
                    key={d}
                    onClick={() => setDiff(d)}
                    aria-pressed={diff === d}
                    className={`rounded-lg px-2.5 py-1.5 text-xs font-semibold capitalize transition ${
                      diff === d ? "bg-emerald-400/15 text-emerald-200" : "text-stone-400 hover:text-stone-200"
                    }`}
                  >
                    {d}
                  </button>
                ))}
              </div>
              <label className="relative">
                <span className="sr-only">Sort</span>
                <select
                  value={sort}
                  onChange={(e) => setSort(e.target.value as Sort)}
                  aria-label="Sort games"
                  className="h-full appearance-none rounded-xl border border-white/10 bg-black/30 py-2 pr-8 pl-3 text-xs font-semibold text-stone-300 outline-none focus:border-emerald-400/50"
                >
                  {SORTS.map((s) => (
                    <option key={s.id} value={s.id} className="bg-stone-900">
                      {s.label}
                    </option>
                  ))}
                </select>
                <span aria-hidden className="pointer-events-none absolute top-1/2 right-3 -translate-y-1/2 text-[10px] text-stone-500">
                  ▼
                </span>
              </label>
            </div>
          </div>
          <div className="-mx-1 flex flex-wrap gap-1.5 px-1" role="group" aria-label="Tags">
            {ALL_TAGS.map((t) => {
              const on = tags.includes(t);
              return (
                <button
                  key={t}
                  onClick={() => toggleTag(t)}
                  aria-pressed={on}
                  data-tag={t}
                  className={`rounded-full border px-3 py-1 font-mono text-[11px] transition ${
                    on
                      ? "border-emerald-400/60 bg-emerald-400/15 text-emerald-200 shadow-[0_0_16px_-6px_rgba(52,211,153,0.8)]"
                      : "border-white/10 bg-white/[0.03] text-stone-400 hover:border-white/20 hover:text-stone-200"
                  }`}
                >
                  #{t}
                </button>
              );
            })}
            {filtered && (
              <button
                onClick={() => {
                  setQuery("");
                  setTags([]);
                  setDiff("all");
                }}
                className="rounded-full px-3 py-1 font-mono text-[11px] text-rose-300/80 hover:text-rose-200"
              >
                ✕ clear
              </button>
            )}
          </div>
          <p className="font-mono text-[11px] text-stone-500" aria-live="polite" data-testid="result-count">
            {results.length} {results.length === 1 ? "game" : "games"}
            {first && query ? (
              <>
                {" "}
                · <kbd className="text-stone-400">Enter</kbd> opens <span className="text-stone-300">{first.name}</span>
              </>
            ) : null}
          </p>
        </section>

        {/* grid */}
        {results.length ? (
          <ul className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3" data-testid="game-grid">
            {results.map((g, i) => (
              <li key={g.id}>
                <GameCard game={g} highlight={i === 0 && !!query} />
              </li>
            ))}
          </ul>
        ) : (
          <div className="rounded-2xl border border-dashed border-white/10 py-16 text-center text-stone-400">
            <p className="text-4xl">🔍</p>
            <p className="mt-2">No games match. Try another search or clear the filters.</p>
          </div>
        )}

        <footer className="pb-4 text-center font-mono text-[10px] text-stone-600">
          Press <kbd className="text-stone-400">/</kbd> to search · <kbd className="text-stone-400">Enter</kbd> to play the
          first match
        </footer>
      </main>
    </div>
  );
};

/** Lazily renders level 0 with the engine once the card scrolls into view. */
const useThumb = (game: Game) => {
  const ref = useRef<HTMLAnchorElement>(null);
  const [url, setUrl] = useState<string | undefined>(() => cachedThumb(game.id, 0));
  useEffect(() => {
    if (url || !ref.current) return;
    let cancelled = false;
    const io = new IntersectionObserver(
      (entries) => {
        if (!entries.some((e) => e.isIntersecting)) return;
        io.disconnect();
        thumb(game, 0, () => cancelled)
          .then((u) => !cancelled && u && setUrl(u))
          .catch(() => undefined);
      },
      { rootMargin: "200px" }
    );
    io.observe(ref.current);
    return () => {
      cancelled = true;
      io.disconnect();
    };
  }, [game, url]);
  return { ref, url };
};

const GameCard = ({ game, highlight }: { game: Game; highlight: boolean }) => {
  const { ref, url } = useThumb(game);
  return (
    <Link
      ref={ref}
      to="/play/$gameId"
      params={{ gameId: game.id }}
      data-game={game.id}
      style={accentVars(game.accent)}
      className={`group flex h-full flex-col overflow-hidden rounded-2xl border bg-white/[0.03] shadow-xl shadow-black/40 transition duration-200 hover:-translate-y-0.5 hover:border-[var(--accent-50)] hover:shadow-[0_12px_40px_-12px_var(--accent-50)] focus-visible:border-[var(--accent)] focus-visible:outline-none ${
        highlight ? "border-[var(--accent-50)] ring-1 ring-[var(--accent-50)]" : "border-white/10"
      }`}
    >
      {/* thumbnail / pattern */}
      <div
        className="relative aspect-[16/9] overflow-hidden border-b border-white/5"
        style={{
          background: `radial-gradient(circle at 30% 20%, var(--accent-30), transparent 60%), repeating-linear-gradient(45deg, rgba(255,255,255,.03) 0 10px, transparent 10px 20px), #0b0f0c`,
        }}
      >
        {url ? (
          <img
            src={url}
            alt={`${game.name}, first level`}
            className="absolute inset-0 h-full w-full object-cover opacity-90 transition duration-500 group-hover:scale-[1.04] group-hover:opacity-100"
            style={{ imageRendering: "pixelated" }}
          />
        ) : (
          <span className="absolute inset-0 flex items-center justify-center text-6xl opacity-60 blur-[0.5px]">{game.icon}</span>
        )}
        <div className="absolute inset-x-0 bottom-0 h-1/2 bg-gradient-to-t from-black/80 to-transparent" />
        <span className="absolute top-2 right-2">
          <DifficultyBadge difficulty={game.difficulty} />
        </span>
        <span className="absolute bottom-2 left-3 flex h-10 w-10 items-center justify-center rounded-xl border border-white/15 bg-black/60 text-2xl shadow-lg backdrop-blur">
          {game.icon}
        </span>
        <span className="absolute right-3 bottom-2 font-mono text-[10px] text-stone-300">
          {game.levels.length} levels
        </span>
      </div>
      <div className="flex flex-1 flex-col gap-2 p-4">
        <div className="flex items-start justify-between gap-2">
          <h2 className="text-lg leading-tight font-bold text-stone-100 group-hover:text-[var(--accent)]">{game.name}</h2>
          <span className="mt-0.5 shrink-0 text-stone-500 transition group-hover:translate-x-0.5 group-hover:text-[var(--accent)]">
            ▶
          </span>
        </div>
        <p className="text-sm leading-snug text-stone-400">{game.tagline}</p>
        <div className="mt-auto flex flex-wrap gap-1 pt-2">
          {game.tags.map((t) => (
            <span key={t} className="rounded-full bg-white/[0.05] px-2 py-0.5 font-mono text-[10px] text-stone-400">
              #{t}
            </span>
          ))}
        </div>
      </div>
      <div className="h-1 w-full bg-[var(--accent)] opacity-60 transition group-hover:opacity-100" />
    </Link>
  );
};

export const Route = createFileRoute("/")({
  component: Library,
});
