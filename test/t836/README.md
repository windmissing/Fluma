# T836 PC window input fixture

Run `python test/t836/check.py emit` for the frozen action sequence and `python test/t836/check.py drive` to stream timed JSONL to a window input adapter. The generation numbers in the action stream are aliases. The adapter binds them to observed resize generations and records actual action QPC, including the action rejected under the previous generation. It records the measured QPC frequency and start QPC separately. Input and Present receipts must come from the PC run.

Run `python test/t836/check.py check receipt.json` on the joined receipt. Record `actions` with kind, actual generation (`geometry`), args, QPC and stale admission result. Record `roots` with acceptedRoot, acceptance, QPC, geometry, admitted source Q, originating input QPC and gravity input QPC/vector. Record `frames` with acceptedRoot, Present QPC/result, geometry, S/F/P and measured bed/particle pixel counts. Use decimal strings for clocks and generations. Initial fixture extent is at least 341×121. The scheduled resize is 800×500. The actual adapter, GPU receipts and pixel readback depend on T830.

`python test/t836/check.py selftest` exercises the judge with synthetic evidence. The check command rejects synthetic receipts.
