import type { LandingFallback } from "./landing";

/**
 * Every landing fallback installed in this app, in filename order. Opt in by adding a module under
 * `./landing-fallbacks/` whose default export is a `LandingFallback`.
 *
 * Only `router.tsx` imports this module: `import.meta.glob` is Vite's, and bun's test runner has no
 * such thing, so tests hand `landingPath` their own fallbacks instead.
 */
const modules = import.meta.glob<{ default: LandingFallback }>(
  ["./landing-fallbacks/*.{ts,tsx}", "!./landing-fallbacks/*.test.{ts,tsx}"],
  { eager: true }
);

export const landingFallbacks: readonly LandingFallback[] = Object.entries(
  modules
)
  .toSorted(([left], [right]) => left.localeCompare(right))
  .map(([path, module]) => {
    // oxlint-disable-next-line anti-slop/no-runtime-typeof -- a globbed module is unparsed input: its default export is only typed, so check it is callable
    if (typeof module.default !== "function") {
      throw new TypeError(
        `${path.slice(2)} must default-export a LandingFallback.`
      );
    }

    return module.default;
  });
