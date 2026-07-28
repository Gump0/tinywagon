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

    /////////////////////////////////////////
    // LIBRARY SETUP, CLEANUP AND  RUNTIME //
    /////////////////////////////////////////
    static bool hasInitialized = false;
    static Shader defaultShader = { };
    static Texture white1pxSquareTexture = { };

    void init()
    {
        if (hasInitialized)
            return;

        // if we don't have OpenGL functions, we can assume that OpenGL isn't setup correctly.
        // so let's check for this random OpenGL function.
        if (!glGenTextures)
        {
            reportError("OpenGL failed to initialize,\ have you forgot to call gladLoad()? \
                or gladLoadGLLoader() or glfwInit()?");

            return;
        }

        defaultShader.createDefaultShader();
        white1pxSquareTexture.create1PxSquare();

        hasInitialized = true;
    }

    void cleanup()
    {
        white1pxSquareTexture.cleanup();
        defaultShader.clear();
        hasInitialized = false;
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
        color = v_color * texture2D(u_sampler, v_uv);
        // color = vec4(1, 0, 0, 1);
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
    
    /////////////
    // TEXTURE //
    /////////////

    glm::ivec2 Texture::GetSize()
    {
        if (id == 0)
            return{};

        glm::ivec2 size = {};
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &size.x);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &size.y);
        return size;
    }

    void Texture::createFromBuffer(const char* image_data, const int width, const int height,
        bool pixelated, bool useMipMap)
    {
        GLuint id = 0;

        glActiveTexture(GL_TEXTURE0);

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        if (pixelated)
        {
            if (useMipMap)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            if (useMipMap)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        if (useMipMap)
            glGenerateMipmap(GL_TEXTURE_2D);

        this->id = id;
    }

    void Texture::create1PxSquare(const char* b)
    {
        if (b == nullptr)
        {
            const unsigned char buff[] =
            {
                0xff,
                0xff,
                0xff,
                0xff
            };

            createFromBuffer((char*)buff, 1, 1);
        }
        else
        {
            createFromBuffer(b, 1, 1);
        }
    }

    void Texture::createFromFileData(const unsigned char* image_file_data, const size_t image_file_size,
        bool pixelated, bool useMipMap)
    {
        stbi_set_flip_vertically_on_load(true);

        int width = 0;
        int height = 0;
        int channels = 0;

        const unsigned char* decodedImage = stbi_load_from_memory(image_file_data, (int)image_file_size, &width, &height, &channels, 4);

        if (!decodedImage)
            return;

        createFromBuffer((const char*)decodedImage, width, height, pixelated, useMipMap);

        STBI_FREE(decodedImage);
    }

    void Texture::loadFromFile(const char* fileName, bool pixelated, bool useMipMap)
    {
        std::ifstream file(fileName, std::ios::binary);

        if (!file.is_open())
        {
            std::string s = "error opening :";
            s += fileName;
            reportError(s.c_str());
            return;
        }

        int fileSize = 0;
        file.seekg(0, std::ios::end);
        fileSize = (int)file.tellg();
        file.seekg(0, std::ios::beg);
        unsigned char* fileData = new unsigned char[fileSize];
        file.read((char*)fileData, fileSize);
        file.close();

        createFromFileData(fileData, fileSize, pixelated, useMipMap);

        delete[] fileData;
    }

    size_t Texture::getMemorySize(int mipLevel, glm::ivec2* outsize)
    {
        glBindTexture(GL_TEXTURE_2D, id);

        glm::ivec2 stub = {};
        
        if (!outsize)
        {
            outsize = &stub;
        }
        
        glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, &outsize->x);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, &outsize->y);

        glBindTexture(GL_TEXTURE_2D, 0);

        return outsize->x * outsize->y * 4;
    }

    void Texture::readTextureData(void* buffer, int miplevel)
    {
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexImage(GL_TEXTURE_2D, miplevel, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }

    std::vector<unsigned char> Texture::readTextureData(int mipLevel, glm::ivec2* outsize)
    {
        glBindTexture(GL_TEXTURE_2D, id);

        glm::ivec2 stub = {};
        
        if (!outsize)
        {
            outsize = &stub;
        }
        
        glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, &outsize->x);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, &outsize->y);

        std::vector<unsigned char> data;
        data.resize(outsize->x * outsize->y * 4);
        glGetTexImage(GL_TEXTURE_2D, mipLevel, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        glBindTexture(GL_TEXTURE_2D, 0);

        return data;
    }

    void Texture::bind(unsigned int sample)
    {
        glActiveTexture(GL_TEXTURE0 + sample);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void Texture::unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::cleanup()
    {
        glDeleteTextures(1, &id);
        *this = {};
    }

    //////////////
    // RENDERER //
    //////////////

    void Renderer2D::create(GLuint fbo)
    {
        if (!hasInitialized)
        {
            reportError("Library not initialized. Have you forgot to run tw::init()??");
        }

        FBO = fbo;

        renderTriangleData.reserve(20);

        resetShader();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &triangleDataBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, triangleDataBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 4));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));

        glBindVertexArray(0);
    }
    
    void Renderer2D::resetShader()
    {
        shader = defaultShader;
    }

    void Renderer2D::cleanup()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &triangleDataBuffer);

        vao = 0;
        triangleDataBuffer = 0;
    }

	void Renderer2D::renderTriangleFromNormalizedPositions(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3,
		Texture texture, glm::vec4 textureCoords, glm::vec4 colors)
	{
		glm::vec4 colorsVector[3] = {colors, colors, colors};

		float u0 = textureCoords.x;
		float v0 = textureCoords.y;
		float u1 = textureCoords.z;
		float v1 = textureCoords.w;

		glm::vec2 textureCoordsVector[3] = {{u0, v0}, {u1, v0}, {u1, v1}};
		renderTriangleFromNormalizedPositions(p1, p2, p3, texture, textureCoordsVector, colorsVector);

	}

	void Renderer2D::renderTriangleFromNormalizedPositions(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3,
		Texture texture, const glm::vec2 textureCoords[3], const glm::vec4 colors[3])
	{

		TriangleData triangleData;

		TriangleVertexData first = {p1, colors[0], textureCoords[0]};
		TriangleVertexData second = {p2, colors[1], textureCoords[1]};
		TriangleVertexData third = {p3, colors[2], textureCoords[2]};

		triangleData.v1 = first;
		triangleData.v2 = second;
		triangleData.v3 = third;

		renderTriangleData.push_back(triangleData);
	}

    void Renderer2D::clearDrawData()
    {
        renderTriangleData = { };
    }

    void Renderer2D::flush(bool dontBindAnyFBO, bool dontClearDrawData, bool dontEnableGLFeatures)
    {
        if (!hasInitialized)
        {
            reportError("Library not initialized. Have you forgot to run tw::init()??");
        
            if (!dontClearDrawData)
            {
                clearDrawData();
            }

            return;
        }

        if (!vao)
        {
            reportError("Renderer not initialized. Have you forgot to run tw::Renderer2D::create()??");
        
            if (!dontClearDrawData)
            {
                clearDrawData();
            }

            return;
        }

        // for the case that the window is minimized
        if (windowH == 0 || windowW == 0)
        {
            clearDrawData();
        }

        if (windowH < 0 || windowW < 0)
        {
            if (!dontClearDrawData)
            {
                clearDrawData();
            }   

            reportError("Negative windowW or windowH, have you forgotten to call updateWindowMetrics(w, h)??");

            return;
        }

        if (!dontBindAnyFBO)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        }   

        if (!dontEnableGLFeatures)
        {
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }

        // draw logic
        if (!renderTriangleData.empty())
        {
            glBindVertexArray(vao);

            glViewport(0, 0, windowW, windowH);
            glUseProgram(shader.id);
            glUniform1i(shader.u_sampler, 0);

            // buffer orphaning.
            // small optmization made to help tell the GPU driver that new buffer data
            // will come in to replace the old buffer data.
            glBindBuffer(GL_ARRAY_BUFFER, triangleDataBuffer);
            glBufferData(GL_ARRAY_BUFFER, renderTriangleData.size() * sizeof(renderTriangleData[0]), 
                renderTriangleData.data(), GL_STREAM_DRAW);

            glDrawArrays(GL_TRIANGLES, 0, 3 * renderTriangleData.size());

            glBindVertexArray(0);
        }

        if (!dontClearDrawData)
        {
            clearDrawData();
        }

        if (!dontBindAnyFBO)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
};