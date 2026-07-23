#pragma once
#include <glad/glad.h>

namespace tw
{
    struct Shader
    {
        GLuint id = 0;
        int u_sampler = -1;

        void bind() 
        {
            glUseProgram(id);
        };

        void clear()
        {
            glDeleteProgram(id);
            *this = {};
        };

        void createShader(const char* fragment, const char* vertex);

        void createShaderFromFile(const char* filePath);

        void createShader(const char* fragment);

        void createDefaultShader();
    };
};