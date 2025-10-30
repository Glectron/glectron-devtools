declare interface DevToolsListItem {
  description: string,
  devtoolsFrontendUrl: string,
  id: string,
  title: string,
  type: string,
  url: string,
  webSocketDebuggerUrl: string
}

declare interface InjectorState {
  ProcessId: number;
  Status: import("./types").InjectorStatus;
  Title: string | null;
  DebuggingPort: number;
}

declare interface DevTools {
  getInjectorState(): Promise<InjectorState[]>;
  performRequest(processId: number, url: string): Promise<string>;
  openDevTools(processId: number, id: string, webSocketUrl: string, title?: string): Promise<void>;
  setDevToolsTitle(processId: number, id: string, title: string): Promise<void>;
  setSetting(key: string, value: any): Promise<void>;
}

interface WindowEventMap {
  "updateinjectorstate": Event;
}

declare global {
  interface Window {
    devtools: DevTools;
  }
}

declare const devtools: DevTools;
