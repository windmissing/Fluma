const fs = require('node:fs');
const cp = require('node:child_process');
const path = require('node:path');
const project = path.resolve(process.argv[2] || '');
const out = path.resolve(process.argv[3] || '');
if (!process.argv[2] || !process.argv[3] || fs.existsSync(out)) {
  throw Error('EXPLICIT_PROJECT_AND_FRESH_OUTPUT_REQUIRED');
}
fs.mkdirSync(out, { recursive: true });
const sourceRoot = project.replaceAll('\\', '/');
const outputRoot = out.replaceAll('\\', '/');
let source = fs.readFileSync('test/t481/recovery_diff.cpp', 'utf8');
source = source.replace('../../.codex/local-test/t467-unit', sourceRoot)
  .replace('test/reports/t481/t481-recovery-diff.json', outputRoot + '/recovery-diff.json');
fs.writeFileSync(path.join(out, 'recovery.cpp'), source);
let command = fs.readFileSync('test/t481/build.cmd', 'utf8');
command = command.replaceAll('.codex/local-test/t467-unit', sourceRoot)
  .replaceAll('.codex\\local-test\\t481', out)
  .replaceAll('.codex/local-test/t481', outputRoot)
  .replace('test/t481/recovery_diff.cpp', outputRoot + '/recovery.cpp');
command = command.replace('exit /b 0', '"' + path.join(out, 't481.exe') + '"\r\nexit /b %errorlevel%');
fs.writeFileSync(path.join(out, 'build.cmd'), command);
cp.execFileSync('cmd.exe', ['/c', path.join(out, 'build.cmd')], {
  stdio: 'inherit', timeout: 300000
});
