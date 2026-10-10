/* glcheck: functional OpenGL 4.3 feature test + limits + micro-benchmarks.
 * usage: glcheck.exe <major> <minor> [core|compat] [full|benchonly]
 * Writes results to stdout and glcheck.txt in the working directory. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static FILE *g_out;
#define P(...) do { printf(__VA_ARGS__); fflush(stdout); if (g_out) { fprintf(g_out, __VA_ARGS__); fflush(g_out); } } while (0)

#define GLFUNCS \
  X(PFNGLGENBUFFERSPROC, glGenBuffers) X(PFNGLBINDBUFFERPROC, glBindBuffer) \
  X(PFNGLBUFFERDATAPROC, glBufferData) X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
  X(PFNGLGETBUFFERSUBDATAPROC, glGetBufferSubData) X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
  X(PFNGLCREATESHADERPROC, glCreateShader) X(PFNGLSHADERSOURCEPROC, glShaderSource) \
  X(PFNGLCOMPILESHADERPROC, glCompileShader) X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
  X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
  X(PFNGLATTACHSHADERPROC, glAttachShader) X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
  X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
  X(PFNGLUSEPROGRAMPROC, glUseProgram) X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
  X(PFNGLDELETESHADERPROC, glDeleteShader) X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
  X(PFNGLUNIFORM4FPROC, glUniform4f) X(PFNGLUNIFORM2FPROC, glUniform2f) \
  X(PFNGLDISPATCHCOMPUTEPROC, glDispatchCompute) X(PFNGLMEMORYBARRIERPROC, glMemoryBarrier) \
  X(PFNGLGETSTRINGIPROC, glGetStringi) X(PFNGLGETINTEGERI_VPROC, glGetIntegeri_v) \
  X(PFNGLGETINTEGER64VPROC, glGetInteger64v) \
  X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
  X(PFNGLDEBUGMESSAGECONTROLPROC, glDebugMessageControl) \
  X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
  X(PFNGLVERTEXATTRIBFORMATPROC, glVertexAttribFormat) X(PFNGLVERTEXATTRIBBINDINGPROC, glVertexAttribBinding) \
  X(PFNGLBINDVERTEXBUFFERPROC, glBindVertexBuffer) X(PFNGLVERTEXBINDINGDIVISORPROC, glVertexBindingDivisor) \
  X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
  X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
  X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
  X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
  X(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer) X(PFNGLINVALIDATEFRAMEBUFFERPROC, glInvalidateFramebuffer) \
  X(PFNGLTEXSTORAGE2DPROC, glTexStorage2D) X(PFNGLTEXSTORAGE3DPROC, glTexStorage3D) \
  X(PFNGLTEXSTORAGE2DMULTISAMPLEPROC, glTexStorage2DMultisample) \
  X(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D) X(PFNGLCOMPRESSEDTEXIMAGE2DPROC, glCompressedTexImage2D) \
  X(PFNGLACTIVETEXTUREPROC, glActiveTexture) X(PFNGLBINDIMAGETEXTUREPROC, glBindImageTexture) \
  X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
  X(PFNGLMULTIDRAWARRAYSINDIRECTPROC, glMultiDrawArraysIndirect) \
  X(PFNGLPATCHPARAMETERIPROC, glPatchParameteri) X(PFNGLTEXTUREVIEWPROC, glTextureView) \
  X(PFNGLCOPYIMAGESUBDATAPROC, glCopyImageSubData) X(PFNGLCLEARBUFFERDATAPROC, glClearBufferData) \
  X(PFNGLGENQUERIESPROC, glGenQueries) X(PFNGLBEGINQUERYPROC, glBeginQuery) \
  X(PFNGLENDQUERYPROC, glEndQuery) X(PFNGLGETQUERYOBJECTUI64VPROC, glGetQueryObjectui64v) \
  X(PFNGLGETPROGRAMRESOURCEINDEXPROC, glGetProgramResourceIndex) \
  X(PFNGLGETINTERNALFORMATIVPROC, glGetInternalformativ) \
  X(PFNGLBUFFERSTORAGEPROC, glBufferStorage) X(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange) \
  X(PFNGLUNMAPBUFFERPROC, glUnmapBuffer) X(PFNGLFLUSHMAPPEDBUFFERRANGEPROC, glFlushMappedBufferRange) \
  X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) X(PFNGLFENCESYNCPROC, glFenceSync) \
  X(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync) X(PFNGLDELETESYNCPROC, glDeleteSync) \
  X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
  X(PFNGLNAMEDBUFFERDATAPROC, glNamedBufferData) X(PFNGLTEXIMAGE3DPROC, glTexImage3D) X(PFNGLUNIFORM1FPROC, glUniform1f) X(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation) X(PFNGLUNIFORM1IPROC, glUniform1i) X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) X(PFNGLPROVOKINGVERTEXPROC, glProvokingVertex) X(PFNGLMULTIDRAWELEMENTSINDIRECTPROC, glMultiDrawElementsIndirect) X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
  X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)

#define X(t, n) static t n;
GLFUNCS
#undef X

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static void load_funcs(void)
{
#define X(t, n) n = (t)wglGetProcAddress(#n);
  GLFUNCS
#undef X
}

/* ---------- test bookkeeping ---------- */
static int g_dbg_errs, g_dbg_printed, g_pass, g_fail, g_skip, t_errs0;

static void APIENTRY dbgcb(GLenum src, GLenum type, GLuint id, GLenum sev, GLsizei len, const GLchar *msg, const void *user)
{
  (void)src; (void)id; (void)len; (void)user;
  if (type == GL_DEBUG_TYPE_ERROR || sev == GL_DEBUG_SEVERITY_HIGH) {
    g_dbg_errs++;
    if (g_dbg_printed++ < 60) P("      [gl-debug error] %s\n", msg);
  } else if (sev == GL_DEBUG_SEVERITY_MEDIUM && g_dbg_printed++ < 60) {
    P("      [gl-debug warn] %s\n", msg);
  }
}

static void tbegin(void) { while (glGetError() != GL_NO_ERROR) {} t_errs0 = g_dbg_errs; }

static void tend(const char *name, int ok, const char *detail)
{
  GLenum e = glGetError();
  int de = g_dbg_errs - t_errs0;
  int pass = ok && e == GL_NO_ERROR && de == 0;
  if (pass) g_pass++; else g_fail++;
  P("  %-4s %-46s %s%s\n", pass ? "PASS" : "FAIL", name, detail ? detail : "",
    (e != GL_NO_ERROR || de) ? " [GL errors raised]" : "");
}

static void tskip(const char *name, const char *why) { g_skip++; P("  SKIP %-46s %s\n", name, why); }

static int has_ext(const char *e)
{
  GLint n = 0; glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (GLint i = 0; i < n; i++) if (!strcmp((const char *)glGetStringi(GL_EXTENSIONS, i), e)) return 1;
  return 0;
}

static GLuint compile(GLenum type, const char *src)
{
  GLuint s = glCreateShader(type); GLint ok = 0;
  glShaderSource(s, 1, &src, NULL); glCompileShader(s);
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) { char log[4096]; glGetShaderInfoLog(s, sizeof log, NULL, log); P("      compile error (stage 0x%x): %s\n", type, log); glDeleteShader(s); return 0; }
  return s;
}

static GLuint program(const char *vs, const char *tcs, const char *tes, const char *gs, const char *fs, const char *cs)
{
  const char *src[6] = { vs, tcs, tes, gs, fs, cs };
  GLenum ty[6] = { GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER };
  GLuint p = glCreateProgram(); GLint ok = 0;
  for (int i = 0; i < 6; i++) {
    if (!src[i]) continue;
    GLuint s = compile(ty[i], src[i]);
    if (!s) { glDeleteProgram(p); return 0; }
    glAttachShader(p, s); glDeleteShader(s);
  }
  glLinkProgram(p); glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) { char log[4096]; glGetProgramInfoLog(p, sizeof log, NULL, log); P("      link error: %s\n", log); glDeleteProgram(p); return 0; }
  return p;
}

typedef struct { GLuint fbo, color, depth; int w, h; } Target;

static Target target(int w, int h)
{
  Target t = { 0 }; t.w = w; t.h = h;
  glGenTextures(1, &t.color); glBindTexture(GL_TEXTURE_2D, t.color); glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glGenTextures(1, &t.depth); glBindTexture(GL_TEXTURE_2D, t.depth); glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);
  glGenFramebuffers(1, &t.fbo); glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.color, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, t.depth, 0);
  GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (st != GL_FRAMEBUFFER_COMPLETE) P("      framebuffer incomplete: 0x%x\n", st);
  glViewport(0, 0, w, h);
  return t;
}

