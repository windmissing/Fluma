// T24 PREPARED_NOT_DEVICE_VALIDATED. Not embedded or linked into product shaders.
// P1_ALBEDO_DELTA_V1 / LIN_SRGB_REINHARD_V1
vec3 preparedQ(vec3 c) {
  vec3 lo=c/12.92;
  vec3 hi=pow((c+0.055)/1.055,vec3(2.4));
  vec3 l=min(mix(hi,lo,lessThanEqual(c,vec3(0.04045))),vec3(1.0-1.0/4096.0));
  return l/(1.0-l);
}
vec3 preparedMaterial(vec3 baseByte, vec3 grainByte, vec3 grainBaseByte,
  vec3 strataByte, float c3Byte, vec3 strengths, float lmat, bool lightEnabled) {
  vec3 neutral=preparedQ(vec3(128.0/255.0));
  vec3 dc=strengths.y*(preparedQ(strataByte/255.0)-neutral)
    +strengths.z*(preparedQ(vec3(c3Byte/255.0))-neutral);
  vec3 db=strengths.x*(preparedQ(grainByte/255.0)-preparedQ(grainBaseByte/255.0));
  vec3 r=max(vec3(0.0),preparedQ(baseByte/255.0)+dc+db)*(lightEnabled?lmat:1.0);
  return r/(1.0+r); // linear; never add shader OETF/gamma.
}
