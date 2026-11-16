// Included only by the explicit T522 bridge adapter, in the existing App.
std::atomic<bool> m3Selected{false};
grs::m3a::PhysicalConfigV4 m3Config;
grs::m3a::ControlIdentityV4 m3Identity;
std::shared_ptr<grs::m3a::ControllerV4> m3Controller;
uint64_t m3Generation=0;
std::array<uint32_t,18> M3IdentityWordsLocked()const{
 std::array<uint32_t,18> out{};const uint64_t values[]={m3Identity.run,m3Identity.owner,m3Identity.context,m3Identity.inputEpoch,m3Identity.pageSession,m3Identity.permission,m3Identity.window,m3Identity.config,m3Identity.surface};
 for(size_t i=0;i<9;++i){out[i*2]=uint32_t(values[i]);out[i*2+1]=uint32_t(values[i]>>32);}return out;
}
