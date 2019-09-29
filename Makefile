INTRO ?= intro
WIDTH ?= 1920
HEIGHT ?= 1080

WINE = wine
SHMIN = mono shader_minifier.exe
#CRINKLER = $(WINE) crinkler20a/crinkler.exe
#CRINKLER = $(WINE) crinkler21a/win64/crinkler.exe
CRINKLER = $(WINE) link.exe

BUILDDIR = build
OBJDIR = $(BUILDDIR)
OBJDIR_WIN32 = $(BUILDDIR)/win32

INTRO_WIN32_SRCS = 4klang.asm $(INTRO).asm
INTRO_WIN32_OBJS = $(INTRO_WIN32_SRCS:%=$(OBJDIR_WIN32)/%.obj)
INTRO_WIN32_LIBS = /LIBPATH:libs opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib
INTRO_WIN32_CRINKLER_OPTS = \
	/CRINKLER \
	/ENTRY:entrypoint \
	/NOINITIALIZERS \
	/PRINT:IMPORTS \
	/PRINT:LABELS \
	/RANGE:opengl32 \
	/SUBSYSTEM:WINDOWS \
	/TRANSFORM:CALLS

	# /UNSAFEIMPORT \
	# /TINYIMPORT \ unportable
	# 1k? /TINYHEADER

#all: stameska
#STAMESKA_BASEDIR = tool
#include $(STAMESKA_BASEDIR)/stameska.mk

fast: $(INTRO)-fast.exe
slow: $(INTRO)-slow.exe

$(INTRO)-fast.exe: $(INTRO_WIN32_OBJS)
	$(CRINKLER) \
		$(INTRO_WIN32_CRINKLER_OPTS) \
		/COMPMODE:FAST /REPORT:report-fast.html \
		$(INTRO_WIN32_LIBS) \
		$(INTRO_WIN32_OBJS) \
		/OUT:$@

$(INTRO)-slow.exe: $(INTRO_WIN32_OBJS)
	$(CRINKLER) \
		$(INTRO_WIN32_CRINKLER_OPTS) \
		/HASHTRIES:10000 /COMPMODE:SLOW /ORDERTRIES:20000 /REPORT:report-slow.html /SATURATE /HASHSIZE:900 /UNSAFEIMPORT /TINYIMPORT \
		$(INTRO_WIN32_LIBS) \
		$(INTRO_WIN32_OBJS) \
		/OUT:$@

$(OBJDIR_WIN32)/$(INTRO).asm.obj: shader_glsl.inc

$(OBJDIR_WIN32)/%.asm.obj: %.asm
	@mkdir -p $(dir $@)
	nasm -fwin32 -i4klang_win32/ $< -o $@

shaders/%.inc: shaders/%.glsl
	$(SHMIN) -o $@ --format nasm --preserve-externals $<

shaders/%.h: shaders/%.glsl
	$(SHMIN) -o $@ --preserve-externals $<

capture: $(INTRO)_$(WIDTH)_$(HEIGHT).mp4
test-capture: test_$(INTRO)_$(WIDTH)_$(HEIGHT).mp4

DUMP_AUDIO_EXE = $(OBJDIR)/dump_audio
DUMP_AUDIO_SRCS = dump_audio.c
DUMP_AUDIO_OBJS = $(DUMP_AUDIO_SRCS:%=$(OBJDIR)/%.o32)
DUMP_AUDIO_DEPS = $(DUMP_AUDIO_OBJS:%=%.d)

-include $(DUMP_AUDIO_DEPS)

$(OBJDIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) -c $< -o $@

$(OBJDIR)/%.asm.obj: %.asm main.shader.inc
	@mkdir -p $(dir $@)
	nasm -f win32 -i4klang_win32/ $< -o $@

$(OBJDIR)/%.c.o32: %.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) -ggdb3 -m32 -c $< -o $@

$(OBJDIR)/4klang.o32: 4klang.asm ./4klang_linux/4klang.inc
	nasm -f elf32 -gdwarf -I./4klang_linux/ 4klang.asm -o $@

$(DUMP_AUDIO_EXE): $(DUMP_AUDIO_OBJS) $(OBJDIR)/4klang.o32
	$(CC) -m32 $(LIBS) $^ -o $@

audio.raw: $(DUMP_AUDIO_EXE)
	$(DUMP_AUDIO_EXE) $@

CRF=23

FFMPEG_ARGS = \
	-f f32le -ar 44100 -ac 2 \
	-i audio.raw \
	-s:v $(WIDTH)x$(HEIGHT) -pix_fmt rgb24 \
	-y -f rawvideo -vcodec rawvideo \
	-framerate 60 \
	-i - \
	-s:v 3840:2160 \
	-vf scale=1920:1200:flags=neighbor \
	-c:a aac -b:a 160k \
	-c:v libx264 -vf vflip \
	-movflags +faststart \
	-level 4.1 -profile:v high -preset veryslow -crf $(CRF) -pix_fmt yuv420p \
	-tune film

#-s:v 3840:2160 \
#-vf scale=1920:1200:flags=neighbor \
# -vf scale=3840:2160:flags=neighbor \
#	-x264-params keyint=600:bframes=3:scenecut=60:ref=3:qpmin=10:qpstep=8:vbv-bufsize=24000:vbv-maxrate=24000:merange=32 \

$(INTRO)_$(WIDTH)_$(HEIGHT).mp4: $(INTRO).capture audio.raw
	./$(INTRO).capture | ffmpeg \
	$(FFMPEG_ARGS) \
	$@

test_$(INTRO)_$(WIDTH)_$(HEIGHT).mp4: $(INTRO).capture audio.raw
	./$(INTRO).capture | ffmpeg \
	$(FFMPEG_ARGS) \
	-t 10 \
	$@

intro.c: shaders/shader_glsl.h 4klang_linux/4klang.h

$(INTRO).capture: intro.c
	$(CC) -DXRES=$(WIDTH) -DYRES=$(HEIGHT) -O0 -ggdb3 -Wall -Wno-unknown-pragmas -I. \
		-DCAPTURE `pkg-config --cflags --libs sdl` -lGL \
		$^ -o $@

$(INTRO)-c: intro.c
	$(CC) -DXRES=$(WIDTH) -DYRES=$(HEIGHT) -O0 -ggdb3 -Wall -Wno-unknown-pragmas -I. \
		-DNO_AUDIO -DDEBUG `pkg-config --cflags --libs sdl` -lGL \
		$^ -o $@

.PHONY: all clean run_tool debug_tool
