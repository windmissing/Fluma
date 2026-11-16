 cold503::TraceManager coldTrace;
 cold503::Gate diagnosticGate;
 cold503::Scope ColdScopeLocked()const{
  cold503::Scope s;s.control=DiagnosticScopeLocked();
 #ifdef _WIN32
  s.pid=_getpid();
 #else
  s.pid=getpid();
 #endif
  std::strncpy(s.route.data(),kT492Route,s.route.size()-1);std::strncpy(s.source.data(),kT492Source,s.source.size()-1);return s;
 }
 void ColdMaintainLocked(){coldTrace.Maintain(Stamp(),ColdScopeLocked());}
