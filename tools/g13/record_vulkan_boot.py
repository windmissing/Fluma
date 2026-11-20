"""Bind the T742 unsigned HAP and ARM64 ELF to their generated BOOT SPIR-V."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from zipfile import ZipFile

from build_pair_receipt import build_id, digest, write_json
from vulkan_boot_assets import require

NATIVE = "libgrs_g13vk_boot.so"
CACHE = "entry/.cxx/default/default/debug/arm64-v8a"
HAP = "entry/build/default/outputs/default/entry-default-unsigned.hap"
LINKED = f"entry/build/default/intermediates/cmake/default/obj/arm64-v8a/{NATIVE}"
NATIVE_ABI = f"libs/arm64-v8a/{NATIVE}"


def record(project, readelf):
    assets = project / CACHE / "gpu_resident/g13vulkan_boot/g13_assets"
    receipt = json.loads((assets / "T742_BOOT.json").read_bytes())
    spirv = (assets / "T742_BOOT.spv").read_bytes()
    glsl = (assets / "T742_BOOT.comp").read_bytes()
    require(len(spirv) == receipt["moduleBytes"] and digest(spirv) == receipt["moduleSHA"],
            "SPIR-V receipt mismatch")
    require(len(glsl) == receipt["sourceBytes"] and digest(glsl) == receipt["sourceSHA"],
            "GLSL receipt mismatch")
    log = project.with_suffix(".log").read_bytes()
    require(b"BUILD SUCCESSFUL" in log and b"ERROR:" not in log, "No successful Hvigor build")
    linked = project / LINKED
    elf = linked.read_bytes()
    hap = project / HAP
    with ZipFile(hap) as archive:
        require(sorted(name for name in archive.namelist() if name.endswith(".so")) ==
                ["libs/arm64-v8a/libc++_shared.so", NATIVE_ABI],
                "Unexpected HAP native libraries")
        archived = archive.read(NATIVE_ABI)
        abc = archive.read("ets/modules.abc")
        profile = json.loads(archive.read("module.json"))
    ability = next(item for item in profile["module"]["abilities"]
                   if item["name"] == profile["module"]["mainElement"])
    require(ability["srcEntry"] == "./ets/entryability/G13VulkanBootAbility.ets", "Wrong HAP route")
    for binary in (elf, archived):
        require(binary[:6] == b"\x7fELF\x02\x01" and binary[18:20] == b"\xb7\x00",
                "Expected little-endian AArch64 ELF")
        require(binary.count(spirv) == 1, "Expected one exact embedded BOOT module")
    packed_file = project / "entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a" / NATIVE
    require(packed_file.read_bytes() == archived, "Packaged ELF differs from strip output")
    identity = build_id(readelf, linked)
    require(build_id(readelf, packed_file) == identity, "Linked and packaged BuildId mismatch")
    dynamic = subprocess.check_output([str(readelf), "-d", str(packed_file)], text=True)
    needed = re.findall(r"Shared library: \[([^\]]+)\]", dynamic)
    return {
        "schema": "T742_G13_VULKAN_BOOT_COMPILE_V1", "project": project.as_posix(),
        "unsignedHap": HAP, "unsignedHapSHA256": digest(hap.read_bytes()),
        "nativePath": NATIVE_ABI, "nativeSHA256": digest(archived),
        "linkedSHA256": digest(elf), "buildId": identity, "abi": "arm64-v8a", "dtNeeded": needed,
        "abilitySource": ability["srcEntry"], "abcSHA256": digest(abc),
        "hvigorLogSHA256": digest(log), "spirvEmbeddedOnceInLinkedAndPackagedELF": True,
        "module": receipt, "deviceUsed": False, "signed": False, "runtime": "NOT_MEASURED",
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--readelf", type=Path, required=True)
    args = parser.parse_args()
    receipt = record(args.project.resolve(), args.readelf.resolve())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    write_json(args.out, receipt)
