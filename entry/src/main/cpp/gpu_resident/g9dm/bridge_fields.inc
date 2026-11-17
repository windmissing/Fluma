std::atomic<bool> g9Selected{false};
grs::g9dm::Config g9Config;
grs::g9dm::ControlIdentity g9Identity;
std::shared_ptr<grs::g9dm::Controller> g9Controller;
uint64_t g9Generation=0;
std::array<uint32_t,18> G9IdentityWordsLocked()const{
 std::array<uint32_t,18> out{};const uint64_t values[]={g9Identity.run,g9Identity.owner,g9Identity.context,g9Identity.inputEpoch,g9Identity.pageSession,g9Identity.permission,g9Identity.window,g9Identity.config,g9Identity.surface};
 for(size_t i=0;i<9;++i){out[2*i]=uint32_t(values[i]);out[2*i+1]=uint32_t(values[i]>>32);}return out;
}
