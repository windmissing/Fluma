from pathlib import Path
import json,sys,tempfile,importlib.util
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('v5reader',ROOT/'tools/v5/read_archive.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def run():
 cases=[]
 with tempfile.TemporaryDirectory() as d:
  for name,row in [('empty',None),('old-reader',{'revision':'T503_M2_COLD_TRACE_V1'}),('bad-root-mode',{'revision':'T527_SEGMENT_PAGE_V2','payloadRevision':'GRS_M3B_LINEAGE_COLD_V5_2','state':'TRACE_READY','page':0,'mode':0}),('bad-full-profile',{'revision':'T527_SEGMENT_PAGE_V2','payloadRevision':'GRS_M3B_LINEAGE_COLD_V5_2','state':'TRACE_READY','page':0,'mode':2,'K':193,'Kw':64})]:
   p=Path(d)/(name+'.jsonl');p.write_text('' if row is None else json.dumps(row)+'\n')
   try:m.read_archive(p)
   except (ValueError,KeyError,TypeError) as e:cases.append({'case':name,'rejected':str(e)})
   else:raise AssertionError(name)
 print(json.dumps({'result':'PASS','negativeCases':cases,'ROOT_FULL_complete_device_archives':'NOT_RUN','device':'NOT_RUN'}))
if __name__=='__main__':run()
