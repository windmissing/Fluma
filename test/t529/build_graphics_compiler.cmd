@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk test/t529/compile_graphics.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t529-graphics-compile.exe /Fo.codex/local-test/
exit /b %ERRORLEVEL%
