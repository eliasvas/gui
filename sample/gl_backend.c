#include "gui.h"

#define SDL_MAIN_NOIMPL

#include <SDL.h>
#include <SDL_main.h>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
    //#include <SDL_opengles2.h>
#else
    #ifdef _WIN32
        #include <GL/glew.h>
        #include <GL/wglew.h>
    #else
        #include <SDL_syswm.h>
	    #include <GLES3/gl3.h>
        #include <GLES/egl.h>
    #endif
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// globals
GLuint vao,vbo,sp,atlas_tex,atlas_sampler;
SDL_Window *window;
SDL_GLContext glcontext;
const char* gui_vs =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = 0) in vec2 inPos0;\n"
"layout(location = 1) in vec2 inPos1;\n"
"layout(location = 2) in vec2 inUV0;\n"
"layout(location = 3) in vec2 inUV1;\n"
"layout(location = 4) in vec4 inColor;\n"
"layout(location = 5) in float inCornerRadius;\n"
"layout(location = 6) in float inEdgeSoftness;\n"
"layout(location = 7) in float inBorderThickness;\n"
"\n"
"uniform vec2 winDim;\n"
"\n"
"out vec2 fragUV;\n"
"out vec2 fragDstPos;\n"
"out vec2 fragDstCenter;\n"
"out vec2 fragHalfSize;\n"
"out vec4 fragColor;\n"
"out float fragCornerRadius;\n"
"out float fragEdgeSoftness;\n"
"out float fragBorderThickness;\n"
"\n"
"const vec2 vertices[4] = vec2[4](\n"
"    vec2(-1, -1),\n"
"    vec2(-1, +1),\n"
"    vec2(+1, -1),\n"
"    vec2(+1, +1)\n"
");\n"
"\n"
"void main() {\n"
"    vec2 dstHalfSize = (inPos1 - inPos0) / 2.0;\n"
"    vec2 dstCenter = (inPos1 + inPos0) / 2.0;\n"
"    vec2 dstPos = vertices[gl_VertexID] * dstHalfSize + dstCenter;\n"
"\n"
"    gl_Position = vec4(2.0 * dstPos / winDim - 1.0, 0.0, 1.0);\n"
"    gl_Position.y *= -1.0;\n"
"\n"
"    vec2 uvHalfSize = (inUV1 - inUV0) / 2.0;\n"
"    vec2 uvCenter = (inUV1 + inUV0) / 2.0;\n"
"    vec2 uvPos = vertices[gl_VertexID] * uvHalfSize + uvCenter;\n"
"\n"
"    fragUV = uvPos;\n"
"    fragColor = inColor;\n"
"    fragCornerRadius = inCornerRadius;\n"
"    fragEdgeSoftness = inEdgeSoftness;\n"
"    fragBorderThickness = inBorderThickness;\n"
"    fragDstPos = dstPos;\n"
"    fragDstCenter = dstCenter;\n"
"    fragHalfSize = dstHalfSize;\n"
"}\n";

const char* gui_fs =
"#version 300 es\n"
"precision mediump float;\n"
"in vec2 fragUV;\n"
"in vec2 fragDstPos;\n"
"in vec2 fragDstCenter;\n"
"in vec2 fragHalfSize;\n"
"in vec4 fragColor;\n"
"in float fragCornerRadius;\n"
"in float fragEdgeSoftness;\n"
"in float fragBorderThickness;\n"
"\n"
"out vec4 outColor;\n"
"\n"
"uniform sampler2D texture0;\n"
"\n"
"float roundedRectSDF(vec2 samplePos, vec2 rectCenter, vec2 rectHalfSize, float r) {\n"
"    vec2 d2 = abs(rectCenter - samplePos) - rectHalfSize + vec2(r, r);\n"
"    return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;\n"
"}\n"
"\n"
"void main() {\n"
"    ivec2 texSize = textureSize(texture0, 0);\n"
"    float col = texture(texture0, fragUV / vec2(texSize)).r;\n"
"    vec4 tex = vec4(col, col, col, col);\n"
"\n"
"    float softness = fragEdgeSoftness + 0.001;\n"
"    float cornerRadius = fragCornerRadius;\n"
"    vec2 softnessPadding = vec2(max(0.0, softness * 2.0 - 1.0), max(0.0, softness * 2.0 - 1.0));\n"
"\n"
"    float dist = roundedRectSDF(fragDstPos, fragDstCenter, fragHalfSize - softnessPadding, cornerRadius);\n"
"    float sdfFactor = 1.0 - smoothstep(0.0, 2.0 * softness, dist);\n"
"\n"
"    float borderFactor = 1.0;\n"
"    if (fragBorderThickness != 0.0) {\n"
"        vec2 interiorHalfSize = fragHalfSize - vec2(fragBorderThickness);\n"
"        float interiorRadiusReduceF = min(interiorHalfSize.x / fragHalfSize.x, interiorHalfSize.y / fragHalfSize.y);\n"
"        float interiorCornerRadius = fragCornerRadius * interiorRadiusReduceF * interiorRadiusReduceF;\n"
"\n"
"        float insideD = roundedRectSDF(fragDstPos, fragDstCenter, interiorHalfSize - softnessPadding, interiorCornerRadius);\n"
"        float insideF = smoothstep(0.0, 2.0 * softness, insideD);\n"
"        borderFactor = insideF;\n"
"    }\n"
"\n"
"    outColor = fragColor * tex * sdfFactor * borderFactor;\n"
"    if (outColor.a < 0.01) discard;\n"
"}\n";

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char errorLog[256];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        SDL_Log("Shader compilation error: %s",errorLog);

        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint create_sp(const char* gui_vs, const char* gui_fs) {
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, gui_vs);
    if (vertexShader == 0) {
        return 0;
    }

    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, gui_fs);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        char errorLog[512];
        glGetProgramInfoLog(program, maxLength, &maxLength, errorLog);
        SDL_Log("Shader compilation error: %s",errorLog);



        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
