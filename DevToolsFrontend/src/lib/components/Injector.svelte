<script lang="ts">
    import he from "he";
    import {
        RefreshCcw,
        FileQuestionMark,
        PanelsTopLeft,
        Cog,
    } from "@lucide/svelte";
    import { Button } from "$lib/components/ui/button";
    import { ScrollArea } from "$lib/components/ui/scroll-area";
    import { InjectorStatus } from "../../types";
    import { onMount } from "svelte";

    const {
        injectorState,
    }: {
        injectorState: InjectorState;
    } = $props();

    let inspectables: DevToolsListItem[] = $state([]);
    let listRefreshTimeout: number;
    let statusText = $state("");

    $effect(() => {
        switch (injectorState.Status) {
            case InjectorStatus.NotInjected:
                statusText = "This process is not injected. To inject, restart the process.";
                break;
            case InjectorStatus.InjectFailed:
                statusText = "Injection failed.";
                break;
            case InjectorStatus.NotInjectable:
                statusText = "This process is not injectable.";
                break;
            case InjectorStatus.NoAvailablePort:
                statusText = "No available debugging port found. Restart the process to try again.";
                break;
            default:
                statusText = "";
                break;
        }
    });

    async function requestList(): Promise<DevToolsListItem[]> {
        return await devtools
            .performRequest(injectorState.ProcessId, "/json?t=" + Date.now())
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
                    listRefreshTimeout = setTimeout(
                        () => refreshList(true),
                        3000,
                    );
                refreshing = false;
            });
    }

    onMount(() => {
        refreshList(true);
    });

    let lastPid = injectorState.ProcessId;
    $effect(() => {
        void injectorState.ProcessId; // depend on ProcessId
        if (lastPid === injectorState.ProcessId) return;
        lastPid = injectorState.ProcessId;
        inspectables = [];
        refreshList();
    });
</script>

{#if statusText}
    <div class="w-full h-full flex items-center justify-center p-4 text-center">
        <p class="opacity-85">{statusText}</p>
    </div>
{:else}
    <div class="flex flex-col max-h-[100vh]">
        <div class="p-4 pb-0 flex">
            <div class="flex-1">
                <h1 class="text-2xl font-thin">Inspectable Contents</h1>
                <p class="text-sm opacity-85">
                    Click on a page to open DevTools.<br />
                    DevTools will be closed if this window is closed.
                </p>
            </div>
            <div class="flex flex-col items-end">
                {#if injectorState.DebuggingPort > 0}
                    <p class="text-xs opacity-85 my-2">
                        Remote Debugging at {injectorState.DebuggingPort}
                    </p>
                {:else}
                    <p class="text-xs opacity-85 my-2">
                        Remote Debugging not available
                    </p>
                {/if}
                <Button class="cursor-pointer" disabled={refreshing}>
                    <RefreshCcw />
                </Button>
            </div>
        </div>
        <ScrollArea class="p-4 pb-0 grow-1 min-h-0">
            <div role="menu">
                {#each inspectables as inspectable (inspectable.id)}
                    {#if inspectable.webSocketDebuggerUrl}
                        {@const Title =
                            {
                                page: PanelsTopLeft,
                                worker: Cog,
                            }[inspectable.type] || FileQuestionMark}
                        {@const title = he.decode(inspectable.title)}
                        {@const openDevTools = () =>
                            devtools.openDevTools(
                                injectorState.ProcessId,
                                inspectable.id,
                                inspectable.webSocketDebuggerUrl
                                    .split("://")
                                    .pop()!,
                                title,
                            )}
                        <div
                            role="menuitem"
                            class="p-4 border rounded-lg shadow mb-4 hover:bg-gray-100 cursor-pointer"
                            onclick={openDevTools}
                            onkeydown={(e) =>
                                e.key === "Enter" && openDevTools()}
                            tabindex="0"
                        >
                            <h2
                                class="flex gap-1 items-center text-lg font-bold overflow-hidden"
                                class:italic={!title}
                            >
                                <Title class="size-6 shrink-0" /><span
                                    class="truncate pr-1"
                                    >{title || "Untitled"}</span
                                >
                            </h2>
                            <p class="text-xs opacity-85 truncate">
                                {inspectable.id}
                            </p>
                            <p class="text-xs opacity-85 truncate">
                                {inspectable.url}
                            </p>
                        </div>
                    {/if}
                {:else}
                    <p class="text-center">No inspectable view found.</p>
                {/each}
            </div>
        </ScrollArea>
    </div>
{/if}
