# griddly-demo

A [Mistral Studio app](https://docs.mistral.ai/studio/apps/quickstart) hosting a
basic in-browser demo of the [Griddly](https://github.com/bam4d/Griddly) game
engine, running as WebAssembly via the `@griddly/wasm` package built in this repo
(`js/griddly-wasm`).

## What it does

- `apps/web` — a Vite + TanStack Start + React + TypeScript frontend.
- `/` — games library: search (`/` focuses, Enter opens the first match), tag / difficulty filters,
  sorting, and engine-rendered thumbnails. The catalog (`src/games/catalog.ts`) bundles every
  remastered game in `src/games/remastered/<id>.{yaml,json}` (written by `tools/remaster/`) plus
  Spiders Garden (`src/games/minigrid-spiders-garden.yaml`).
- `/play/$gameId` — player for any catalog game: level cards, HUD from the game's metadata, d-pad /
  arrows / WASD / swipe, extra actions, cameras = the GDY's render observers (C cycles), win/lose from
  the GDY's termination. `/demo` redirects to `/play/spiders`.

## Develop locally

```bash
mistral dev            # full local stack behind a gateway
# or just the frontend:
bun run dev:web        # from this directory (nx runs apps/web dev)
```

## Deploy to Studio

The app lives in the Griddly repo and deploys from this folder (no separate repo):

```bash
cd studio/griddly-demo
bun run deploy   # tools/deploy.sh: sync js/griddly-wasm in, bun install, mistral deploy --from-disk, clean up
```

`mistral.json` is bound to the existing `griddly-demo` app in Studio, so this updates it in
place. The deploy uploads only this folder (and skips git-ignored files), so `tools/deploy.sh`
copies `../../js/griddly-wasm` into `packages/ts/griddly-wasm` for the upload and deletes it
afterwards. For local dev run `bun run sync-wasm` first, and don't commit `packages/ts/`.

See https://docs.mistral.ai/studio/apps/deploy for the full flow.
