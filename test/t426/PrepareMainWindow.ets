import window from '@ohos.window';
// EntryAbility owns this reference. Pages never select an unrelated last window.
export class PrepareMainWindow {
 static value:window.Window|null=null;
 static generation:number=0;
 static set(value:window.Window|null):void {PrepareMainWindow.value=value;PrepareMainWindow.generation++;}
}
