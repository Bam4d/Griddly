declare const process: { readonly env: Record<string, string | undefined> };

interface ImportMetaEnv {
  /** The app's display name, defined at build time by `vite.config.ts`; see `src/app-name.ts`. */
  readonly APP_NAME?: string;
}
