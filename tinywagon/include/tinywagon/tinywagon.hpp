#pragma once
#include <glad/glad.h>

namespace tw
{
    // standard error function, this writes to the console
    void defaultErrorFunc(const char* msg, void* userDefinedData);

    // set by the user, this is passes to the default error function
    void setUserDefinedData(void* data);

    using errorFuncType = decltype(defaultErrorFunc);

    // for the user to set a custom error function. Set to 0 to disable reporting.
    void setErrorFuncCallback(errorFuncType* newFunc);
    errorFuncType* getErrorFuncCallback();

    // shader struct that allows to easily implement shader code
    struct Shader
    {
        GLuint id = 0;
        int u_sampler = -1;

        // binds shader to frame buffer
        void bind()
        {
            glUseProgram(id);
        };
        
        // cleans-up shader
        void clear()
        {
            glDeleteProgram(id);
            *this = {};
        };

        // create shader by passing in glsl code as a c string
        void createShader(const char* fragment, const char* vertex);
        void createShader(const char* fragment);

        // create shader from a file
        void createShaderFromFile(const char* filePath);
        
        // generates default shader (see tinywagon.cpp for details)
        void createDefaultShader();
    };
};