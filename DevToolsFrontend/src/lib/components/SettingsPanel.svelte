<script lang="ts">
    import { Checkbox } from "$lib/components/ui/checkbox";
    import { Label } from "$lib/components/ui/label";
    import { disableSandbox, registerAssetAsSecured } from "$lib/settings";
</script>

<div class="flex flex-col gap-4 p-4">
    <div>
        <h1 class="text-2xl font-bold">Settings</h1>
        <p class="text-muted-foreground text-sm">The following settings are not applied to currently running Garry's Mod instances.</p>
    </div>
    <div class="flex items-start gap-3">
        <Checkbox id="disable-sandbox" bind:checked={$disableSandbox} />
        <div class="grid gap-2">
            <Label for="disable-sandbox">Disable sandbox</Label>
            <p class="text-muted-foreground text-sm">
                This will disable the sandbox for all web contents. <strong>Use with caution as it may expose security vulnerabilities.</strong>
            </p>
        </div>
    </div>
    <div class="flex items-start gap-3">
        <Checkbox id="register-asset-as-secured" bind:checked={$registerAssetAsSecured} />
        <div class="grid gap-2">
            <Label for="register-asset-as-secured">Register <code>asset://</code>as secure context</Label>
            <p class="text-muted-foreground text-sm">
                This will register the <code>asset://</code> protocol as a secure context in CEF. <strong>Requires sandbox to be disabled.</strong>
            </p>
        </div>
    </div>
</div>
