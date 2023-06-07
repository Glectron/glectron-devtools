<script lang="ts">
    export let iconUrl: string = undefined;
    export let message: string;
    export let description: string = undefined;
    export let error: string = undefined;
</script>

<style lang="scss">
    .container {
        position: fixed;
        top: 0;
        left: 0;
        width: 100vw;
        height: 100vh;
        background-color: white;

        .message {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 80vw;
            text-align: center;
        }
    }
</style>

<div class="container">
    <div class="message">
        {#if iconUrl}
        <img src={iconUrl} alt="Icon"/>
        {/if}
        <h1>{message}</h1>
        {#if description}
        <p>{description}</p>
        {/if}
        {#if error}
        <p style="color: red;font-size: smaller;">{error}</p>
        {/if}
    </div>
</div>