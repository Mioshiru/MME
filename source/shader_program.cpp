#include "shader_program.h"

#ifdef _WIN32
#  include <windows.h>
static void* safe_gl_get_proc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 || p == (void*)-1) {
        static HMODULE hMod = GetModuleHandleA("opengl32.dll");
        if (hMod) p = (void*)GetProcAddress(hMod, name);
    }
    return p;
}
#  define GL_GET_PROC(name) safe_gl_get_proc(name)
#endif

#include <wx/log.h>
#include <string>

namespace RME_Rendering {

// ── Static member definitions ─────────────────────────────────────────────────
bool ShaderProgram::s_procsLoaded = false;

#ifdef _WIN32
PFNGLCREATESHADERPROC       ShaderProgram::s_CreateShader      = nullptr;
PFNGLSHADERSOURCEPROC       ShaderProgram::s_ShaderSource      = nullptr;
PFNGLCOMPILESHADERPROC      ShaderProgram::s_CompileShader     = nullptr;
PFNGLCREATEPROGRAMPROC      ShaderProgram::s_CreateProgram     = nullptr;
PFNGLATTACHSHADERPROC       ShaderProgram::s_AttachShader      = nullptr;
PFNGLLINKPROGRAMPROC        ShaderProgram::s_LinkProgram       = nullptr;
PFNGLGETSHADERIVPROC        ShaderProgram::s_GetShaderiv       = nullptr;
PFNGLGETPROGRAMIVPROC       ShaderProgram::s_GetProgramiv      = nullptr;
PFNGLGETSHADERINFOLOGPROC   ShaderProgram::s_GetShaderInfoLog  = nullptr;
PFNGLGETPROGRAMINFOLOGPROC  ShaderProgram::s_GetProgramInfoLog = nullptr;
PFNGLDELETESHADERPROC       ShaderProgram::s_DeleteShader      = nullptr;
PFNGLDELETEPROGRAMPROC      ShaderProgram::s_DeleteProgram     = nullptr;
PFNGLGETUNIFORMLOCATIONPROC ShaderProgram::s_GetUniformLocation= nullptr;
PFNGLBINDATTRIBLOCATIONPROC ShaderProgram::s_BindAttribLocation= nullptr;
PFNGLUNIFORM1IPROC          ShaderProgram::s_Uniform1i         = nullptr;
PFNGLUNIFORM1FPROC          ShaderProgram::s_Uniform1f         = nullptr;
PFNGLUNIFORMMATRIX4FVPROC   ShaderProgram::s_UniformMatrix4fv  = nullptr;
PFNGLUSEPROGRAMPROC         ShaderProgram::s_UseProgram        = nullptr;
#endif

// ── Proc loading ──────────────────────────────────────────────────────────────
void ShaderProgram::loadProcs() {
    if (s_procsLoaded) return;
    s_procsLoaded = true;
#ifdef _WIN32
    s_CreateShader       = (PFNGLCREATESHADERPROC)      GL_GET_PROC("glCreateShader");
    s_ShaderSource       = (PFNGLSHADERSOURCEPROC)      GL_GET_PROC("glShaderSource");
    s_CompileShader      = (PFNGLCOMPILESHADERPROC)     GL_GET_PROC("glCompileShader");
    s_CreateProgram      = (PFNGLCREATEPROGRAMPROC)     GL_GET_PROC("glCreateProgram");
    s_AttachShader       = (PFNGLATTACHSHADERPROC)      GL_GET_PROC("glAttachShader");
    s_LinkProgram        = (PFNGLLINKPROGRAMPROC)       GL_GET_PROC("glLinkProgram");
    s_GetShaderiv        = (PFNGLGETSHADERIVPROC)       GL_GET_PROC("glGetShaderiv");
    s_GetProgramiv       = (PFNGLGETPROGRAMIVPROC)      GL_GET_PROC("glGetProgramiv");
    s_GetShaderInfoLog   = (PFNGLGETSHADERINFOLOGPROC)  GL_GET_PROC("glGetShaderInfoLog");
    s_GetProgramInfoLog  = (PFNGLGETPROGRAMINFOLOGPROC) GL_GET_PROC("glGetProgramInfoLog");
    s_DeleteShader       = (PFNGLDELETESHADERPROC)      GL_GET_PROC("glDeleteShader");
    s_DeleteProgram      = (PFNGLDELETEPROGRAMPROC)     GL_GET_PROC("glDeleteProgram");
    s_GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GL_GET_PROC("glGetUniformLocation");
    s_BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GL_GET_PROC("glBindAttribLocation");
    s_Uniform1i          = (PFNGLUNIFORM1IPROC)         GL_GET_PROC("glUniform1i");
    s_Uniform1f          = (PFNGLUNIFORM1FPROC)         GL_GET_PROC("glUniform1f");
    s_UniformMatrix4fv   = (PFNGLUNIFORMMATRIX4FVPROC)  GL_GET_PROC("glUniformMatrix4fv");
    s_UseProgram         = (PFNGLUSEPROGRAMPROC)        GL_GET_PROC("glUseProgram");
#endif
}

// ── Internal: compile single shader stage ────────────────────────────────────
GLuint ShaderProgram::compileShader(GLenum type, const char* src) const {
#ifdef _WIN32
    if (!s_CreateShader || !s_ShaderSource || !s_CompileShader || !s_GetShaderiv) return 0;
    GLuint sh = s_CreateShader(type);
    if (!sh) return 0;
    s_ShaderSource(sh, 1, &src, nullptr);
    s_CompileShader(sh);
    GLint ok = 0;
    s_GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        if (s_GetShaderiv && s_GetShaderInfoLog) {
            GLint len = 0;
            s_GetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            if (len > 1) {
                std::string log(static_cast<size_t>(len), '\0');
                s_GetShaderInfoLog(sh, len, nullptr, log.data());
                wxLogError("ShaderProgram: compile error:\n%s", log.c_str());
            }
        }
        if (s_DeleteShader) s_DeleteShader(sh);
        return 0;
    }
    return sh;
#else
    (void)type; (void)src;
    return 0;
#endif
}

