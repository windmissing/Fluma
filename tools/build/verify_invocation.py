"""Validate caller-supplied invocation evidence before classifying a build."""
import re
from pathlib import Path
from build_invocation import (SCHEMA, TARGET, contained, describe, load,
                              snapshot)

PREFIX = ["configure", "prepare-link", "initial"]
INCREMENTAL = ["reconfigure", "reprepare-link", "no-work", "rsp-relink", "repeat"]
OUTPUTS = ("lib/libgrs_cold_slots.so", "libgrsT666Consumer.so")

def require(condition, reason):
    if not condition:
        raise ValueError("SOURCE_BINDING_INCOMPLETE " + reason)

def digest(record, expected=None):
    path = Path(record["path"])
    if expected is not None:
        require(path == expected, "EVIDENCE_PATH " + str(path))
    require(describe(path) == record, "EVIDENCE_BYTES " + str(path))
    return path

def unchanged(step, files):
    return all(name in step["before"] and step["before"][name] == step["after"].get(name)
               for name in files)

def check(build, manifest):
    manifest = contained(manifest, build)
    data = load(manifest)
    ident = data["invocation"]
    require(bool(re.fullmatch(r"[0-9a-f]{32}", ident)), "INVOCATION_ID")
    folder = build / "grs-invocations" / ident
    require(manifest == folder / "invocation.json", "MANIFEST_PATH")
    require(data["schema"] == SCHEMA and data["target"] == TARGET and
            data["buildRoot"] == str(build), "MANIFEST_SCOPE")
    policy_file = digest(data["policy"])
    policy = load(policy_file)
    for name in ("cmake", "ninja", "sdkToolchain"):
        digest(data["inputs"][name], Path(policy[name]).resolve())
    plan = data["plan"]
    require(plan in (PREFIX, PREFIX + INCREMENTAL), "PLAN")
    require(data["state"] in {"SEALED", "FAILED"}, "MANIFEST_NOT_SEALED")
    steps = data["steps"]
    require([item["stage"] for item in steps] == plan[:len(steps)], "STEP_ORDER")
    if data["state"] == "SEALED":
        require(len(steps) == len(plan), "MISSING_STEP")
    prior = 0
    ids = set()
    result = {}
    for index, step in enumerate(steps):
        key = step["id"]
        stage = step["stage"]
        require(re.fullmatch(r"[0-9a-f]{32}", key) and key not in ids and
                step["parent"] == ident and step["buildRoot"] == str(build) and
                step["target"] == TARGET and prior <= step["beginNs"] <= step["endNs"],
                "STEP_ID_SCOPE_TIME")
        ids.add(key)
        prior = step["endNs"]
        base = folder / f"{index:02d}-{key}"
        stdout = digest(step["stdout"], base.with_suffix(".stdout")).read_bytes()
        digest(step["stderr"], base.with_suffix(".stderr"))
        digest(step["tool"], Path(step["argv"][0]).resolve())
        for when in ("before", "after"):
            for name, row in step[when].items():
                require(Path(row["path"]) == contained(build / name, build), "OUTPUT_SCOPE")
        if step["returnCode"]:
            require(index == len(steps) - 1 and data["state"] == "FAILED", "FAILED_ORDER")
            result[stage] = "FAILED"
            continue
        if stage in ("configure", "reconfigure", "prepare-link", "reprepare-link"):
            configure = stage in ("configure", "reconfigure")
            cwd = Path(policy["source"]).resolve() if configure else build
            tool = Path(__file__).with_name("owned_build.py").resolve()
            action = "configure" if configure else "prepare-link"
            args = [str(tool), action, str(policy_file), str(build)]
            require(step["argv"][1:] == args + ([] if configure else ["grs_cold_slots"])
                    and step["cwd"] == str(cwd), "WRAPPER_ARGV")
            # A stable already-patched edge can skip compdb on reprepare.
            if not configure and stage == "reprepare-link" and "native" not in step:
                require(unchanged(step, ["grs_cold_slots.link.json", "build.ninja"]), "REUSE_PATCH")
            else:
                native = load(digest(step["native"], folder / f"{key}.native.json"))
                require(native["parent"] == ident and native["step"] == key and
                        native["buildRoot"] == str(build) and native["cwd"] == str(cwd) and
                        step["beginNs"] <= native["beginNs"] <= native["endNs"] <= step["endNs"]
                        and not native["returnCode"], "NATIVE_STEP")
                tool = Path(policy["cmake" if configure else "ninja"]).resolve()
                digest(native["tool"], tool)
                require(Path(native["argv"][0]).resolve() == tool, "NATIVE_TOOL")
                if configure:
                    require(native["argv"][1:7] == ["-S", str(cwd), "-B", str(build), "-G", "Ninja"]
                            and "-DCMAKE_TOOLCHAIN_FILE=" + (build / "grs_owned_toolchain.cmake").as_posix()
                            in native["argv"], "CMAKE_ARGV")
                else:
                    require(native["argv"][1:] == ["-t", "compdb", "-x"], "COMPDB_ARGV")
                for stream in ("stdout", "stderr"):
                    digest(native[stream], folder / f"{key}.native.{stream}")
            result[stage] = "OBSERVED"
            continue
        require(step["cwd"] == str(build) and step["argv"] ==
                [str(Path(policy["ninja"]).resolve()), "-C", str(build), "-v", TARGET],
                "NINJA_ARGV_TARGET")
        if stage in ("no-work", "repeat"):
            require(b"ninja: no work to do." in stdout and "link" not in step and
                    unchanged(step, [*OUTPUTS, ".ninja_log"]), "NO_WORK_PROOF")
            result[stage] = "INCREMENTAL_NO_WORK"
            continue
        link = load(digest(step["link"], base.with_suffix(".link.json")))
        after = step["after"]
        require(link["parent"] == ident and link["step"] == key and link["cwd"] == str(build)
                and step["beginNs"] <= link["beginNs"] <= link["endNs"] <= step["endNs"] and
                link["returnCode"] == 0 and link["output"] == str(build / OUTPUTS[0]) and
                link["ELFSHA256"] == after[OUTPUTS[0]]["sha256"] and
                link["manifestSHA256"] == after["grs_cold_slots.link.json"]["sha256"] and
                link["rspSHA256"] == after["grs_cold_slots.full.rsp"]["sha256"] and
                link["argv"][1:] == ["@" + (build / "grs_cold_slots.full.rsp").as_posix()],
                "CHILD_LINK_CLOSURE")
        require(after["grs_cold_slots.link-receipt.json"]["sha256"] == step["link"]["sha256"],
                "LINK_RECEIPT_BYTES")
        require(describe(link["argv"][0])["sha256"] == link["driverSHA256"], "LINK_TOOL")
        for name, sha in link["objects"].items():
            require(after[contained(name, build).relative_to(build).as_posix()]["sha256"] == sha,
                    "LINK_OBJECT")
        for stream in ("stdout", "stderr"):
            archived = step["linkStreams"][stream]
            digest(archived, base.with_suffix(f".link.{stream}"))
            require(link[stream] == after[f"grs_cold_slots.link.{stream}"] and
                    link[stream]["sha256"] == archived["sha256"], "LINK_STREAM")
        require(OUTPUTS[1] in after and ".ninja_log" in after and
                not unchanged(step, [".ninja_log"]) and Path(OUTPUTS[1]).name.encode() in stdout,
                "CONSUMER_LINK_PROOF")
        compile_lines = re.findall(rb"(?m)^\[\d+/\d+\].* -o [^\r\n]+\.o -c ", stdout)
        if stage == "initial":
            require(compile_lines and any(name.endswith(".o") and row != step["before"].get(name)
                                          for name, row in after.items()), "COMPILE_PROOF")
        if stage == "rsp-relink":
            require(not compile_lines and
                    unchanged(step, [name for name in step["before"] if name.endswith(".o")])
                    and unchanged(step, ["grs_cold_slots.full.rsp"]), "RSP_ONLY_RELINK")
        result[stage] = ["COMPILE_OBSERVED", "LINK_OBSERVED"] if stage == "initial" else "EXPECTED_RSP_RELINK"
    if data["state"] == "FAILED":
        raise ValueError("FAILED " + str(manifest))
    require(snapshot(build) == steps[-1]["after"], "CURRENT_OUTPUT_CLOSURE")
    if "reconfigure" not in result:
        result["reconfigure"] = "NOT_RUN"
    return data, result
