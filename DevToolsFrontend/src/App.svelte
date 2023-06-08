<script lang="ts">
  import type { DevToolsListItem } from "./devtools";
  import AppStatus from "./lib/AppStatus.svelte";

  let injected = false;
  let injectedProcess = 0;
  let injectorStatus = 0;
  let injectorStatusText: string | null = null;
  
  let injectId = 0;
  let lists = [];
  let listRefreshTimeout: number;

  async function requestList() {
    return new Promise((resolve, reject) => {
      if ((window as any).devtools?.performRequest) {
        (window as any).devtools.performRequest((injectedProcess == 0 ? "/json/list" : "/json") + "?t=" + new Date().valueOf()).then((val: string) => JSON.parse(val)).then(resolve).catch(reject);
      } else {
        reject();
      }
    });
  }

  function refreshList(routine: boolean = false) {
    let id = injectId;
    requestList().then((val: DevToolsListItem[]) => {
      if (injectId != id) return;
      lists = val.filter((x) => x.title.indexOf("\u2061") != -1).map((x) => {
        return {
          devtoolsUrl: "http://127.0.0.1:46587" +  x.devtoolsFrontendUrl,
          id: x.id || x.sessionId,
          title: x.title.substring(0, x.title.length - 1)
        }
      });
    }).finally(() => {
      if (routine && injectId == id)
        listRefreshTimeout = setTimeout(() => refreshList(true), 5000);
    });
  }

  addEventListener("injected", (e: CustomEvent) => {
    injected = true;
    injectedProcess = e.detail;
    refreshList(true);
  });

  addEventListener("uninjected", () => {
    clearTimeout(listRefreshTimeout);
    injected = false;
    lists = [];
    injectId++;
  });

  addEventListener("injectorstatus", (e: CustomEvent) => {
    injectorStatus = e.detail;
    switch (injectorStatus) {
      case 0:
        injectorStatusText = "No process found";
        break;
      case 1:
        injectorStatusText = "Process cannot be injected with this version's DevTools";
        break;
      case 2:
        injectorStatusText = "Failed to inject";
        break;
      case 3:
        injectorStatusText = "Injected";
        break;
      default:
        injectorStatusText = null;
        break;
    }
  });

  (async function()
  {
    await (window as any).CefSharp.BindObjectAsync("devtools");

    (window as any).devtools.injectionStatus().then((val: {Item1: boolean, Item2: number}) => {
      if (val?.Item1) {
        injected = true;
        injectedProcess = val.Item2;
        refreshList(true);
      }
    });
  })();
</script>

<style lang="scss">
  .app-list {
    padding: 0;

    .app {
      width: 100%;

      a {
        color: black;
        text-decoration: none;
      }

      .app-item {
        padding: 5px 10px;
        border-top: 1px solid rgb(59, 59, 59);
        border-bottom: 1px solid rgb(59, 59, 59);
        
        h2 {
          font-size: 1.2em;
          text-overflow: ellipsis;
          overflow: hidden;
          white-space: nowrap;
          margin-bottom: 0;
        }

        p {
          margin-top: 0;
          text-overflow: ellipsis;
          overflow: hidden;
          white-space: nowrap;
          opacity: .5;
        }

        &:hover {
          background-color: rgb(201, 240, 253);
        }
      }
    }
  }
  .refresh-btn {
    cursor: pointer;
    background: none;
    outline: none;
    border: 1px solid rgb(0, 120, 160);
    padding: 5px 15px;
    border-radius: 6px;
    color: rgb(0, 120, 160);

    &:hover {
      background-color: rgb(0, 120, 160);
      color: white;
    }
  }
</style>

{#if injected}
<h1 style="margin-top: 0;">Running Glectron Apps</h1>
<p>Applications can only be visible while Glectron is in debug mode.</p>
{#if lists.length > 0}
<ul class="app-list">
  {#each lists as item}
    <li class="app">
      <a href={item.devtoolsUrl}>
        <div class="app-item">
          <h2>{item.title}</h2>
          <p>{item.id}</p>
        </div>
      </a>
    </li>
  {/each}
</ul>
{:else}
<p style="text-align: center;">No Glectron application is running</p>
{/if}
<p style="text-align: center;"><button on:click={() => refreshList()} class="refresh-btn">Refresh</button></p>
{:else}
<AppStatus message="Waiting to inject" description="Run Garry's Mod to start the browser view injection" error={injectorStatus == 0 || injectorStatus == 3 ? undefined : injectorStatusText}/>
{/if}