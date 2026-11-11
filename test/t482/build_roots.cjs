const fs = require('node:fs');
const cp = require('node:child_process');
const path = require('node:path');
const out = path.resolve(process.argv[2]);
if (!process.argv[2] || fs.existsSync(out)) throw Error('FRESH_OUTPUT_REQUIRED');
fs.mkdirSync(out, { recursive: true });
const forward = out.replaceAll('\\', '/');
const fixture = fs.readFileSync('test/t452/source_host.cpp', 'utf8')
  .replaceAll('.codex/local-test/t452-host', forward);
fs.writeFileSync(path.join(out, 'roots.cpp'), fixture);
const command = fs.readFileSync('test/t452/build_source.cmd', 'utf8')
  .replaceAll('.codex\\local-test\\t452-host', out)
  .replaceAll('.codex/local-test/t452-host', forward)
  .replace('test/t452/source_host.cpp', forward + '/roots.cpp')
  .replace('/Ientry/src/main/cpp/gpu_resident', '/Itest/t452 /Ientry/src/main/cpp/gpu_resident');
fs.writeFileSync(path.join(out, 'build.cmd'), command);
cp.execFileSync('cmd.exe', ['/c', path.join(out, 'build.cmd')], {
  stdio: 'inherit', timeout: 650000
});
