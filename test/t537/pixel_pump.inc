if(pixelSelected){
 bool allowed=false,drawn=false;
 {std::lock_guard<std::mutex> lock(mutex);allowed=PixelAllowedLocked(seen,held,Stamp());drawn=pixelDrawn;}
 if(!allowed||drawn)continue;
 EGLint w=0,h=0;const bool queried=eglQuerySurface(display,surface,EGL_WIDTH,&w)&&eglQuerySurface(display,surface,EGL_HEIGHT,&h);const auto queryError=queried?EGL_SUCCESS:eglGetError();
 ResidentSceneFrame diagnosticFrame;diagnosticFrame.width=w;diagnosticFrame.height=h;diagnosticFrame.boxWidth=float(w);diagnosticFrame.boxHeight=float(h);diagnosticFrame.context=diagnosticFrame.surface=diagnosticFrame.geometry=u64(seen);diagnosticFrame.renderFrame=1;
 struct PixelArgs {App* self;uint64_t epoch;void* window;EGLDisplay display;EGLSurface surface;};PixelArgs args{this,seen,held,display,surface};
 auto check=[](void* raw){auto& a=*static_cast<PixelArgs*>(raw);std::lock_guard<std::mutex> lock(a.self->mutex);return a.self->PixelAllowedLocked(a.epoch,a.window,a.self->Stamp());};
 auto swap=[](void* raw){auto& a=*static_cast<PixelArgs*>(raw);{std::lock_guard<std::mutex> lock(a.self->mutex);if(!a.self->PixelAllowedLocked(a.epoch,a.window,a.self->Stamp()))return false;}return eglSwapBuffers(a.display,a.surface)==EGL_TRUE;};
 input537::Pixel result;result.epoch=seen;result.egl=queryError==EGL_SUCCESS?0:uint32_t(queryError);const bool completed=queried&&w>0&&h>0&&scene.DrawDiagnostic(diagnosticFrame,seen,check,swap,&args,result);result.failed=!completed;
 {std::lock_guard<std::mutex> lock(mutex);if(CurrentLocked(seen,held)){pixel=result;pixelFailed=!completed;pixelDrawn=completed;status=completed?"{\"state\":\"DIAGNOSTIC_HOLD\",\"evidenceOrigin\":\"DIAGNOSTIC_NOT_PHYSICS\",\"physicsValidated\":false}":"{\"state\":\"FAULT\",\"stage\":\"T537_DIAGNOSTIC_DRAW\",\"physicsValidated\":false}";}}
 continue;
}
