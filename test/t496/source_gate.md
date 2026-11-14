// T496 build checklist: single-source check helper (host side).
// The actual build command lives in build.cmd; this file documents the
// pre-run source-consistency gate per T494 §2:
//   1. all -I flags point ONLY at .codex/local-test/t492-m1-delivery/.../gpu_resident
//   2. all implementation .cpp come from the same tree
//   3. /Zs /d1reportSingleClassLayoutFullM1Transport must match between the
//      driver TU and full_transport.cpp (size 216704-tree == same-tree impl)
// build.cmd fails the run if any include resolves outside the frozen tree.
