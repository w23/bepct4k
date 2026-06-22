BITS 32

%include "4klang.inc"
extern __4klang_render@4

%macro WINAPI_FUNC 2
%if 1
	extern __imp__ %+ %1 %+ @ %+ %2
	%define %1 [__imp__ %+ %1 %+ @ %+ %2]
%else
	extern _ %+ %1 %+ @ %+ %2
	%define %1 _ %+ %1 %+ @ %+ %2
%endif
%endmacro

WINAPI_FUNC ExitProcess, 4
WINAPI_FUNC CreateFileA, 28
WINAPI_FUNC WriteFile, 20
WINAPI_FUNC CloseHandle, 4

GENERIC_READ EQU 0x80000000
GENERIC_WRITE EQU 0x40000000
FILE_SHARE_READ EQU 0x00000001
CREATE_ALWAYS EQU 2
FILE_ATTRIBUTE_NORMAL EQU 0x80

section _sndbuf bss align=1
sound_buffer: resd MAX_SAMPLES * 2
sound_buffer_end:

%macro FNCALL 1-*
	%rep %0-1
		%rotate -1
		push %1
	%endrep
	%rotate -1
	call %1
%endmacro

section _filename data align=1
filename: db 'music.raw', 0

section _text text align=1
_start:
	FNCALL __4klang_render@4, sound_buffer
	FNCALL CreateFileA, filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0
	push eax
	FNCALL WriteFile, eax, sound_buffer, sound_buffer_end - sound_buffer, 0, 0
	pop eax
	FNCALL CloseHandle, eax
	call ExitProcess
