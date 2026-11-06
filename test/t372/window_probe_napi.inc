SingleSandWindowProbe windowProbe;
SingleSandReceipt windowReceipt;

napi_value TakeSingleSandWindowProbe(napi_env e,napi_callback_info){
 std::lock_guard<std::mutex> lock(g.mutex);
 napi_value o=nullptr;
 if(!windowProbe.available){napi_get_null(e,&o);return o;}
 const auto& probe=windowProbe;
 const auto& p=probe.packet;
 const auto& r=windowReceipt;
 o=SingleResult(e,r,p);
 napi_value envelope=nullptr;napi_create_object(e,&envelope);napi_set_named_property(e,envelope,"manifest",o);
 const auto clean=[](const ProbeTarget& t){return t.read&&!t.setupError&&!t.readError&&!t.restoreError;};
 const bool ok=clean(probe.target[0])&&clean(probe.target[1])&&probe.unbound&&
    r.swapped&&r.state=="CURRENT"&&p.token==r.actual;
 Put(e,o,"captureRevision","T372_SINGLE_SAND_WINDOW_PROBE_V1");
 Put(e,o,"captureStatus",ok?"CAPTURED":"CAPTURE_FAILED");
 Put(e,o,"captureStage",probe.stage);
 Put(e,o,"session",double(p.session));Put(e,o,"frameSerial",double(p.frame));
 Put(e,o,"windowWidth",double(p.width));Put(e,o,"windowHeight",double(p.height));
 Put(e,o,"boxX",p.box[0]);Put(e,o,"boxY",p.box[1]);Put(e,o,"boxWidth",p.box[2]);Put(e,o,"boxHeight",p.box[3]);
 Put(e,o,"scale",p.scale);Put(e,o,"inverted",p.inverted);Put(e,o,"drawRawEGLBeforeUnbind",double(probe.drawEglError));
 Put(e,o,"particleHash",p.particleHash.c_str());Put(e,o,"unbindSucceeded",probe.unbound);
 Put(e,o,"unbindRawEGL",double(probe.unbindError));Put(e,o,"captureMs",probe.ms);
 Put(e,o,"readbackBytes",double((probe.target[0].read+probe.target[1].read)*probe.used));
 Put(e,o,"glVersion",probe.version);Put(e,o,"glRenderer",probe.renderer);
 Put(e,o,"ditherEnabled",probe.dither);Put(e,o,"blendEnabled",probe.blend);
 Put(e,o,"format","RGBA8_UNSIGNED_BYTE");Put(e,o,"rowOrder","BOTTOM_TO_TOP");
 napi_value rois=nullptr;napi_create_array_with_length(e,probe.count,&rois);
 int offset=0;
 for(int i=0;i<probe.count;i++){
  const auto& roi=probe.roi[i];napi_value v=nullptr;napi_create_object(e,&v);
  Put(e,v,"x",double(roi.x));Put(e,v,"y",double(roi.y));Put(e,v,"width",double(roi.w));Put(e,v,"height",double(roi.h));
  Put(e,v,"stride",double(4*roi.w));Put(e,v,"byteOffset",double(offset));
  offset+=4*roi.w*roi.h;napi_set_element(e,rois,i,v);
 }
 napi_set_named_property(e,o,"rois",rois);
 napi_value config=nullptr;napi_create_array_with_length(e,probe.config.size(),&config);
 for(uint32_t i=0;i<probe.config.size();i++){
  napi_value v=nullptr;napi_create_double(e,probe.config[i],&v);napi_set_element(e,config,i,v);
 }
 napi_set_named_property(e,o,"configValues",config);
 for(int i=0;i<2;i++){
  const auto& t=probe.target[i];
  napi_value v=nullptr;napi_create_object(e,&v);
  Put(e,v,"stage",i==0?"PRE_SWAP_CANDIDATE":"PRE_SWAP_DEFAULT_BACK");
  Put(e,v,"framebuffer",double(t.framebuffer));Put(e,v,"readBuffer",double(t.buffer));Put(e,v,"encoding",double(t.encoding));
  Put(e,v,"samples",double(t.samples));Put(e,v,"setupGL",double(t.setupError));Put(e,v,"readGL",double(t.readError));
  Put(e,v,"restoreGL",double(t.restoreError));Put(e,v,"readSucceeded",t.read);
  if(t.read){
   napi_value buffer=nullptr,view=nullptr;void* data=nullptr;
   if(napi_create_arraybuffer(e,probe.used,&data,&buffer)!=napi_ok||
      napi_create_typedarray(e,napi_uint8_array,probe.used,buffer,0,&view)!=napi_ok)return nullptr;
   const auto* first=probe.bytes.data()+i*SingleSandWindowProbe::TargetStride;
   std::memcpy(data,first,probe.used);
   Put(e,v,"sha256",MaterialSha256(first,probe.used).c_str());napi_set_named_property(e,envelope,i==0?"candidate":"back",view);
  }
  napi_set_named_property(e,o,i==0?"candidate":"back",v);
 }
 windowProbe.available=false;
 return envelope;
}
