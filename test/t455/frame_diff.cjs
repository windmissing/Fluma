const fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
let s=fs.readFileSync('test/t432/prepare_diff.cjs','utf8').replaceAll('\r\n','\n');
function one(a,b){if(s.split(a).length!==2)throw Error('T432_HARNESS_DRIFT '+a);s=s.replace(a,b);}
one('  const p = new Page();\n  p.getUIContext = () => ({',`  const p = new Page();
  const observers=new Map();
  const visual={anchor:true};
  const observer={on(name,cb){assert(!observers.has(name));observers.set(name,cb);},
    off(name,cb){assert.equal(observers.get(name),cb);observers.delete(name);}};
  const ui = ({`);
one('getComponentUtils: () => ({ getRectangleById: () => box }), postFrameCallback: cb => queued.push(cb) });',`getComponentUtils: () => ({ getRectangleById: id => id.startsWith('t449-action-')
      ? {size:{width:visual.anchor&&!p.configuring?4:0,height:4},windowOffset:{x:0,y:0}} : box }),
    postFrameCallback: cb => queued.push(cb),getUIObserver:()=>observer });
  p.getUIContext = () => ui;`);
one("    flush() { const cb = queued.find(q => typeof q.onIdle === 'function'); assert(cb); queued.splice(queued.indexOf(cb), 1); cb.onFrame(++frame); cb.onIdle(2000000); },",`    visual,observers,
    frame(ns=++frame) { const cb=p.prepareFrame;if(!cb)return;assert(queued.includes(cb));queued.splice(queued.indexOf(cb),1);cb.onFrame(ns); },
    layout(){observers.get('didLayout')?.();},draw(){observers.get('willDraw')?.();},
    flush(){this.frame();this.layout();this.draw();},`);
one('oldFrame.onFrame(500); oldFrame.onIdle(2e6);', 'oldFrame.onFrame(500);');
one('x.p.prepare(); x.flush(); x.flush();\n    x.p.layout.globalX = 110;',
    'x.p.prepare(); x.flush();\n    x.p.prepareLayoutChanged();\n    x.p.layout.globalX = 110;');
s=s.replaceAll('t426-prepare-', 't455-prepare-');
s=s.replaceAll('test/reports/t432','test/reports/t455').replaceAll('t432_prepare_diff.json','frame-differential.json');
s=s.replace('T432_INDEPENDENT_PREPARE_DIFF_V1','T455_T432_SEMANTIC_REGRESSION_V1');
const at=s.indexOf('  global.setTimeout = realTimeout; global.clearTimeout = realClear;');if(at<0)throw Error('T432_OUTPUT_ANCHOR');
s=s.slice(0,at)+`
  {
    const x=page();x.p.prepare();x.draw();x.frame();x.draw();
    record('T455_FRAME_WITHOUT_LAYOUT',0,x.p.prepareObservation.stages[4].count);
    x.visual.anchor=false;x.layout();x.frame();x.draw();
    record('T455_LATE_ANCHOR',0,x.p.prepareObservation.stages[4].count);
    x.visual.anchor=true;x.layout();x.frame();x.draw();x.draw();
    record('T455_DUPLICATE_DRAW',1,x.p.prepareObservation.stages[4].count);
    x.frame();x.draw();
    record('T455_SAME_LAYOUT_SECOND_FRAME','READY',x.p.prepareGate.state);
    record('T455_READY_OBSERVERS_DETACHED',0,x.observers.size);
    x.visual.anchor=false;
    const c=x.p.singleSand;const sequence=c.t394.sequence;
    const refusal=c.reserveCapture(new Int32Array(12),'',1000);
    record('T455_ANCHOR_DRIFT_REJECT','REJECTED',refusal.state);
    record('T455_ANCHOR_DRIFT_ZERO_PHASE',sequence,c.t394.sequence);
  }
  {
    const x=page();x.p.prepare();const oldDraw=x.observers.get('willDraw');const oldLayout=x.observers.get('didLayout');
    x.p.onPageHide();x.p.onPageShow();x.p.prepare();const cb=x.p.prepareFrame;
    oldDraw();oldLayout();record('T455_OLD_OBSERVER_CANNOT_CLEAR_NEW_SLOT',true,x.p.prepareFrame===cb);
    now=JSON.parse(x.p.prepareGate.report()).startedMonoMs+3000;timers.at(-1).fn();
    record('T455_TIMEOUT_MISSING_FRAME','PREPARE_TIMEOUT:WAIT_FRAME_ENTER',x.p.prepareGate.reason);
  }
  {
    const x=page();x.p.prepare();x.frame();now=JSON.parse(x.p.prepareGate.report()).startedMonoMs+3000;timers.at(-1).fn();
    record('T455_TIMEOUT_MISSING_LAYOUT','PREPARE_TIMEOUT:WAIT_LAYOUT_DONE',x.p.prepareGate.reason);
    const y=page();y.p.prepare();y.layout();y.frame();now=JSON.parse(y.p.prepareGate.report()).startedMonoMs+3000;timers.at(-1).fn();
    record('T455_TIMEOUT_MISSING_DRAW','PREPARE_TIMEOUT:WAIT_DRAW_PAIR',y.p.prepareGate.reason);
  }
`+s.slice(at);
const mod=new Module(__filename);mod.filename=__filename;mod.paths=module.paths;mod._compile(s,__filename);
