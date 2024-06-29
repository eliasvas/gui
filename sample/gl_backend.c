#include "gui.h"
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL.h>
//#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_main.h>
#include <GL/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// globals
GLuint vao,vbo,sp,atlas_tex,atlas_sampler;
SDL_Window *window;
SDL_GLContext glcontext;
const char* vertexShaderSource =
"#version 330 core\n"
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

const char* fragmentShaderSource =
"#version 330 core\n"
"\n"
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
"    vec2 softnessPadding = vec2(max(0, softness * 2.0 - 1.0), max(0, softness * 2.0 - 1.0));\n"
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

GLuint create_sp(const char* vertexShaderSource, const char* fragmentShaderSource) {
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) {
        return 0;
    }

    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource);
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

        char errorLog[256];
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

GLuint create_tex_and_sampler(const GLubyte* pixels, GLsizei width, GLsizei height, GLuint* sampler) {
    GLuint texture;

    // Generate and bind texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Specify texture parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Create and configure sampler
    glGenSamplers(1, sampler);
    glSamplerParameteri(*sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(*sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
    // create a window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    window = SDL_CreateWindow("sample", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        exit(1);
    }
    SDL_GL_MakeCurrent(window,glcontext);
    glewInit();

    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth){
            SDL_Log("This is a highdpi environment.");
        }
    }
    SDL_Log("Application started successfully!");
    vbo = create_instance_vbo(NULL, 0);
    fill_instance_vbo(vbo, NULL, 0);
    SDL_Log("vbo OK!");
    vao = setup_instance_vao(vbo);
    SDL_Log("vao OK!");
    sp = create_sp(vertexShaderSource, fragmentShaderSource);
    SDL_Log("Shader program OK!");
    atlas_tex = create_tex_and_sampler((GLubyte*)font_atlas_data, 512, 512,&atlas_sampler);
    SDL_Log("Atlas texture OK!");
}

void platform_update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)exit(1);
    }
}

vec2 platform_get_windim() {
    int ww,wh;
    SDL_GetWindowSize(window, &ww, &wh);
    return v2(ww,wh);
}

void platform_render(guiRenderCommand *rcommands, u32 command_count)
{
    //printf("instance count: %d\n", command_count);
    float time = SDL_GetTicks() / 1000.f;
    float red = (sin(time) + 1) / 2.0;
    float green = (sin(time / 2) + 1) / 2.0;
    float blue = (sin(time) * 2 + 1) / 2.0;
    glClearColor(red, green, blue, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Here we draw
    fill_instance_vbo(vbo, (InstanceData*)rcommands, command_count);
    glUseProgram(sp);
    glBindVertexArray(vao);
    glUniform2f(glGetUniformLocation(sp, "winDim"), platform_get_windim().x, platform_get_windim().y);
    glBindTexture(GL_TEXTURE_2D, atlas_tex);
    glBindSampler(0, atlas_sampler);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, command_count);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindSampler(0, 0);
    glBindVertexArray(0);
    //---------
    SDL_GL_SwapWindow(window);
}

void platform_deinit() {}