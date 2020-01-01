set LIBS= /LIBPATH:libs opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib

set SHADER_MINIFIER=deps\shader_minifier.exe
set NASM=deps\nasm-2.14.02\nasm.exe
set CRINKLER=deps\crinkler22\win64\crinkler.exe

if not exist %SHADER_MINIFIER% goto get_deps
if not exist %NASM% goto get_deps
if not exist %CRINKLER% goto get_deps
goto end

:get_deps
Powershell.exe -ExecutionPolicy RemoteSigned -File deps\get-deps.ps1

:end
