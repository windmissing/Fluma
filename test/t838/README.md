# T838 PC mirror and three-phase fixture

Run `python test/t838/check.py emit input.json` to generate exact A/B inputs and canonical JSON hashes. Positions mirror on the original Q20 lattice. Negative velocities, command impulses and gravity are reflected, with columns and walls reversed.

Run `python test/t838/check.py check pair.json` to compare a pair of accepted receipts. The checker verifies decimal-string accepted intervals, both input hashes and the exact reflection. Each supplied observation carries its own accepted root and geometry. Supply the original cold archive and its SHA for each run. Native acceptance and pixel readback still require the PC lifecycle receipt. The fixture reports S/F/P, signed face Q, total inventory difference, origin Q and the exact horizontally reflected RGBA8 differential independently. Missing observations retain null.

Run `python test/t838/check.py selftest` without GPU evidence. It also replays the T814 arithmetic, including the A05 integer tie and A06 symmetric control. Synthetic pairs do not count as eligible PC pairs.
