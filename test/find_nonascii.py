data = open('D:/hmprojects/HelloWorld/tools/run_p012_c_verify.ps1', 'rb').read()
lines = data.split(b'\n')
for i, l in enumerate(lines, 1):
    if any(b > 127 for b in l):
        print(i, repr(l[:70]))
