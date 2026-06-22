if not defined in_subprocess (cmd /k set in_subprocess=y ^& %0 %*) & exit )

call common.bat

set OPTS= ^
	/ENTRY:start ^
	/SUBSYSTEM:WINDOWS ^
	/PRINT:IMPORTS ^
	/PRINT:LABELS

REM very slow unpack, avoid /TINYHEADER

%SHADER_MINIFIER% --format nasm -o %OBJ%\shader.inc shader.frag || exit /b 1
%NASM% -fwin32 -o %OBJ%\intro.obj -i%OBJ% -DDEBUG src\intro.asm || exit /b 2

%CRINKLER% ^
	%OPTS% ^
	%LIBS% ^
	%OBJ%\intro.obj /OUT:%OUT%\intro-debug.exe ^
	|| exit /b 2
