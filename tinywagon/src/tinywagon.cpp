#include <iostream>
#include <fstream>
#include <tinywagon/tinywagon.hpp>

namespace tw
{
    ////////////////////
    // ERROR HANDLING //
    ////////////////////
    
    void defaultErrorFunc(const char* msg, void* userDefinedData)
    {
        std::cerr << "Tinywagon error: " << msg << std::endl;
    }

    static errorFuncType* currentErrorFunc = defaultErrorFunc;

    static void* userDefinedData = 0;
    void setUserDefinedData(void* data)
    {
        userDefinedData = data;
    }

    void setErrorFuncCallback(errorFuncType* newFunc)
    {
        currentErrorFunc = newFunc;
    }

    errorFuncType* getErrorFuncCallback()
    {
        return currentErrorFunc;
    }

    // used internally
    void reportError(const char* errorMessage)
    {
        if (!errorMessage || !currentErrorFunc)
        {
            return;
        }

        currentErrorFunc(errorMessage, userDefinedData);
    }

    /////////////
    // SHADERS //
    ///////////// 

    static const char* defaultVertexShader =
        R"(
    #version 330
    precision highp float;

    layout(location = 0) in vec4 position;
    layout(location = 1) in vec4 triangle_colors;
    layout(location = 2) in vec2 texturePositions;
    out vec4 v_color;
    out vec2 v_uv;

    void main()
    {
        gl_Position = position;
        v_color = triangle_colors;
        v_uv = texturePositions;
    }
        )";

    static const char* defaultFragmentShader = 
        R"(
    #version 330
    precision highp float;

    out vec4 color;
    in vec4 v_color;
    in vec2 v_uv;
    uniform sampler2D u_sampler;

    void main()
    {
        //color = v_color * texture2D(u_sampler, v_uv);
        color = vec4(1, 0, 0, 1);
    }
        )";

    GLuint loadShader(const char* source, GLenum shaderType)
    {
        GLuint id = glCreateShader(shaderType);

        glShaderSource(id, 1, &source, 0);
        glCompileShader(id);

        int result = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);

        if (result == 0)
        {
            char* message = 0;
            int l = 0;

            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &l);

            message = new char[l];

            glGetShaderInfoLog(id, l, &l, message);

            message[l - 1] = 0;

            reportError(message);

            delete[] message;
        }

        return id;
    }

    void validateProgram(GLuint id)
    {
        int info = 0;

        glGetProgramiv(id, GL_LINK_STATUS, &info);

        if (info != GL_TRUE)
        {
            char* message = 0;
            int l = 0;

            glGetProgramiv(id, GL_INFO_LOG_LENGTH, &l);

            message = new char[l];

            glGetProgramInfoLog(id, l, &l, message);

            reportError(message);

            delete[] message;
        }
    }

    void Shader::createShader(const char* fragment, const char* vertex)
    {
        // reset data to default value
        *this = {};

        const GLuint fragmentId = loadShader(fragment, GL_FRAGMENT_SHADER);
        const GLuint vertexId = loadShader(vertex, GL_VERTEX_SHADER);

        id = glCreateProgram();
        glAttachShader(id, vertexId);
        glAttachShader(id, fragmentId);

        glLinkProgram(id);

        glDeleteShader(vertexId);
        glDeleteShader(fragmentId);

        validateProgram(id);

        u_sampler = glGetUniformLocation(id, "u_sampler");
    }

    // use-case where user only wants to make a custom fragment shader
    void Shader::createShader(const char* fragment)
    {
        createShader(fragment, defaultVertexShader);
    }

    void Shader::createDefaultShader()
    {
        createShader(defaultFragmentShader, defaultVertexShader);
    }

    void Shader::createShaderFromFile(const char* filePath)
    {
        std::ifstream fileFont(filePath, std::ios::binary);

        if (!fileFont.is_open())
        {
            std::string e = "error opening: "; e += filePath;
            reportError(e.c_str());
            return;
        }

        int fileSize = 0;
        fileFont.seekg(0, std::ios::end);
        fileSize = (int)fileFont.tellg();
        fileFont.seekg(0, std::ios::beg);
        char* fileData = new char[fileSize + 1] {}; // null terminated
        fileFont.read((char*)fileData, fileSize);
        fileFont.close();
        fileData[fileSize] = 0; // null terminated

        createShader(fileData);

        delete[] fileData;
    }
};