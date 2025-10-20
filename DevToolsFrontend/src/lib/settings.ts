import { writable, type Writable } from "svelte/store";

async function setSetting(key: string, value: any) {
    if (!("devtools" in window)) {
        await (window as any).CefSharp.BindObjectAsync("devtools");
    }
    await devtools.setSetting(key, value);
}

const settingWritable = <T>(key: string, managedKeyName: string, defaultValue: T): Writable<T> => {
    const storedValue = localStorage.getItem(key);
    const initialValue = storedValue !== null ? JSON.parse(storedValue) as T : defaultValue;
    const store = writable<T>(initialValue);
    store.subscribe((value) => {
        localStorage.setItem(key, JSON.stringify(value));
        setSetting(managedKeyName, value);
    });
    return store;
}

export const disableSandbox = settingWritable("disableSandbox", "DisableSandbox", false);
export const registerAssetAsSecured = settingWritable("registerAssetAsSecured", "RegisterAssetAsSecured", false);