export type Hub = {
  listModules: (cb: (list: string[]) => void) => void;
  moduleUpdated: { connect: (fn: (name: string, data: any) => void) => void };
  moduleError: { connect: (fn: (name: string, err: string) => void) => void };
};
declare global { interface Window { qt?: any; QWebChannel?: any; } }
export function connectWebChannel(): Promise<Hub> {
  return new Promise((resolve, reject) => {
    if (!window.qt || !window.QWebChannel) return reject(new Error('Qt WebChannel non dispo'));
    const QWebChannel = window.QWebChannel;
    new QWebChannel(window.qt.webChannelTransport, (channel: any) => {
      resolve(channel.objects.hub as Hub);
    });
  });
}
