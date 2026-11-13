// Pure helpers that read what the UI needs out of a GDY string and the catalog metadata JSON
// (written by studio/griddly-demo/tools/remaster/common.py `emit()`). No Vite-only APIs here, so
// bun's test runner can import this module.
import { parse } from "yaml";

export type Tone = "fuchsia" | "amber" | "rose" | "sky" | "emerald" | "violet" | "lime" | "orange";
export type HudKind = "collected" | "remaining" | "count" | "var" | "global";
export type HudItem = { label: string; kind: HudKind; object?: string; variable?: string; tone?: Tone | string };
export type ExtraControl = { key: string; label: string; action: string; directional?: boolean };
export type Difficulty = "easy" | "medium" | "hard";

/** Catalog metadata of one game (the remaster generators' JSON schema, normalised). */
export type GameMeta = {
  id: string;
  name: string;
  tagline: string;
  description: string;
  tags: string[];
  difficulty: Difficulty;
  original: string;
  accent: string;
  icon: string;
  /**
   * scheme "absolute": ↑/↓/←/→ move in that direction (move ids 1 left, 2 up, 3 right, 4 down).
   * scheme "relative": the avatar turns; ↑ forward (2), ↓ back (4), ← turn left (1), → turn right (3).
   */
  controls: { move: string; scheme: "absolute" | "relative"; extra: ExtraControl[] };
  hud: HudItem[];
  win_text: string;
  lose_text: string;
};

export type LevelInfo = { name: string; blurb: string; width: number; height: number };
export type Camera = {
  name: string;
  label: string;
  /** Tracks the avatar: rendered via the player's observer (observePlayerImage). */
  follows: boolean;
  /** Rotates with the avatar (egocentric view). */
  rotates: boolean;
  kind: "sprite" | "isometric";
};

const DIFFICULTIES: Difficulty[] = ["easy", "medium", "hard"];

const str = (v: unknown, fallback = ""): string => (typeof v === "string" ? v : fallback);

/** Fill in every optional field so the UI never has to guard. Unknown input is tolerated. */
export function normaliseMeta(raw: Record<string, unknown>, fallbackId: string, gdyName = ""): GameMeta {
  const id = str(raw.id, fallbackId);
  const controls = (raw.controls ?? {}) as { move?: unknown; extra?: unknown };
  const extra = Array.isArray(controls.extra) ? (controls.extra as ExtraControl[]) : [];
  const difficulty = str(raw.difficulty).toLowerCase() as Difficulty;
  return {
    id,
    name: str(raw.name, gdyName || id),
    tagline: str(raw.tagline),
    description: str(raw.description),
    tags: Array.isArray(raw.tags) ? raw.tags.filter((t): t is string => typeof t === "string") : [],
    difficulty: DIFFICULTIES.includes(difficulty) ? difficulty : "medium",
    original: str(raw.original),
    accent: /^#[0-9a-f]{3,8}$/i.test(str(raw.accent)) ? str(raw.accent) : "#34d399",
    icon: str(raw.icon, "🎮"),
    controls: {
      move: str(controls.move, "move"),
      scheme: (controls as { scheme?: unknown }).scheme === "relative" ? "relative" : "absolute",
      extra: extra.filter((e) => e && typeof e.action === "string" && typeof e.key === "string"),
    },
    hud: Array.isArray(raw.hud) ? (raw.hud as HudItem[]).filter((h) => h && typeof h.kind === "string") : [],
    win_text: str(raw.win_text, "Level complete!"),
    lose_text: str(raw.lose_text, "Game over"),
  };
}

/**
 * Levels with names/blurbs from the `# Name | blurb` comment above each `    - |` entry, and the
 * map size read from the ASCII rows. Levels without a comment get a generic name.
 */
export function parseLevels(gdy: string): LevelInfo[] {
  const lines = gdy.split("\n");
  const start = lines.findIndex((l) => /^ {2}Levels:\s*$/.test(l));
  if (start < 0) return [];
  const levels: LevelInfo[] = [];
  let comment: string | null = null;
  for (let i = start + 1; i < lines.length; i++) {
    const line = lines[i]!;
    if (/^\S/.test(line) || /^ {2}\S/.test(line)) break; // left the Levels block
    const c = /^ {4}# (.+)$/.exec(line);
    if (c) {
      comment = c[1]!;
      continue;
    }
    if (/^ {4}- \|/.test(line)) {
      const rows: string[] = [];
      for (let j = i + 1; j < lines.length && /^ {6}\S/.test(lines[j]!); j++) rows.push(lines[j]!.trim());
      const [name, blurb = ""] = (comment ?? `Level ${levels.length + 1}`).split("|").map((x) => x.trim());
      levels.push({
        name: name || `Level ${levels.length + 1}`,
        blurb,
        width: rows.reduce((w, r) => Math.max(w, r.split(/\s+/).length), 0),
        height: rows.length,
      });
      comment = null;
    }
  }
  return levels;
}