static void clear(float r, float g, float b)
{
  glClearColor(r, g, b, 1); glClearDepth(1.0); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static int pixel_is(int x, int y, int r, int g, int b, char *detail)
{
  unsigned char p[4] = { 0 };
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
  int ok = abs(p[0] - r) <= 3 && abs(p[1] - g) <= 3 && abs(p[2] - b) <= 3;
  if (!ok && detail) sprintf(detail + strlen(detail), "(%d,%d)=%d,%d,%d want %d,%d,%d ", x, y, p[0], p[1], p[2], r, g, b);
  return ok;
}

static double now_ms(void)
{
  LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
  return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}

static const char *FS_TRI_VS =
  "#version 430 core\n"
  "void main(){ vec2 p=vec2(gl_VertexID==1?3.0:-1.0, gl_VertexID==2?3.0:-1.0); gl_Position=vec4(p,0.5,1.0); }\n";
static const char *SOLID_FS =
  "#version 430 core\n layout(location=0) uniform vec4 color; layout(location=0) out vec4 o; void main(){ o=color; }\n";

/* ---------- feature tests ---------- */
static void run_feature_tests(void)
{
  char d[512]; GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao);

  P("\n[functional tests]\n");

  /* KHR_debug delivers errors */
  {
    int before = g_dbg_errs;
    glBindBuffer(0xDEAD, 0); /* deliberate error */
    int got = g_dbg_errs > before;
    while (glGetError() != GL_NO_ERROR) {}
    if (got) g_pass++; else g_fail++;
    P("  %-4s %-46s %s\n", got ? "PASS" : "FAIL", "KHR_debug callback reports errors", "");
  }

  /* compute + SSBO */
  {
    tbegin();
    GLuint p = program(NULL, NULL, NULL, NULL, NULL,
      "#version 430\nlayout(local_size_x=64) in;\nlayout(std430,binding=0) buffer B { uint d[]; };\n"
      "void main(){ uint i=gl_GlobalInvocationID.x; d[i]=i*2u+1u; }\n");
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_SHADER_STORAGE_BUFFER, b);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 1024 * 4, NULL, GL_DYNAMIC_COPY); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, b);
    int ok = 0; d[0] = 0;
    if (p) {
      glUseProgram(p); glDispatchCompute(16, 1, 1); glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
      unsigned v[1024]; glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof v, v);
      ok = 1; for (int i = 0; i < 1024; i++) if (v[i] != (unsigned)i * 2 + 1) { ok = 0; sprintf(d, "v[%d]=%u ", i, v[i]); break; }
      GLuint idx = glGetProgramResourceIndex(p, GL_SHADER_STORAGE_BLOCK, "B");
      if (idx == GL_INVALID_INDEX) { ok = 0; strcat(d, "program_interface_query failed "); }
    }
    tend("compute shader writes SSBO (+interface query)", ok, d);
  }

  /* compute shared memory + barrier */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(NULL, NULL, NULL, NULL, NULL,
      "#version 430\nlayout(local_size_x=256) in;\nlayout(std430,binding=0) buffer B { uint d[]; };\nshared uint s[256];\n"
      "void main(){ uint l=gl_LocalInvocationIndex; s[l]=l; memoryBarrierShared(); barrier();\n"
      " for(uint st=128u; st>0u; st>>=1u){ if(l<st) s[l]+=s[l+st]; memoryBarrierShared(); barrier(); }\n"
      " if(l==0u) d[gl_WorkGroupID.x]=s[0]; }\n");
    int ok = 0;
    if (p) {
      glUseProgram(p); glDispatchCompute(4, 1, 1); glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
      unsigned v[4]; glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof v, v);
      ok = 1; for (int i = 0; i < 4; i++) if (v[i] != 32640u) { ok = 0; sprintf(d, "group %d sum=%u want 32640 ", i, v[i]); break; }
    }
    tend("compute shared memory + barrier()", ok, d);
  }

  /* SSBO atomics + atomic counters */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(NULL, NULL, NULL, NULL, NULL,
      "#version 430\nlayout(local_size_x=64) in;\nlayout(std430,binding=0) buffer B { uint counter; };\n"
      "layout(binding=0, offset=0) uniform atomic_uint ac;\n"
      "void main(){ atomicAdd(counter,1u); atomicCounterIncrement(ac); }\n");
    unsigned zero = 0; GLuint ab; glGenBuffers(1, &ab);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ab); glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, &zero, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, ab);
    GLuint sb; glGenBuffers(1, &sb); glBindBuffer(GL_SHADER_STORAGE_BUFFER, sb);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4, &zero, GL_DYNAMIC_COPY); glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sb);
    int ok = 0;
    if (p) {
      glUseProgram(p); glDispatchCompute(64, 1, 1); glMemoryBarrier(GL_ALL_BARRIER_BITS);
      unsigned a = 0, s = 0;
      glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4, &s);
      glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ab); glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 4, &a);
      ok = s == 4096 && a == 4096; if (!ok) sprintf(d, "ssbo=%u counter=%u want 4096 ", s, a);
    }
    tend("SSBO atomicAdd + atomic_uint counters", ok, d);
  }

  /* image load/store */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(NULL, NULL, NULL, NULL, NULL,
      "#version 430\nlayout(local_size_x=8,local_size_y=8) in;\nlayout(r32f,binding=0) uniform image2D img;\n"
      "void main(){ ivec2 q=ivec2(gl_GlobalInvocationID.xy); float v=imageLoad(img,q).r; imageStore(img,q,vec4(v+float(q.x+q.y*64),0,0,0)); }\n");
    static float init[64 * 64], out[64 * 64];
    for (int i = 0; i < 64 * 64; i++) init[i] = 0.5f;
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t); glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, 64, 64);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RED, GL_FLOAT, init);
    int ok = 0;
    if (p) {
      glUseProgram(p); glBindImageTexture(0, t, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
      glDispatchCompute(8, 8, 1); glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, out);
      ok = 1; for (int i = 0; i < 64 * 64; i++) if (fabsf(out[i] - (0.5f + i)) > 0.01f) { ok = 0; sprintf(d, "texel %d=%f ", i, out[i]); break; }
    }
    tend("image load/store (r32f)", ok, d);
  }

  Target T = target(256, 256);

  /* basic raster: vertex_attrib_binding, explicit uniform location, UBO, depth */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(
      "#version 430 core\nlayout(location=0) in vec2 pos; void main(){ gl_Position=vec4(pos,0.5,1.0); }\n",
      NULL, NULL, NULL,
      "#version 430 core\nlayout(location=0) uniform vec4 color;\nlayout(std140,binding=1) uniform U { vec4 tint; };\n"
      "layout(location=0) out vec4 o; void main(){ o=color*tint; }\n", NULL);
    float tri[] = { -1, -1, 3, -1, -1, 3 }, tint[4] = { 1, 1, 1, 1 };
    GLuint vb, ub, va; glGenBuffers(1, &vb); glBindBuffer(GL_ARRAY_BUFFER, vb); glBufferData(GL_ARRAY_BUFFER, sizeof tri, tri, GL_STATIC_DRAW);
    glGenBuffers(1, &ub); glBindBuffer(GL_UNIFORM_BUFFER, ub); glBufferData(GL_UNIFORM_BUFFER, 16, tint, GL_STATIC_DRAW); glBindBufferBase(GL_UNIFORM_BUFFER, 1, ub);
    glGenVertexArrays(1, &va); glBindVertexArray(va);
    glEnableVertexAttribArray(0); glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0); glVertexAttribBinding(0, 0); glBindVertexBuffer(0, vb, 0, 8);
    int ok = 0;
    if (p) {
      clear(0, 0, 0); glEnable(GL_DEPTH_TEST); glUseProgram(p); glUniform4f(0, 1.0f, 0.5f, 0.25f, 1.0f);
      glDrawArrays(GL_TRIANGLES, 0, 3); glDisable(GL_DEPTH_TEST);
      ok = pixel_is(128, 128, 255, 128, 64, d);
    }
    glBindVertexArray(vao);
    tend("draw: vertex_attrib_binding + UBO + uniform loc", ok, d);
  }

  /* instancing + base instance + multi draw indirect */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(
      "#version 430 core\nlayout(location=0) in vec2 pos; layout(location=1) in vec2 off;\n"
      "void main(){ gl_Position=vec4(off+pos,0.5,1.0); }\n", NULL, NULL, NULL, SOLID_FS, NULL);
    float quad[] = { 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1 };
    float offs[] = { -1, -1, 0, -1, -1, 0, 0, 0 };
    GLuint cmds[] = { 6, 2, 0, 0, 6, 2, 0, 2 };
    GLuint vq, vo, ib, va; glGenBuffers(1, &vq); glBindBuffer(GL_ARRAY_BUFFER, vq); glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glGenBuffers(1, &vo); glBindBuffer(GL_ARRAY_BUFFER, vo); glBufferData(GL_ARRAY_BUFFER, sizeof offs, offs, GL_STATIC_DRAW);
    glGenBuffers(1, &ib); glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ib); glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof cmds, cmds, GL_STATIC_DRAW);
    glGenVertexArrays(1, &va); glBindVertexArray(va);
    glEnableVertexAttribArray(0); glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0); glVertexAttribBinding(0, 0); glBindVertexBuffer(0, vq, 0, 8);
    glEnableVertexAttribArray(1); glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 0); glVertexAttribBinding(1, 1); glBindVertexBuffer(1, vo, 0, 8);
    glVertexBindingDivisor(1, 1);
    int ok = 0;
    if (p) {
      clear(0, 0, 0); glUseProgram(p); glUniform4f(0, 0, 1, 0, 1);
      glMultiDrawArraysIndirect(GL_TRIANGLES, NULL, 2, 0);
      ok = pixel_is(64, 64, 0, 255, 0, d) & pixel_is(192, 64, 0, 255, 0, d) & pixel_is(64, 192, 0, 255, 0, d) & pixel_is(192, 192, 0, 255, 0, d);
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0); glBindVertexArray(vao);
    tend("multi draw indirect + baseInstance + divisor", ok, d);
  }

  /* geometry shader */
  {
    tbegin(); d[0] = 0;
    GLuint p = program("#version 430 core\nvoid main(){ gl_Position=vec4(0,0,0,1); }\n", NULL, NULL,
      "#version 430 core\nlayout(points) in; layout(triangle_strip,max_vertices=4) out;\n"
      "void main(){ gl_Position=vec4(-1,-1,0.5,1); EmitVertex(); gl_Position=vec4(1,-1,0.5,1); EmitVertex();"
      " gl_Position=vec4(-1,1,0.5,1); EmitVertex(); gl_Position=vec4(1,1,0.5,1); EmitVertex(); EndPrimitive(); }\n",
      SOLID_FS, NULL);
    int ok = 0;
    if (p) {
      clear(0, 0, 0); glUseProgram(p); glUniform4f(0, 1, 0, 0, 1); glDrawArrays(GL_POINTS, 0, 1);
      ok = pixel_is(5, 5, 255, 0, 0, d) & pixel_is(250, 250, 255, 0, 0, d);
    }
    tend("geometry shader (point -> quad)", ok, d);
  }

  /* tessellation */
  {
    tbegin(); d[0] = 0;
    GLuint p = program(FS_TRI_VS,
      "#version 430 core\nlayout(vertices=3) out;\nvoid main(){ gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position;\n"
      " if(gl_InvocationID==0){ gl_TessLevelOuter[0]=4.0; gl_TessLevelOuter[1]=4.0; gl_TessLevelOuter[2]=4.0; gl_TessLevelInner[0]=4.0; } }\n",
      "#version 430 core\nlayout(triangles,equal_spacing,ccw) in;\n"
      "void main(){ gl_Position=gl_TessCoord.x*gl_in[0].gl_Position+gl_TessCoord.y*gl_in[1].gl_Position+gl_TessCoord.z*gl_in[2].gl_Position; }\n",
      NULL, SOLID_FS, NULL);
    int ok = 0;
    if (p) {
      clear(0, 0, 0); glUseProgram(p); glUniform4f(0, 0, 0, 1, 1); glPatchParameteri(GL_PATCH_VERTICES, 3); glDrawArrays(GL_PATCHES, 0, 3);
      ok = pixel_is(128, 128, 0, 0, 255, d) & pixel_is(10, 10, 0, 0, 255, d);
    }
    tend("tessellation (TCS + TES)", ok, d);
  }

  /* MSAA render + resolve blit (texture_storage_multisample) */
  {
    tbegin(); d[0] = 0;
    GLuint ms, fb; glGenTextures(1, &ms); glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, 256, 256, GL_TRUE);
    glGenFramebuffers(1, &fb); glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ms, 0);
    GLuint p = program(FS_TRI_VS, NULL, NULL, NULL, SOLID_FS, NULL);
    int ok = 0;
    if (p && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
      glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); glUseProgram(p); glUniform4f(0, 1, 1, 0, 1); glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fb); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, T.fbo);
      glBlitFramebuffer(0, 0, 256, 256, 0, 0, 256, 256, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_FRAMEBUFFER, T.fbo);
      ok = pixel_is(128, 128, 255, 255, 0, d);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, T.fbo);
    tend("4x MSAA render + resolve blit", ok, d);
  }

  /* texture array sampling with layout(binding) */
  {
    tbegin(); d[0] = 0;
    GLuint t; glGenTextures(1, &t); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, t);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 4, 4, 4);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    unsigned char layer[4][16 * 4];
    for (int l = 0; l < 4; l++) for (int i = 0; i < 16; i++) { layer[l][i*4] = l * 60; layer[l][i*4+1] = 200; layer[l][i*4+2] = 255 - l * 60; layer[l][i*4+3] = 255; }
    for (int l = 0; l < 4; l++) glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, l, 4, 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer[l]);
    GLuint p = program(FS_TRI_VS, NULL, NULL, NULL,
      "#version 430 core\nlayout(binding=0) uniform sampler2DArray t; layout(location=0) out vec4 o;\n"
      "void main(){ o=texture(t, vec3(0.5,0.5,2.0)); }\n", NULL);
    int ok = 0;
    if (p) { clear(0, 0, 0); glUseProgram(p); glDrawArrays(GL_TRIANGLES, 0, 3); ok = pixel_is(128, 128, 120, 200, 135, d); }
    tend("2D texture array sampling", ok, d);
  }

  /* clear_buffer_object + copy_image */
  {
    tbegin(); d[0] = 0;
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_SHADER_STORAGE_BUFFER, b); glBufferData(GL_SHADER_STORAGE_BUFFER, 64, NULL, GL_DYNAMIC_COPY);
    unsigned seven = 7, v[16]; glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &seven);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 64, v);
    int ok = 1; for (int i = 0; i < 16; i++) if (v[i] != 7) { ok = 0; sprintf(d, "clear v[%d]=%u ", i, v[i]); break; }
    unsigned char src[16 * 4], dst[16 * 4]; for (int i = 0; i < 64; i++) src[i] = (unsigned char)(i * 3);
    GLuint a, c; glGenTextures(1, &a); glBindTexture(GL_TEXTURE_2D, a); glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 4, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glGenTextures(1, &c); glBindTexture(GL_TEXTURE_2D, c); glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 4, 4);
    glCopyImageSubData(a, GL_TEXTURE_2D, 0, 0, 0, 0, c, GL_TEXTURE_2D, 0, 0, 0, 0, 4, 4, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
    if (memcmp(src, dst, sizeof src)) { ok = 0; strcat(d, "copy_image mismatch "); }
    tend("clear_buffer_object + copy_image", ok, d);
  }

  /* compressed formats */
  {
    unsigned char blk[16] = { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0, 0, 0, 0, 0 };
    struct { GLenum f; const char *n; } fm[] = {
      { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, "BC3/DXT5 (s3tc)" }, { GL_COMPRESSED_RGB_S3TC_DXT1_EXT, "BC1/DXT1 (s3tc)" },
      { GL_COMPRESSED_RED_RGTC1, "BC4 (rgtc)" }, { GL_COMPRESSED_RG_RGTC2, "BC5 (rgtc)" },
      { GL_COMPRESSED_RGBA_BPTC_UNORM, "BC7 (bptc)" }, { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, "BC6H (bptc)" },
    };
    for (unsigned i = 0; i < sizeof fm / sizeof fm[0]; i++) {
      char name[64]; sprintf(name, "compressed upload %s", fm[i].n);
      tbegin(); GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
      int sz = (fm[i].f == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || fm[i].f == GL_COMPRESSED_RED_RGTC1) ? 8 : 16;
      glCompressedTexImage2D(GL_TEXTURE_2D, 0, fm[i].f, 4, 4, 0, sz, blk);
      tend(name, 1, NULL);
    }
  }

  /* internalformat_query2, invalidate_subdata, timer query */
  {
    tbegin(); d[0] = 0; GLint sup = 0;
    glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_INTERNALFORMAT_SUPPORTED, 1, &sup);
    GLenum att = GL_COLOR_ATTACHMENT0; glBindFramebuffer(GL_FRAMEBUFFER, T.fbo); glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &att);
    GLuint q; GLuint64 ns = 0; glGenQueries(1, &q); glBeginQuery(GL_TIME_ELAPSED, q); clear(0, 0, 0); glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectui64v(q, GL_QUERY_RESULT, &ns);
    sprintf(d, "rgba8 supported=%d, timer=%lluns ", sup, (unsigned long long)ns);
    tend("internalformat_query2 + invalidate + timer query", sup == GL_TRUE, d);
  }

  /* texture views (the one GL 4.3 piece d3d12 lacks): check they actually sample right */
  if (!glTextureView) tskip("texture views", "entry point missing");
  else {
    P("  (GL_ARB_texture_view advertised: %s)\n", has_ext("GL_ARB_texture_view") ? "yes" : "no");
    glBindFramebuffer(GL_FRAMEBUFFER, T.fbo); glViewport(0, 0, 256, 256);
    GLuint sp = program(FS_TRI_VS, NULL, NULL, NULL,
      "#version 430 core\nlayout(binding=0) uniform sampler2D t; layout(location=0) out vec4 o;\n"
      "void main(){ o=texture(t, vec2(0.5)); }\n", NULL);
    glActiveTexture(GL_TEXTURE0);

    /* array layer 2 viewed as a plain 2D texture */
    tbegin(); d[0] = 0;
    GLuint arr, v1; glGenTextures(1, &arr); glBindTexture(GL_TEXTURE_2D_ARRAY, arr); glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 4, 4, 4);
    unsigned char layer[16 * 4];
    for (int l = 0; l < 4; l++) {
      for (int i = 0; i < 16; i++) { layer[i*4] = l * 60; layer[i*4+1] = 200; layer[i*4+2] = 255 - l * 60; layer[i*4+3] = 255; }
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, l, 4, 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer);
    }
    glGenTextures(1, &v1); glTextureView(v1, GL_TEXTURE_2D, arr, GL_RGBA8, 0, 1, 2, 1);
    glBindTexture(GL_TEXTURE_2D, v1); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int ok = 0; if (sp) { clear(0, 0, 0); glUseProgram(sp); glDrawArrays(GL_TRIANGLES, 0, 3); ok = pixel_is(128, 128, 120, 200, 135, d); }
    tend("texture view: array layer 2 as 2D", ok, d);

    /* RGBA8 data reinterpreted as sRGB (decode 128 -> ~55) */
    tbegin(); d[0] = 0;
    GLuint lin, v2; unsigned char grey[16 * 4]; memset(grey, 128, sizeof grey);
    glGenTextures(1, &lin); glBindTexture(GL_TEXTURE_2D, lin); glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 4, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, grey);
    glGenTextures(1, &v2); glTextureView(v2, GL_TEXTURE_2D, lin, GL_SRGB8_ALPHA8, 0, 1, 0, 1);
    glBindTexture(GL_TEXTURE_2D, v2); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ok = 0; if (sp) { clear(0, 0, 0); glUseProgram(sp); glDrawArrays(GL_TRIANGLES, 0, 3); ok = pixel_is(128, 128, 55, 55, 55, d); }
    tend("texture view: RGBA8 as SRGB8_ALPHA8", ok, d);

    /* mip level 1 only */
    tbegin(); d[0] = 0;
    GLuint mip, v3; unsigned char l0[16 * 4], l1[4 * 4];
    for (int i = 0; i < 16; i++) { l0[i*4] = 250; l0[i*4+1] = 0; l0[i*4+2] = 0; l0[i*4+3] = 255; }
    for (int i = 0; i < 4; i++) { l1[i*4] = 10; l1[i*4+1] = 20; l1[i*4+2] = 30; l1[i*4+3] = 255; }
    glGenTextures(1, &mip); glBindTexture(GL_TEXTURE_2D, mip); glTexStorage2D(GL_TEXTURE_2D, 3, GL_RGBA8, 4, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, l0);
    glTexSubImage2D(GL_TEXTURE_2D, 1, 0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, l1);
    glGenTextures(1, &v3); glTextureView(v3, GL_TEXTURE_2D, mip, GL_RGBA8, 1, 1, 0, 1);
    glBindTexture(GL_TEXTURE_2D, v3); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ok = 0; if (sp) { clear(0, 0, 0); glUseProgram(sp); glDrawArrays(GL_TRIANGLES, 0, 3); ok = pixel_is(128, 128, 10, 20, 30, d); }
    tend("texture view: mip level 1 subset", ok, d);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* ---------- limits ---------- */
