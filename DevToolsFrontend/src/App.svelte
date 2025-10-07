<script lang="ts">
  import "./app.css";
  import he from "he";
  import { RefreshCcw } from "@lucide/svelte";
  import { Button } from "$lib/components/ui/button";
  import { ScrollArea } from "$lib/components/ui/scroll-area";
  import { InjectorStatus } from "./types";

  let injected = $state(false);
  let injectorStatusText: string | null = $state(null);
  let inspectables: DevToolsListItem[] = $state([]);
  let injectId = 0;
  let listRefreshTimeout: number;

  async function requestList(): Promise<DevToolsListItem[]> {
    if (!("devtools" in window)) throw new Error("DevTools object not found");
    return await devtools
      .performRequest("/json/list?t=" + Date.now())
      .then((val: string) => JSON.parse(val));
  }

  let refreshing = $state(false);
  let refreshId = 0;
  function refreshList(routine: boolean = false) {
    const id = ++refreshId;
    refreshing = true;
    requestList()
      .then((val: DevToolsListItem[]) => {
        if (refreshId != id) return;
        inspectables = val;
      })
      .finally(() => {
        if (refreshId != id) return;
        if (routine)
          listRefreshTimeout = setTimeout(() => refreshList(true), 3000);
        refreshing = false;
      });
  }

  addEventListener("injected", () => {
    injected = true;
    refreshList(true);
  });

  addEventListener("uninjected", () => {
    clearTimeout(listRefreshTimeout);
    injected = false;
    inspectables = [];
    injectId++;
    refreshId++;
  });

  addEventListener("injectorstatus", (e: CustomEvent) => {
    const injectorStatus: InjectorStatus = e.detail;
    switch (injectorStatus) {
      case InjectorStatus.NoProcessFound:
        injectorStatusText = "No process found";
        break;
      case InjectorStatus.ProcessIncompatible:
        injectorStatusText =
          "Process cannot be injected with this version's DevTools";
        break;
      case InjectorStatus.InjectFailed:
        injectorStatusText = "Failed to inject";
        break;
      case InjectorStatus.Injected:
        injectorStatusText = "Injected";
        break;
      default:
        injectorStatusText = null;
        break;
    }
  });

  (async function () {
    await (window as any).CefSharp.BindObjectAsync("devtools");

    devtools.injectionStatus().then((val: boolean) => {
      if (val) {
        injected = true;
        refreshList(true);
      }
    });
  })();
</script>

<div class="select-none">
  {#if injected}
    <div class="flex flex-col max-h-[100vh]">
      <div class="p-4 pb-0 flex">
        <div class="flex-1">
          <h1 class="text-2xl font-thin">Inspectable Contents</h1>
          <p class="text-sm opacity-85">
            Click on a page to open DevTools.<br />
            DevTools will be closed if this window is closed.
          </p>
        </div>
        <Button class="cursor-pointer" disabled={refreshing}>
          <RefreshCcw />
        </Button>
      </div>

      <ScrollArea class="p-4 pb-0 grow-1 min-h-0">
        <div role="menu">
          {#each inspectables as inspectable (inspectable.id)}
            {#if inspectable.webSocketDebuggerUrl}
              {@const title = he.decode(inspectable.title)}
              {@const openDevTools = () =>
                devtools.openDevTools(
                  inspectable.webSocketDebuggerUrl.split("://").pop()!,
                  title,
                )
              }
              <div
                role="menuitem"
                class="p-4 border rounded-lg shadow mb-4 hover:bg-gray-100 cursor-pointer"
                onclick={openDevTools}
                onkeydown={(e) => e.key === "Enter" && openDevTools()}
                tabindex="0"
              >
                <h2 class="text-lg font-bold">{title}</h2>
                <p class="text-xs opacity-85">{inspectable.id}</p>
                <p class="text-xs opacity-85">{inspectable.url}</p>
              </div>
            {/if}
          {:else}
            <p class="text-center">No inspectable view found.</p>
          {/each}
        </div>
      </ScrollArea>
    </div>
  {:else}
    <div class="w-[100vw] h-[100vh] flex items-center justify-center">
      <div class="text-center">
        <h1 class="text-4xl mb-2 font-thin">Waiting to inject</h1>
        <p class="text-sm opacity-85">
          {#if injectorStatusText}
            {injectorStatusText}
          {:else}
            Start Garry's Mod to begin the injection process.
          {/if}
        </p>
      </div>
    </div>
  {/if}
</div>
