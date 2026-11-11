'use strict';
const fs = require('fs'), path = require('path'), cp = require('child_process'), crypto = require('crypto');
const root = path.resolve(__dirname, '../..'), out = path.resolve(process.argv[2]);
cp.execFileSync(process.execPath, [require.resolve('../t468/generate.cjs'), out], { stdio: 'inherit' });
const main = path.join(out, 'entry/src/main'), base = path.join(main, 'cpp/gpu_resident'), edits = [];
function edit(rel, fn) { const p = path.join(main, rel), s = fs.readFileSync(p, 'utf8').replaceAll('\r\n', '\n'), t = fn(s); fs.writeFileSync(p, t); edits.push({ file: rel, before: hash(s), after: hash(t) }); }
function hash(s) { return crypto.createHash('sha256').update(s).digest('hex'); }
function one(s, a, b) { if (s.split(a).length !== 2)
    throw Error('T475_ANCHOR ' + a); return s.replace(a, b); }
// Replay the two recovery overlays on top of the diagnostic M2 generator.
function overlay(file, anchor) {
    const source = fs.readFileSync(file, 'utf8');
    const body = source.slice(source.indexOf(anchor)).replaceAll(' m1::ProgressClockV2 progress;', ' m2route::ProgressClockV2 progress;');
    const scopedRequire = require('module').createRequire(file);
    new Function('fs', 'path', 'cp', 'out', 'main', 'base', 'cpp', 'target', 'profile', '__dirname', 'require', body)(fs, path, cp, out, main, base, path.dirname(base), 'experience', 'experiment', path.dirname(file), scopedRequire);
}
overlay(require.resolve('../t456/generate.cjs'), 'const local=');
overlay(require.resolve('../t467/generate.cjs'), 'const read=');
edit('cpp/gpu_resident/app_bridge.cpp', s => s.replaceAll('m1::NonceOf(', 'm2route::NonceOf('));
edit('cpp/gpu_resident/m2route/full_transport.cpp', s => {
    const begin = s.indexOf('void FullM1Transport::FlushBatch('), end = s.indexOf('TransportCode FullM1Transport::ObserveBatch', begin);
    if (begin < 0 || end < 0)
        throw Error('T475_RECOVERY_FLUSH_SITE');
    const body = s.slice(begin, end);
    const patched = one(one(body, ' glFlush();', ' tailObserver.Emit({TailStage(141),(uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo,before,0,from,passCursor,0,uint32_t(final),batchesIssued});\n glFlush();'), ' const auto after=tailObserver.Stamp();', ' const auto after=tailObserver.Stamp();\n tailObserver.Emit({TailStage(142),(uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo,before,after,from,passCursor,0,uint32_t(final),batchesIssued});');
    return s.slice(0, begin) + patched + s.slice(end);
});
edit('cpp/gpu_resident/m2route/source_owner.h', s => one(s, ' bool HasLeases()const', ' bool RetirementLease(size_t slot,U64& generation)const{if(slot>=2)return false;const auto& p=pubs[slot];generation=p.generation;return p.leased||p.releasePending;}\n bool HasLeases()const'));
edit('ets/pages/GpuSandWaterExperience.ets', s => {
    const seen = new Set();
    return s.split('\n').filter(line => { if (!line.startsWith('import '))
        return true; if (seen.has(line))
        return false; seen.add(line); return true; }).join('\n');
});
const cache = path.join(base, 'program_cache');
fs.mkdirSync(cache, { recursive: true });
for (const n of fs.readdirSync('entry/src/main/cpp/gpu_resident/program_cache'))
    fs.copyFileSync('entry/src/main/cpp/gpu_resident/program_cache/' + n, path.join(cache, n));
