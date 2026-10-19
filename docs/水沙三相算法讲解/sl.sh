#!/bin/bash
# wrapper: run slidep entry directly
ENTRY="C:/Users/admin/.workbuddy/binaries/node/versions/22.22.2-6/lib/node_modules/@tencent/slidep/dist/index.js"
exec "C:/Users/admin/.workbuddy/binaries/node/versions/22.22.2-6/node.exe" "$ENTRY" "$@"