static void print_limits(void)
{
  struct { GLenum e; const char *n; GLint min; int idx; } L[] = {
    { GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, "MAX_COMPUTE_WORK_GROUP_INVOCATIONS", 1024, -1 },
    { GL_MAX_COMPUTE_WORK_GROUP_SIZE, "MAX_COMPUTE_WORK_GROUP_SIZE[x]", 1024, 0 },
    { GL_MAX_COMPUTE_WORK_GROUP_SIZE, "MAX_COMPUTE_WORK_GROUP_SIZE[y]", 1024, 1 },
    { GL_MAX_COMPUTE_WORK_GROUP_SIZE, "MAX_COMPUTE_WORK_GROUP_SIZE[z]", 64, 2 },
    { GL_MAX_COMPUTE_WORK_GROUP_COUNT, "MAX_COMPUTE_WORK_GROUP_COUNT[x]", 65535, 0 },
    { GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, "MAX_COMPUTE_SHARED_MEMORY_SIZE", 32768, -1 },
    { GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, "MAX_COMPUTE_SHADER_STORAGE_BLOCKS", 8, -1 },
    { GL_MAX_COMPUTE_IMAGE_UNIFORMS, "MAX_COMPUTE_IMAGE_UNIFORMS", 8, -1 },
    { GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, "MAX_COMPUTE_TEXTURE_IMAGE_UNITS", 16, -1 },
    { GL_MAX_COMPUTE_UNIFORM_BLOCKS, "MAX_COMPUTE_UNIFORM_BLOCKS", 14, -1 },
    { GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, "MAX_SHADER_STORAGE_BUFFER_BINDINGS", 8, -1 },
    { GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, "MAX_COMBINED_SHADER_STORAGE_BLOCKS", 8, -1 },
    { GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, "MAX_VERTEX_SHADER_STORAGE_BLOCKS", 0, -1 },
    { GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, "MAX_FRAGMENT_SHADER_STORAGE_BLOCKS", 8, -1 },
    { GL_MAX_IMAGE_UNITS, "MAX_IMAGE_UNITS", 8, -1 },
    { GL_MAX_VERTEX_IMAGE_UNIFORMS, "MAX_VERTEX_IMAGE_UNIFORMS", 0, -1 },
    { GL_MAX_FRAGMENT_IMAGE_UNIFORMS, "MAX_FRAGMENT_IMAGE_UNIFORMS", 8, -1 },
    { GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, "MAX_ATOMIC_COUNTER_BUFFER_BINDINGS", 1, -1 },
    { GL_MAX_FRAGMENT_ATOMIC_COUNTERS, "MAX_FRAGMENT_ATOMIC_COUNTERS", 8, -1 },
    { GL_MAX_COMPUTE_ATOMIC_COUNTERS, "MAX_COMPUTE_ATOMIC_COUNTERS", 8, -1 },
    { GL_MAX_UNIFORM_BLOCK_SIZE, "MAX_UNIFORM_BLOCK_SIZE", 16384, -1 },
    { GL_MAX_VERTEX_UNIFORM_BLOCKS, "MAX_VERTEX_UNIFORM_BLOCKS", 14, -1 },
    { GL_MAX_FRAGMENT_UNIFORM_BLOCKS, "MAX_FRAGMENT_UNIFORM_BLOCKS", 14, -1 },
    { GL_MAX_UNIFORM_BUFFER_BINDINGS, "MAX_UNIFORM_BUFFER_BINDINGS", 0, -1 },
    { GL_MAX_VERTEX_UNIFORM_COMPONENTS, "MAX_VERTEX_UNIFORM_COMPONENTS", 1024, -1 },
    { GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, "MAX_FRAGMENT_UNIFORM_COMPONENTS", 1024, -1 },
    { GL_MAX_UNIFORM_LOCATIONS, "MAX_UNIFORM_LOCATIONS", 1024, -1 },
    { GL_MAX_TEXTURE_SIZE, "MAX_TEXTURE_SIZE", 16384, -1 },
    { GL_MAX_3D_TEXTURE_SIZE, "MAX_3D_TEXTURE_SIZE", 2048, -1 },
    { GL_MAX_ARRAY_TEXTURE_LAYERS, "MAX_ARRAY_TEXTURE_LAYERS", 2048, -1 },
    { GL_MAX_CUBE_MAP_TEXTURE_SIZE, "MAX_CUBE_MAP_TEXTURE_SIZE", 16384, -1 },
    { GL_MAX_TEXTURE_IMAGE_UNITS, "MAX_TEXTURE_IMAGE_UNITS", 16, -1 },
    { GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, "MAX_VERTEX_TEXTURE_IMAGE_UNITS", 16, -1 },
    { GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, "MAX_COMBINED_TEXTURE_IMAGE_UNITS", 0, -1 },
    { GL_MAX_TEXTURE_BUFFER_SIZE, "MAX_TEXTURE_BUFFER_SIZE", 65536, -1 },
    { GL_MAX_VERTEX_ATTRIBS, "MAX_VERTEX_ATTRIBS", 16, -1 },
    { GL_MAX_VERTEX_ATTRIB_BINDINGS, "MAX_VERTEX_ATTRIB_BINDINGS", 16, -1 },
    { GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, "MAX_VERTEX_ATTRIB_RELATIVE_OFFSET", 2047, -1 },
    { GL_MAX_VERTEX_OUTPUT_COMPONENTS, "MAX_VERTEX_OUTPUT_COMPONENTS", 64, -1 },
    { GL_MAX_FRAGMENT_INPUT_COMPONENTS, "MAX_FRAGMENT_INPUT_COMPONENTS", 128, -1 },
    { GL_MAX_COLOR_ATTACHMENTS, "MAX_COLOR_ATTACHMENTS", 8, -1 },
    { GL_MAX_DRAW_BUFFERS, "MAX_DRAW_BUFFERS", 8, -1 },
    { GL_MAX_SAMPLES, "MAX_SAMPLES", 4, -1 },
    { GL_MAX_RENDERBUFFER_SIZE, "MAX_RENDERBUFFER_SIZE", 16384, -1 },
    { GL_MAX_FRAMEBUFFER_WIDTH, "MAX_FRAMEBUFFER_WIDTH", 16384, -1 },
    { GL_MAX_TESS_GEN_LEVEL, "MAX_TESS_GEN_LEVEL", 64, -1 },
    { GL_MAX_GEOMETRY_OUTPUT_VERTICES, "MAX_GEOMETRY_OUTPUT_VERTICES", 256, -1 },
    { GL_MAX_CLIP_DISTANCES, "MAX_CLIP_DISTANCES", 8, -1 },
    { GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, "UNIFORM_BUFFER_OFFSET_ALIGNMENT (<=256)", 0, -1 },
    { GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, "SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT (<=256)", 0, -1 },
    { 0x84FF /* MAX_TEXTURE_MAX_ANISOTROPY */, "MAX_TEXTURE_MAX_ANISOTROPY (ext)", 0, -1 },
  };
  P("\n[limits]  (value / GL 4.3 spec minimum; '!!' = below minimum)\n");
  for (unsigned i = 0; i < sizeof L / sizeof L[0]; i++) {
    GLint v = -1;
    while (glGetError() != GL_NO_ERROR) {}
    if (L[i].e == 0x84FF) { GLfloat f = 0; glGetFloatv(L[i].e, &f); v = (GLint)f; }
    else if (L[i].idx >= 0) glGetIntegeri_v(L[i].e, L[i].idx, &v);
    else glGetIntegerv(L[i].e, &v);
    int err = glGetError() != GL_NO_ERROR;
    P("  %s %-46s %10d", (!err && L[i].min && v < L[i].min) ? "!!" : "  ", L[i].n, err ? -1 : v);
    if (L[i].min) P("  / %d", L[i].min);
    P("%s\n", err ? "  (query error)" : "");
  }
  GLint64 msb = 0; glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &msb);
  P("     %-46s %10lld  / 16777216\n", "MAX_SHADER_STORAGE_BLOCK_SIZE", (long long)msb);
}

