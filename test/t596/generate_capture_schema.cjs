const fs=require('node:fs'),crypto=require('node:crypto');
const file='docs/architecture-T614-v5-entry-cursor-v1.json',raw=fs.readFileSync(file),schema=JSON.parse(raw);
if(schema.rawCursor.words!==32||schema.rawCursor.revision!==2||schema.publicWire.MetaRevision!==2||schema.rawCursor.usedMaskU32!==0x8003ffff||schema.publicWire.MetaUsedMaskU32!==0x8007ffff)throw Error('T614_UNSUPPORTED_SCHEMA');
const sha=crypto.createHash('sha256').update(raw).digest('hex');
fs.writeFileSync('entry/src/main/cpp/gpu_resident/m3b/capture_schema_generated.h',`#pragma once\n#include <cstdint>\nnamespace grs::m3b::capture614 {\ninline constexpr uint32_t kRawRevision=2,kMetaRevision=2,kRawUsedMask=0x8003ffffu,kMetaUsedMask=0x8007ffffu;\ninline constexpr const char* kContractSha="${sha}";\n}\n`);
fs.writeFileSync('entry/src/main/cpp/gpu_resident/m3b/capture_schema_generated.glsl',`// T614 contract SHA256 ${sha}\nconst uint V5_ENTRY_REVISION=2u,V5_ENTRY_USED_MASK=0x8003ffffu;\n`);
