"""HDC action adapter for the isolated T413 page. Run only on the tester's device."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time


class Adapter:
    def __init__(self, config, output, action_id):
        self.config, self.output, self.action_id = config, output, action_id
        self.hdc = [config['hdc'], '-t', config['deviceId']]
        self.files = config['appFilesDir'].rstrip('/')
        self.serial = 0

    def call(self, *args, required=True):
        argv = self.hdc+list(args)
        p = subprocess.run(argv, capture_output=True, timeout=15)
        self.serial += 1
        with (self.output/f'transport-{self.action_id}-{self.serial:03}.json').open('x',encoding='utf8') as f:
            json.dump({'argv':argv,'exitCode':p.returncode,'stdout':p.stdout.decode(errors='replace'),
                       'stderr':p.stderr.decode(errors='replace')},f)
        if required and p.returncode:
            raise ValueError('HDC_COMMAND_FAILED '+str(args))
        return p

    def mailbox(self, name):
        p = self.call('shell','cat',self.files+'/'+name,required=False)
        try:return json.loads(p.stdout) if p.returncode==0 else None
        except (ValueError,UnicodeError):return None

    def tap(self, action):
        x,y = self.config['taps'][action]
        if type(x) is not int or type(y) is not int or min(x,y)<0:raise ValueError('INVALID_TAP')
        self.call('shell','uitest','uiInput','click',str(x),str(y))

    def request(self, action, mailbox):
        before = self.mailbox(mailbox)
        self.tap(action)
        end = time.monotonic()+self.config.get('mailboxTimeoutSeconds',10)
        while time.monotonic()<end:
            value = self.mailbox(mailbox)
            if value is not None and value != before:return value
            time.sleep(.12)
        raise ValueError('NO_NEW_APP_MAILBOX '+action)

    def fetch(self, name):
        if not re.fullmatch(r'[A-Za-z0-9_.-]+',name) or name in ('.','..'):raise ValueError('APP_FILENAME')
        target = self.output/name
        with tempfile.TemporaryDirectory(dir=self.output) as tmp:
            part = Path(tmp)/name
            self.call('file','recv',self.files+'/'+name,str(part))
            data = part.read_bytes()
        if target.exists():
            if target.read_bytes()!=data:raise ValueError('APP_FILE_CONFLICT '+name)
        else:
            with target.open('xb') as f:f.write(data)
        return target

    def fetch_receipt(self, receipt):
        path = self.fetch(receipt['name']);data=path.read_bytes()
        if hashlib.sha256(data).hexdigest()!=receipt['sha256'] or len(data)!=receipt['bytes']:
            raise ValueError('APP_FILE_RECEIPT')
        return path

    def run(self, action):
        if action=='reserve':return self.request(action,'t413-reserve.json')['reply']
        if action in ('status','release','cancel'):
            return self.request(action,'t413-status.json')['status']
        if action=='clock':return self.request(action,'t413-clock.json')
        if action=='export':
            receipt=self.request(action,'t413-native.json');path=self.fetch_receipt(receipt)
            m=json.loads(path.read_bytes());self.fetch(path.with_suffix('.sha256').name)
            for key in ('candidate','back'):
                if m[key] is not None:self.fetch_receipt(m[key])
            return receipt
        if action in ('seal','terminal'):
            if action=='seal':receipt=self.request(action,'t413-terminal.json')
            else:
                receipt=self.mailbox('t413-terminal.json')
                if receipt is None:raise ValueError('NO_TERMINAL_RECEIPT')
            self.fetch_receipt(receipt);return receipt
        raise ValueError('ACTION_UNKNOWN')


def main():
    cli=argparse.ArgumentParser(description=__doc__);cli.add_argument('config',type=Path)
    cli.add_argument('output',type=Path);cli.add_argument('action_id');cli.add_argument('action');args=cli.parse_args()
    if not args.action_id.isdecimal():raise ValueError('ACTION_ID')
    config=json.loads(args.config.read_text(encoding='utf-8-sig'))
    print(json.dumps(Adapter(config,args.output,args.action_id).run(args.action)))


if __name__=='__main__':main()