/* ---------- benchmarks (GLSL 410 so Apple's native GL 4.1 can run them too) ---------- */
static void run_benchmarks(HDC dc, HWND wnd)
{
  P("\n[benchmarks]\n");
  GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
  GLuint fbo, col; glGenFramebuffers(1, &fbo); glGenTextures(1, &col);
  glBindTexture(GL_TEXTURE_2D, col); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, col, 0);
  glViewport(0, 0, 1920, 1080);
  GLuint p = program(
    "#version 410 core\nuniform vec2 off;\n"
    "void main(){ vec2 v=vec2(gl_VertexID==1?0.02:0.0, gl_VertexID==2?0.02:0.0); gl_Position=vec4(off+v,0.5,1.0); }\n",
    NULL, NULL, NULL, "#version 410 core\nuniform vec4 color; out vec4 o; void main(){ o=color; }\n", NULL);
  if (!p) { P("  benchmark program failed\n"); return; }
  glUseProgram(p);
  GLint loff = glGetUniformLocation(p, "off"), lcol = glGetUniformLocation(p, "color");

  /* draw-call throughput: what a city builder leans on */
  int draws = 20000; double best = 1e9;
  for (int f = 0; f < 6; f++) {
    double t0 = now_ms();
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < draws; i++) {
      glUniform2f(loff, (i % 100) / 50.0f - 1.0f, (i / 100 % 100) / 50.0f - 1.0f);
      glUniform4f(lcol, (i & 255) / 255.0f, 0.5f, 0.5f, 1.0f);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glFinish();
    double t = now_ms() - t0; if (f > 0 && t < best) best = t;
  }
  P("  draw calls: %d draws/frame -> %.1f ms/frame (%.2f us/draw, ~%.0f draws in a 16.7ms frame)\n",
    draws, best, best * 1000.0 / draws, 16.7 / (best / draws));

  /* instanced geometry throughput */
  best = 1e9;
  for (int f = 0; f < 6; f++) {
    double t0 = now_ms();
    glClear(GL_COLOR_BUFFER_BIT); glUniform2f(loff, -0.5f, -0.5f); glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2000000); glFinish();
    double t = now_ms() - t0; if (f > 0 && t < best) best = t;
  }
  P("  instanced: 2M small triangles in one call -> %.1f ms\n", best);

  /* present path: real window, SwapBuffers */
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  ShowWindow(wnd, SW_SHOWNOACTIVATE); UpdateWindow(wnd);
  if (wglSwapIntervalEXT) wglSwapIntervalEXT(0);
  glViewport(0, 0, 1280, 720);
  int frames = 180; double t0 = now_ms();
  for (int f = 0; f < frames; f++) {
    MSG m; while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
    glClearColor(0.1f, 0.2f + 0.5f * (f % 60) / 60.0f, 0.3f, 1); glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < 500; i++) { glUniform2f(loff, (i % 50) / 25.0f - 1.0f, (i / 50) / 25.0f - 1.0f); glUniform4f(lcol, 1, 1, 1, 1); glDrawArrays(GL_TRIANGLES, 0, 3); }
    SwapBuffers(dc);
  }
  glFinish();
  double el = now_ms() - t0;
  P("  present: %d frames to a 1280x720 window, swap interval 0 -> %.1f fps (GL error 0x%x)\n", frames, frames * 1000.0 / el, glGetError());
  ShowWindow(wnd, SW_HIDE);
}



/* ---------- streaming tests: per-frame CPU->GPU updates, the pattern UI layers use ---------- */
static const char *POS_VS = "#version 430 core\nlayout(location=0) in vec2 pos; void main(){ gl_Position=vec4(pos,0.5,1.0); }\n";
static const char *TEX_VS = "#version 430 core\nlayout(location=0) in vec2 pos; out vec2 uv; void main(){ uv=pos*0.5+0.5; gl_Position=vec4(pos,0.5,1.0); }\n";
static const char *TEX_FS = "#version 430 core\nin vec2 uv; layout(binding=0) uniform sampler2D t; layout(location=0) out vec4 o; void main(){ o=texture(t,uv); }\n";

static void quad_at(float *v, float x0, float y0, float x1, float y1)
{ float q[12] = { x0,y0, x1,y0, x0,y1, x1,y0, x1,y1, x0,y1 }; memcpy(v, q, sizeof q); }

