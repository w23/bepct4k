#include "atto/app.h"

#define ATTO_GL_H_IMPLEMENT
#include "atto/gl.h"

#include <math.h>

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

static const char shader_vertex[] =
	"attribute vec2 av2_pos;\n"
	"void main() {\n"
		"gl_Position = vec4(av2_pos, 0., 1.);\n"
	"}"
;

/*
static const char shader_fragment[] =
	"void main() {\n"
		"gl_FragColor = vec4(vv3_color, 1.);\n"
	"}"
;
*/

static const float vertexes[] = {
	1.f, -1.f,
	1.f, 1.f,
	-1.f, -1.f,
	-1.f, 1.f,
};

static struct {
	AGLAttribute attr[1];
	AGLProgramUniform pun[1];
	AGLDrawSource draw;
	AGLDrawMerge merge;
	AGLDrawTarget target;
} g;

static void init(void) {
#if 0
	g.draw.program = aGLProgramCreateSimple(shader_vertex, shader_fragment);
	if (g.draw.program <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}
#endif

	g.attr[0].name = "av2_pos";
	g.attr[0].buffer = 0;
	g.attr[0].size = 2;
	g.attr[0].type = GL_FLOAT;
	g.attr[0].normalized = GL_FALSE;
	g.attr[0].stride = 0;
	g.attr[0].ptr = vertexes;

	g.pun[0].name = "t";
	g.pun[0].type = AGLAT_Float;
	g.pun[0].count = 1;

	g.draw.primitive.mode = GL_TRIANGLE_STRIP;
	g.draw.primitive.count = 4;
	g.draw.primitive.first = 0;
	g.draw.primitive.index.buffer = 0;
	g.draw.primitive.index.data.ptr = 0;
	g.draw.primitive.index.type = 0;

	g.draw.attribs.p = g.attr;
	g.draw.attribs.n = sizeof g.attr / sizeof *g.attr;

	g.draw.uniforms.p = g.pun;
	g.draw.uniforms.n = sizeof g.pun / sizeof *g.pun;

	g.merge.blend.enable = 0;
	g.merge.depth.mode = AGLDM_Disabled;
}

static void resize(ATimeUs timestamp, unsigned int old_w, unsigned int old_h) {
	(void)(timestamp); (void)(old_w); (void)(old_h);
	g.target.viewport.x = g.target.viewport.y = 0;
	g.target.viewport.w = a_app_state->width;
	g.target.viewport.h = a_app_state->height;

	g.target.framebuffer = 0;
}

static void paint(ATimeUs timestamp, float dt) {
	float t = timestamp * 1e-6f;

	{
		const FrFileTime current_file_time = readFileTime(shader_filename);
		if (!areFileTimesEqual(&shader_file_timestamp, &current_file_time)) {
			shader_file_timestamp = current_file_time;
			const int size = readFileContents(shader_filename, shader_source, sizeof(shader_source) - 1);
			if (size > 0) {
				shader_source[size] = '\0';
				const AGLProgram new_program = aGLProgramCreateSimple(shader_vertex, shader_source);
				if (new_program <= 0) {
					MSG("shader error: %s", a_gl_error);
				} else {
					MSG("read new shader size %d", size);
					g.draw.program = new_program;

					aGLAttributeLocate(g.draw.program, g.attr, g.draw.attribs.n);
					aGLUniformLocate(g.draw.program, g.pun, g.draw.uniforms.n);
				}
			}
		}
	}


	if (g.draw.program > 0) {
		g.pun[0].value.pf = &t;
		aGLDraw(&g.draw, &g.merge, &g.target);
	} else {
		AGLClearParams clear;
		(void)(dt);

		clear.a = 1.f;
		clear.r = 1.f;//sinf(t*.1f);
		clear.g = 0.f;//sinf(t*.2f);
		clear.b = 0.f;//sinf(t*.3f);
		clear.depth = 0;
		clear.bits = AGLCB_Everything;

		aGLClear(&clear, &g.target);
	}
}

void attoAppInit(struct AAppProctable *proctable) {
	aGLInit();
	init();

	proctable->resize = resize;
	proctable->paint = paint;
	proctable->key = keyPress;
}
