#define AUDIO_IMPLEMENT
#include "aud_io.h"

#include "atto/app.h"

#define ATTO_GL_H_IMPLEMENT
#include "atto/gl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG(...) aAppDebugPrintf(__VA_ARGS__)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMSG
#define NOMSG
#endif
#include <windows.h>

typedef FILETIME FrFileTime;

static FrFileTime readFileTime(const char *filename) {
	FrFileTime ft = { 0, 0 };
	const HANDLE fh = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		MSG("Cannot check file '%s' modification time: %x", filename, GetLastError());
		return ft;
	}

	if (!GetFileTime(fh, NULL, NULL, &ft)) {
		MSG("Cannot get file '%s' modification time", filename);
		ft.dwHighDateTime = ft.dwLowDateTime = 0;
		return ft;
	}

	CloseHandle(fh);
	return ft;
}

static int areFileTimesEqual(const FrFileTime *a, const FrFileTime *b) {
	return a->dwHighDateTime == b->dwHighDateTime && a->dwLowDateTime == b->dwLowDateTime;
}

static int readFileContents(const char *filename, char *out, int max_size) {
	const HANDLE fh = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		MSG("Cannot open file '%s'", filename);
		return 0;
	}

	LARGE_INTEGER splurge_integer;
	if (!GetFileSizeEx(fh, &splurge_integer))
	{
		MSG("Cannot get file '%s'", filename);
		return 0;
	}

	if (splurge_integer.QuadPart > max_size) {
		MSG("File '%s' size %d is too large, max %d",
			filename, (int)splurge_integer.QuadPart, max_size);
		return 0;
	}

	const int size = (int)splurge_integer.QuadPart;
	DWORD bytes_read = 0;
	if (!ReadFile(fh, out, size, &bytes_read, NULL) || bytes_read != size) {
		MSG("Cannot read %d bytes from file '%s'", size, filename);
		return 0;
	}
	CloseHandle(fh);

	return size;
}

#define MAX_SHADER_SOURCE_SIZE 65536

static const char shader_filename[] = "shader.frag";
static FrFileTime shader_file_timestamp;
char shader_source[MAX_SHADER_SOURCE_SIZE];

static void keyPress(ATimeUs timestamp, AKey key, int pressed) {
	(void)(timestamp); (void)(pressed);
	if (key == AK_Esc)
		aAppTerminate(0);
}

static const char shader_vertex_passthrough[] =
	"attribute vec2 av2_pos;\n"
	"void main() {\n"
		"gl_Position = vec4(av2_pos, 0., 1.);\n"
	"}"
;

static const char shader_fragment_blit[] =
	"#version 130\n"
	"uniform sampler2D frame;\n"
	"uniform vec2 R;\n"
	"void main() {\n"
		"vec2 ts = vec2(textureSize(frame, 0));\n"
		"vec2 k = ts / R;\n"
		"float scale = max(k.x, k.y);\n"
		"vec2 off = (R-ts/scale)/2. * vec2(step(k.x, k.y), step(k.y, k.x));\n"
		"vec2 tc = scale * (gl_FragCoord.xy - off);\n"
		"gl_FragColor = vec4(0.);\n"
		"if (tc.x >= 0. && tc.x < ts.x && tc.y >= 0. && tc.y < ts.y)\n"
			"gl_FragColor = texture2D(frame, tc / (ts + vec2(1.)));\n"
	"}\n"
;

static const float vertexes_full_quad[] = {
	1.f, -1.f,
	1.f, 1.f,
	-1.f, -1.f,
	-1.f, 1.f,
};

static struct {
	AGLDrawMerge merge;

	AGLAttribute frame_attr[1];
	AGLProgramUniform frame_uniform[1];
	AGLDrawSource draw_frame;
	AGLDrawTarget target_frame;

	AGLTexture frame_texture;
	AGLFramebufferParams frame_buffer_params;

	AGLAttribute screen_attr[1];
	AGLProgramUniform screen_uniform[2];
	AGLDrawSource draw_screen;
	AGLDrawTarget target_screen;
} g;