static void run_stream_tests(HDC dc)
{
  char d[512]; int ok;
  P("\n[streaming tests] (each 'frame' = write, draw, SwapBuffers)\n");
  Target T = target(256, 256);
  GLuint va; glGenVertexArrays(1, &va); glBindVertexArray(va);
  glEnableVertexAttribArray(0); glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0); glVertexAttribBinding(0, 0);
  GLuint solid = program(POS_VS, NULL, NULL, NULL, SOLID_FS, NULL);
  GLuint texp = program(TEX_VS, NULL, NULL, NULL, TEX_FS, NULL);
  /* quadrant centers: frame i draws quad i, then we check all previously drawn quads still show */
  float qx[4][4] = { {-1,-1,0,0}, {0,-1,1,0}, {-1,0,0,1}, {0,0,1,1} };
  int px[4][2] = { {64,64}, {192,64}, {64,192}, {192,192} };

  /* 1. persistent + coherent mapped ring buffer */
  if (glBufferStorage) {
    tbegin(); d[0] = 0; ok = 1;
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b);
    glBufferStorage(GL_ARRAY_BUFFER, 4 * 48, NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    float *map = (float *)glMapBufferRange(GL_ARRAY_BUFFER, 0, 4 * 48, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    if (!map) { ok = 0; strcpy(d, "map failed "); }
    glBindVertexBuffer(0, b, 0, 8); glUseProgram(solid); glUniform4f(0, 0, 1, 0, 1); clear(0, 0, 0);
    for (int f = 0; f < 4 && map; f++) {
      quad_at(map + f * 12, qx[f][0], qx[f][1], qx[f][2], qx[f][3]);
      glDrawArrays(GL_TRIANGLES, f * 6, 6); SwapBuffers(dc);
      GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); glClientWaitSync(s, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); glDeleteSync(s);
      for (int k = 0; k <= f; k++) if (!pixel_is(px[k][0], px[k][1], 0, 255, 0, d)) { ok = 0; sprintf(d + strlen(d), "[frame %d] ", f); break; }
    }
    tend("persistent+coherent mapped VBO, per-frame writes", ok, d);
  } else tskip("persistent mapped VBO", "no glBufferStorage");

  /* 2. glMapBufferRange UNSYNCHRONIZED + FLUSH_EXPLICIT per frame (classic streaming) */
  {
    tbegin(); d[0] = 0; ok = 1;
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b); glBufferData(GL_ARRAY_BUFFER, 4 * 48, NULL, GL_STREAM_DRAW);
    glBindVertexBuffer(0, b, 0, 8); glUseProgram(solid); glUniform4f(0, 1, 1, 0, 1); clear(0, 0, 0);
    for (int f = 0; f < 4; f++) {
      float *m = (float *)glMapBufferRange(GL_ARRAY_BUFFER, f * 48, 48, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
      if (!m) { ok = 0; strcpy(d, "map failed "); break; }
      quad_at(m, qx[f][0], qx[f][1], qx[f][2], qx[f][3]); glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, 48); glUnmapBuffer(GL_ARRAY_BUFFER);
      glDrawArrays(GL_TRIANGLES, f * 6, 6); SwapBuffers(dc); glFinish();
      for (int k = 0; k <= f; k++) if (!pixel_is(px[k][0], px[k][1], 255, 255, 0, d)) { ok = 0; sprintf(d + strlen(d), "[frame %d] ", f); break; }
    }
    tend("MapBufferRange UNSYNC+FLUSH_EXPLICIT per frame", ok, d);
  }

  /* 3. orphan + glBufferSubData each frame, re-drawing all quads from the same buffer */
  {
    tbegin(); d[0] = 0; ok = 1;
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b);
    glBindVertexBuffer(0, b, 0, 8); glUseProgram(solid); glUniform4f(0, 0, 1, 1, 1);
    for (int f = 0; f < 4; f++) {
      float v[48]; for (int k = 0; k <= f; k++) quad_at(v + k * 12, qx[k][0], qx[k][1], qx[k][2], qx[k][3]);
      glBufferData(GL_ARRAY_BUFFER, 4 * 48, NULL, GL_STREAM_DRAW); glBufferSubData(GL_ARRAY_BUFFER, 0, (f + 1) * 48, v);
      clear(0, 0, 0); glDrawArrays(GL_TRIANGLES, 0, (f + 1) * 6); SwapBuffers(dc); glFinish();
      for (int k = 0; k <= f; k++) if (!pixel_is(px[k][0], px[k][1], 0, 255, 255, d)) { ok = 0; sprintf(d + strlen(d), "[frame %d] ", f); break; }
    }
    tend("orphan + BufferSubData per frame", ok, d);
  }

  /* 4. font-atlas pattern: draw with texture, then TexSubImage2D a sub-rect, draw again (GPU read -> CPU write hazard) */
  {
    tbegin(); d[0] = 0; ok = 1;
    GLuint t; glGenTextures(1, &t); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, t);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 64, 64);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    static unsigned char img[64 * 64 * 4]; memset(img, 0, sizeof img); for (int i = 3; i < (int)sizeof img; i += 4) img[i] = 255;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, img);
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b); float v[12]; quad_at(v, -1, -1, 1, 1);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW); glBindVertexBuffer(0, b, 0, 8); glUseProgram(texp);
    for (int f = 0; f < 4; f++) {
      /* each frame paints one 32x32 quadrant of the atlas red, after it was sampled last frame */
      static unsigned char sub[32 * 32 * 4]; for (int i = 0; i < 32 * 32; i++) { sub[i*4] = 255; sub[i*4+1] = 0; sub[i*4+2] = 0; sub[i*4+3] = 255; }
      glTexSubImage2D(GL_TEXTURE_2D, 0, (f & 1) * 32, (f >> 1) * 32, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, sub);
      clear(0, 0, 0); glDrawArrays(GL_TRIANGLES, 0, 6); SwapBuffers(dc); glFinish();
      for (int k = 0; k <= f; k++) if (!pixel_is(px[k][0], px[k][1], 255, 0, 0, d)) { ok = 0; sprintf(d + strlen(d), "[frame %d] ", f); break; }
    }
    tend("TexSubImage2D sub-rect after sampling, per frame", ok, d);
  }

  /* 5. UI blend pattern: alpha-blended quad with BlendFuncSeparate + scissor onto an opaque scene */
  {
    tbegin(); d[0] = 0;
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b); float v[12]; quad_at(v, -1, -1, 1, 1);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW); glBindVertexBuffer(0, b, 0, 8); glUseProgram(solid);
    clear(0, 0, 1); glEnable(GL_BLEND); glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST); glScissor(0, 0, 128, 256);
    glUniform4f(0, 1, 0, 0, 0.5f); glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); SwapBuffers(dc); glFinish();
    ok = pixel_is(64, 128, 128, 0, 128, d) & pixel_is(192, 128, 0, 0, 255, d);
    tend("alpha blend (separate) + scissor over opaque scene", ok, d);
  }

  /* 6. same as 5 but drawn directly to the window backbuffer, read back from it */
  {
    tbegin(); d[0] = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); glViewport(0, 0, 256, 256);
    GLuint b; glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b); float v[12]; quad_at(v, -1, -1, 1, 1);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW); glBindVertexBuffer(0, b, 0, 8); glUseProgram(solid);
    glClearColor(0, 0, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform4f(0, 1, 0, 0, 0.5f); glDrawArrays(GL_TRIANGLES, 0, 6); glDisable(GL_BLEND);
    glReadBuffer(GL_BACK); ok = pixel_is(64, 128, 128, 0, 128, d);
    SwapBuffers(dc); glFinish();
    tend("alpha blend into window backbuffer (pre-swap readback)", ok, d);
    glBindFramebuffer(GL_FRAMEBUFFER, T.fbo); glViewport(0, 0, 256, 256);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/* ---------- Foundation's exact UI pattern: per element gen buffer -> NamedBufferData -> draw -> delete, name reused ---------- */
static void run_ui_pattern_test(HDC dc, int n, int finish_each, const char *label)
{
  char d[512] = ""; int ok = 1;
  if (!glNamedBufferData) { tskip(label, "no glNamedBufferData"); return; }
  Target T = target(256, 256);
  GLuint prog = program(
    "#version 430 core\nlayout(location=0) in vec2 pos; layout(location=2) in vec2 uv; layout(location=5) in vec4 col; out vec4 c;\n"
    "void main(){ c=col; gl_Position=vec4(pos,0.5,1.0); }\n", NULL, NULL, NULL,
    "#version 430 core\nin vec4 c; layout(location=0) out vec4 o; void main(){ o=c; }\n", NULL);
  GLuint ibo; glGenBuffers(1, &ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  GLuint idx[6] = { 0, 1, 2, 2, 1, 3 }; glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glUseProgram(prog); glEnable(GL_BLEND); glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  clear(0, 0, 0);
  int grid = 1; while (grid * grid < n) grid++;
  for (int i = 0; i < n; i++) {
    int gx = i % grid, gy = i / grid; float cell = 2.0f / grid;
    float x0 = -1 + gx * cell + cell * 0.1f, y0 = -1 + gy * cell + cell * 0.1f, x1 = x0 + cell * 0.8f, y1 = y0 + cell * 0.8f;
    unsigned char r = 40 + (i * 37) % 200, g = 40 + (i * 91) % 200, b = 40 + (i * 53) % 200;
    struct { float x, y, u, v; unsigned char c[4]; } verts[4] = { { x0, y0, 0, 0, { r, g, b, 255 } }, { x1, y0, 1, 0, { r, g, b, 255 } }, { x0, y1, 0, 1, { r, g, b, 255 } }, { x1, y1, 1, 1, { r, g, b, 255 } } };
    GLuint buf, va;
    glGenBuffers(1, &buf); glBindBuffer(GL_ARRAY_BUFFER, buf); glBindBuffer(GL_ARRAY_BUFFER, 0);
    glGenVertexArrays(1, &va); glBindVertexArray(va); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glNamedBufferData(buf, sizeof verts, verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, (void *)0); glVertexAttribDivisor(0, 0);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 20, (void *)8); glVertexAttribDivisor(2, 0);
    glEnableVertexAttribArray(5); glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (void *)16); glVertexAttribDivisor(5, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0); glDeleteVertexArrays(1, &va); glDeleteBuffers(1, &buf);
    if (finish_each) glFinish();
    if (i == 0) sprintf(d, "names buf=%u va=%u ", buf, va);
  }
  SwapBuffers(dc); glFinish();
  int bad = 0, first = -1;
  for (int i = 0; i < n; i++) {
    int gx = i % grid, gy = i / grid; float cell = 2.0f / grid;
    int px = (int)((( -1 + gx * cell + cell * 0.5f) * 0.5f + 0.5f) * 256), py = (int)(((-1 + gy * cell + cell * 0.5f) * 0.5f + 0.5f) * 256);
    unsigned char r = 40 + (i * 37) % 200, g = 40 + (i * 91) % 200, b = 40 + (i * 53) % 200;
    char tmp[128] = ""; if (!pixel_is(px, py, r, g, b, tmp)) { bad++; if (first < 0) { first = i; strncat(d, tmp, 100); } }
  }
  if (bad) { ok = 0; sprintf(d + strlen(d), "-> %d of %d quads wrong (first #%d)", bad, n, first); }
  glDisable(GL_BLEND); glBindFramebuffer(GL_FRAMEBUFFER, 0);
  tend(label, ok, d);
}


/* ---------- texture re-specification: 2D array created with 1 layer, sampled, then glTexImage3D'd to 8 layers and refilled ---------- */
static void run_respec_test(HDC dc)
{
  char d[512] = ""; int ok = 1;
  Target T = target(256, 256);
  GLuint va; glGenVertexArrays(1, &va); glBindVertexArray(va);
  GLuint p = program(FS_TRI_VS, NULL, NULL, NULL,
    "#version 430 core\nlayout(binding=0) uniform sampler2DArray t; layout(location=0) uniform float layer; layout(location=0) out vec4 o;\n"
    "void main(){ o=texture(t, vec3(0.5,0.5,layer)); }\n", NULL);
  static unsigned char img[64 * 64 * 4];
  GLuint tex; glGenTextures(1, &tex); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 64, 64, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  for (int i = 0; i < 64 * 64; i++) { img[i*4] = 200; img[i*4+1] = 30; img[i*4+2] = 30; img[i*4+3] = 255; }
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, img);
  glUseProgram(p); glUniform1f(0, 0); clear(0, 0, 0); glDrawArrays(GL_TRIANGLES, 0, 3); SwapBuffers(dc);
  if (!pixel_is(128, 128, 200, 30, 30, d)) { ok = 0; strcat(d, "[initial] "); }
  /* grow to 8 layers (same pattern as the game), refill layers 0..7 with distinct colors */
  glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 64, 64, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  for (int l = 0; l < 8; l++) {
    for (int i = 0; i < 64 * 64; i++) { img[i*4] = 20 + l * 25; img[i*4+1] = 220 - l * 20; img[i*4+2] = 60 + l * 10; img[i*4+3] = 255; }
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, l, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, img);
  }
  for (int l = 0; l < 8; l += 7) {
    glUniform1f(0, (float)l); clear(0, 0, 0); glDrawArrays(GL_TRIANGLES, 0, 3); SwapBuffers(dc);
    if (!pixel_is(128, 128, 20 + l * 25, 220 - l * 20, 60 + l * 10, d)) { ok = 0; sprintf(d + strlen(d), "[after grow, layer %d] ", l); }
  }
  tend("2D array re-spec 1 -> 8 layers (glTexImage3D) then sample", ok, d);
  /* variant: shrink/regrow with same size but different data between draws in one frame */
  tbegin(); d[0] = 0; ok = 1;
  for (int round = 0; round < 3; round++) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 64, 64, 2 + round, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    for (int i = 0; i < 64 * 64; i++) { img[i*4] = 100 + round * 50; img[i*4+1] = 100; img[i*4+2] = 100 + round * 50; img[i*4+3] = 255; }
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, img);
    glUniform1f(0, 1.0f); clear(0, 0, 0); glDrawArrays(GL_TRIANGLES, 0, 3);
    if (!pixel_is(128, 128, 100 + round * 50, 100, 100 + round * 50, d)) { ok = 0; sprintf(d + strlen(d), "[round %d] ", round); }
  }
  SwapBuffers(dc);
  tend("2D array re-spec 3x within one frame, sample layer 1", ok, d);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/* ---------- mirror of Foundation's text program (27): sampler2DArray font atlas, SDF branch, flat float varying ---------- */
