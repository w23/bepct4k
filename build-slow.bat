set LIBS= /LIBPATH:libs opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib
set OPTS= ^
	/ENTRY:start ^
	/CRINKLER ^
	/SUBSYSTEM:WINDOWS ^
	/UNSAFEIMPORT ^
	/NOINITIALIZERS ^
	/RANGE:opengl32 ^
	/PRINT:IMPORTS ^
	/PRINT:LABELS ^
	/TRANSFORM:CALLS ^
	/TINYIMPORT ^
	/TINYHEADER

nasm.exe -fwin32 -o jl13.o32 win1k.asm || exit /b 1

crinkler22\win64\crinkler.exe ^
	%OPTS% ^
	/COMPMODE:SLOW /ORDERTRIES:16000 /REPORT:report-slow.html ^
	%LIBS% ^
	jl13.o32 /OUT:jl13-slow.exe ^
	|| exit /b 2

pause