static void init(void) {
	g.draw_screen.program = aGLProgramCreateSimple(shader_vertex_passthrough, shader_fragment_blit);
	if (g.draw_screen.program <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.frame_attr[0].name = "av2_pos";
	g.frame_attr[0].buffer = 0;
	g.frame_attr[0].size = 2;
	g.frame_attr[0].type = GL_FLOAT;
	g.frame_attr[0].normalized = GL_FALSE;
	g.frame_attr[0].stride = 0;
	g.frame_attr[0].ptr = vertexes_full_quad;

	g.frame_uniform[0].name = "t";
	g.frame_uniform[0].type = AGLAT_Float;
	g.frame_uniform[0].count = 1;

	g.draw_frame.primitive.mode = GL_TRIANGLE_STRIP;
	g.draw_frame.primitive.count = 4;
	g.draw_frame.primitive.first = 0;
	g.draw_frame.primitive.index.buffer = 0;
	g.draw_frame.primitive.index.data.ptr = 0;
	g.draw_frame.primitive.index.type = 0;

	g.draw_frame.attribs.p = g.frame_attr;
	g.draw_frame.attribs.n = sizeof g.frame_attr / sizeof *g.frame_attr;

	g.draw_frame.uniforms.p = g.frame_uniform;
	g.draw_frame.uniforms.n = sizeof g.frame_uniform / sizeof *g.frame_uniform;

	{
		AGLTextureUploadData data;
		data.format = AGLTF_U8_RGBA;
		data.x = data.y = 0;
		data.width = 1920;
		data.height = 1080;
		data.pixels = 0;

		g.frame_texture = aGLTextureCreate();
		aGLTextureUpload(&g.frame_texture, &data);
		g.frame_texture.min_filter = AGLTmF_Linear;
	}

	g.frame_buffer_params.depth.texture = 0;
	g.frame_buffer_params.depth.mode = AGLDBM_Texture;
	g.frame_buffer_params.color = &g.frame_texture;

	g.target_frame.viewport.x = g.target_frame.viewport.y = 0;
	g.target_frame.viewport.w = 1920;
	g.target_frame.viewport.h = 1080;
	g.target_frame.framebuffer = &g.frame_buffer_params;

	g.screen_attr[0].name = "av2_pos";
	g.screen_attr[0].buffer = 0;
	g.screen_attr[0].size = 2;
	g.screen_attr[0].type = GL_FLOAT;
	g.screen_attr[0].normalized = GL_FALSE;
	g.screen_attr[0].stride = 0;
	g.screen_attr[0].ptr = vertexes_full_quad;

	g.screen_uniform[0].name = "frame";
	g.screen_uniform[0].type = AGLAT_Texture;
	g.screen_uniform[0].value.texture = &g.frame_texture;
	g.screen_uniform[0].count = 1;

	g.screen_uniform[1].name = "R";
	g.screen_uniform[1].type = AGLAT_Vec2;
	g.screen_uniform[1].count = 1;

	g.draw_screen.primitive.mode = GL_TRIANGLE_STRIP;
	g.draw_screen.primitive.count = 4;
	g.draw_screen.primitive.first = 0;
	g.draw_screen.primitive.index.buffer = 0;
	g.draw_screen.primitive.index.data.ptr = 0;
	g.draw_screen.primitive.index.type = 0;

	g.draw_screen.attribs.p = g.screen_attr;
	g.draw_screen.attribs.n = sizeof g.screen_attr / sizeof *g.screen_attr;

	g.draw_screen.uniforms.p = g.screen_uniform;
	g.draw_screen.uniforms.n = sizeof g.screen_uniform / sizeof *g.screen_uniform;

	aGLAttributeLocate(g.draw_screen.program, g.screen_attr, g.draw_screen.attribs.n);
	aGLUniformLocate(g.draw_screen.program, g.screen_uniform, g.draw_screen.uniforms.n);

	g.merge.blend.enable = 0;
	g.merge.depth.mode = AGLDM_Disabled;
}

static void resize(ATimeUs timestamp, unsigned int old_w, unsigned int old_h) {
	(void)(timestamp); (void)(old_w); (void)(old_h);
	g.target_screen.viewport.x = g.target_screen.viewport.y = 0;
	g.target_screen.viewport.w = a_app_state->width;
	g.target_screen.viewport.h = a_app_state->height;

	g.target_screen.framebuffer = 0;
}

static struct {
	int pos;
	int paused;
	int start, end;
	int set;
} loop;

static struct {
	float *data;
	size_t samples;
	float samples_per_row;
} audio;

static void paint(ATimeUs timestamp, float dt) {
	(void)timestamp;
	(void)(dt);

	{
		const FrFileTime current_file_time = readFileTime(shader_filename);
		if (!areFileTimesEqual(&shader_file_timestamp, &current_file_time)) {
			shader_file_timestamp = current_file_time;
			const int size = readFileContents(shader_filename, shader_source, sizeof(shader_source) - 1);
			if (size > 0) {
				shader_source[size] = '\0';
				const AGLProgram new_program = aGLProgramCreateSimple(shader_vertex_passthrough, shader_source);
				if (new_program <= 0) {
					MSG("shader error: %s", a_gl_error);
				} else {
					MSG("read new shader size %d", size);

					if (g.draw_frame.program > 0)
						aGLProgramDestroy(g.draw_frame.program);
					g.draw_frame.program = new_program;

					aGLAttributeLocate(g.draw_frame.program, g.frame_attr, g.draw_frame.attribs.n);
					aGLUniformLocate(g.draw_frame.program, g.frame_uniform, g.draw_frame.uniforms.n);
				}
			}
		}
	}

	if (g.draw_frame.program > 0) {
		const float time_row = (float)loop.pos / audio.samples_per_row;
		g.frame_uniform[0].value.pf = &time_row;
		aGLDraw(&g.draw_frame, &g.merge, &g.target_frame);
	} else {
		AGLClearParams clear;

		clear.a = 1.f;
		clear.r = 1.f;//sinf(t*.1f);
		clear.g = 0.f;//sinf(t*.2f);
		clear.b = 0.f;//sinf(t*.3f);
		clear.depth = 0;
		clear.bits = AGLCB_Everything;

		aGLClear(&clear, &g.target_frame);
	}

	float R[2] = { a_app_state->width, a_app_state->height };
	g.screen_uniform[1].value.pf = R;
	aGLDraw(&g.draw_screen, &g.merge, &g.target_screen);
}

static void audioCallback(void *unused, float *samples, int nsamples) {
	(void)unused;
	if (loop.paused || !audio.data) {
		memset(samples, 0, sizeof(*samples) * nsamples * 2);
		if (!loop.paused)
			loop.pos = (loop.pos + nsamples) % audio.samples;
		return;
	}

	for (int i = 0; i < nsamples; ++i) {
		samples[i * 2] = audio.data[loop.pos * 2];
		samples[i * 2 + 1] = audio.data[loop.pos * 2 + 1];
		loop.pos = (loop.pos + 1) % audio.samples;

		if (loop.set == 2)
			if (loop.pos >= loop.end)
				loop.pos = loop.start;
	}
}

void attoAppInit(struct AAppProctable *proctable) {
	aGLInit();
	init();

	proctable->resize = resize;
	proctable->paint = paint;
	proctable->key = keyPress;

	FILE *f = fopen("music.raw", "rb");
	if (!f) {
		MSG("Cannot open music.raw");
	} else {
		fseek(f, 0, SEEK_END);
		const size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		audio.data = (float*)malloc(size);
		audio.samples = size / (sizeof(float) * 2);
		fread(audio.data, size, 1, f);
		fclose(f);
	}

	audio.samples_per_row = /*FIXME*/5512;

	loop.start = 0;
	loop.end = audio.samples ? audio.samples : 44100 * 60;

	audioOpen(44100, 2, nullptr, audioCallback, nullptr, nullptr);
}