static const char *P27_VS_FLAT =
  "#version 430 compatibility\nin vec2 attribPosition; in vec3 attribUv3d; in vec4 attribColor; in float attribSdfWidthThreshold; in float attribSdfSoftnessWidth;\n"
  "uniform mat4 uniMatMVP; out vec3 var_attribUv3d; out vec4 var_attribColor; out float var_attribSdfWidthThreshold; flat out float var_attribSdfSoftnessWidth;\n"
  "void main(){ gl_Position = uniMatMVP * vec4(attribPosition, 0.0, 1.0); var_attribUv3d = attribUv3d; var_attribColor = attribColor;\n"
  " var_attribSdfWidthThreshold = attribSdfWidthThreshold; var_attribSdfSoftnessWidth = attribSdfSoftnessWidth; }\n";
static const char *P27_FS_FLAT =
  "#version 430 compatibility\nuniform sampler2DArray uniTextureArrayDiffuse; uniform sampler2DArray uniFontTextureArrayDiffuse;\n"
  "in vec3 var_attribUv3d; in vec4 var_attribColor; in float var_attribSdfWidthThreshold; flat in float var_attribSdfSoftnessWidth; layout(location = 0) out vec4 outColor;\n"
  "void main(){ vec4 ATTRIB_COLOR = var_attribColor; vec4 TEXTURE_COLOR = vec4(1); vec4 COLOR = vec4(1);\n"
  " if(var_attribSdfWidthThreshold == 0.0) { int mode = floatBitsToInt(var_attribSdfSoftnessWidth); TEXTURE_COLOR = texture(uniTextureArrayDiffuse, var_attribUv3d);\n"
  "   if(mode == 1) { COLOR = vec4(ATTRIB_COLOR.rgb * TEXTURE_COLOR.a, 0) + TEXTURE_COLOR; } else { COLOR = ATTRIB_COLOR * TEXTURE_COLOR; } }\n"
  " else { vec4 texSample = texture(uniFontTextureArrayDiffuse, var_attribUv3d); float alpha = 0.0;\n"
  "   if(var_attribSdfWidthThreshold > 0.0) { float dist = max(min(texSample.r, texSample.g), min(max(texSample.r, texSample.g), texSample.b)); float smoothModifier = 1.0;\n"
  "     dist = (dist - var_attribSdfWidthThreshold) * var_attribSdfSoftnessWidth; alpha = smoothstep(- 1.0 * smoothModifier, smoothModifier, dist); }\n"
  "   else { float dist_1 = texSample.a; dist_1 = (dist_1 - abs(var_attribSdfWidthThreshold)) * var_attribSdfSoftnessWidth; alpha = clamp((dist_1 / 2.0) + 0.5, 0.0, 1.0); alpha = pow(alpha, 1.5); }\n"
  "   TEXTURE_COLOR = vec4(alpha); COLOR = ATTRIB_COLOR * alpha; }\n"
  " COLOR.rgb *= ATTRIB_COLOR.a; outColor = COLOR; }\n";


static char *str_replace_all(const char *src, const char *from, const char *to)
{
  size_t fl = strlen(from), tl = strlen(to), n = 0; const char *r = src;
  while ((r = strstr(r, from))) { n++; r += fl; }
  char *o = malloc(strlen(src) + n * (tl > fl ? tl - fl : 0) + 1), *w = o; r = src;
  while (*r) { if (!strncmp(r, from, fl)) { memcpy(w, to, tl); w += tl; r += fl; } else *w++ = *r++; }
  *w = 0; return o;
}
static char *chain(char *s, const char *from, const char *to) { char *o = str_replace_all(s, from, to); free(s); return o; }

static char *strip_flat(const char *src) { char *o = malloc(strlen(src) + 1), *w = o; const char *r = src; while (*r) { if (!strncmp(r, "flat ", 5)) { r += 5; continue; } *w++ = *r++; } *w = 0; return o; }

static void run_p27_test(HDC dc, int variant, const char *label)
{
  char d[512] = ""; int ok = 1;
  Target T = target(256, 256);
  const char *vs = P27_VS_FLAT, *fs = P27_FS_FLAT; char *vs2 = NULL, *fs2 = NULL;
  if (variant == 1) { vs2 = strip_flat(P27_VS_FLAT); fs2 = strip_flat(P27_FS_FLAT); vs = vs2; fs = fs2; }
  if (variant == 2) { /* flat varying gets its own vec4 register */
    vs2 = chain(strdup(P27_VS_FLAT), "flat out float var_attribSdfSoftnessWidth;", "flat out vec4 var_attribSdfSoftnessWidth;");
    vs2 = chain(vs2, "var_attribSdfSoftnessWidth = attribSdfSoftnessWidth;", "var_attribSdfSoftnessWidth = vec4(attribSdfSoftnessWidth);");
    fs2 = chain(strdup(P27_FS_FLAT), "flat in float var_attribSdfSoftnessWidth;", "flat in vec4 var_attribSdfSoftnessWidth;");
    fs2 = chain(fs2, "floatBitsToInt(var_attribSdfSoftnessWidth)", "floatBitsToInt(var_attribSdfSoftnessWidth.x)");
    fs2 = chain(fs2, "* var_attribSdfSoftnessWidth;", "* var_attribSdfSoftnessWidth.x;");
    vs = vs2; fs = fs2; }
  if (variant == 3) { /* no flat at all: mode passed as a plain float */
    vs2 = chain(strdup(P27_VS_FLAT), "flat out float var_attribSdfSoftnessWidth;", "out float var_attribSdfSoftnessWidth; out float var_sdfMode;");
    vs2 = chain(vs2, "var_attribSdfSoftnessWidth = attribSdfSoftnessWidth;", "var_attribSdfSoftnessWidth = attribSdfSoftnessWidth; var_sdfMode = float(floatBitsToInt(attribSdfSoftnessWidth));");
    fs2 = chain(strdup(P27_FS_FLAT), "flat in float var_attribSdfSoftnessWidth;", "in float var_attribSdfSoftnessWidth; in float var_sdfMode;");
    fs2 = chain(fs2, "floatBitsToInt(var_attribSdfSoftnessWidth)", "int(var_sdfMode + 0.5)");
    vs = vs2; fs = fs2; }
  if (variant == 4) { /* flat int instead of flat float */
    vs2 = chain(strdup(P27_VS_FLAT), "flat out float var_attribSdfSoftnessWidth;", "flat out int var_sdfBits;");
    vs2 = chain(vs2, "var_attribSdfSoftnessWidth = attribSdfSoftnessWidth;", "var_sdfBits = floatBitsToInt(attribSdfSoftnessWidth);");
    fs2 = chain(strdup(P27_FS_FLAT), "flat in float var_attribSdfSoftnessWidth;", "flat in int var_sdfBits;");
    fs2 = chain(fs2, "floatBitsToInt(var_attribSdfSoftnessWidth)", "var_sdfBits");
    fs2 = chain(fs2, "* var_attribSdfSoftnessWidth;", "* intBitsToFloat(var_sdfBits);");
    vs = vs2; fs = fs2; }
  GLuint p = glCreateProgram(); GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
  if (!v || !f) { tend(label, 0, "compile failed"); return; }
  glAttachShader(p, v); glAttachShader(p, f);
  glBindAttribLocation(p, 0, "attribPosition"); glBindAttribLocation(p, 5, "attribColor"); glBindAttribLocation(p, 2, "attribUv3d");
  glBindAttribLocation(p, 1, "attribSdfWidthThreshold"); glBindAttribLocation(p, 3, "attribSdfSoftnessWidth");
  glLinkProgram(p); GLint lk = 0; glGetProgramiv(p, GL_LINK_STATUS, &lk); if (!lk) { char log[2048]; glGetProgramInfoLog(p, sizeof log, NULL, log); P("      link: %s\n", log); tend(label, 0, "link failed"); return; }
  glUseProgram(p);
  /* font atlas: 2D array, 8 layers; layer 3 = MSDF-ish constant 0.75 (inside glyph), alpha 0.9 */
  static unsigned char img[64 * 64 * 4];
  GLuint font, diff; glGenTextures(1, &font); glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, font);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 64, 64, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  for (int i = 0; i < 64 * 64; i++) { img[i*4] = 191; img[i*4+1] = 191; img[i*4+2] = 191; img[i*4+3] = 230; }
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 3, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, img);
  glGenTextures(1, &diff); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, diff);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 64, 64, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  for (int i = 0; i < 64 * 64; i++) { img[i*4] = 0; img[i*4+1] = 0; img[i*4+2] = 255; img[i*4+3] = 255; }
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, img);
  glUniform1i(glGetUniformLocation(p, "uniTextureArrayDiffuse"), 0); glUniform1i(glGetUniformLocation(p, "uniFontTextureArrayDiffuse"), 1);
  float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }; glUniformMatrix4fv(glGetUniformLocation(p, "uniMatMVP"), 1, GL_FALSE, I);
  /* vertex layout identical to the game: pos(2f)@0 uv3(3f)@8 color(ubyte4)@20 thr(f)@24 soft(f)@28 stride 32 */
  struct V { float x, y, u, v, w; unsigned char c[4]; float thr, soft; };
  float one_as_int; { int m = 1; memcpy(&one_as_int, &m, 4); }
  struct V q[12]; int k = 0;
  /* left quad: text path (thr 0.5, soft 4) layer 3 -> alpha = smoothstep(-1,1,(0.75-0.5)*4=1) = 1 -> white */
  float L[4][2] = { {-1,-1}, {0,-1}, {-1,1}, {0,1} }, R[4][2] = { {0,-1}, {1,-1}, {0,1}, {1,1} };
  for (int i = 0; i < 4; i++) { q[k++] = (struct V){ L[i][0], L[i][1], 0.5f, 0.5f, 3.0f, { 255, 255, 255, 255 }, 0.5f, 4.0f }; }
  /* right quad: mode path (thr 0, soft = int 1) layer 1 of diffuse -> COLOR = rgb*a + tex = (1,1,1)*1 + (0,0,1,1) -> clamps to white-ish blue */
  for (int i = 0; i < 4; i++) { q[k++] = (struct V){ R[i][0], R[i][1], 0.5f, 0.5f, 1.0f, { 255, 0, 0, 255 }, 0.0f, one_as_int }; }
  GLuint idx[12] = { 0,1,2, 2,1,3, 4,5,6, 6,5,7 };
  GLuint buf, ibo, va; glGenBuffers(1, &buf); glGenBuffers(1, &ibo); glGenVertexArrays(1, &va); glBindVertexArray(va);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, buf); glBindBuffer(GL_ARRAY_BUFFER, 0);
  glNamedBufferData(buf, sizeof q, q, GL_DYNAMIC_DRAW); glBindBuffer(GL_ARRAY_BUFFER, buf);
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 32, (void *)0);
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void *)8);
  glEnableVertexAttribArray(5); glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, 32, (void *)20);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 32, (void *)24);
  glEnableVertexAttribArray(3); glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 32, (void *)28);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_DEPTH_TEST);
  if (glProvokingVertex && variant == 5) glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
  clear(0, 0, 0); glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, NULL); SwapBuffers(dc); glFinish();
  if (!pixel_is(64, 128, 255, 255, 255, d)) { ok = 0; strcat(d, "[text path] "); }
  if (!pixel_is(192, 128, 255, 0, 255, d)) { ok = 0; strcat(d, "[mode path] "); }
  glDisable(GL_BLEND); glBindVertexArray(0); glBindFramebuffer(GL_FRAMEBUFFER, 0);
  tend(label, ok, d);
  free(vs2); free(fs2);
}


