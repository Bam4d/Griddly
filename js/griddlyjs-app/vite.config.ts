import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  base: "./",
  plugins: [react()],
  css: {
    preprocessorOptions: {
      scss: {
        loadPaths: ["node_modules"],
      },
    },
  },
  build: {
    outDir: "build",
    chunkSizeWarningLimit: 4000,
  },
});
