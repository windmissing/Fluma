#!/bin/bash
# T547-compliant runner: bound to this child PID, unique attempt, wall watchdog.
# Usage: run_create_only.sh <arm> <attemptId>
ARM="$1"; ATTEMPT="$2"
[ -z "$ARM" ] && { echo "usage: run_create_only.sh ARM ATTEMPTID"; exit 1; }
[ -z "$ATTEMPT" ] && { echo "attemptId required"; exit 1; }
cd /d/hmprojects/HelloWorld || exit 1
export PATH="/c/Users/admin/AppData/Local/Programs/CokePLUS:$PATH"
EXE=".codex/local-test/t496/t496.exe"
PIDFILE=".codex/local-test/t496/child-pid-$ATTEMPT.txt"
rm -f "$PIDFILE"
"$EXE" "$ARM" "$ATTEMPT" > ".codex/local-test/t496/out-$ATTEMPT-stdout.txt" 2> ".codex/local-test/t496/out-$ATTEMPT-stderr.txt" &
CPID=$!
echo "$CPID" > "$PIDFILE"
echo "CHILD_PID=$CPID"
DEADLINE=$(( $(date +%s) + 900 ))
while [ "$(date +%s)" -lt "$DEADLINE" ] && kill -0 "$CPID" 2>/dev/null; do sleep 0.5; done
if kill -0 "$CPID" 2>/dev/null; then
  kill -9 "$CPID" 2>/dev/null
  echo "WATCHDOG_KILL pid=$CPID at=$(date -Iseconds)" > ".codex/local-test/t496/watchdog-$ATTEMPT.txt"
else
  echo "CHILD_EXITED_BEFORE_DEADLINE" > ".codex/local-test/t496/watchdog-$ATTEMPT.txt"
fi
cat ".codex/local-test/t496/watchdog-$ATTEMPT.txt"