/* ---------- terrain-style workload: N indirect draws of small quads, triangles vs tessellated patches ---------- */
static double time_frames(void (*fn)(void *), void *arg, HDC dc, int reps)
{
  double best = 1e9;
  for (int r = 0; r < reps; r++) { double t0 = now_ms(); fn(arg); SwapBuffers(dc); glFinish(); double t = now_ms() - t0; if (r > 0 && t < best) best = t; }
  return best;
}
struct mdi_arg { GLenum mode; int count; GLuint ibo; };
static void mdi_fn(void *a) { struct mdi_arg *m = a; glClear(GL_COLOR_BUFFER_BIT); glMultiDrawElementsIndirect(m->mode, GL_UNSIGNED_INT, NULL, m->count, 0); }
static void loop_fn(void *a) { struct mdi_arg *m = a; glClear(GL_COLOR_BUFFER_BIT); for (int i = 0; i < m->count; i++) glDrawElements(m->mode, m->mode == GL_PATCHES ? 4 : 6, GL_UNSIGNED_INT, (void *)(size_t)(i * 6 * 4)); }
static void run_terrain_bench(HDC dc)
{
  P("\n[terrain-style benchmark] (8186 draws per frame like Foundation's terrain)\n");
  const int N = 8186; int grid = 91; /* 91*91 > 8186 */
  Target T = target(1024, 1024); glViewport(0, 0, 1024, 1024);
  GLuint va; glGenVertexArrays(1, &va); glBindVertexArray(va);
  /* each draw: its own 4 verts (quad cell) and 6 indices; index buffer holds 6 indices per draw, vertex buffer 4 verts per draw */
  float *verts = malloc((size_t)N * 4 * 2 * sizeof(float)); GLuint *idx = malloc((size_t)N * 6 * sizeof(GLuint));
  for (int i = 0; i < N; i++) { float cs = 2.0f / grid; float x0 = -1 + (i % grid) * cs, y0 = -1 + (i / grid) * cs; float *v = verts + i * 8;
    v[0] = x0; v[1] = y0; v[2] = x0 + cs; v[3] = y0; v[4] = x0; v[5] = y0 + cs; v[6] = x0 + cs; v[7] = y0 + cs;
    GLuint b = i * 4; GLuint *q = idx + i * 6; q[0] = b; q[1] = b + 1; q[2] = b + 2; q[3] = b + 2; q[4] = b + 1; q[5] = b + 3; }
  GLuint vb, ib; glGenBuffers(1, &vb); glBindBuffer(GL_ARRAY_BUFFER, vb); glBufferData(GL_ARRAY_BUFFER, (size_t)N * 32, verts, GL_STATIC_DRAW);
  glGenBuffers(1, &ib); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib); glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_t)N * 24, idx, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0); glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0); glVertexAttribBinding(0, 0); glBindVertexBuffer(0, vb, 0, 8);
  /* indirect commands: triangles (6 indices) and patches (4 indices, use the quad's first 4 indices order b,b+1,b+2,b+3 -> need separate index buffer) */
  typedef struct { GLuint count, instanceCount, firstIndex, baseVertex, baseInstance; } Cmd;
  Cmd *tri = malloc(sizeof(Cmd) * N), *pat = malloc(sizeof(Cmd) * N);
  GLuint *pidx = malloc((size_t)N * 4 * sizeof(GLuint)); for (int i = 0; i < N; i++) { pidx[i*4] = i*4; pidx[i*4+1] = i*4+1; pidx[i*4+2] = i*4+3; pidx[i*4+3] = i*4+2; }
  for (int i = 0; i < N; i++) { tri[i] = (Cmd){ 6, 1, (GLuint)i * 6, 0, 0 }; pat[i] = (Cmd){ 4, 1, (GLuint)i * 4, 0, 0 }; }
  GLuint ind_tri, ind_pat, pib; glGenBuffers(1, &ind_tri); glGenBuffers(1, &ind_pat); glGenBuffers(1, &pib);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ind_tri); glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(Cmd) * N, tri, GL_STATIC_DRAW);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ind_pat); glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(Cmd) * N, pat, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pib); glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_t)N * 16, pidx, GL_STATIC_DRAW);
  GLuint ptri = program(POS_VS, NULL, NULL, NULL, SOLID_FS, NULL);
  GLuint ptess = program("#version 430 core\nlayout(location=0) in vec2 pos; out vec2 vpos; void main(){ vpos=pos; }\n",
    "#version 430 core\nlayout(vertices=4) out; in vec2 vpos[]; out vec2 tpos[]; uniform float lvl;\n"
    "void main(){ tpos[gl_InvocationID]=vpos[gl_InvocationID]; if(gl_InvocationID==0){ gl_TessLevelOuter[0]=lvl; gl_TessLevelOuter[1]=lvl; gl_TessLevelOuter[2]=lvl; gl_TessLevelOuter[3]=lvl; gl_TessLevelInner[0]=lvl; gl_TessLevelInner[1]=lvl; } }\n",
    "#version 430 core\nlayout(quads,equal_spacing,ccw) in; in vec2 tpos[];\n"
    "void main(){ vec2 a=mix(tpos[0],tpos[1],gl_TessCoord.x); vec2 b=mix(tpos[3],tpos[2],gl_TessCoord.x); gl_Position=vec4(mix(a,b,gl_TessCoord.y),0.5,1.0); }\n",
    NULL, SOLID_FS, NULL);
  if (!ptri || !ptess) { P("  program failed\n"); return; }
  struct mdi_arg a;
  glUseProgram(ptri); glUniform4f(0, 0, 1, 0, 1); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib); glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ind_tri);
  a = (struct mdi_arg){ GL_TRIANGLES, N, ib }; P("  MDI triangles, %d draws:            %.1f ms/frame\n", N, time_frames(mdi_fn, &a, dc, 6));
  a = (struct mdi_arg){ GL_TRIANGLES, N, ib }; P("  glDrawElements loop, %d draws:      %.1f ms/frame\n", N, time_frames(loop_fn, &a, dc, 4));
  a = (struct mdi_arg){ GL_TRIANGLES, 1000, ib }; P("  MDI triangles, 1000 draws:           %.1f ms/frame\n", time_frames(mdi_fn, &a, dc, 6));
  glUseProgram(ptess); glUniform4f(glGetUniformLocation(ptess, "color"), 0, 0, 1, 1); glPatchParameteri(GL_PATCH_VERTICES, 4);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pib); glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ind_pat);
  GLint ll = glGetUniformLocation(ptess, "lvl");
  glUniform1f(ll, 1.0f); a = (struct mdi_arg){ GL_PATCHES, N, pib }; P("  MDI patches (tess level 1), %d draws: %.1f ms/frame\n", N, time_frames(mdi_fn, &a, dc, 4));
  glUniform1f(ll, 1.0f); a = (struct mdi_arg){ GL_PATCHES, 1000, pib }; P("  MDI patches (tess level 1), 1000 draws: %.1f ms/frame\n", time_frames(mdi_fn, &a, dc, 4));
  glUniform1f(ll, 1.0f); a = (struct mdi_arg){ GL_PATCHES, 100, pib }; P("  MDI patches (tess level 1), 100 draws: %.1f ms/frame\n", time_frames(mdi_fn, &a, dc, 4));
  glUniform1f(ll, 8.0f); a = (struct mdi_arg){ GL_PATCHES, 1000, pib }; P("  MDI patches (tess level 8), 1000 draws: %.1f ms/frame\n", time_frames(mdi_fn, &a, dc, 4));
  a = (struct mdi_arg){ GL_PATCHES, 1000, pib }; P("  glDrawElements loop patches, 1000 draws: %.1f ms/frame\n", time_frames(loop_fn, &a, dc, 4));
  /* one big patch draw: all quads in a single glDrawElements */
  { double t0; double best = 1e9; for (int r = 0; r < 4; r++) { t0 = now_ms(); glClear(GL_COLOR_BUFFER_BIT); glDrawElements(GL_PATCHES, N * 4, GL_UNSIGNED_INT, NULL); SwapBuffers(dc); glFinish(); double t = now_ms() - t0; if (r && t < best) best = t; } P("  single glDrawElements, %d patches:    %.1f ms/frame\n", N, best); }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* hold mode: visible window, solid magenta + white triangle, for eyeballing presentation */
