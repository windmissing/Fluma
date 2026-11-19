from pathlib import Path
import sys,json,shutil
sys.path.insert(0,str(Path(__file__).parents[1]/'offline_handoff'))
from clone import clone,ROOT
out=clone(ROOT/'.codex/local-test/t582-g9-final',sys.argv[1]);main=out/'entry/src/main'
shutil.copyfile(ROOT/'test/t669/T669Probe.ets',main/'ets/pages/T669Probe.ets')
(main/'ets/entryability/T669ProbeAbility.ets').write_text("import UIAbility from '@ohos.app.ability.UIAbility';\nimport window from '@ohos.window';\nexport default class T669ProbeAbility extends UIAbility { onWindowStageCreate(stage: window.WindowStage): void { stage.loadContent('pages/T669Probe'); } }\n")
p=main/'module.json5';d=json.loads(p.read_text());d['module']['abilities'][0]['srcEntry']='./ets/entryability/T669ProbeAbility.ets';p.write_text(json.dumps(d,indent=2))
p=main/'resources/base/profile/main_pages.json';d=json.loads(p.read_text());d['src'].append('pages/T669Probe');p.write_text(json.dumps(d,indent=2))
print(out)
