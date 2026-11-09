// T417: all reservation transitions are linearized by App::mutex.
#ifdef GRS_PROGRESS_V2_EXPERIMENT
struct CaptureRequest {
 enum State { NONE, RESERVED, ARMED, CANCELLED, TIMED_OUT };
 State state=NONE;
 uint64_t id=0,requestNs=0,armByNs=0,detectedNs=0,captureId=0,startNs=0,observedNs=0;
 uint64_t safePointSerial=0,phaseSinceNs=0,rootNonce=0,permitStartNs=0,permitDeadlineNs=0,completed=0;
 uint32_t rootKind=0;
 bool draining=false,drawing=false,swapping=false;
 const char* reason="",*blocker="NO_OWNER_CHECKPOINT",*workerPhase="NO_OWNER_CHECKPOINT";
 EvidenceConfig config;
 Identity identity{};
 void* window=nullptr;
 std::array<char,96> initialPhase{},initialPermission{},initialFailure{};
};
CaptureRequest captureRequest;
std::unique_ptr<EvidenceCapture> capturePrepared,captureRetired;
uint64_t captureRequestSerial=0,captureSafeSerial=0,captureCommittedEpoch=0;
const char* captureOwnerPhase="NO_OWNER_CHECKPOINT";
uint64_t captureOwnerPhaseNs=0,captureCompleted=0;
bool captureWake=false;

