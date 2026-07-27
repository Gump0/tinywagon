#pragma once
#include <glad/glad.h>
#include <stb_image/stb_image.h>
#include <glm/glm.hpp>
#include <vector>

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

    // texture struct allows for easy 2D image rendering.
    struct Texture
    {
        GLuint id = 0;

        Texture() = default;
        Texture(const char* fileName, bool pixelated = true, bool useMipMap = false)
        {
            loadFromFile(fileName, pixelated, useMipMap);
        };

        // simple method to return the size of a texture buffer as a vector2 (x,y)
        glm::ivec2 GetSize();

        // this function expects a buffer of bytes in GL_RGBA format
        void createFromBuffer(const char* image_data, const int width, const int height,
            bool pixelated = true, bool useMipMap = false);

        // this is used internally to create a 1x1 white texture. the parameter can be used
        // to supply another color if needed.
        void create1PxSquare(const char* b = 0);

        // creates from a texture file
        void createFromFileData(const unsigned char* image_file_data, const size_t image_file_size,
            bool pixelated = true, bool useMipMap = false);
        
        // load texture from file
        void loadFromFile(const char* fileName, bool pixelated = true, bool useMipMap = false);

        // returns the size of the texture stored in memory in bytes
        // used when allocating your buffer when using readTextureData
        // optionally you can also return the width and height with the outsize parameter.
        size_t getMemorySize(int mipLevel = 0, glm::ivec2* outsize = 0);
        
        // reads the texture data back to ram, you need to specify
        // the buffer to read into yourself, allocate it using
        // getMemorySize to know the size yourself.
        // The data will be in RGBA format, one byte each component.
        void readTextureData(void* buffer, int mipLevel = 0);

        // reads the texture data back into ram
        // the data will be in RBGA format, one byte each component.
        // You can also optionally get the width and height of the texture using the outsize parameter.
        std::vector<unsigned char> readTextureData(int mipLevel = 0, glm::ivec2* outsize = 0);
        
        void bind(unsigned int sample = 0);
        void unbind();

        void cleanup();
    };
};