export interface DevToolsListItem {
  description: string,
  devtoolsFrontendUrl: string,
  id: string,
  title: string,
  type: string,
  url: string,
  webSocketDebuggerUrl: string
}

export interface AwesomiumDevToolsListItem {
  devtoolsFrontendUrl: string,
  faviconUrl: string,
  processId: string,
  routingId: string,
  sessionId: string,
  thumbnailUrl: string,
  title: string,
  url: string,
  versionStr: string,
  webSocketDebuggerUrl: string
}