for (const route of ['m1', 'm2route']) {
    const layout = fs.readFileSync(path.join(base, route, 'layout_generated.h'), 'utf8');
    for (const [file, object, source, supplied, program, error] of [['source_transport.cpp', 0, 'shader', route === 'm1' ? 'expectedSha' : 'kSourceActualSha', 'program', 'stats.firstGlError'], ['quantum_candidate.cpp', 1, 'QuantumKernel()', 'QuantumSha()', 'program', 'firstGlError'], ['full_transport.cpp', 2, 'QuantumCommitKernel()', route === 'm1' ? 'FullM1ShaderSha()' : 'kFinalActualSha', 'quantum.program', 'stats.firstGlError']]) {
        edit('cpp/gpu_resident/' + route + '/' + file, s => {
            s = '#include "../program_cache/program_factory.h"\n' + s;
            const begin = s.indexOf(route === 'm2route' ? `if(!create468::Build(${object},` : (file === 'full_transport.cpp' ? 'GLuint sh=glCreateShader' : 'auto sh=glCreateShader'));
            const end = s.indexOf('\n ' + (route === 'm2route' && object === 2 ? 'create468::Enter(create468::Stage::ALLOCATE)' : 'SubmitScope resources'), begin);
            if (begin < 0 || end < 0)
                throw Error('PROGRAM_SITE ' + route + '/' + file);
            const failure = file === 'source_transport.cpp' ? 'return fail(TransportCode::SHADER_FAILED);' : 'Release();return TransportCode::SHADER_FAILED;';
            s = s.slice(0, begin) + `if(!programcache::Build(${object},${source},${supplied},"${hash(layout)}",${program},${error},submitObserver)){${failure}}` + s.slice(end);
            if (file === 'source_transport.cpp') {
                const needle = route === 'm1' ? 'ready=true;return TransportCode::OK;' : 'ready=true;return TransportCode::OK;';
                s = one(s, needle, 'programcache::Prepared(program);' + needle);
            }
            else if (file === 'quantum_candidate.cpp') {
                s = one(s, 'resources.End(0);BuildPassTable();', `resources.End(0);programcache::Prepared(program);BuildPassTable();`);
            }
            else
                s = one(s, ' // T341:', ' programcache::Prepared(quantum.program);\n // T341:');
            s = s.replaceAll('programcache::Prepared(program);', 'if(programcache::Prepared(program)!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}').replaceAll('programcache::Prepared(quantum.program);', 'if(programcache::Prepared(quantum.program)!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}');
            s = s.replaceAll('quantum.uniformLocations.Prepare(quantum.program)', 'programcache::Reflect(quantum.uniformLocations,quantum.program)').replaceAll('uniformLocations.Prepare(program)', 'programcache::Reflect(uniformLocations,program)');
            s = s.replace(/((?:create468::Prepare|PrepareProgram)\((program|quantum.program),[^;\n]*?submitObserver\))/g, (_, call, p) => `programcache::ObservePrepare(${p},[&]{return ${call};})`);
            if (file === 'full_transport.cpp')
                s = one(s, 'warmupWallNs=TailObserver::Now()-start;', 'warmupWallNs=TailObserver::Now()-start;programcache::Warmup(quantum.program,start,start+warmupWallNs,error);');
            s = s.replaceAll('if(program)glDeleteProgram(program);', 'if(program){programcache::Forget(program);glDeleteProgram(program);}');
            return s;
        });
    }
}
edit('cpp/gpu_resident/app_bridge.cpp', s => {
    s = '#include "program_cache/program_factory.h"\n' + s;
    s = one(s, 'napi_value setProgressV2(', fs.readFileSync(path.join(__dirname,'napi.inc'),'utf8')+'\nnapi_value setProgressV2(');
    s = one(s, '{"setSource",nullptr,sourceMode', '{"programCacheState",nullptr,programCacheState,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
    return one(s, '{"setSource",nullptr,sourceMode', '{"configureProgramCache",nullptr,configureProgramCache,nullptr,nullptr,nullptr,napi_default,nullptr},{"setSource",nullptr,sourceMode');
});
edit('cpp/types/libgrsm1/index.d.ts', s => s + '\nexport const configureProgramCache:(filesDir:string,packageSha?:string)=>string;\nexport const programCacheState:()=>string;\n');
edit('ets/entryability/EntryAbility.ets', s => one(s, "    initializeM2Package(want.parameters?.gpusha,want.parameters?.gpuelfsha);", "    grsm1.configureProgramCache(this.context.filesDir,String(want.parameters?.gpusha??''));\n    initializeM2Package(want.parameters?.gpusha,want.parameters?.gpuelfsha);"));
fs.writeFileSync(path.join(out, 't475-manifest.json'), JSON.stringify({ revision: 'T475_CACHE_V1', base: ['T468 (T452 M2)', 'T456 boot', 'T467 recovery'],cacheFiles:fs.readdirSync(cache).map(name=>({name,sha256:hash(fs.readFileSync(path.join(cache,name)))})), edits, coverage: ['M1 SOURCE', 'M1 INTERMEDIATE_QUANTUM', 'M1 FINAL_QUANTUM', 'M2 SOURCE', 'M2 INTERMEDIATE_QUANTUM', 'M2 FINAL_QUANTUM'], excluded: ['scene vertex/fragment shaders'], device: 'NOT_RUN' }, null, 2));
console.log(out);
