from pathlib import Path
import math,json,hashlib,sys
r=Path(__file__).resolve().parents[2];out=r/'.codex/local-test/t281';report=r/'test/reports/t281';passed=0
def srgb(v):return 12.92*v if v<=.0031308 else 1.055*v**(1/2.4)-.055
for k,w in [(192,64),(192,128),(512,64),(512,128)]:
 if len(sys.argv)>1 and (k,w)!=(int(sys.argv[1]),int(sys.argv[2])):continue
 def image(name,layer):return (out/f'{name}-{k}-{w}-layer{layer}.rgba').read_bytes()
 def pixel(data,x,y):return list(data[(y*256+x)*4:(y*256+x)*4+4])
 deep=image('deep',3);alpha=1-math.exp(-.03*(64-32.5));background=[.006,.008,.014];tint=[.04,.13,.18];expected=[round(255*srgb(a*(1-alpha)+b*alpha)) for a,b in zip(background,tint)]+[255]
 actual=pixel(deep,129,32);assert max(abs(a-b) for a,b in zip(actual,expected))<=2,(k,w,actual,expected);passed+=1
 dry=image('empty',2);assert pixel(deep,127,32)==pixel(dry,127,32),'dry adjacent column fog/bridge';passed+=1
 assert image('empty',2)==image('empty',3)==image('empty',4)==image('empty',6);passed+=1
 # High J is visible before/after; the arrived low J produces no glyph. Its
 # own column 66 is untouched by the high packet in column64.
 surface=image('arrival',4);glyph=image('arrival',6)
 assert all(pixel(surface,x,y)==pixel(glyph,x,y) for x in range(132,136) for y in range(55,75)),(k,w,'retired J double displayed');passed+=1
 assert image('flight',4)!=image('flight',6);passed+=1
 capture=[]
 for name in ['empty','deep','flight','arrival','sand']:
  for layer in range(7):
   data=image(name,layer);capture.append({'name':name,'layer':layer,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
 (report/f'pixels-{k}-{w}.json').write_text(json.dumps({'K':k,'Kw':w,'deepPixel':actual,'independentLinearPremultSrgbExpected':expected,'toleranceLsb':2,'captures':capture,'device':'NOT_RUN'},indent=2))
print(json.dumps({'revision':'T281_PIXEL_ORACLE_V1','passed':passed,'failed':0,'device':'NOT_RUN'}))
