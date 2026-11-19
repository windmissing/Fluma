// Included only by the explicit T522 bridge adapter, in the existing App.
std::atomic<bool> m5Selected{false};
grs::m3b::PhysicalConfigV5 m5Config;
grs::m3b::ControlIdentityV5 m5Identity;
std::shared_ptr<grs::m3b::ControllerV5> m5Controller;
uint64_t m5Generation=0;
std::array<uint32_t,18> M5IdentityWordsLocked()const{
 std::array<uint32_t,18> out{};const uint64_t values[]={m5Identity.run,m5Identity.owner,m5Identity.context,m5Identity.inputEpoch,m5Identity.pageSession,m5Identity.permission,m5Identity.window,m5Identity.config,m5Identity.surface};
 for(size_t i=0;i<9;++i){out[i*2]=uint32_t(values[i]);out[i*2+1]=uint32_t(values[i]>>32);}return out;
}

grs::m3b::init650::Store m5InitStore;
uint64_t m5InitTicket=0,m5InitEpoch=0;
