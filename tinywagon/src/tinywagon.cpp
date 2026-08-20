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
            reportError("OpenGL failed to initialize, have you forgot to call gladLoad()? "
                "or gladLoadGLLoader() or glfwInit()?");

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

    glm::ivec2 Texture::getSize()
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
        {
            reportError("Error decode image failure: ");
            return;
        }

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

	void Texture::createFromFileDataWithPixelPadding(const unsigned char *image_file_data,
		const size_t image_file_size, int xCount, int yCount, bool pixelated, bool useMipMaps)
	{

		stbi_set_flip_vertically_on_load(true);

		int width = 0;
		int height = 0;
		int channels = 0;

		const unsigned char *decodedImage = stbi_load_from_memory(image_file_data, (int)image_file_size, &width, &height, &channels, 4);

		if (!decodedImage) { return; }

		// how big is one element in the atlas in pixels
		int blockSizeX = width / xCount;
		int blockSizeY = height / yCount;

		// the new dimensions start from last dimension + 2 pixel for each block there is
		int newW = width + xCount * 2;
		int newH = height + yCount * 2;

		unsigned char *newData = new unsigned char[newW * newH * 4] {};


		auto getOld = [decodedImage, width](int x, int y, int c)->const unsigned char
		{
			return decodedImage[4 * (x + (y * width)) + c];
		};

		auto getNew = [newData, newW](int x, int y, int c)
		{
			return &newData[4 * (x + (y * newW)) + c];
		};

		int newDataCursor = 0;
		int dataCursor = 0;

		// first copy data
		for (int y = 0; y < newH; y++)
		{
			// we set this to true for collums that are padding zones
			int yNo = 0;
			if ((y == 0 || y == newH - 1
				|| ((y) % (blockSizeY + 2)) == 0 ||
				((y + 1) % (blockSizeY + 2)) == 0
				))
			{
				yNo = 1;
			}

			for (int x = 0; x < newW; x++)
			{
				if (
					yNo ||

					((
					x == 0 || x == newW - 1
					|| (x % (blockSizeX + 2)) == 0 ||
					((x + 1) % (blockSizeX + 2)) == 0
					)
					)

					)
				{
					// this is a padding zone, for now we set it to 0
					newData[newDataCursor++] = 0;
					newData[newDataCursor++] = 0;
					newData[newDataCursor++] = 0;
					newData[newDataCursor++] = 0;
				}
				else
				{
					newData[newDataCursor++] = decodedImage[dataCursor++];
					newData[newDataCursor++] = decodedImage[dataCursor++];
					newData[newDataCursor++] = decodedImage[dataCursor++];
					newData[newDataCursor++] = decodedImage[dataCursor++];
				}

			}

		}

		// then add margins
		for (int x = 1; x < newW - 1; x++)
		{
			// copy on left
			if (x == 1 ||
				(x % (blockSizeX + 2)) == 1
				)
			{
				for (int y = 0; y < newH; y++)
				{
					*getNew(x - 1, y, 0) = *getNew(x, y, 0);
					*getNew(x - 1, y, 1) = *getNew(x, y, 1);
					*getNew(x - 1, y, 2) = *getNew(x, y, 2);
					*getNew(x - 1, y, 3) = *getNew(x, y, 3);
				}

			}
			else // copy on right
				if (x == newW - 2 ||
					(x % (blockSizeX + 2)) == blockSizeX
					)
				{
					for (int y = 0; y < newH; y++)
					{
						*getNew(x + 1, y, 0) = *getNew(x, y, 0);
						*getNew(x + 1, y, 1) = *getNew(x, y, 1);
						*getNew(x + 1, y, 2) = *getNew(x, y, 2);
						*getNew(x + 1, y, 3) = *getNew(x, y, 3);
					}
				}
		}

		for (int y = 1; y < newH - 1; y++)
		{
			if (y == 1 ||
				(y % (blockSizeY + 2)) == 1
				)
			{
				for (int x = 0; x < newW; x++)
				{
					*getNew(x, y - 1, 0) = *getNew(x, y, 0);
					*getNew(x, y - 1, 1) = *getNew(x, y, 1);
					*getNew(x, y - 1, 2) = *getNew(x, y, 2);
					*getNew(x, y - 1, 3) = *getNew(x, y, 3);
				}
			}
			else
				if (y == newH - 2 ||
					(y % (blockSizeY + 2)) == blockSizeY
					)
				{
					for (int x = 0; x < newW; x++)
					{
						*getNew(x, y + 1, 0) = *getNew(x, y, 0);
						*getNew(x, y + 1, 1) = *getNew(x, y, 1);
						*getNew(x, y + 1, 2) = *getNew(x, y, 2);
						*getNew(x, y + 1, 3) = *getNew(x, y, 3);
					}
				}

		}

		createFromBuffer((const char *)newData, newW, newH, pixelated, useMipMaps);

		STBI_FREE(decodedImage);
		delete[] newData;
	}

    void Texture::loadFromFileWithPixelPadding(const char* fileName, int xCount, int yCount,
        bool pixelated, bool useMipMaps)
    {
        std::ifstream file(fileName, std::ios::binary);

        if (!file.is_open())
        {
            std::string s = "error opening: ";
            s += fileName;
            reportError(s.c_str());
            return;
        }

        int fileSize = 0;
        file.seekg(0, std::ios::end);
        fileSize = (int)file.tellg();
        file.seekg(0, std::ios::beg);
        unsigned char* fileData = new unsigned char[fileSize];
        file.read((char*) fileData, fileSize);
        file.close();

        createFromFileDataWithPixelPadding(fileData, fileSize, xCount, yCount, pixelated, useMipMaps);

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

    //////////
    // FONT //
    //////////

    bool Font::createFromTTF(const unsigned char* ttfData, const size_t ttfDataSize, bool monospaced)
    {
        *this = { };

        if (!ttfData || ttfDataSize == 0)
        {
            reportError("Invalid TTF data");
            return false;
        }

        this->monospaced = monospaced;

        textureSize.x = 2048;
        textureSize.y = 2048;
        packedCharBuffersSize = ('~' - ' ' + 1);

        // initialize stbtt_fontinfo to get font metrics.
        stbtt_fontinfo fontInfo;
        int fontOffset = stbtt_GetFontOffsetForIndex(ttfData, 0);
        if (fontOffset < 0 || !stbtt_InitFont(&fontInfo, ttfData, fontOffset))
        {
            reportError("Failed to initialize TTF font");
            return false;
        }

        int ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        const int FONT_PIXEL_SCALE = 64;

        float scale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_PIXEL_SCALE);
        lineHeight = (ascent - descent + lineGap) * scale;

        // STB TrueType will give us a one channel buffer of the font,
        // so we need to convert it to RGBA for OpenGL
        const size_t fontMonochromeBufferSize = textureSize.x * textureSize.y;
        const size_t fontRgbaBufferSize = textureSize.x * textureSize.y * 4;

        unsigned char* fontMonochromeBuffer = new unsigned char[fontMonochromeBufferSize] { };
        unsigned char* fontRgbaBuffer = new unsigned char[fontRgbaBufferSize] { };

        packedCharsBuffer = new stbtt_packedchar[packedCharBuffersSize] { };

        auto cleanupFontCreation = [&]()
        {
            delete[] fontMonochromeBuffer;
            delete[] fontRgbaBuffer;
            delete[] packedCharsBuffer;
        };

        stbtt_pack_context stbtt_context;
        if (!stbtt_PackBegin(&stbtt_context, fontMonochromeBuffer, textureSize.x, textureSize.y, 0, 2, nullptr))
        {
            reportError("Failed to call stbtt_PackBegin");
            cleanupFontCreation();
            *this = { };
            return false;
        }

        stbtt_PackSetOversampling(&stbtt_context, 2, 2);
        if (!stbtt_PackFontRange(&stbtt_context, ttfData, 0, FONT_PIXEL_SCALE, ' ', packedCharBuffersSize, packedCharsBuffer))
        {
            stbtt_PackEnd(&stbtt_context);
            reportError("Failed to call stbtt_PackFontRange");
            cleanupFontCreation();
            *this = { };
            return false;
        }

        stbtt_PackEnd(&stbtt_context);

        // convert monochrome to RGBA as noted above.
        for (int i = 0; i < fontMonochromeBufferSize; i++)
        {
            fontRgbaBuffer[( i * 4)] = 255;
            fontRgbaBuffer[( i * 4) + 1] = 255;
            fontRgbaBuffer[( i * 4 + 2)] = 255;
            fontRgbaBuffer[( i * 4) + 3] = fontMonochromeBuffer[i];
        }

        // initialize texture.
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureSize.x, textureSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, fontRgbaBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        maxLetterWidth = 0.0f;
        for (int i = 0; i < packedCharBuffersSize; i++)
        {
            if (packedCharsBuffer[i].xadvance > maxLetterWidth)
            {
                maxLetterWidth = packedCharsBuffer[i].xadvance;
            }
        }

        spaceWidth = packedCharsBuffer[' ' - ' '].xadvance;

        delete[] fontMonochromeBuffer;
        delete[] fontRgbaBuffer;

        return true;
    }

    void Font::createFromFile(const char* fileName, bool monospaced)
    {
        std::ifstream fileFont(fileName, std::ios::binary);

        if (!fileFont.is_open())
        {
            std::string s = "error opening: ";
            s += fileName;
            reportError(s.c_str());
            return;
        }

        int fileSize = 0;
        fileFont.seekg(0, std::ios::end);
        fileSize = (int)fileFont.tellg();
        fileFont.seekg(0, std::ios::beg);
        unsigned char* fileData = new unsigned char[fileSize];
        fileFont.read((char*)fileData, fileSize);
        fileFont.close();
    
        if (!createFromTTF(fileData, fileSize, monospaced))
        {
            std::string s = "error creating the TTF from: ";
            s += fileName;
            reportError(s.c_str());
        }

        delete[] fileData;
    }
    
    // for this character, where is it's rectangle in the front atlas? and how big should it be on screen?
    stbtt_aligned_quad fontGetGlyphQuad(Font font, const char c)
    {
        stbtt_aligned_quad quad = { 0 };
        float x = 0;
        float y = 0;

        stbtt_GetPackedQuad(font.packedCharsBuffer, font.textureSize.x, font.textureSize.y,
            c - ' ', &x, &y, &quad, 1);
        
        return quad;
    }

	TextLayout computeTextLayout(const char *text, Font font, float sizePixels, 
        float spacing, float lineHeightSpacing, bool writeLetters)
	{

		TextLayout layout;
		float size = sizePixels / 64.f;
		float penX = 0.0f;
		float lineY = 0.0f;
		float advanceSpaceSize = font.monospaced ? font.maxLetterWidth : font.spaceWidth;
		float scaledSpaceSize = advanceSpaceSize * size;
		float letterSpacing = (spacing - 1.0f) * scaledSpaceSize;

		if (!text || text[0] == 0)
		{
			return layout;
		}

		int text_length = (int)strlen(text);
		if (writeLetters)
		{
			layout.letters.reserve(text_length);
		}

		bool firstTimeWroteLetter = 1;
		auto writeLetter = [&](const TextGlyphLayout &glyph)
		{
			if (writeLetters)
			{
				layout.letters.push_back(glyph);
			}

			//calculate min max metrics
			if (firstTimeWroteLetter)
			{
				layout.min.x = glyph.rectangle.x;
				layout.min.y = glyph.rectangle.y;

				layout.max.x = glyph.rectangle.x + glyph.rectangle.z;
				layout.max.y = glyph.rectangle.y + glyph.rectangle.w;

				firstTimeWroteLetter = 0;
			}
			else
			{
				layout.min.x = std::min(layout.min.x, glyph.rectangle.x);
				layout.min.y = std::min(layout.min.y, glyph.rectangle.y);

				layout.max.x = std::max(layout.max.x, glyph.rectangle.x + glyph.rectangle.z);
				layout.max.y = std::max(layout.max.y, glyph.rectangle.y + glyph.rectangle.w);
			}

		};

		for (int i = 0; i < text_length; i++)
		{
			if (text[i] == '\n')
			{
				TextGlyphLayout glyph;
				glyph.rectangle = {penX, lineY, 0, 0};
				glyph.textureCoords = {};
				writeLetter(glyph);

				penX = 0.0f;
				lineY += (font.lineHeight) * size * lineHeightSpacing;
			}
			else if (text[i] == '\t')
			{
				float tabSize = (scaledSpaceSize + letterSpacing) * 3;

				TextGlyphLayout glyph;
				glyph.rectangle = {penX, lineY, tabSize, 0};
				glyph.textureCoords = {};
				writeLetter(glyph);

				penX += tabSize;
			}
			else if (text[i] == ' ')
			{
				TextGlyphLayout glyph;
				glyph.rectangle = {penX, lineY, scaledSpaceSize, 0};
				glyph.textureCoords = {};
				writeLetter(glyph);

				penX += scaledSpaceSize + letterSpacing;
			}
			else if (text[i] >= ' ' && text[i] <= '~')
			{
				stbtt_aligned_quad quad = fontGetGlyphQuad(font, text[i]);
				float xAdvance = font.monospaced ? font.maxLetterWidth : font.packedCharsBuffer[text[i] - ' '].xadvance;

				TextGlyphLayout glyph = {};
				glyph.rectangle.x = penX + quad.x0 * size;
				glyph.rectangle.y = lineY + quad.y0 * size;
				glyph.rectangle.z = (quad.x1 - quad.x0) * size;
				glyph.rectangle.w = (quad.y1 - quad.y0) * size;
				glyph.textureCoords = {quad.s0, quad.t1, quad.s1, quad.t0};

				writeLetter(glyph);

				penX += xAdvance * size + letterSpacing;
			}
			else
			{
				TextGlyphLayout glyph;
				glyph.rectangle = {penX, lineY, 0.0f, 0.0f};
				writeLetter(glyph);
			}
		}

		return layout;
	}

    glm::vec2 getTextSize(const char* text, Font font, const float sizePixels,
        const float spacing, const float line_space)
    {
        return computeTextLayout(text, font, sizePixels, spacing, line_space, false).getSize();
    }

    ////////////
    // CAMERA // 
    ////////////

    glm::mat4 Camera::getMatrix(float w, float h)
    {
        glm::vec2 center = {w / 2.0f, h / 2.0f};

        glm::mat4 matrix {1.0f};
        matrix = glm::translate(matrix, glm::vec3(center, 0.0f));
        matrix = glm::rotate(matrix, -rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::scale(matrix, glm::vec3(zoom, zoom, 1.0f));
        matrix = glm::translate(matrix, glm::vec3(-positionTopLeftCorner - center, 0.0f));

        return matrix;
    }

    void Camera::follow(glm::vec2 pos, float w, float h, float speed, float min, float max)
    {
        pos.x -= w / 2.0f;
        pos.y -= h / 2.0f;

        glm::vec2 delta = pos - positionTopLeftCorner;

        float len = glm::length(delta);

        if (len == 0)
        {
            return;
        }

        delta = glm::normalize(delta);

        if (len > min)
        {
            if (speed >= len)
            {
                positionTopLeftCorner = pos;
            }
            else if (len > max)
            {
                positionTopLeftCorner = pos - (max * delta);
            }
            else
            {
                positionTopLeftCorner += delta * speed;
            }
        }
    }

    glm::vec2 Camera::worldToScreen(glm::vec2 worldPos, float w, float h)
    {
        glm::vec4 screenPos = getMatrix(w, h) * glm::vec4(worldPos, 0.0f, 1.0f);
        return { screenPos.x, screenPos.y };
    }

    glm::vec2 Camera::screenToWorld(glm::vec2 screenPos, float w, float h)
    {
        glm::vec4 worldPos = glm::inverse(getMatrix(w, h)) * glm::vec4(screenPos, 0.0f, 1.0f);
        return { worldPos.x, worldPos.y };
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

        if (texture.id == 0)
        {
            texture = white1pxSquareTexture;
        }

        textureData.push_back(texture);
	}

    void Renderer2D::renderRect(const glm::vec4 &position, Texture texture, glm::vec4 colors,
        glm::vec4 textureCoords, float rotationRadians, glm::vec2 pivot)
    {
        if (windowH == 0 || windowW == 0)
        {
            return;
        }

        glm::mat4 projection = glm::ortho(0.0f, (float)windowW, (float)windowH, 0.0f, -1.0f, 1.0f);

        float x = position.x;
        float y = position.y;
        float width = position.z;
        float height = position.w;

        glm::vec2 center = {x + width / 2.0f, y + height / 2.0f};
        glm::vec2 pivotPosition = center + pivot;

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(pivotPosition, 0.0f));
        model = glm::rotate(model, rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-pivotPosition, 0.0f));

        glm::mat4 transform = projection * camera.getMatrix((float)windowW, (float)windowH) * model;

        glm::vec4 topLeft = transform * glm::vec4(x, y, 0.0f, 1.0f);
        glm::vec4 topRight = transform * glm::vec4(x + width, y, 0.0f, 1.0f);
        glm::vec4 bottomLeft = transform * glm::vec4(x, y + height, 0.0f, 1.0f);
        glm::vec4 bottomRight = transform * glm::vec4(x + width, y + height, 0.0f, 1.0f);

        glm::vec2 uvTopLeft = { textureCoords.x, textureCoords.w };
        glm::vec2 uvTopRight = { textureCoords.z, textureCoords.w };
        glm::vec2  uvBottomLeft = { textureCoords.x, textureCoords.y };
        glm::vec2 uvBottomRight = { textureCoords.z, textureCoords.y };

        glm::vec4 colorsVector[3] = { colors, colors, colors };
        glm::vec2 firstTriangleTextureCoords[3] = { uvTopLeft, uvTopRight, uvBottomRight };
        glm::vec2 secondTriangleTextureCoords[3] = { uvTopLeft, uvBottomRight, uvBottomLeft };

        renderTriangleFromNormalizedPositions(topLeft, topRight, bottomRight, texture, firstTriangleTextureCoords, colorsVector);
        renderTriangleFromNormalizedPositions(topLeft, bottomRight, bottomLeft, texture, secondTriangleTextureCoords, colorsVector);
    }

    void Renderer2D::clearDrawData()
    {
        renderTriangleData = { };
        textureData = { };
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

            // render in batches
            {
                const int size = textureData.size();
                int pos = 0;
                unsigned int id = textureData[0].id;

                textureData[0].bind();

                for (int i = 1; i < size; i++)
                {
                    if (textureData[i].id != id)
                    {
                        glDrawArrays(GL_TRIANGLES, pos * 3, 3 * (i - pos));

                        pos = i;
                        id = textureData[i].id;

                        textureData[i].bind();
                    }
                }

                glDrawArrays(GL_TRIANGLES, pos * 3, 3 * (size - pos));
            }

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

    TextLayout Renderer2D::renderText(glm::vec2 position, const char *text, Font font,
        float sizePixels, bool showInCenter, glm::vec4 color, const float spacing,
        const float lineHeightSpacing, glm::vec4 ShadowColor)
    {

        TextLayout textLayout = computeTextLayout(text, font, sizePixels, spacing, lineHeightSpacing, true);

        float size = sizePixels / 64.f;
        glm::vec2 drawOffset = position;

        if (showInCenter)
        {
            glm::vec2 textSize = textLayout.getSize();
            drawOffset.x -= textSize.x / 2.0f;
            drawOffset.y -= (textLayout.min.y + textLayout.max.y) / 2.0f;
        }

        textLayout.min += drawOffset;
        textLayout.max += drawOffset;

        for (TextGlyphLayout &glyph : textLayout.letters)
        {
            glyph.rectangle.x += drawOffset.x;
            glyph.rectangle.y += drawOffset.y;

            //display debug letter rect
            //renderRect(glyph.rectangle, {}, {0.9,0.9,0.2,0.5});

            if (glyph.rectangle.z <= 0 || glyph.rectangle.w <= 0 ||
                (glyph.textureCoords.x == 0 && glyph.textureCoords.y == 0 &&
                glyph.textureCoords.z == 0 && glyph.textureCoords.w == 0))
            {
                continue;
            }

            if (ShadowColor.w > 0)
            {
                glm::vec2 pos = {-5, 3};
                pos *= size;
                renderRect({glyph.rectangle.x + pos.x, glyph.rectangle.y + pos.y,  glyph.rectangle.z, glyph.rectangle.w},
                    font.texture, ShadowColor, glyph.textureCoords);

            }

            renderRect(glyph.rectangle, font.texture, color, glyph.textureCoords);
        }

        return textLayout;
    }
    
    //////////////////////////
    // TEXTURE ATLAS HELPER // 
    //////////////////////////

    glm::vec4 computeTextureAtlas(int xCount, int yCount, int x, int y, bool flipHorizontal)
    {
        // calculate size of each texture in atlas
        float xSize = 1.0f / xCount;
        float ySize = 1.0f / yCount;

        if (flipHorizontal)
        {
            return { (x + 1) * xSize, 1 - (y + 1) * ySize, x * xSize, 1 - y * ySize };
        }   
        else
        {
            return { x * xSize, 1 - (y + 1) * ySize, (x + 1) * xSize, 1 - y * ySize };
        }
    }

    glm::vec4 computeTextureAtlasWithPadding(int mapXSize, int mapYSize, int xCount, int yCount,
        int x, int y, bool flipHorizontal)
    {
        float xSize = 1.0f / xCount;
        float ySize = 1.0f / yCount;

        float xPadding = 1.0f / mapXSize;
        float yPadding = 1.0f / mapYSize;

        glm::vec4 noFlip = {x * xSize + xPadding, 1.0f - ((y + 1) * ySize) + yPadding,
            (x + 1) * xSize - xPadding, 1 - (y * ySize) - yPadding};

        if (flipHorizontal)
        {
            glm::vec4 flip = {noFlip.z, noFlip.y, noFlip.x, noFlip.w};
            return flip;
        }
        else
        {
            return noFlip;
        }
    }
};