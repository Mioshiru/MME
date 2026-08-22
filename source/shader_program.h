#pragma once

// Include glext.h first — it defines all PFNGL* types and GL_* constants we need.
// On Windows, glext.h is provided via vcpkg (opengl package).
#ifdef _WIN32
#  include <windows.h>
#  include <GL/gl.h>
#  include <GL/glext.h>  // PFNGLSHADERSOURCEPROC, PFNGLCREATESHADERPROC, etc.
#endif

namespace RME_Rendering {

/// Compiles and links a GLSL vertex + fragment shader program.
/// Extension function pointers are resolved via wglGetProcAddress on first use.
class ShaderProgram {
public:
    ShaderProgram()  = default;
    ~ShaderProgram() { destroy(); }

    ShaderProgram(const ShaderProgram&)            = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    /// Compile + link from embedded source strings. Returns true on success.
    bool build(const char* vertSrc, const char* fragSrc);

    void        use()   const;  ///< glUseProgram(m_program)
    static void unuse();        ///< glUseProgram(0)

    bool   isValid() const { return m_program != 0; }
    GLuint id()      const { return m_program; }

    // Uniform helpers — silently no-op if program is 0
    void setInt  (const char* name, int v)          const;
    void setFloat(const char* name, float v)        const;
    void setMat4 (const char* name, const float* m) const;

    void destroy();

private:
    GLuint m_program = 0;

    static void  loadProcs();
    static bool  s_procsLoaded;
    GLuint compileShader(GLenum type, const char* src) const;

#ifdef _WIN32
    static PFNGLCREATESHADERPROC       s_CreateShader;
    static PFNGLSHADERSOURCEPROC       s_ShaderSource;
    static PFNGLCOMPILESHADERPROC      s_CompileShader;
    static PFNGLCREATEPROGRAMPROC      s_CreateProgram;
    static PFNGLATTACHSHADERPROC       s_AttachShader;
    static PFNGLLINKPROGRAMPROC        s_LinkProgram;
    static PFNGLGETSHADERIVPROC        s_GetShaderiv;
    static PFNGLGETPROGRAMIVPROC       s_GetProgramiv;
    static PFNGLGETSHADERINFOLOGPROC   s_GetShaderInfoLog;
    static PFNGLGETPROGRAMINFOLOGPROC  s_GetProgramInfoLog;
    static PFNGLDELETESHADERPROC       s_DeleteShader;
    static PFNGLDELETEPROGRAMPROC      s_DeleteProgram;
    static PFNGLGETUNIFORMLOCATIONPROC s_GetUniformLocation;
    static PFNGLBINDATTRIBLOCATIONPROC s_BindAttribLocation;
    static PFNGLUNIFORM1IPROC          s_Uniform1i;
    static PFNGLUNIFORM1FPROC          s_Uniform1f;
    static PFNGLUNIFORMMATRIX4FVPROC   s_UniformMatrix4fv;
    static PFNGLUSEPROGRAMPROC         s_UseProgram;
#endif
};

} // namespace RME_Rendering
