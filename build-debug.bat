if not defined in_subprocess (cmd /k set in_subprocess=y ^& %0 %*) & exit )

set WIDTH=1920
set HEIGHT=1080

set LIBS= /LIBPATH:libs opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib
set OPTS= ^
	/ENTRY:start ^
	/SUBSYSTEM:WINDOWS ^
	/PRINT:IMPORTS ^
	/PRINT:LABELS

REM very slow unpack, avoid /TINYHEADER

shader_minifier.exe --format nasm -o shader.inc shader.frag || exit /b 1
nasm.exe -fwin32 -o intro.obj -DDEBUG intro.asm || exit /b 2

crinkler.exe ^
	%OPTS% ^
	%LIBS% ^
	intro.obj /OUT:intro-debug.exe ^
	|| exit /b 2