#include <stddef.h> // for offsetof

typedef struct InstanceData {
    float pos0[2];
    float pos1[2];
    float uv0[2];
    float uv1[2];
    float color[4];
    float corner_radius;
    float edge_softness;
    float border_thickness;
}InstanceData;

void fill_instance_vbo(GLuint instanceVBO, const InstanceData* instances, GLsizei instance_count) {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instance_count * sizeof(InstanceData), instances, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint create_instance_vbo(const InstanceData* instances, GLsizei instance_count) {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, instance_count * sizeof(InstanceData), instances, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

GLuint create_atlas_tex_and_sampler(const GLubyte* pixels, GLsizei width, GLsizei height, GLuint* sampler) {
    GLuint texture;

    // Generate and bind texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Specify texture parameters
    #ifdef __EMSCRIPTEN__
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    #else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    #endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Create and configure sampler
    // glGenSamplers(1, sampler);
    // glSamplerParameteri(*sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glSamplerParameteri(*sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Unbind texture and sampler to clean state
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

GLuint setup_instance_vao(GLuint vbo) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0); // inPos0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, pos0));
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1); // inPos1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, pos1));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2); // inUV0
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, uv0));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3); // inUV1
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, uv1));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4); // inColor
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5); // inCornerRadius
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, corner_radius));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6); // inEdgeSoftness
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, edge_softness));
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7); // inBorderThickness
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, border_thickness));
    glVertexAttribDivisor(7, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void platform_init(u8 *font_atlas_data) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (SDL_Init(SDL_INIT_VIDEO)) {
        exit(1);
    }
    #ifdef __EMSCRIPTEN__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    #elif _WIN32
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    #endif
    
    window = SDL_CreateWindow("sample",SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        exit(1);
    }
    SDL_GL_MakeCurrent(window,glcontext);
    #ifdef __EMSCRIPTEN__
        //glewInit();
    #elif _WIN32
        glewInit();
        assert(GLEW_ARB_ES3_compatibility);
        glEnable(GL_FRAMEBUFFER_SRGB);
    #endif
    {
        int majorv, minorv, profilem;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorv);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorv);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profilem);
        SDL_Log("OpenGL ES [%s] [%d.%d]\n", (SDL_GL_CONTEXT_PROFILE_ES == profilem) ? "true" : "false", majorv, minorv);
    }

    SDL_ShowWindow(window);
    {
        int width, height;//, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_Log("Window size: %ix%i", width, height);
    }
    SDL_Log("Application started successfully!");
    vbo = create_instance_vbo(NULL, 0);
    fill_instance_vbo(vbo, NULL, 0);
    SDL_Log("vbo OK!");
    vao = setup_instance_vao(vbo);
    SDL_Log("vao OK!");
    sp = create_sp(gui_vs, gui_fs);
    SDL_Log("Shader program OK!");
    font_atlas_data[0] = 0xFF;
    atlas_tex = create_atlas_tex_and_sampler((GLubyte*)font_atlas_data, 1024, 1024,&atlas_sampler);

    SDL_Log("Atlas texture OK!");
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

guiVec2 platform_get_windim() {
    int ww,wh;
    SDL_GetWindowSize(window, &ww, &wh);
    return gv2(ww,wh);
}

void platform_update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        guiInputEventNode e = {0};
        switch (event.type) {
            case SDL_QUIT:
                exit(1);
            case SDL_MOUSEMOTION:
                e.type = GUI_INPUT_EVENT_TYPE_MOUSE_MOVE;
                e.param0 = event.motion.x;
                e.param1 = event.motion.y;
                gui_input_push_event(e);
                break;
            case SDL_MOUSEBUTTONDOWN:
                e.type = GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
                e.param1 = 1;
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        e.param0 = GUI_LMB;
                        break;
                    case SDL_BUTTON_RIGHT:
                        e.param0 = GUI_RMB;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        e.param0 = GUI_MMB;
                        break;
                }
                gui_input_push_event(e);
                break;

            case SDL_MOUSEBUTTONUP:
                e.type = GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
                e.param1 = 0;
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        e.param0 = GUI_LMB;
                        break;
                    case SDL_BUTTON_RIGHT:
                        e.param0 = GUI_RMB;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        e.param0 = GUI_MMB;
                        break;
                }
                gui_input_push_event(e);
                break;
            case SDL_MOUSEWHEEL:
                e.type = GUI_INPUT_EVENT_TYPE_SCROLLWHEEL_EVENT;
                s32 scroll_y = event.wheel.y;
                e.param0 = scroll_y;
                gui_input_push_event(e);
                break;
            default:
                break;
        }
    }

    guiVec2 windim = platform_get_windim();
    glViewport(0,0,windim.x,windim.y);
}



void platform_render(guiRenderCommand *rcommands, u32 command_count)
{
    //printf("instance count: %d\n", command_count);
    //glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Here we draw
    fill_instance_vbo(vbo, (InstanceData*)rcommands, command_count);
    glUseProgram(sp);
    glBindVertexArray(vao);
    glUniform2f(glGetUniformLocation(sp, "winDim"), platform_get_windim().x, platform_get_windim().y);
    glBindTexture(GL_TEXTURE_2D, atlas_tex);
    //glBindSampler(0, atlas_sampler);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, command_count);
    glBindTexture(GL_TEXTURE_2D, 0);
    //glBindSampler(0, 0);
    glBindVertexArray(0);
    //---------
    SDL_GL_SwapWindow(window);
}

void platform_deinit() {}