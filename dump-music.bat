if not defined in_subprocess (cmd /k set in_subprocess=y ^& %0 %*) & exit )

call common.bat

set OPTS= ^
	/ENTRY:start ^
	/SUBSYSTEM:WINDOWS ^
	/PRINT:IMPORTS ^
	/PRINT:LABELS

%NASM% -fwin32 -o dump-music.obj -DDEBUG dump-music.asm || exit /b 2
%NASM% -fwin32 -o 4klang.obj 4klang.asm || exit /b 3

%CRINKLER% ^
	%OPTS% ^
	%LIBS% ^
	dump-music.obj 4klang.obj /OUT:dump-music.exe ^
	|| exit /b 2

dump-music.exe
