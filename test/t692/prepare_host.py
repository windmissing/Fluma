"""Prepare the isolated T492 replay compile; leaves production and T496 inputs intact."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
local = root / ".codex/local-test"
out = Path(sys.argv[1]).resolve()
if not out.is_relative_to(local) or out.exists():
    raise SystemExit("FRESH_HOST_BUILD_DIR_REQUIRED")
out.mkdir()
frozen = local / "t492-m1-delivery/entry/src/main/cpp/gpu_resident"
hashes = {}

def edit(source, anchor, replacement):
    if source.count(anchor) != 1:
        raise ValueError(f"T692_EXPECTED_ONE_ANCHOR {anchor}")
    return source.replace(anchor, replacement, 1)

def frozen_text(name, sha):
    raw = (frozen / name).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != sha:
        raise ValueError(f"T692_FROZEN_INPUT_MISMATCH {name} {digest}")
    hashes[name] = digest
    return raw.decode("utf8").replace("\r\n", "\n")

run = frozen_text("m1/run.cpp", "db0172ff21448d64db1fff53880324904364fa467e33c4f3cef0a722477ad167")
run = '#include "accepted_readback.h"\n' + run
anchor = "receipt.accepted++;receipt.latest=accepted;"
run = edit(run, anchor, anchor + "\n  FullM1HostAccess::Observe(transport, accepted);")
(out / "run.cpp").write_text(run, encoding="utf8")

app = frozen_text("app_bridge.cpp", "7444aab6c9ef278e904051e5979002ada7505c957b8fcd4d50274c4dc91d32f0")
app = '#include "accepted_readback.h"\n' + app
app = edit(app, " isolation.context=this;", """ isolation.context=this;
 m1::t692::authority={this,[](void* ptr,const RootRequest& request,const AcceptedRoot& root){
  auto& app=*static_cast<App*>(ptr);
  std::unique_lock<std::mutex> lock(app.mutex,std::try_to_lock);
  if(!lock.owns_lock())return false;
  return app.AllowedLocked() && app.CurrentLocked(app.permit.epoch,app.permit.window) &&
      app.permit.revision==app.permissionRevision &&
      app.Stamp()<=app.permit.deadline && app.permit.nonce==m1::NonceOf(request) &&
      app.rootNonce.lo==root.nonce.lo && app.rootNonce.hi==root.nonce.hi;
 }};""")
(out / "app_bridge.cpp").write_text(app, encoding="utf8")

driver = (root / "test/t496/external_source_diff.cpp").read_text(encoding="utf8")
driver = edit(driver, '#include "../t492-m1-delivery/entry/src/main/cpp/gpu_resident/app_bridge.cpp"',
              '#include "app_bridge.cpp"')
# Keep original relative includes resolving from test/t496.
(out / "external_source_diff.cpp").write_text(driver, encoding="utf8")
build_template_blob = "613b0023f23c6a6a9402ce66b441267db9ff8b0b"
template = subprocess.check_output(["git", "cat-file", "blob", build_template_blob], cwd=root).decode("utf8")
path = out.relative_to(root).as_posix()
template = template.replace("test/t496/external_source_diff.cpp", f"{path}/external_source_diff.cpp")
template = template.replace("%SRC%/gpu_resident/m1/run.cpp", f"{path}/run.cpp")
template = template.replace(".codex/local-test/t496/", path + "/")
template = template.replace("set INCS=", "set INCS=/DGRS_T692_HOST_READBACK /Itest/t692 /Itest/t496 /I%SRC%/gpu_resident/m1 ")
(out / "build.cmd").write_text(template.replace("\r\n", "\n"), encoding="utf8")
receipt = {
    "buildTemplateGitBlob": build_template_blob,
    "frozen": hashes,
    "templates": {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                  for name in ["test/t692/accepted_readback.h", "test/t496/external_source_diff.cpp",
                               "test/t496/gl_forward_sink.cpp", "test/t496/gl_sink.inc"]},
    "generated": {name: hashlib.sha256((out / name).read_bytes()).hexdigest()
                  for name in ["run.cpp", "app_bridge.cpp", "external_source_diff.cpp", "build.cmd"]},
}
(out / "inputs.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf8")
