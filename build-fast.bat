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

nasm.exe -fwin32 -o intro.o32 intro.asm || exit /b 1

link.exe ^
	%OPTS% ^
	/COMPMODE:FAST /REPORT:report-fast.html ^
	%LIBS% ^
	intro.o32 /OUT:intro-fast.exe ^
	|| exit /b 2

pause
