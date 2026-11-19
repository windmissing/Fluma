export interface InputStagePort { recordInputStageV1?(code: number, raw: string): string; }
export function recordInputStage(port: InputStagePort, code: number, raw: string): string {
  if (port.recordInputStageV1 === undefined) return 'INPUT_STAGE_API_UNAVAILABLE';
  return port.recordInputStageV1(code, raw);
}
export function recordPageTouch(port: InputStagePort, event: TouchEvent, width: number, height: number,
  layout: number, node: string): void {
  const code: number = event.type === TouchType.Down ? 1 : event.type === TouchType.Move ? 2 :
    event.type === TouchType.Up ? 3 : event.type === TouchType.Cancel ? 4 : 8;
  if (event.changedTouches.length === 0) {
    recordInputStage(port, code, JSON.stringify({ origin: 'PAGE_HANDLER', type: event.type,
      count: 0, rawEventTimestamp: String(event.timestamp), node: node, layout: layout,
      width: width, height: height, coordinateUnit: 'ARKUI_TOUCH_API_RAW' }));
    return;
  }
  const point = event.changedTouches[0];
  recordInputStage(port, code, JSON.stringify({ origin: 'PAGE_HANDLER', type: event.type,
    count: event.changedTouches.length, rawEventTimestamp: String(event.timestamp), pointer: point.id,
    local: [point.x, point.y], window: [point.windowX, point.windowY], screen: [point.screenX, point.screenY],
    node: node, layout: layout, bounds: [width, height], coordinateUnit: 'ARKUI_TOUCH_API_RAW' }));
}
