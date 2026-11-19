#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib,"Dbghelp.lib")
namespace t496diag {
inline std::string prefix;
inline HANDLE activeFile=INVALID_HANDLE_VALUE,map=nullptr;inline uint64_t* words=nullptr;
inline void Sample(uint64_t phase,uint64_t function,uint64_t call,uint64_t ns){if(!words)return;++words[0];words[1]=phase;words[2]=function;words[3]=call;words[4]=GetCurrentThreadId();words[5]=ns;++words[0];}
inline void EvidenceBudget(){if(words)words[6]=1;}
inline LONG WINAPI Exception(EXCEPTION_POINTERS* e){
 std::fprintf(stderr,"EXCEPTION code=0x%08lx address=%p tid=%lu\n",e->ExceptionRecord->ExceptionCode,e->ExceptionRecord->ExceptionAddress,GetCurrentThreadId());std::fflush(stderr);
 const auto path=prefix+".dmp";HANDLE f=CreateFileA(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f!=INVALID_HANDLE_VALUE){MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(),e,FALSE};const auto ok=MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),f,MiniDumpNormal,&info,nullptr,nullptr);LARGE_INTEGER bytes{};GetFileSizeEx(f,&bytes);std::fprintf(stderr,"MINIDUMP result=%d error=%lu bytes=%lld\n",int(ok),GetLastError(),bytes.QuadPart);CloseHandle(f);if(bytes.QuadPart>67108864){DeleteFileA(path.c_str());std::fprintf(stderr,"MINIDUMP_EVIDENCE_BUDGET\n");}}
 return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Begin(const std::string& p){prefix=p;activeFile=CreateFileA((p+".active").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(activeFile==INVALID_HANDLE_VALUE)return false;map=CreateFileMappingA(activeFile,nullptr,PAGE_READWRITE,0,64,nullptr);if(!map)return false;words=static_cast<uint64_t*>(MapViewOfFile(map,FILE_MAP_WRITE,0,0,64));if(!words)return false;SetUnhandledExceptionFilter(Exception);return true;}
}
#else
namespace t496diag {inline void EvidenceBudget(){}inline void Sample(uint64_t,uint64_t,uint64_t,uint64_t){}inline bool Begin(const std::string&){return true;}}
#endif