type GdyDoc = {
  Environment?: {
    Name?: string;
    Observers?: Record<
      string,
      { Type?: string; TrackAvatar?: boolean; RotateWithAvatar?: boolean; Width?: number; Height?: number } | null
    >;
    Player?: { AvatarObject?: string };
  };
};

/** The GDY facts the player page needs: cameras (render observers) and the avatar object name. */
export function parseGdy(gdy: string): { name: string; avatar: string; cameras: Camera[] } {
  const env = (parse(gdy) as GdyDoc)?.Environment ?? {};
  // Render observers: the built-in Sprite2D / Isometric (whole map) and any named SPRITE_2D /
  // ISOMETRIC observer (typically avatar-tracking, optionally rotating with the avatar).
  const kindOf = (name: string, type?: string): Camera["kind"] | null => {
    const t = (type ?? name).toUpperCase().replace(/[^A-Z0-9]/g, "");
    if (t === "SPRITE2D") return "sprite";
    if (t === "ISOMETRIC") return "isometric";
    return null;
  };
  const cameras: Camera[] = [];
  for (const [name, o] of Object.entries(env.Observers ?? {})) {
    const builtin = name === "Sprite2D" || name === "Isometric";
    const kind = kindOf(name, builtin ? undefined : o?.Type);
    if (!kind || (!builtin && !o?.Type)) continue;
    const follows = !!o?.TrackAvatar;
    const rotates = follows && !!o?.RotateWithAvatar;
    const iso = kind === "isometric" ? "Isometric · " : "";
    const label = follows
      ? `${iso}${rotates ? "First person" : "Follow"} ${o?.Width ?? "?"}×${o?.Height ?? "?"}`
      : `${iso}Whole map`;
    cameras.push({ name, label, follows, rotates, kind });
  }
  if (!cameras.some((c) => c.name === "Sprite2D" || c.name === "Isometric"))
    cameras.unshift({ name: "Sprite2D", follows: false, rotates: false, kind: "sprite", label: "Whole map" });
  return { name: str(env.Name), avatar: str(env.Player?.AvatarObject), cameras };
}

/** Cheap GDY name lookup without a full YAML parse. */
export const gdyName = (gdy: string): string => /^ {2}Name: *(.+)$/m.exec(gdy)?.[1]?.trim() ?? "";

// ---------------------------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------------------------

/** True when every char of `needle` appears in order in `hay` (loose typo-tolerant match). */
const subsequence = (needle: string, hay: string): boolean => {
  let i = 0;
  for (const ch of hay) if (ch === needle[i]) i++;
  return i === needle.length;
};

/**
 * Fuzzy-ish relevance of a game for a query. Every whitespace-separated term must match somewhere
 * (name, tags, tagline, description; subsequence on the name as a last resort); 0 = no match.
 */
export function searchScore(
  game: Pick<GameMeta, "name" | "tagline" | "tags" | "description" | "id">,
  query: string
): number {
  const terms = query.toLowerCase().split(/\s+/).filter(Boolean);
  if (!terms.length) return 1;
  const name = game.name.toLowerCase();
  const words = name.split(/[^a-z0-9]+/);
  const tags = game.tags.map((t) => t.toLowerCase());
  const tagline = game.tagline.toLowerCase();
  const desc = game.description.toLowerCase();
  let total = 0;
  for (const t of terms) {
    let s = 0;
    if (words.some((w) => w.startsWith(t)) || game.id.startsWith(t)) s = 10;
    else if (name.includes(t)) s = 8;
    else if (tags.some((tag) => tag.startsWith(t))) s = 6;
    else if (tagline.includes(t)) s = 4;
    else if (desc.includes(t)) s = 2;
    else if (t.length >= 3 && subsequence(t, name.replace(/\s+/g, ""))) s = 1;
    if (!s) return 0;
    total += s;
  }
  return total;
}
