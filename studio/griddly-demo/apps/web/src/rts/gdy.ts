/**
 * What the RTS page reads from the GDY itself: the levels, and each external action's inputs with
 * their descriptions and suggested `KeyBinding`s ("ArrowLeft", "w", or chords like "b+a"). The UI
 * turns key presses into action ids with these instead of hard-coding them.
 */
import { parse } from "yaml";

import { parseLevels } from "../games/gdy-meta";
import type { LevelInfo } from "../games/gdy-meta";
import rtsGdy from "./GriddlyRTS-FoW.yaml?raw";

export const RTS_GDY: string = rtsGdy;

export type ActionInput = { id: number; description: string; key: string | null; dx: number; dy: number };
export type ActionInfo = { name: string; internal: boolean; inputs: ActionInput[] };

type RawInput = { Description?: string; KeyBinding?: string; VectorToDest?: [number, number] };
type RawAction = { Name: string; InputMapping?: { Internal?: boolean; Inputs?: Record<string, RawInput> } };

export function parseActions(gdy: string): Record<string, ActionInfo> {
  const doc = parse(gdy) as { Actions?: RawAction[] };
  const out: Record<string, ActionInfo> = {};
  for (const a of doc.Actions ?? []) {
    const inputs = Object.entries(a.InputMapping?.Inputs ?? {}).map(([id, i]) => ({
      id: Number(id),
      description: i.Description ?? `${a.Name} ${id}`,
      key: i.KeyBinding ?? null,
      dx: i.VectorToDest?.[0] ?? 0,
      dy: i.VectorToDest?.[1] ?? 0,
    }));
    out[a.Name] = { name: a.Name, internal: a.InputMapping?.Internal ?? false, inputs };
  }
  return out;
}

/**
 * The command key of an action: the shared first key of its chord bindings ("b+a" -> "b"), or null
 * for single-key bindings (those map straight to one action id, see `keyToActionId`).
 */
export function commandKey(info: ActionInfo | undefined): string | null {
  const keys = (info?.inputs ?? []).map((i) => i.key).filter((k): k is string => !!k && k.includes("+"));
  const first = keys.map((k) => k.split("+")[0]!);
  return first.length && first.every((k) => k === first[0]) ? first[0]! : null;
}

/** The action id a plain key is bound to for `action` (e.g. move_camera + ArrowLeft -> 1). */
export function keyToActionId(info: ActionInfo | undefined, key: string): number | null {
  const hit = info?.inputs.find((i) => i.key !== null && i.key.toLowerCase() === key.toLowerCase());
  return hit ? hit.id : null;
}

/** The action id of a chord `prefix+key` (e.g. research + "r+3" -> 3). */
export function chordToActionId(info: ActionInfo | undefined, prefix: string, key: string): number | null {
  const want = `${prefix}+${key}`.toLowerCase();
  const hit = info?.inputs.find((i) => i.key?.toLowerCase() === want);
  return hit ? hit.id : null;
}

export const RTS_ACTIONS = parseActions(RTS_GDY);
export const RTS_LEVELS: LevelInfo[] = parseLevels(RTS_GDY);
