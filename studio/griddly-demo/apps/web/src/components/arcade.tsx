// Shared UI pieces for the library and player pages (dark mossy arcade theme).
import type { CSSProperties, ReactNode } from "react";

import type { GameMeta } from "../games/catalog";

/** CSS variables derived from a game's accent hex, for Tailwind arbitrary values. */
export const accentVars = (hex: string): CSSProperties => {
  const h = hex.replace("#", "");
  const full = h.length === 3 ? [...h].map((c) => c + c).join("") : h.slice(0, 6);
  const n = Number.parseInt(full, 16);
  const [r, g, b] = [(n >> 16) & 255, (n >> 8) & 255, n & 255];
  return {
    "--accent": `rgb(${r} ${g} ${b})`,
    "--accent-50": `rgb(${r} ${g} ${b} / 0.5)`,
    "--accent-30": `rgb(${r} ${g} ${b} / 0.3)`,
    "--accent-15": `rgb(${r} ${g} ${b} / 0.15)`,
  } as CSSProperties;
};

export const TONE_TEXT: Record<string, string> = {
  fuchsia: "text-fuchsia-300",
  amber: "text-amber-300",
  rose: "text-rose-300",
  sky: "text-sky-300",
  emerald: "text-emerald-300",
  violet: "text-violet-300",
  lime: "text-lime-300",
  orange: "text-orange-300",
};

const DIFF_STYLE: Record<GameMeta["difficulty"], string> = {
  easy: "border-emerald-400/40 bg-emerald-950/70 text-emerald-200",
  medium: "border-amber-400/40 bg-amber-950/70 text-amber-200",
  hard: "border-rose-400/40 bg-rose-950/70 text-rose-200",
};
const DIFF_PIPS: Record<GameMeta["difficulty"], number> = { easy: 1, medium: 2, hard: 3 };

export const DifficultyBadge = ({ difficulty }: { difficulty: GameMeta["difficulty"] }) => (
  <span
    className={`inline-flex items-center gap-1.5 rounded-full border px-2 py-0.5 font-mono text-[10px] uppercase tracking-wider backdrop-blur ${DIFF_STYLE[difficulty]}`}
  >
    <span className="flex gap-0.5" aria-hidden>
      {[0, 1, 2].map((i) => (
        <span key={i} className={`h-1.5 w-1.5 rounded-full ${i < DIFF_PIPS[difficulty] ? "bg-current" : "bg-current/25"}`} />
      ))}
    </span>
    {difficulty}
  </span>
);

export const Pill = ({ tone, children }: { tone: "emerald" | "rose"; children: ReactNode }) => (
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

export const Stat = ({ label, value, tone }: { label: string; value: ReactNode; tone: string }) => (
  <div className="min-w-0 rounded-lg border border-white/5 bg-black/30 px-3 py-2">
    <p className="truncate text-[10px] uppercase tracking-widest text-stone-500">{label}</p>
    <p className={`text-lg font-bold tabular-nums ${tone}`}>{value}</p>
  </div>
);

export const Pad = ({ onClick, children, label }: { onClick: () => void; children: ReactNode; label: string }) => (
  <button
    onClick={onClick}
    aria-label={label}
    className="flex h-14 w-14 touch-manipulation items-center justify-center rounded-xl border border-white/10 bg-gradient-to-b from-stone-700/60 to-stone-900/80 text-stone-200 shadow-[inset_0_1px_0_rgba(255,255,255,0.08),0_4px_12px_-4px_rgba(0,0,0,0.8)] transition select-none hover:border-[var(--accent-50,rgba(52,211,153,.4))] hover:text-[var(--accent,#a7f3d0)] active:translate-y-px active:shadow-none sm:h-12 sm:w-12"
  >
    {children}
  </button>
);

export const Action = ({
  onClick,
  children,
  primary,
  active,
  className = "",
  title,
}: {
  onClick: () => void;
  children: ReactNode;
  primary?: boolean;
  active?: boolean;
  className?: string;
  title?: string;
}) => (
  <button
    onClick={onClick}
    title={title}
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

export const Overlay = ({
  children,
  tone = "emerald",
  testId,
}: {
  children: ReactNode;
  tone?: "emerald" | "rose";
  testId?: string;
}) => (
  <div
    data-testid={testId}
    className={`absolute inset-3 flex animate-[fadein_.25s_ease-out] flex-col items-center justify-center rounded-xl px-4 text-center backdrop-blur-[2px] ${
      tone === "rose" ? "bg-rose-950/60" : "bg-black/60"
    }`}
  >
    {children}
  </div>
);

export const Kbd = ({ children }: { children: ReactNode }) => (
  <kbd className="rounded border border-white/10 bg-white/5 px-1.5 py-px font-mono text-[10px] text-stone-300">{children}</kbd>
);
