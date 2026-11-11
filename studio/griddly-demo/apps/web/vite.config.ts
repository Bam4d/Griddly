import { existsSync, readdirSync } from "node:fs";
import { fileURLToPath } from "node:url";

import tailwindcss from "@tailwindcss/vite";
import { tanstackStart } from "@tanstack/react-start/plugin/vite";
import viteReact from "@vitejs/plugin-react";
import { nitro } from "nitro/vite";
import { defineConfig, loadEnv, runnerImport } from "vite";
import type { Plugin } from "vite";

/**
 * Where other capabilities plug into this build: every module in `vite-plugins/` default-exports
 * one Vite `Plugin`, or an array of them for a library that ships its plugin as several, and all of
 * them are appended after the stock plugins, in filename order. A plugin changes settings through
 * its own `config()` hook, which Vite merges; one that must run before or after the others says so
 * with `enforce`. Nothing here names a capability. The directory is read once, when the config
 * loads: restart the dev server after adding or editing a plugin.
 */
const PLUGINS_DIR = fileURLToPath(new URL("vite-plugins/", import.meta.url));

/** The generated app's root, whose `.env` holds `APP_NAME`. */
const WORKSPACE_ROOT = fileURLToPath(new URL("../../", import.meta.url));

/**
 * Loaded through Vite's module runner rather than a bare `import()`, so a plugin file is TypeScript
 * whatever Node version runs the build.
 */
const contributedPlugins = async (): Promise<Plugin[]> => {
  if (!existsSync(PLUGINS_DIR)) {
    return [];
  }

  const files = readdirSync(PLUGINS_DIR)
    .filter(
      (file) =>
        /\.[cm]?[jt]s$/u.test(file) && !/\.(?:test|d)\.[cm]?[jt]s$/u.test(file)
    )
    .toSorted();

  const contributed = await Promise.all(
    files.map(async (file) => {
      const { module } = await runnerImport<{ default?: Plugin | Plugin[] }>(
        `${PLUGINS_DIR}${file}`
      );

      const plugins = [module.default ?? []].flat();

      // A file here that exports no plugin is a mistake, not an opt-out: Vite would drop it silently.
      if (
        plugins.length === 0 ||
        plugins.some(
          (plugin) => plugin.name === undefined || plugin.name === ""
        )
      ) {
        throw new Error(
          `vite-plugins/${file} must default-export a Vite plugin, or an array of them.`
        );
      }

      return plugins;
    })
  );

  return contributed.flat();
};

export default defineConfig(async ({ mode }) => ({
  define: {
    // `src/app-name.ts` reads this, and falls back to a default name when it is empty. A variable
    // already in the environment (the web image's `APP_NAME` build arg) wins over the file.
    "import.meta.env.APP_NAME": JSON.stringify(
      loadEnv(mode, WORKSPACE_ROOT, "APP_NAME").APP_NAME ?? ""
    ),
  },
  server: {
    port: 3001,
  },
  resolve: {
    tsconfigPaths: true,
  },
  plugins: [
    tailwindcss(),
    tanstackStart(),
    nitro({
      // Brotli and gzip copies of each compressible build asset over 1 KB, which the server sends
      // to a browser that accepts them: `node .output/server/index.mjs` compresses nothing on the
      // fly, so without them a page's JS and CSS cross the wire uncompressed.
      compressPublicAssets: { brotli: true, gzip: true },
    }),
    viteReact({ compiler: true }),
    ...(await contributedPlugins()),
  ],
}));
