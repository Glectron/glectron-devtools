import tailwindcss from "@tailwindcss/vite";
import path from "path";
import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => {
  return {
    plugins: [tailwindcss(), svelte()],
    build: {
      outDir: `../build/${process.env.ARCH === "x86" ? "x86" : "x64"}/${mode === "production" ? "Release" : "Debug"}/frontend`,
      emptyOutDir: true,
    },
    resolve: { alias: { $lib: path.resolve("./src/lib") } }
  };
});
