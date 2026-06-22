if not defined in_subprocess (cmd /k set in_subprocess=y ^& %0 %*) & exit )

call common.bat

set OPTS= ^
	/ENTRY:start ^
	/SUBSYSTEM:WINDOWS ^
	/PRINT:IMPORTS ^
	/PRINT:LABELS

%NASM% -fwin32 -o %OBJ%\dump-music.obj -DDEBUG src\dump-music.asm || exit /b 2
%NASM% -fwin32 -o %OBJ%\4klang.obj -i%OBJ% src\4klang.asm || exit /b 3

%CRINKLER% ^
	%OPTS% ^
	%LIBS% ^
	%OBJ%\dump-music.obj %OBJ%\4klang.obj /OUT:%OUT%\dump-music.exe ^
	|| exit /b 2

%OUT%\dump-music.exe
move music.raw %OBJ%\
