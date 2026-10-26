@echo off
rem Core suite is included in the combined host executable to share one verified runner.
call test\run_t75_material_bridge.cmd
exit /b %errorlevel%
