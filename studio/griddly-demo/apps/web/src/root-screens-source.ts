import type { RootScreens } from "./root-screens";
import { plainRootScreens } from "./root-screens";

/**
 * The `RootScreens` a capability contributes as the default export of the one module under
 * `./root-screens/` (the design system's translates them and gives them its look), else English
 * and plain. The directory holds that module and nothing else: a second one fails the app at start
 * (a registry test keeps it to one capability).
 *
 * Only `router.tsx` and `routes/__root.tsx` import this module: `import.meta.glob` is Vite's, and
 * bun's test runner has no such thing, so tests provide `RootScreensContext` themselves.
 */
const modules = import.meta.glob<{ default: RootScreens }>(
  ["./root-screens/*.{ts,tsx}", "!./root-screens/*.test.{ts,tsx}"],
  { eager: true }
);

// What `RootScreens` holds: an unknown key is a screen under another name (or a typo), which would
// otherwise leave the plain screen in its place unnoticed.
const KEYS = {
  resolveLocale: true,
  texts: true,
  MissingChunk: true,
  ErrorScreen: true,
  NotFound: true,
  Starter: true,
} satisfies Record<keyof RootScreens, true>;

// A globbed module is unparsed input: its default export is only typed, so check what the frontend
// calls outside React. The screens are left to React, which names a non-component when it renders.
const isRootScreens = (value: unknown): value is RootScreens =>
  // oxlint-disable-next-line anti-slop/no-runtime-typeof -- see above
  typeof value === "object" &&
  value !== null &&
  "resolveLocale" in value &&
  // oxlint-disable-next-line anti-slop/no-runtime-typeof -- see above
  typeof value.resolveLocale === "function" &&
  "texts" in value &&
  // oxlint-disable-next-line anti-slop/no-runtime-typeof -- see above
  typeof value.texts === "function";

const contributed = (): RootScreens => {
  const entries = Object.entries(modules);

  // Two would leave the app's language to file order; an app module beside a capability's counts.
  if (entries.length > 1) {
    throw new Error(
      `root-screens/ must hold one source module, found ${entries.map(([path]) => path.slice(2)).join(", ")}.`
    );
  }

  const [entry] = entries;

  if (entry === undefined) {
    return plainRootScreens;
  }

  const [path, module] = entry;

  const file = path.slice(2);

  if (!isRootScreens(module.default)) {
    throw new TypeError(`${file} must default-export RootScreens.`);
  }

  const unknown = Object.keys(module.default).find(
    (key) => !Object.hasOwn(KEYS, key)
  );

  if (unknown !== undefined) {
    throw new TypeError(
      `${file} default-exports RootScreens with an unknown key, ${unknown}: update tanstack-start, or remove the key.`
    );
  }

  return module.default;
};

export const contributedRootScreens: RootScreens = contributed();
