from pathlib import Path
r=Path(__file__).resolve().parents[2];s=(r/'test/gpu_resident/build_t281_run.cmd').read_text().replace('t281_run_test.cpp','t228_units.cpp').replace('local-test/t281','local-test/t228').replace('local-test\\t281','local-test\\t228');(r/'.codex/local-test/t228/build-units.cmd').write_text(s)
