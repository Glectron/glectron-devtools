declare interface DevToolsListItem {
  description: string,
  devtoolsFrontendUrl: string,
  id: string,
  title: string,
  type: string,
  url: string,
  webSocketDebuggerUrl: string
}

declare interface DevTools {
  injectionStatus: () => Promise<[import("./types").InjectorStatus, number]>;
  performRequest: (url: string) => Promise<string>;
  openDevTools: (webSocketUrl: string, title?: string) => void;
}

interface WindowEventMap {
  "injected": CustomEvent<int>;
  "uninjected": Event;
  "injectorstatus": CustomEvent<number>;
}

declare global {
  interface Window {
    devtools: DevTools;
  }
}

declare const devtools: DevTools;
