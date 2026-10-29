const assert=require('assert/strict'),a=require('../../.codex/local-test/t272/bridge.node');
const b=new ArrayBuffer(128),w=new Uint32Array(b),v=new DataView(b);w[0]=1129534023;w[1]=1;w[4]=1;w[5]=77;w[6]=7;w[8]=2;w[9]=99;w[10]=1;w[12]=3;w[13]=23;w[14]=1;w[15]=1;w[24]=1;w[28]=4;w[29]=66;v.setFloat64(64,64.5,true);v.setFloat64(72,100,true);v.setFloat64(80,.125,true);
assert.equal(a.decode(b).stage,'OK');assert.equal(a.decode(w).stage,'OK');assert.deepEqual(a.decode(b).words,Array.from(w));
const bad=[[],[b,b],[new ArrayBuffer(124)],[new ArrayBuffer(132)],[new Uint32Array(31)],[new Uint32Array(33)],[new Uint32Array(new ArrayBuffer(132),4,32)],[new Float32Array(32)],[[...w]],[new DataView(b)]];for(const args of bad)assert.notEqual(a.decode(...args).stage,'OK');
const detached=new ArrayBuffer(128);structuredClone(detached,{transfer:[detached]});assert.notEqual(a.decode(detached).stage,'OK');
const old=w[9];w[9]=0;assert.equal(a.decode(b).stage,'PACKET_IDENTITY');w[9]=old;w[31]=1;assert.equal(a.decode(b).stage,'PACKET_FEATURE_RESERVED');w[31]=0;v.setFloat64(80,NaN,true);assert.equal(a.decode(b).stage,'PACKET_RAW_RANGE');v.setFloat64(80,.125,true);
console.log(JSON.stringify({revision:'T272_REAL_NODE_NAPI_V1',passed:17,failed:0,abi:'NODE_NAPI_NOT_OHOS_DEVICE',words32:'BIT_IDENTICAL',device:'NOT_RUN'},null,2));
