import subprocess
import sys
from pathlib import Path

subprocess.run([sys.executable, "-X", "utf8", "test/t372/prepare_host.py", sys.argv[1]], check=True)
path = Path(".codex/local-test/t372-host/host.cpp")
source = path.read_text()
needle = " auto old=p.token;p.frame++;packetHash(p);failSwap=true;probe.target={};probe.packet=p;"
assert source.count(needle) == 1
fragment = """
 probe.calibration=true;
 p.frame++;packetHash(p);probe.packet=p;probe.target={};
 r=pass.Draw(egl,p,&probe);
 assert(r.swapped&&r.state=="CURRENT"&&probe.target[0].read&&probe.target[1].read);
 int byteOffset=0,decodeDifferences=0;
 for(int i=0;i<probe.count;i++){
  const auto& roi=probe.roi[i];
  for(int yy=0;yy<roi.h;yy++)for(int xx=0;xx<roi.w;xx++){
   const unsigned x=roi.x+xx,y=roi.y+yy;
   const unsigned h=x*73+y*151+(x/17)*31+(y/23)*47;
   const uint8_t expected[]={uint8_t(32+h%192),uint8_t(32+(x*37+y*19)%192),
    uint8_t(32+(x*11+y*97)%192),255};
   assert(!std::memcmp(probe.bytes.data()+byteOffset,expected,4));
   const auto* actual=probe.bytes.data()+SingleSandWindowProbe::TargetStride+byteOffset;
   for(int channel=0;channel<3;channel++){
    const double v=expected[channel]/255.;
    const auto decoded=uint8_t(std::round((v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4))*255));
    decodeDifferences+=actual[channel]!=decoded;
   }
   assert(actual[3]==255);
   byteOffset+=4;
  }
 }
 assert(probe.candidateNs>0&&probe.backNs>=probe.candidateNs&&probe.captureEndNs>=probe.backNs);
 probe.calibration=false;
 lastGood=pass.TestCommittedFramebuffer();
 std::cout<<"T406 asymmetric shader candidate exact, native back complete alpha255, PC sRGB-decode differences="<<decodeDifferences<<"; no device display claim PASS"<<std::endl;
"""
path.write_text(source.replace("const auto lastGood=pass.TestCommittedFramebuffer();",
                               "auto lastGood=pass.TestCommittedFramebuffer();").replace(needle, fragment+needle))
