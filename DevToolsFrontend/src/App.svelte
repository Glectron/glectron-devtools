<script lang="ts">
  import "./app.css";
  import * as Resizable from "$lib/components/ui/resizable";
  import Injector from "$lib/components/Injector.svelte";
  import { InjectorStatus } from "./types";

  let injectorCount = $state(0);
  let injectorStates = $state<Record<number, InjectorState>>({});
  let currentActiveInjector = $state(0);

  async function refreshInjectorState() {
    await devtools.getInjectorState().then((val) => {
      const states = val.filter(
        (state) => state.Status !== InjectorStatus.IsSubprocess,
      );
      injectorCount = states.length;
      injectorStates = Object.fromEntries(states.map((state) => [state.ProcessId, state]));
      if (!injectorStates[currentActiveInjector] && states.length > 1) {
        currentActiveInjector = states[0].ProcessId;
      }
    });
  }

  addEventListener("updateinjectorstate", () => {
    refreshInjectorState();
  });

  (async function () {
    await (window as any).CefSharp.BindObjectAsync("devtools");

    await refreshInjectorState();
  })();
</script>

<div class="select-none w-[100vw] h-[100vh]">
  {#if injectorCount == 1}
    <Injector injectorState={Object.values(injectorStates)[0]} />
  {:else if injectorCount > 1}
    <Resizable.PaneGroup direction="horizontal">
      <Resizable.Pane defaultSize={25}>
        <div class="flex flex-col gap-2 p-2">
          {#each Object.values(injectorStates) as injectorState (injectorState.ProcessId)}
            {@const active = injectorState.ProcessId == currentActiveInjector}
            <button
              class="text-left p-2 cursor-pointer rounded-lg hover:bg-gray-200 dark:hover:bg-gray-700 overflow-hidden"
              class:shadow-md={active}
              class:border-1={active}
              onclick={() => (currentActiveInjector = injectorState.ProcessId)}
            >
              <h2 class="flex items-center">
                {#if injectorState.Title}
                  {injectorState.Title}<span class="text-xs opacity-70 ml-2">(#{injectorState.ProcessId})</span>
                {:else}
                  Process #{injectorState.ProcessId}
                {/if}
              </h2>
            </button>
          {/each}
        </div>

      </Resizable.Pane>
      <Resizable.Handle />
      <Resizable.Pane defaultSize={75}>
        <Injector injectorState={injectorStates[currentActiveInjector]} />
      </Resizable.Pane>
    </Resizable.PaneGroup>
  {:else}
    <div class="w-[100vw] h-[100vh] flex items-center justify-center">
      <div class="text-center">
        <h1 class="text-4xl mb-2 font-thin">Waiting to inject</h1>
        <p class="text-sm opacity-85">Start Garry's Mod to begin the injection process.</p>
      </div>
    </div>
  {/if}
</div>
