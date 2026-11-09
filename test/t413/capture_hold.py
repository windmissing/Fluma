"""Explicit T413 capture transaction runner. Commands are argv arrays, never shell text."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid
from PIL import Image
from clock import check, envelope


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write_new(path, value):
    with path.open('x', encoding='utf8') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)


def same(s, scope):
    return all(s[k] == scope[k] for k in ('attemptId', 'holdId', 'requestId', 'phaseSequence'))


def safe_local(root, name):
    check(type(name) is str and Path(name).name == name and name not in ('', '.', '..'), 'LOCAL_FILENAME')
    return root/name


def verify_bundle(spec_path):
    spec = read(spec_path);root = spec_path.parent
    receipt = read(safe_local(root, spec['capture']))
    hold_record = read(safe_local(root, spec['hold']))
    check(hold_record['revision'] == 'T413_DISPLAY_HOLD_V2' and hold_record['valid'] is True and
          hold_record['firstFailure'] == '', 'HOLD_INVALID')
    hold = hold_record['hold']
    check(hold['state'] == 'SEALED' and hold['archiveState'] == 'NATIVE_ARCHIVE_SAVED' and hold['holdId'] and
          hold_record['endToken'] == hold['endToken'] == hold['token'] and
          hold_record['endFrame'] == hold['endFrame'] == hold['frame'] and
          hold_record['startMonoMs'] == hold['startMonoMs'] and hold_record['endMonoMs'] == hold['endMonoMs'], 'HOLD_IDENTITY')
    check(receipt['revision'] == 'T413_CAPTURE_CALL_V2' and receipt['state'] == 'COMPLETE' and
          receipt['clock'] == 'PYTHON_MONOTONIC_NS' and same(receipt, hold), 'CAPTURE_TOOL_RECEIPT')
    png = safe_local(root, spec['png'])
    check(png.is_file() and receipt['localPng'] == png.name and png.stat().st_size == receipt['fileBytes'] > 0 and
          receipt['fileSha256'] == sha(png) and len(receipt['fileSha256']) == 64, 'CAPTURE_TOOL_RECEIPT')
    with Image.open(png) as image:
        check(image.format == 'PNG' and image.mode in ('RGB', 'RGBA'), 'LOSSLESS_RGB_REQUIRED');image.verify()
    start, end = receipt['startMonoNs'], receipt['endMonoNs']
    check(type(start) is int and type(end) is int and start <= end and receipt['remotePng'], 'CAPTURE_TOOL_RECEIPT')
    for key in ('screenshot', 'pull'):
        cmd = receipt[key]
        check(cmd['argv'] and cmd['exitCode'] == 0 and start <= cmd['startMonoNs'] <= cmd['endMonoNs'] <= end, 'CAPTURE_TOOL_RECEIPT')
    pre, post = receipt['preStatus'], receipt['postStatus']
    check(pre['state'] == post['state'] == 'HOLD_OPEN' and same(pre, hold) and same(post, hold) and
          pre['actionSerial'] <= post['actionSerial'] < hold['actionSerial'] and
          pre['token'] == post['token'] == hold['token'] and pre['frame'] == post['frame'] == hold['frame'] and
          pre['owner'] == post['owner'] == hold['owner'] and pre['geometry'] == post['geometry'] == hold['geometry'], 'HOLD_IDENTITY')
    check(receipt['sealCommand']['startMonoNs'] > end and receipt['sealCommand']['exitCode'] == 0, 'CAPTURE_INTERVAL_OUTSIDE_HOLD')
    records = read(safe_local(root, spec['clockSamples']))
    for row in records:
        command = read(safe_local(root, row['rawCommand']))
        check(command['exitCode'] == 0 and command['startMonoNs'] == row['hostBeforeNs'] and
              command['endMonoNs'] == row['hostAfterNs'] and json.loads(command['stdout']) == row['sample'], 'CLOCK_MAPPING_UNKNOWN')
    result = envelope(records, start, end, hold)
    native = safe_local(root, spec['manifest'])
    check(sha(native) == hold['manifestSha256'] and native.with_suffix('.sha256').read_text(encoding='utf-8-sig').split() ==
          [hold['manifestSha256'],native.name], 'MANIFEST_SHA')
    m = read(native);c = m['capture'];p = m['probe'];phase = m['phase'];sample = phase['sample']
    check(same(c, hold) and c['geometry'] == hold['geometry'] and c['owner'] == hold['owner'] and
          p['requestId'] == hold['requestId'] and p['frameSerial'] == hold['frame'] and
          phase['sequence'] == hold['phaseSequence'] and sample['token'] == p['requestedToken'] == p['actualToken'] == hold['token'] and
          sample['frame'] == hold['frame'] and phase['hold']['token'] == hold['token'], 'HOLD_IDENTITY')
    check(p['captureStatus'] == 'CAPTURED' and p['captureStage'] == 'COMPLETE' and
          p['presentationState'] == 'CURRENT' and p['swapSucceeded'] and p['unbindSucceeded'], 'NATIVE_EXCHANGE_UNVERIFIED')
    for key in ('candidate', 'back'):
        f = safe_local(root, m[key]['name'])
        check(f.stat().st_size == m[key]['bytes'] and sha(f) == m[key]['sha256'] == p[key]['sha256'], 'NATIVE_RGBA_SHA')
    return result


class Runner:
    def __init__(self, config, output):
        self.config, self.output = config, output
        self.scope = {};self.actions = [];self.clock = [];self.sequence = 0
        self.remote = '/data/local/tmp/t413-'+uuid.uuid4().hex+'.png'

    def command(self, action, parse=True):
        self.sequence += 1
        values = {'output': str(self.output.resolve()), 'remotePng': self.remote,
                  'localPng': str((self.output/'screen.partial.png').resolve()), 'actionId': str(self.sequence), **self.scope}
        argv = [str(s).format_map(values) for s in self.config['commands'][action]]
        before = time.monotonic_ns()
        try:
            result = subprocess.run(argv, capture_output=True, timeout=self.config.get('commandTimeoutSeconds', 30))
            record = {'argv': argv, 'startMonoNs': before, 'endMonoNs': time.monotonic_ns(), 'exitCode': result.returncode,
                      'stdout': result.stdout.decode('utf8', errors='replace'), 'stderr': result.stderr.decode('utf8', errors='replace')}
        except (OSError, subprocess.TimeoutExpired) as error:
            record = {'argv': argv, 'startMonoNs': before, 'endMonoNs': time.monotonic_ns(), 'exitCode': -1,
                      'stdout': '', 'stderr': str(error)}
        name = f'action-{self.sequence:03}-{action}.json'
        write_new(self.output/name, record);self.actions.append(name)
        check(record['exitCode'] == 0, 'COMMAND_FAILED_'+action)
        return (json.loads(record['stdout']) if parse else None), record, name

    def sample(self):
        sample, command, name = self.command('clock')
        self.clock.append({'source': 'APP_SAMPLE_HOLD_CLOCK', 'rawCommand': name,
                           'clockUncertaintySource': self.config['clockUncertaintySource'],
                           'quantizationMs': self.config['clockQuantizationMs'], 'hostBeforeNs': command['startMonoNs'],
                           'hostAfterNs': command['endMonoNs'], 'sample': sample})

    def run(self):
        # Every action's stdout is original JSON from the app, never host-fabricated app timestamps.
        reserved, _, _ = self.command('reserve')
        check(reserved['revision']=='T413_CAPTURE_REPLY_V2' and reserved['state']=='RESERVED' and reserved['attemptId'], 'RESERVE_REJECTED')
        self.scope = {k: reserved[k] for k in ('attemptId', 'holdId', 'requestId', 'phaseSequence')}
        status, _, _ = self.command('status')
        check(status['state'] == 'HOLD_OPEN' and status['geometry']['visible'] and
              all(status[k] == self.scope[k] for k in ('attemptId','requestId','phaseSequence')), 'HOLD_NOT_OPEN')
        self.scope['holdId'] = status['holdId']
        archive, _, _ = self.command('export')
        pre, _, _ = self.command('status')
        check(pre['state'] == 'HOLD_OPEN' and same(pre, self.scope) and pre['archiveState'] == 'NATIVE_ARCHIVE_SAVED' and
              pre['manifestSha256'] == archive['sha256'], 'ARCHIVE_ACK_HOLD')
        check(pre['geometry'] == self.config['spec']['registeredGeometry'], 'UNREGISTERED_OR_OCCLUDED_GEOMETRY')
        self.sample();self.sample()
        png = self.output/'screen.png';partial = self.output/'screen.partial.png'
        check(not png.exists() and not partial.exists(), 'PREEXISTING_PNG')
        start = time.monotonic_ns()
        _, screenshot, _ = self.command('screenshot', False)
        _, pull, _ = self.command('pull', False)
        check(partial.is_file() and partial.stat().st_size > 0, 'PNG_MISSING')
        with Image.open(partial) as image:
            check(image.format == 'PNG' and image.mode in ('RGB', 'RGBA'), 'LOSSLESS_RGB_REQUIRED');image.verify()
        png_hash, png_bytes = sha(partial), partial.stat().st_size
        check(not png.exists(), 'PREEXISTING_PNG')
        partial.rename(png)
        end = time.monotonic_ns()
        self.sample()
        post, _, _ = self.command('status')
        check(post['state'] == 'HOLD_OPEN' and same(post, self.scope) and pre['geometry'] == post['geometry'], 'HOLD_INVALIDATED')
        terminal, seal_command, _ = self.command('seal')
        check(sha(safe_local(self.output, terminal['name'])) == terminal['sha256'], 'TERMINAL_SHA')
        # terminal action must fetch original seal and all export manifest references, no renamed aliases.
        receipt = {'revision': 'T413_CAPTURE_CALL_V2', 'state': 'COMPLETE', 'clock': 'PYTHON_MONOTONIC_NS', **self.scope,
                   'startMonoNs': start, 'endMonoNs': end, 'remotePng': self.remote, 'localPng': png.name,
                   'fileBytes': png_bytes, 'fileSha256': png_hash, 'screenshot': screenshot, 'pull': pull,
                   'preStatus': pre, 'postStatus': post, 'sealCommand': seal_command}
        write_new(self.output/'capture.json', receipt);write_new(self.output/'clock-samples.json', self.clock)
        spec = dict(self.config['spec']);spec.update(manifest=archive['name'], hold=terminal['name'], png='screen.png',
                                                    capture='capture.json', clockSamples='clock-samples.json')
        write_new(self.output/'spec.json', spec)
        result = verify_bundle(self.output/'spec.json')
        self.command('release')
        status, _, _ = self.command('status');check(status['state'] == 'RELEASED', 'RELEASE_FAILED')
        write_new(self.output/'result.json', {'state': 'CAPTURE_BUNDLE_PREFLIGHT_PASS', 'clockEnvelope': result, 'actions': self.actions})
        return result


def hdc_commands(config, config_path):
    adapter = Path(__file__).with_name('hdc_action.py').resolve()
    path = str(config_path.resolve())
    hdc = config['hdc']
    check(Path(hdc).is_file() or shutil.which(hdc), 'COMMAND_PATH_hdc')
    check(config['appFilesDir'].startswith('/') and all(c not in config['appFilesDir'] for c in ' \t\r\n;&|'),
          'APP_FILES_PATH')
    actions = ('reserve','status','export','clock','seal','cancel','terminal','release')
    for action in actions:
        if action=='terminal':continue
        x,y = config['taps'][action]
        check(type(x) is int and type(y) is int and min(x,y)>=0, 'INVALID_TAP_'+action)
    commands = {a:[sys.executable,str(adapter),path,'{output}','{actionId}',a] for a in actions}
    base = [hdc,'-t',config['deviceId']]
    commands['screenshot'] = base+['shell','uitest','screenCap','-p','{remotePng}']
    commands['pull'] = base+['file','recv','{remotePng}','{localPng}']
    return commands


def preflight(config, output):
    check(not output.exists(), 'FRESH_OUTPUT_REQUIRED')
    required = ('reserve', 'status', 'export', 'clock', 'screenshot', 'pull', 'seal', 'cancel', 'terminal', 'release')
    check(config['deviceId'] and config['toolConfiguration'] and config['clockUncertaintySource'] and
          0 <= config['clockQuantizationMs'], 'DEVICE_CONFIGURATION_MISSING')
    spec = config['spec']
    if 'deviceId' in spec:check(spec['deviceId']==config['deviceId'], 'DEVICE_CONFIGURATION_MISMATCH')
    if 'toolConfiguration' in spec:check(spec['toolConfiguration']==config['toolConfiguration'], 'DEVICE_CONFIGURATION_MISMATCH')
    spec.update(deviceId=config['deviceId'],toolConfiguration=config['toolConfiguration'])
    bounds, size = spec['bounds'], spec['screenshotSize']
    check(spec['rotation'] == 0 and spec['screenshotScale'] == 1 and spec['registeredGeometry']['visible'] is True and
          len(bounds) == 4 and len(size) == 2 and all(type(n) is int for n in bounds+size) and
          0 <= bounds[0] < bounds[2] <= size[0] and 0 <= bounds[1] < bounds[3] <= size[1], 'VISIBLE_GEOMETRY_PREFLIGHT')
    for action in required:
        argv = config['commands'][action]
        check(type(argv) is list and argv and all(type(s) is str for s in argv), 'COMMAND_ARGV_'+action)
        check('{' not in argv[0] and (Path(argv[0]).is_file() or shutil.which(argv[0])), 'COMMAND_PATH_'+action)
        # Placeholder validation must complete before any device command.
        for s in argv:
            s.format_map(dict(output='out', remotePng='remote', localPng='local', actionId='1', attemptId='a', holdId='h', requestId=1, phaseSequence=1))
    output.mkdir(parents=True, exist_ok=False)
    with (output/'write-probe').open('xb') as stream:
        stream.write(b'preflight');stream.flush();os.fsync(stream.fileno())
    (output/'write-probe').unlink()


def main():
    cli = argparse.ArgumentParser(description=__doc__);cli.add_argument('config', type=Path);cli.add_argument('output', type=Path)
    args = cli.parse_args();runner = None
    try:
        config = read(args.config)
        if 'commands' not in config:config['commands'] = hdc_commands(config,args.config)
        preflight(config, args.output);runner = Runner(config, args.output)
        write_new(args.output/'input-config.json', config)
        print(json.dumps(runner.run()))
    except (ValueError, KeyError, OSError, TypeError, IndexError) as error:
        record = {'state': 'INCOMPLETE', 'reason': str(error), 'actions': runner.actions if runner else []}
        if runner and runner.scope:
            for action in ('cancel', 'export', 'terminal'):
                try:runner.command(action)
                except (ValueError, KeyError, OSError, TypeError):pass
        if args.output.is_dir() and runner:
            write_new(args.output/'failure.json', record)
        print(json.dumps(record));return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
