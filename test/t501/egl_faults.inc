static unsigned failureStage=0,failureRemaining=0;
static bool Once(unsigned stage){if(stage==failureStage&&failureRemaining){--failureRemaining;return true;}return false;}
static EGLBoolean ObservedUnbind(EGLDisplay d,EGLSurface a,EGLSurface b,EGLContext c){return eglMakeCurrent(Once(1)?EGL_NO_DISPLAY:d,a,b,c);}
static EGLBoolean ObservedDestroySurface(EGLDisplay d,EGLSurface s){return eglDestroySurface(Once(2)?EGL_NO_DISPLAY:d,s);}
static EGLBoolean ObservedDestroyContext(EGLDisplay d,EGLContext c){return eglDestroyContext(Once(3)?EGL_NO_DISPLAY:d,c);}
static EGLBoolean ObservedTerminate(EGLDisplay d){return eglTerminate(Once(4)?EGL_NO_DISPLAY:d);}
#define eglMakeCurrent ObservedUnbind
#define eglDestroySurface ObservedDestroySurface
#define eglDestroyContext ObservedDestroyContext
#define eglTerminate ObservedTerminate
