from pathlib import Path
r=Path(__file__).resolve().parents[2];s=(r/'.codex/local-test/t281/build-app.cmd').read_text().replace('t281_app_host.cpp','t228_app_host.cpp').replace('/Fe.codex/local-test/t281/app.exe','/Fe.codex/local-test/t228/app.exe').replace('/Fo.codex/local-test/t281/','/Fo.codex/local-test/t228/');(r/'.codex/local-test/t228/build-app.cmd').write_text(s)
