#!/bin/sh
# In-container build driver. Mirrors build-fast.bat / build-slow.bat /
# build-debug.bat; the repository is expected to be mounted at /work.
#
# Usage: bepct4k-build fast|slow|debug

set -eu

TARGET="${1:-fast}"

SHADER_MINIFIER="mono /opt/tools/shader_minifier.exe"
CRINKLER="wine /opt/tools/crinkler.exe"
NASM=nasm
OBJ=obj
OUT=out

# Same set as common.bat. Crinkler resolves these case-insensitively through
# wine, so libs/OpenGL32.Lib and friends are found as written here.
LIBS="/LIBPATH:libs opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib"

# /TINYHEADER is deliberately absent: very slow unpack.
RELEASE_OPTS="/ENTRY:start /CRINKLER /SUBSYSTEM:WINDOWS /UNSAFEIMPORT /NOINITIALIZERS /RANGE:opengl32 /PRINT:IMPORTS /PRINT:LABELS /TRANSFORM:CALLS /TINYIMPORT"
DEBUG_OPTS="/ENTRY:start /SUBSYSTEM:WINDOWS /PRINT:IMPORTS /PRINT:LABELS"

mkdir -p "$OBJ" "$OUT"

case "$TARGET" in
fast|slow)
	$SHADER_MINIFIER --format nasm -o "$OBJ/shader.inc" shader.frag
	$NASM -fwin32 -o "$OBJ/intro.obj" -i"$OBJ" src/intro.asm
	$NASM -fwin32 -o "$OBJ/4klang.obj" -i"$OBJ" src/4klang.asm
	;;
debug)
	# No music at all in this build: only intro.obj gets linked.
	$SHADER_MINIFIER --format nasm -o "$OBJ/shader.inc" shader.frag
	$NASM -fwin32 -o "$OBJ/intro.obj" -i"$OBJ" -DDEBUG src/intro.asm
	;;
*)
	echo "unknown target '$TARGET' (expected fast, slow or debug)" >&2
	exit 64
	;;
esac

case "$TARGET" in
fast)
	# shellcheck disable=SC2086 # option words must split
	$CRINKLER $RELEASE_OPTS /COMPMODE:FAST "/REPORT:$OUT/report-fast.html" \
		$LIBS "$OBJ/intro.obj" "$OBJ/4klang.obj" "/OUT:$OUT/intro-fast.exe"
	;;
slow)
	# shellcheck disable=SC2086
	$CRINKLER $RELEASE_OPTS /COMPMODE:SLOW /ORDERTRIES:16000 "/REPORT:$OUT/report-slow.html" \
		$LIBS "$OBJ/intro.obj" "$OBJ/4klang.obj" "/OUT:$OUT/intro-slow.exe"
	;;
debug)
	# shellcheck disable=SC2086
	$CRINKLER $DEBUG_OPTS \
		$LIBS "$OBJ/intro.obj" "/OUT:$OUT/intro-debug.exe"
	;;
esac
