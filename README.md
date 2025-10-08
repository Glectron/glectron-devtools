![Glectron](https://raw.githubusercontent.com/Glectron/glectron/main/assets/glectron.svg)
# DevTools
A tool built for debugging Garry's Mod's DHTML panels.

If you are looking for Awesomium support, see [awesomium](https://github.com/Glectron/glectron-devtools/tree/awesomium) branch.

## Usage
1. Start `DevTools.exe` **before** starting Garry's Mod.
2. Start Garry's Mod
3. Find the view you want to inspect in the list and click it to open the DevTools
4. Start debugging!

# Download
Latest version can be downloaded [here](https://github.com/Glectron/glectron-devtools/releases/latest).

## Build
To build it, you will need `Visual Studio 2022`, `Node.JS`.

Make sure you've installed `C++ Desktop Development` and `.NET Desktop Development` workload for your Visual Studio.
This DevTools is built with `C++ 17` and `.NET 6.0`, also make sure you've installed the corresponding components.

We recommend that you should install `Yarn` package manager for your Node, but this is optional.

### Build Steps
1. Install frontend dependencies by executing `yarn install` or `npm install` command inside `DevToolsFrontend` directory.
2. Open the solution file with Visual Studio
3. Build solution

All build artifacts can be found inside `build` folder.