static void run_hold(HDC dc, HWND wnd, int seconds, int interval)
{
  ShowWindow(wnd, SW_SHOW); SetForegroundWindow(wnd); UpdateWindow(wnd);
  if (wglSwapIntervalEXT) wglSwapIntervalEXT(interval);
  GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
  GLuint p = program("#version 410 core\nvoid main(){ vec2 v=vec2(gl_VertexID==1?0.5:-0.5, gl_VertexID==2?0.5:-0.5); gl_Position=vec4(v,0.5,1.0); }\n",
                     NULL, NULL, NULL, "#version 410 core\nout vec4 o; void main(){ o=vec4(1,1,1,1); }\n", NULL);
  double t0 = now_ms(), last = t0; int frames = 0, total = 0;
  while (now_ms() - t0 < seconds * 1000.0) {
    MSG m; while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
    RECT r; GetClientRect(wnd, &r); glViewport(0, 0, r.right, r.bottom);
    glClearColor(1, 0, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
    if (p) { glUseProgram(p); glDrawArrays(GL_TRIANGLES, 0, 3); }
    SwapBuffers(dc); frames++; total++;
    if (now_ms() - last >= 1000.0) { P("  hold: %d fps (GL error 0x%x)\n", frames, glGetError()); frames = 0; last = now_ms(); }
  }
  P("  hold done: %d frames\n", total);
}

static void print_pixel_formats(HDC dc)
{
  PIXELFORMATDESCRIPTOR pfd;
  int n = DescribePixelFormat(dc, 1, sizeof pfd, &pfd);
  P("\n[pixel formats] %d exposed\n", n);
  for (int i = 1; i <= n && i <= 64; i++) {
    DescribePixelFormat(dc, i, sizeof pfd, &pfd);
    if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL) || !(pfd.dwFlags & PFD_DRAW_TO_WINDOW)) continue;
    P("  #%-3d color=%2d alpha=%d depth=%2d stencil=%d %s%s\n", i, pfd.cColorBits, pfd.cAlphaBits, pfd.cDepthBits, pfd.cStencilBits,
      (pfd.dwFlags & PFD_DOUBLEBUFFER) ? "double" : "single", (pfd.dwFlags & PFD_GENERIC_FORMAT) ? " generic" : "");
  }
  if (!wglChoosePixelFormatARB) { P("  wglChoosePixelFormatARB missing\n"); return; }
  struct { const char *n; int a[24]; } q[] = {
    { "RGBA8 D24S8 double", { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32, WGL_ALPHA_BITS_ARB, 8, WGL_DEPTH_BITS_ARB, 24, WGL_STENCIL_BITS_ARB, 8, 0 } },
    { "RGBA8 D24S8 double sRGB", { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32, WGL_ALPHA_BITS_ARB, 8, WGL_DEPTH_BITS_ARB, 24, WGL_STENCIL_BITS_ARB, 8, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, 1, 0 } },
    { "RGBA8 D24S8 double 4xMSAA", { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32, WGL_DEPTH_BITS_ARB, 24, WGL_STENCIL_BITS_ARB, 8, WGL_SAMPLE_BUFFERS_ARB, 1, WGL_SAMPLES_ARB, 4, 0 } },
    { "RGB8 no depth double", { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 24, 0 } },
  };
  for (unsigned i = 0; i < sizeof q / sizeof q[0]; i++) {
    int fmts[16]; UINT cnt = 0; BOOL ok = wglChoosePixelFormatARB(dc, q[i].a, NULL, 16, fmts, &cnt);
    P("  wglChoosePixelFormatARB %-28s -> %s, %u match(es)\n", q[i].n, ok ? "ok" : "FAILED", cnt);
  }
}

int main(int argc, char **argv)
{
  int maj = argc > 1 ? atoi(argv[1]) : 4, min = argc > 2 ? atoi(argv[2]) : 3;
  int core = !(argc > 3 && !strcmp(argv[3], "compat"));
  int benchonly = argc > 4 && !strcmp(argv[4], "benchonly");
  int hold = argc > 4 && !strcmp(argv[4], "hold");
  int terrain = argc > 4 && !strcmp(argv[4], "terrain");
  int hold_secs = argc > 5 ? atoi(argv[5]) : 20, hold_interval = argc > 6 ? atoi(argv[6]) : 1;
  g_out = fopen("glcheck.txt", "w");

  WNDCLASSA wc = { 0 }; wc.style = CS_OWNDC; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "glcheck";
  RegisterClassA(&wc);
  HWND wnd = CreateWindowA("glcheck", "glcheck MAGENTA TEST", WS_OVERLAPPEDWINDOW, 200, 150, 800, 600, NULL, NULL, wc.hInstance, NULL);
  HDC dc = GetDC(wnd);
  PIXELFORMATDESCRIPTOR pfd = { sizeof pfd, 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32 };
  pfd.cAlphaBits = (argc > 7) ? (BYTE)atoi(argv[7]) : 8; pfd.cDepthBits = 24; pfd.cStencilBits = 8;
  int pf = ChoosePixelFormat(dc, &pfd);
  if (pfd.cAlphaBits == 0) { /* pick an explicit no-alpha, double-buffered D24S8 format */
    int n = DescribePixelFormat(dc, 1, sizeof pfd, NULL);
    for (int i = 1; i <= n; i++) { PIXELFORMATDESCRIPTOR c; DescribePixelFormat(dc, i, sizeof c, &c);
      if ((c.dwFlags & PFD_SUPPORT_OPENGL) && (c.dwFlags & PFD_DRAW_TO_WINDOW) && (c.dwFlags & PFD_DOUBLEBUFFER) && !(c.dwFlags & PFD_SUPPORT_GDI)
          && c.cAlphaBits == 0 && c.cDepthBits == 24 && c.cStencilBits == 8) { pf = i; break; } }
  }
  { PIXELFORMATDESCRIPTOR got; DescribePixelFormat(dc, pf, sizeof got, &got); P("[pixel format] #%d color=%d alpha=%d depth=%d stencil=%d\n", pf, got.cColorBits, got.cAlphaBits, got.cDepthBits, got.cStencilBits); }
  if (!pf || !SetPixelFormat(dc, pf, &pfd)) { P("FATAL: ChoosePixelFormat/SetPixelFormat failed (pf=%d err=%lu)\n", pf, GetLastError()); return 1; }
  HGLRC legacy = wglCreateContext(dc); wglMakeCurrent(dc, legacy);
  wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
  wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
  wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
  if (!wglCreateContextAttribsARB) { P("FATAL: no wglCreateContextAttribsARB\n"); return 1; }

  int attrs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, maj, WGL_CONTEXT_MINOR_VERSION_ARB, min,
                  WGL_CONTEXT_PROFILE_MASK_ARB, core ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                  WGL_CONTEXT_FLAGS_ARB, benchonly ? WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB : WGL_CONTEXT_DEBUG_BIT_ARB, 0 };
  HGLRC ctx = wglCreateContextAttribsARB(dc, NULL, attrs);
  P("[context] request %d.%d %s debug -> %s (err=0x%lx)\n", maj, min, core ? "core" : "compat", ctx ? "OK" : "FAILED", ctx ? 0 : GetLastError());
  if (!ctx) { print_pixel_formats(dc); return 2; }
  wglMakeCurrent(dc, ctx); wglDeleteContext(legacy);
  load_funcs();

  GLint nx = 0; glGetIntegerv(GL_NUM_EXTENSIONS, &nx);
  P("  GL_VENDOR   %s\n  GL_RENDERER %s\n  GL_VERSION  %s\n  GLSL        %s\n  extensions  %d\n",
    glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION), nx);
  if (glDebugMessageCallback) {
    glEnable(GL_DEBUG_OUTPUT); glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); glDebugMessageCallback(dbgcb, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
  }

  if (hold) { run_hold(dc, wnd, hold_secs, hold_interval); return 0; }
  if (terrain) { run_terrain_bench(dc); return 0; }
  if (!benchonly) {
    const char *exts43[] = { "GL_ARB_arrays_of_arrays", "GL_ARB_ES3_compatibility", "GL_ARB_clear_buffer_object", "GL_ARB_compute_shader",
      "GL_ARB_copy_image", "GL_KHR_debug", "GL_ARB_explicit_uniform_location", "GL_ARB_fragment_layer_viewport",
      "GL_ARB_framebuffer_no_attachments", "GL_ARB_internalformat_query2", "GL_ARB_invalidate_subdata", "GL_ARB_multi_draw_indirect",
      "GL_ARB_program_interface_query", "GL_ARB_robust_buffer_access_behavior", "GL_ARB_shader_image_size",
      "GL_ARB_shader_storage_buffer_object", "GL_ARB_stencil_texturing", "GL_ARB_texture_buffer_range", "GL_ARB_texture_query_levels",
      "GL_ARB_texture_storage_multisample", "GL_ARB_texture_view", "GL_ARB_vertex_attrib_binding" };
    const char *extra[] = { "GL_EXT_texture_compression_s3tc", "GL_ARB_texture_compression_bptc", "GL_EXT_texture_filter_anisotropic",
      "GL_ARB_texture_filter_anisotropic", "GL_ARB_buffer_storage", "GL_ARB_direct_state_access", "GL_ARB_clip_control",
      "GL_ARB_shader_draw_parameters", "GL_ARB_indirect_parameters", "GL_ARB_bindless_texture", "GL_ARB_sparse_texture",
      "GL_ARB_gpu_shader_int64", "GL_ARB_shader_ballot", "GL_ARB_seamless_cubemap_per_texture", "GL_ARB_texture_barrier",
      "GL_ARB_get_program_binary", "GL_ARB_parallel_shader_compile", "GL_KHR_parallel_shader_compile", "GL_ARB_gl_spirv" };
    P("\n[GL 4.3 core extensions]\n");
    int missing = 0;
    for (unsigned i = 0; i < sizeof exts43 / sizeof exts43[0]; i++) if (!has_ext(exts43[i])) { P("  MISSING %s\n", exts43[i]); missing++; }
    if (!missing) P("  all 22 present\n");
    P("\n[common extras games use]\n");
    for (unsigned i = 0; i < sizeof extra / sizeof extra[0]; i++) P("  %-7s %s\n", has_ext(extra[i]) ? "yes" : "no", extra[i]);
    GLint nbf = 0; glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &nbf);
    P("  program binary formats: %d (shader cache possible: %s)\n", nbf, nbf > 0 ? "yes" : "no");
    print_limits();
    run_feature_tests();
    run_stream_tests(dc);
    P("\n[program 27 mirror]\n");
    run_p27_test(dc, 0, "text shader exact (flat float varying)");
    run_p27_test(dc, 1, "text shader without flat");
    run_p27_test(dc, 2, "flat varying as its own vec4");
    run_p27_test(dc, 3, "no flat; mode as plain float from VS");
    run_p27_test(dc, 4, "flat int instead of flat float");
    run_p27_test(dc, 5, "exact shader + glProvokingVertex(FIRST)");
    run_p27_test(dc, 0, "exact shader again (LAST, default)");
    P("\n[texture re-spec tests]\n");
    run_respec_test(dc);
    P("\n[UI pattern tests]\n");
    run_ui_pattern_test(dc, 8, 0, "gen/NamedBufferData/draw/delete x8, same frame");
    run_ui_pattern_test(dc, 64, 0, "gen/NamedBufferData/draw/delete x64, same frame");
    run_ui_pattern_test(dc, 400, 0, "gen/NamedBufferData/draw/delete x400, same frame");
    run_ui_pattern_test(dc, 64, 1, "same x64 but glFinish after each draw");
    print_pixel_formats(dc);
  }
  run_benchmarks(dc, wnd);
  if (!benchonly) P("\n[summary] %d pass, %d fail, %d skip\n", g_pass, g_fail, g_skip);
  wglMakeCurrent(NULL, NULL); wglDeleteContext(ctx); ReleaseDC(wnd, dc); DestroyWindow(wnd);
  if (g_out) fclose(g_out);
  return 0;
}