static std::unique_ptr<EvidenceCapture> NewCaptureStorage(){
 auto block=std::make_unique<EvidenceCapture>();
 block->config.packageSha256.reserve(64);block->config.expectedRevision.reserve(64);
 return block;
}
static bool SameCaptureConfig(const EvidenceConfig& a,const EvidenceConfig& b){
 return a.packageSha256==b.packageSha256&&a.expectedRevision==b.expectedRevision&&a.epoch==b.epoch&&
  a.pageSession==b.pageSession&&a.durationNs==b.durationNs&&a.runLo==b.runLo&&a.runHi==b.runHi&&
  a.maxRows==b.maxRows&&a.maxBytes==b.maxBytes;
}
bool CaptureScopeLocked(const EvidenceConfig& c)const{
 return c.expectedRevision==(progressV2?"GRS_M1_PROGRESS_CLOCK_V2":"GRS_M1_PROGRESS_CLOCK_V1")&&
  c.epoch==epoch.load()&&c.runLo==identity.run.lo&&c.runHi==identity.run.hi&&c.pageSession==progress.session;
}
void CancelCaptureScopeLocked(const char* reason,uint64_t stamp){
 if(captureRequest.state!=CaptureRequest::RESERVED)return;
 captureRequest.state=CaptureRequest::CANCELLED;captureRequest.reason=reason;captureRequest.detectedNs=stamp;
}
void CheckCaptureRequestLocked(uint64_t stamp){
 auto& r=captureRequest;if(r.state!=CaptureRequest::RESERVED)return;
 if(stamp>=r.armByNs){r.state=CaptureRequest::TIMED_OUT;r.reason="CAPTURE_SAFE_POINT_TIMEOUT";r.detectedNs=stamp;return;}
 if(!active||window!=r.window||!CaptureScopeLocked(r.config)||captureCommittedEpoch!=r.config.epoch||
    Compare64(identity.owner,r.identity.owner)||Compare64(identity.context,r.identity.context)||
    Compare64(identity.surface,r.identity.surface)||Compare64(identity.geometry,r.identity.geometry)||
    Compare64(identity.config,r.identity.config))CancelCaptureScopeLocked("IDENTITY_CHANGED",stamp);
}
static std::string CaptureReject(const char* reason){
 return std::string("{\"revision\":\"T417_CAPTURE_REQUEST_V1\",\"state\":\"REJECTED\",\"reason\":\"")+reason+"\"}";
}
const char* CaptureRequestStateLocked()const{
 switch(captureRequest.state){
  case CaptureRequest::RESERVED:return "RESERVED";
  case CaptureRequest::ARMED:return evidence&&evidence->id==captureRequest.captureId&&evidence->frozen?"FROZEN":"ARMED";
  case CaptureRequest::CANCELLED:return "CANCELLED";
  case CaptureRequest::TIMED_OUT:return "TIMED_OUT";
  default:return "IDLE";
 }
}
void CaptureRequestJsonLocked(std::ostream& out)const{
 const auto& r=captureRequest;const auto& c=r.config;
 if(r.state==CaptureRequest::NONE){out<<"{\"revision\":\"T417_CAPTURE_REQUEST_V1\",\"state\":\"IDLE\",\"reason\":\"\"}";return;}
 const auto* reason=r.state==CaptureRequest::ARMED&&evidence&&evidence->frozen?evidence->reason.data():r.reason;
 auto word=[](U64 value){return (uint64_t(value.hi)<<32)|value.lo;};
 out<<"{\"revision\":\"T417_CAPTURE_REQUEST_V1\",\"state\":\""<<CaptureRequestStateLocked()<<"\",\"reason\":\""<<reason
  <<"\",\"requestId\":\""<<r.id<<"\",\"requestNs\":\""<<r.requestNs<<"\",\"armByNs\":\""<<r.armByNs
  <<"\",\"detectedNs\":\""<<r.detectedNs<<"\",\"detectionDelayNs\":\""<<(r.detectedNs>=r.armByNs&&r.state==CaptureRequest::TIMED_OUT?r.detectedNs-r.armByNs:0)
  <<"\",\"captureId\":\""<<r.captureId<<"\",\"startNs\":\""<<r.startNs<<"\",\"epoch\":\""<<c.epoch
  <<"\",\"runLo\":"<<c.runLo<<",\"runHi\":"<<c.runHi<<",\"pageSession\":\""<<c.pageSession
  <<"\",\"actualRevision\":\""<<c.expectedRevision<<"\",\"packageSha256\":\""<<c.packageSha256
  <<"\",\"owner\":\""<<word(r.identity.owner)<<"\",\"context\":\""<<word(r.identity.context)<<"\",\"surface\":\""<<word(r.identity.surface)
  <<"\",\"geometry\":\""<<word(r.identity.geometry)<<"\",\"config\":\""<<word(r.identity.config)
  <<"\",\"lastBlocker\":\""<<r.blocker<<"\",\"observedNs\":\""<<r.observedNs<<"\",\"safePointSerial\":\""<<r.safePointSerial
  <<"\",\"workerPhase\":\""<<r.workerPhase<<"\",\"phaseSinceNs\":\""<<r.phaseSinceNs<<"\",\"rootNonce\":\""<<r.rootNonce
  <<"\",\"rootKind\":"<<r.rootKind<<",\"permitStartNs\":\""<<r.permitStartNs<<"\",\"permitDeadlineNs\":\""<<r.permitDeadlineNs
  <<"\",\"lastCompleted\":\""<<r.completed<<"\",\"draining\":"<<(r.draining?"true":"false")<<",\"drawing\":"<<(r.drawing?"true":"false")
  <<",\"swapping\":"<<(r.swapping?"true":"false")<<",\"initialPhase\":\""<<r.initialPhase.data()
  <<"\",\"initialPermission\":\""<<r.initialPermission.data()<<"\",\"initialFailure\":\"";
 for(const char* p=r.initialFailure.data();*p;++p){if(*p=='"'||*p=='\\')out<<'\\';if(static_cast<unsigned char>(*p)>=32)out<<*p;}
 out<<"\"}";
}
std::string ReadCaptureRequestLocked(uint64_t id,bool cancel=false){
 if(!id||id!=captureRequest.id)return CaptureReject("STALE_CAPTURE_REQUEST");
 if(cancel){const auto stamp=Stamp();CheckCaptureRequestLocked(stamp);CancelCaptureScopeLocked("EXPLICIT_CANCEL",stamp);}
 std::ostringstream out;CaptureRequestJsonLocked(out);return out.str();
}
void RecordCaptureOwnerLocked(const char* worker,const char* blocker,uint64_t stamp){
 if(std::strcmp(worker,captureOwnerPhase)!=0){captureOwnerPhase=worker;captureOwnerPhaseNs=stamp;}
 auto& r=captureRequest;
 if(r.state!=CaptureRequest::RESERVED)return;
 r.observedNs=stamp;r.workerPhase=worker;r.phaseSinceNs=captureOwnerPhaseNs;r.blocker=blocker;
 r.rootNonce=permit.nonce;r.rootKind=permitKind;r.permitStartNs=permitStarted;r.permitDeadlineNs=permit.deadline;
 r.completed=captureCompleted;r.draining=std::strcmp(blocker,"WAIT_REVOKED_DRAIN")==0;
 r.drawing=std::strcmp(worker,"DRAW")==0;r.swapping=std::strcmp(worker,"SWAP")==0;
}
std::string ReserveEvidenceLocked(EvidenceConfig config){
 const auto stamp=Stamp();CheckCaptureRequestLocked(stamp);
 if(!config.Valid())return CaptureReject("INVALID_CAPTURE_CONFIG");
 if(config.maxRows<16||config.maxBytes<EvidenceCapture::EnvelopeBudget)return CaptureReject("CAPTURE_BUDGET_TOO_SMALL");
 if(!active||!window||!config.pageSession||!(identity.run.lo||identity.run.hi)||captureCommittedEpoch!=epoch.load())return CaptureReject("CAPTURE_SCOPE_UNAVAILABLE");
 if(!CaptureScopeLocked(config))return CaptureReject("CAPTURE_IDENTITY_MISMATCH");
 if(captureRequest.state==CaptureRequest::RESERVED){
  if(SameCaptureConfig(config,captureRequest.config))return ReadCaptureRequestLocked(captureRequest.id);
  return CaptureReject("CAPTURE_REQUEST_PENDING");
 }
 if(evidence&&evidence->armed)return CaptureReject("CAPTURE_ALREADY_ARMED");
 if(!capturePrepared)return CaptureReject("CAPTURE_STORAGE_NOT_READY");
 if(captureRequestSerial==UINT64_MAX||captureSerial==UINT64_MAX||stamp>UINT64_MAX-1000000000ULL-config.durationNs)
  return CaptureReject("CAPTURE_ID_EXHAUSTED");
 captureRequest={};auto& r=captureRequest;r.state=CaptureRequest::RESERVED;r.id=++captureRequestSerial;
 r.requestNs=stamp;r.armByNs=stamp+1000000000ULL;r.config=std::move(config);r.identity=identity;r.window=window;
 const auto waiting=std::strcmp(captureOwnerPhase,"DRAW")==0||std::strcmp(captureOwnerPhase,"SWAP")==0||std::strcmp(captureOwnerPhase,"PRESENT_HANDOFF")==0;
 const auto root=std::strcmp(captureOwnerPhase,"ROOT")==0;
 const auto drain=std::strcmp(captureOwnerPhase,"DRAIN")==0;
 RecordCaptureOwnerLocked(captureOwnerPhase,waiting?"WAIT_PRESENT":drain?"WAIT_REVOKED_DRAIN":root?"WAIT_ROOT":"NO_OWNER_CHECKPOINT",stamp);
 captureWake=true;wake.notify_all();return ReadCaptureRequestLocked(r.id);
}
void TryArmReservedLocked(const char* blocker,uint64_t stamp){
 CheckCaptureRequestLocked(stamp);
 if(captureRequest.state!=CaptureRequest::RESERVED)return;
 RecordCaptureOwnerLocked("OWNER_CHECKPOINT",blocker,stamp);
 if(*blocker||!capturePrepared)return;
 auto& r=captureRequest;r.safePointSerial=++captureSafeSerial;r.state=CaptureRequest::ARMED;
 r.captureId=++captureSerial;r.startNs=stamp;r.detectedNs=stamp;
 EvidenceCapture::Copy(r.initialPhase,phase);EvidenceCapture::Copy(r.initialPermission,permission);
 EvidenceCapture::Copy(r.initialFailure,firstFailure.c_str());
#ifdef GRS_BRIDGE_HOST_TEST
 capturePrepared->evidenceSource="HOST_SCRIPTED_OFFLINE";capturePrepared->clockDomain="SCRIPTED_OFFLINE_STAMP_NS";
 capturePrepared->uiDomain="SCRIPTED_UI_CALLBACK_NS";capturePrepared->probeDomain="HOST_STEADY_NS";
#endif
 capturePrepared->Begin(r.config,r.captureId,stamp,EvidenceLedger::Of(progress));
 captureRetired=std::move(evidence);evidence=std::move(capturePrepared);downRecorded={};
 evidence->lifetimeNativeStart=progress.maxNativeGap;evidence->lifetimeUiStart=progress.maxUiGap;
 EvidenceKeyLocked("BEGIN","");CaptureReadyLocked();
}
#endif
