#include "Texture.h"

#include <cassert>		// IDK why I have to have this here right now.... weird.
#include <iostream>
#include <mutex>
#include <glad/glad.h>
#include <stb/stb_image.h>


std::vector<std::future<void>>	Texture::asyncFutures;
std::mutex						Texture::textureMutex;
std::vector<Texture*>			Texture::syncTexturesToLoad;
std::vector<ImageDataLoaded>	Texture::syncTexturesToLoadParams;


#pragma region Helper Functions

namespace INTERNALTextureHelper
{
	void loadTexture2DAsync(Texture* you, const ImageFile& file)
	{
		ImageDataLoaded idl;

		stbi_set_flip_vertically_on_load(file.flipVertical);
		int imgWidth, imgHeight, numColorChannels;
		if (file.isHDR)
		{
			float* pixels = stbi_loadf(file.fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

			if (pixels == nullptr)
			{
				std::cout << "Error on loading \"" << file.fname << "\"" << std::endl;
				std::cout << stbi_failure_reason() << std::endl;
				assert(0);
			}

			idl.pixelType = GL_FLOAT;
			idl.pixels = pixels;
		}
		else
		{
			unsigned char* bytes = stbi_load(file.fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

			if (bytes == nullptr)
			{
				std::cout << "Error on loading \"" << file.fname << "\"" << std::endl;
				std::cout << stbi_failure_reason() << std::endl;
				assert(0);
			}

			idl.pixelType = GL_UNSIGNED_BYTE;
			idl.pixels = bytes;
		}

		GLenum internalFormat = GL_RED;
		if (numColorChannels == 2)
			internalFormat = GL_RG;
		else if (numColorChannels == 3)
			internalFormat = GL_RGB;
		else if (numColorChannels == 4)
			internalFormat = GL_RGBA;

		idl.imgWidth = imgWidth;
		idl.imgHeight = imgHeight;
		idl.internalFormat = internalFormat;

		Texture::INTERNALaddTextureToLoadSynchronously(you, idl);
	}
}

#pragma endregion


Texture::Texture() : loaded(false), textureHandle(0) {}

Texture::~Texture()
{
	glDeleteTextures(1, &textureHandle);
}

void Texture::INTERNALtriggerCreateGraphicsAPITextureHandles()
{
	std::lock_guard<std::mutex> resLock(textureMutex);		// Pause loading in the textures into the syncTexturesToLoad queue

	for (size_t i = 0; i < syncTexturesToLoad.size(); i++)
	{
		syncTexturesToLoad[i]->INTERNALgenerateGraphicsAPITextureHandleSync(syncTexturesToLoadParams[i]);
	}
	syncTexturesToLoad.clear();
	syncTexturesToLoadParams.clear();
}

void Texture::INTERNALaddTextureToLoadSynchronously(Texture* tex, const ImageDataLoaded& idl)
{
	std::lock_guard<std::mutex> resLock(Texture::textureMutex);		// To make sure another thread doesn't insert at the same time as this!

	syncTexturesToLoad.push_back(tex);
	syncTexturesToLoadParams.push_back(idl);
}

Texture2DFromFile::Texture2DFromFile(const ImageFile file, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT) : file(file), toTexture(toTexture), minFilter(minFilter), magFilter(magFilter), wrapS(wrapS), wrapT(wrapT)
{
	asyncFutures.push_back(std::async(std::launch::async, INTERNALTextureHelper::loadTexture2DAsync, this, file));
}

void Texture2DFromFile::INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data)
{
	glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, wrapS);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, wrapT);
	glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, magFilter);

	glTextureStorage2D(textureHandle, 1, data.internalFormat, data.imgWidth, data.imgHeight);
	glTextureSubImage2D(textureHandle, 0, 0, 0, data.imgWidth, data.imgHeight, toTexture, data.pixelType, data.pixels);

	if (file.generateMipmaps)
		glGenerateTextureMipmap(textureHandle);

	stbi_image_free(data.pixels);

	loaded = true;
}
