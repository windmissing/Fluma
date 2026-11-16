"""Reuse V4 fixture mechanics while testing the separately compiled V5 source.
The producer itself is assembled by assemble_source.py, never substituted by
host arithmetic or accepted from a fabricated completion.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
text=(ROOT/'test/t522/source_gl.cpp').read_text(encoding='utf-8')
start=text.index(' if(scenario=="quantum"){')
text=text[:start]+text[text.index(' glDeleteBuffers(8,buffers);',text.index('#include "quantum_root.inc"')):]
text=text.replace('"gl_api.inc"','"test/t522/gl_api.inc"')
text=text.replace('"../../entry/','"entry/')
text=text.replace('.codex/local-test/t522-solver/source-driver.glsl','.codex/local-test/t517-source.glsl')
text=text.replace('INPUT=16384','INPUT=131072').replace('DEDUP=32768','DEDUP=262144').replace('base(6400),blank(6400)','base(43136),blank(43136)')
text=text.replace('work(ANGLES+61444)','work(ANGLES+61444+8192)')
text=text.replace('header(1,4)','header(1,5)').replace('header(146,7)','header(146,15)')
text=text.replace('1,1,1,3,4,4,4,15,0','1,1,1,3,5,5,5,31,0').replace('input(1,4)','input(1,5)')
text=text.replace('header(160,1);','header(160,1);header(161,1);header(181,64);')
text=text.replace('{"uM3Words",6400}','{"uM3Words",43136}')
text=text.replace('for(auto u:uniforms)uniform(u.first,u.second);', '''for(auto u:uniforms)uniform(u.first,u.second);
 const std::pair<const char*,uint32_t> v5uniforms[]={{"uV5Cohort",6400},{"uV5Depth",39168},{"uV5Certificate",39296},{"uV5Aux",42368},{"uV5SourceLedger",LEDGER},{"uV5SourceMeta",META},{"uV5OriginScratch",ANGLES+61444}};
 for(auto u:v5uniforms)uniform(u.first,u.second);''')
text=text.replace('tail[1]==4','tail[1]==5').replace('tail[99]==32767','tail[99]==262143').replace('tail[174]==7','tail[174]==15')
text=text.replace('for(unsigned i=189;i<256;++i)assert(tail[i]==0);','assert(tail[189]==1&&tail[209]==64);for(unsigned i=211;i<256;++i)assert(tail[i]==0);')
text=text.replace('T522','T517').replace('V4 seal','V5 seal')
(ROOT/'.codex/local-test/t517-source-host.cpp').write_text(text,encoding='utf-8')
