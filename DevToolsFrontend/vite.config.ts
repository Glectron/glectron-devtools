import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

// https://vitejs.dev/config/
export default defineConfig(({mode}) => {
  return {
    base: "./",
    plugins: [svelte()],
    build: {
      outDir: `../build/${process.env.ARCH === "x86" ? "x86" : "x64"}/${mode === "production" ? "Release" : "Debug"}/frontend`
    }
  };
})
