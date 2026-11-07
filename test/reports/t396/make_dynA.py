import json
base=r'D:\hmprojects\HelloWorld\test\reports\t396'
spec={
 'manifest':'t394-2-1-1629.json',
 'hold':'t394-hold-2-1629.json',
 'png':'dynA.png',
 'bounds':[51,1968,1229,2650],
 'captureWallMs':1794018093737
}
json.dump(spec,open(base+r'\dynA.json','w'),indent=1)
print('written dynA.json')
