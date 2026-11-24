"""Publish the build's executable and exact role assets as a verified, new directory."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def publish(exe, assets, destination):
    manifest = json.loads((assets/"roles.json").read_bytes())
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        raise FileExistsError(destination)
    entries = [r for r in manifest["roles"] if r["state"]=="STATIC_SPIRV"]
    graphics = [r for group in manifest["graphics"].values() for r in group]
    with tempfile.TemporaryDirectory(prefix="sand-pack-",dir=destination.parent) as tmp:
        staged = Path(tmp)/"verified"
        modules = staged/"sand_assets"
        modules.mkdir(parents=True)
        shutil.copyfile(exe,staged/exe.name)
        shutil.copyfile(assets/"roles.json",modules/"roles.json")
        for row in entries+graphics:
            name = row["spv"]
            if Path(name).name!=name:
                raise ValueError("NONLOCAL_MODULE_PATH")
            shutil.copyfile(assets/name,modules/name)
        result = subprocess.run([str(staged/exe.name),"--check-package",str(modules)],
                                stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,encoding="utf8")
        (staged/"owner-receipt.jsonl").write_text(result.stdout,encoding="utf8")
        result.check_returncode()
        rows = [json.loads(line) for line in result.stdout.splitlines()]
        owners = [r for r in rows if r["stage"]=="OWNER_ROLE_SET"]
        if len(owners)!=1:
            raise ValueError("OWNER_RECEIPT_MISSING")
        owner = owners[0]
        variant = manifest.get("roleVariant","baseline")
        if (owner["family"].lower()!=manifest["family"] or owner["variant"]!=variant or
                owner["roleSetSHA"]!=manifest["roleSetSHA"] or owner["manifestSHA"]!=digest(modules/"roles.json")):
            raise ValueError("OWNER_MANIFEST_IDENTITY")
        for row in entries+graphics:
            if (modules/row["spv"]).stat().st_size!=row["spvBytes"] or digest(modules/row["spv"])!=row["spvSHA"]:
                raise ValueError(f"PACKAGED_MODULE_MISMATCH {row['spv']}")
        files = {str(p.relative_to(staged).as_posix()):dict(bytes=p.stat().st_size,sha256=digest(p))
                 for p in sorted(staged.rglob("*")) if p.is_file()}
        seal = dict(schema="PC_SAND_ROLE_PACKAGE_V1",family=manifest["family"],variant=variant,
                    roleSetSHA=owner["roleSetSHA"],files=files)
        (staged/"package.json").write_text(json.dumps(seal,indent=2)+"\n",encoding="utf8")
        staged.rename(destination)
    print(json.dumps(dict(path=str(destination),roleSetSHA=owner["roleSetSHA"],sha256=digest(destination/"package.json"))))


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe",type=Path)
    parser.add_argument("assets",type=Path)
    parser.add_argument("destination",type=Path)
    args=parser.parse_args()
    publish(args.exe.resolve(),args.assets.resolve(),args.destination.resolve())
