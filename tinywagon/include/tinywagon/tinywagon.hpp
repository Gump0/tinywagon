#pragma once
#include <glad/glad.h>
#include <stb_image/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace tw
{
    // initalize the library, call once before using the library.
    void init();

    // deinitializes the library, call on application quit.
    void cleanup();

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

    // handles all logic related to the camera system
    struct Camera
    { 
        glm::vec2 positionTopLeftCorner { };

        // camera rotation in radians
        float rotation = 0.0f;

        // camera zoom (scaling 1.0f = default)
        float zoom = 1.0f;

        void setDefault() { *this = Camera{}; }

        // used to follow an object (like a player for example)
        // the followed glm::vector2 position will be placed in the center of the screen by default.
        // min is for the minimum distance for the camera to start moving and max is the same.
        // w and h are the dimensions of the camera window.
        // and speed is for interperlation speed of the camera.
        void follow(glm::vec2 pos, float w, float h, float speed = 99999, float min = 0, float max = 0);

        glm::mat4 getMatrix(float w, float h);

        // given the position and this camera, return where the point will end up in screen coordinates
        glm::vec2 worldToScreen(glm::vec2 worldPos, float w, float h);

        // given the screen coordinates and this camera, return where the point will end up in world coordinates
        glm::vec2 screenToWorld(glm::vec2 screenPos, float w, float h);
    };

    // Renderer class where all 2D rendering logic is handled
    struct Renderer2D
    {
        Renderer2D() = default;
        Renderer2D(Renderer2D &other) = delete;
        Renderer2D(Renderer2D &&other) = delete;
        Renderer2D operator=(Renderer2D other) = delete;
        Renderer2D operator=(Renderer2D &other) = delete;
        Renderer2D operator=(Renderer2D &&other) = delete;

        // creates the renderer
        // fbo is the frame buffer the renderer will draw to, 0 means drawings to the screen
        void create(GLuint fbo = 0);

        // clears the renderer object's recourses
        // but does not clean up user setup data such as textures, fonts or fbos beware!
        void cleanup();

        GLuint FBO = 0;

        GLuint triangleDataBuffer = 0;
        GLuint vao = 0;

        // window metrics, should be up to date at all times.
        int windowW = -1;
        int windowH = -1;
        void updateWindowMetrics(int width, int height)
        {
            windowW = std::max(width, 0);
            windowH = std::max(height, 0);
        };

        struct TriangleVertexData
        {
            glm::vec4 position = { };
            glm::vec4 color = { };
            glm::vec2 uvPosition = { };
        };

        struct TriangleData
        {
            TriangleVertexData v1 = { };
            TriangleVertexData v2 = { };
            TriangleVertexData v3 = { };
        };

        std::vector<TriangleData> renderTriangleData;
        std::vector<Texture> textureData;

        Shader shader;
        Camera camera;

        // simply resets the current shader to the default shader.
        // used internally.
        void resetShader();

        // render a triange to the screen using opengl normalized coordinates
		void renderTriangleFromNormalizedPositions(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3,
			Texture texture, glm::vec4 textureCoords, glm::vec4 colors);
        
        // ^^ but can utlize vec2 array for the texture coords 
		void renderTriangleFromNormalizedPositions(const glm::vec4 &p1, const glm::vec4 &p2, const glm::vec4 &p3,
			Texture texture, const glm::vec2 textureCoords[3], const glm::vec4 colors[3]);

        // TODO : make pixel coord vers of ^^
        // similar to how renderRect was programmed.
        // void renderTriangle

        // draw a rectangle with color and/or texture (position = {x, y, width, height})
        void renderRect(const glm::vec4 &position, Texture texture = {}, glm::vec4 colors = {1,1,1,1},
            glm::vec4 textureCoords = {0,0,1,1}, float rotationRadians = 0.0f, glm::vec2 pivot = {0, 0});

        // not to be confused with cleanup, this clears the renderer of draw data to reset the frame
        void clearDrawData();

        // used to to draw frame to screen.
        void flush(bool dontBindAnyFBO = false, bool dontClearDrawData = false, bool dontEnableGLFeatures = false);
    };

    // helper function to return a specific texture within a texture atlas in the form of normalized coordinates.
    glm::vec4 computeTextureAtlas(int xCount, int yCount, int x, int y, bool flipHorizontal = false);

    // used to get texture coordinates for a teture atlas
    struct TextureAtlas
    {
        TextureAtlas() { };
        TextureAtlas(int xCount, int yCount):xCount(xCount), yCount(yCount) { };

        int xCount = 0;
        int yCount = 0;

        glm::vec4 get(int x, int y, bool flipHorizontal = false)
        {
            return computeTextureAtlas(xCount, yCount, x, y, flipHorizontal);
        }
    };

    // A few default color constants for easy access :)
    #define RED glm::vec4(1, 0, 0, 1)
    #define GREEN glm::vec4(0, 1, 0, 1)
    #define BLUE glm::vec4(0, 0, 1, 1)
    #define MAGENTA glm::vec4(1, 0, 1, 1)
    #define PURPLE glm::vec4(0.4, 0.2, 0.7, 1)
    #define YELLOW glm::vec4(1, 1, 0, 1)
    #define ORANGE glm::vec4(1, 0.5, 0.1, 1)
    #define PINK glm::vec4(1, 0.75, 0.8, 1)
    #define CYAN glm::vec4(0, 1, 1, 1)
    #define WHITE glm::vec4(1, 1, 1, 1)
    #define BLACK glm::vec4(0, 0, 0, 1)
    #define GRAY glm::vec4(0.5, 0.5, 0.5, 1)
    #define BROWN glm::vec4(0.4, 0.26, 0.13, 1)
    #define TEAL glm::vec4(0, 0.5, 0.5, 1)
    #define LIME glm::vec4(0.75, 1, 0, 1)
    #define NAVY glm::vec4(0, 0, 0.5, 1)
    #define GOLD glm::vec4(1, 0.84, 0, 1)
    #define TRANSPARENT glm::vec4(0, 0, 0, 0)
};