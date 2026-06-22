if not defined in_subprocess (cmd /k set in_subprocess=y ^& %0 %*) & exit )

call common.bat

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
	/TINYIMPORT

REM very slow unpack, avoid /TINYHEADER

%SHADER_MINIFIER% --format nasm -o %OBJ%\shader.inc shader.frag || exit /b 1
%NASM% -fwin32 -o %OBJ%\intro.obj -i%OBJ% src\intro.asm || exit /b 2
%NASM% -fwin32 -o %OBJ%\4klang.obj -i%OBJ% src\4klang.asm || exit /b 3

%CRINKLER% ^
	%OPTS% ^
	/COMPMODE:SLOW /ORDERTRIES:16000 /REPORT:%OUT%\report-slow.html ^
	%LIBS% ^
	%OBJ%\intro.obj %OBJ%\4klang.obj /OUT:%OUT%\intro-slow.exe ^
	|| exit /b 2