// ── Build (compile + link) ────────────────────────────────────────────────────
bool ShaderProgram::build(const char* vertSrc, const char* fragSrc) {
    loadProcs();
#ifdef _WIN32
    if (!s_CreateProgram || !s_AttachShader || !s_LinkProgram || !s_GetProgramiv)
        return false;

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vert || !frag) {
        if (vert && s_DeleteShader) s_DeleteShader(vert);
        if (frag && s_DeleteShader) s_DeleteShader(frag);
        return false;
    }

    GLuint prog = s_CreateProgram();
    s_AttachShader(prog, vert);
    s_AttachShader(prog, frag);
    if (s_BindAttribLocation) {
        s_BindAttribLocation(prog, 0, "aPos");
        s_BindAttribLocation(prog, 1, "aTexCoord");
        s_BindAttribLocation(prog, 2, "aColor");
        s_BindAttribLocation(prog, 3, "aShaderData");
    }
    s_LinkProgram(prog);
    if (s_DeleteShader) { s_DeleteShader(vert); s_DeleteShader(frag); }

    GLint ok = 0;
    s_GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        if (s_GetProgramiv && s_GetProgramInfoLog) {
            GLint len = 0;
            s_GetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
            if (len > 1) {
                std::string log(static_cast<size_t>(len), '\0');
                s_GetProgramInfoLog(prog, len, nullptr, log.data());
                wxLogError("ShaderProgram: link error:\n%s", log.c_str());
            }
        }
        if (s_DeleteProgram) s_DeleteProgram(prog);
        return false;
    }

    m_program = prog;
    wxLogDebug("ShaderProgram: build successful, program ID: %u", prog);
    return true;
#else
    (void)vertSrc; (void)fragSrc;
    return false;
#endif
}

// ── use / unuse ───────────────────────────────────────────────────────────────
void ShaderProgram::use() const {
#ifdef _WIN32
    if (s_UseProgram && m_program) s_UseProgram(m_program);
#endif
}

void ShaderProgram::unuse() {
#ifdef _WIN32
    if (s_UseProgram) s_UseProgram(0);
#endif
}

// ── destroy ───────────────────────────────────────────────────────────────────
void ShaderProgram::destroy() {
#ifdef _WIN32
    if (m_program) {
        if (s_DeleteProgram) s_DeleteProgram(m_program);
        m_program = 0;
    }
#endif
}

// ── Uniform setters ───────────────────────────────────────────────────────────
void ShaderProgram::setInt(const char* name, int v) const {
#ifdef _WIN32
    if (!m_program || !s_GetUniformLocation || !s_Uniform1i) return;
    GLint loc = s_GetUniformLocation(m_program, name);
    if (loc >= 0) s_Uniform1i(loc, v);
#endif
}

void ShaderProgram::setFloat(const char* name, float v) const {
#ifdef _WIN32
    if (!m_program || !s_GetUniformLocation || !s_Uniform1f) return;
    GLint loc = s_GetUniformLocation(m_program, name);
    if (loc >= 0) s_Uniform1f(loc, v);
#endif
}

void ShaderProgram::setMat4(const char* name, const float* m) const {
#ifdef _WIN32
    if (!m_program || !s_GetUniformLocation || !s_UniformMatrix4fv) return;
    GLint loc = s_GetUniformLocation(m_program, name);
    if (loc >= 0) s_UniformMatrix4fv(loc, 1, GL_FALSE, m);
#endif
}

} // namespace RME_Rendering
