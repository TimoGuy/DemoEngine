#include "Texture.h"

#include <cassert>		// IDK why I have to have this here right now.... weird.
#include <iostream>
#include <mutex>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stb/stb_image.h>


bool							Texture::loadSync = false;
std::vector<std::future<void>>	Texture::asyncFutures;
std::mutex						Texture::textureMutex;
std::vector<Texture*>			Texture::syncTexturesToLoad;
std::vector<ImageDataLoaded>	Texture::syncTexturesToLoadParams;


#pragma region Helper Functions

namespace INTERNALTextureHelper
{
	void loadTexture2DAsync(Texture* you, const ImageFile& file, int optionalId = -1)
	{
		ImageDataLoaded idl;

		stbi_set_flip_vertically_on_load_thread(file.flipVertical);		// Thank you nothings!!! (Author of stb_image for making a thread-safe version of this function. NOT SARCASM)
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

			idl.internalFormat = GL_R16F;
			if (numColorChannels == 2)
				idl.internalFormat = GL_RG16F;
			else if (numColorChannels == 3)
				idl.internalFormat = GL_RGB16F;
			else if (numColorChannels == 4)
				idl.internalFormat = GL_RGBA16F;
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

			idl.internalFormat = GL_R8;
			if (numColorChannels == 2)
				idl.internalFormat = GL_RG8;
			else if (numColorChannels == 3)
				idl.internalFormat = GL_RGB8;
			else if (numColorChannels == 4)
				idl.internalFormat = GL_RGBA8;
		}

		idl.imgWidth = imgWidth;
		idl.imgHeight = imgHeight;
		idl.optionalId = optionalId;

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

Texture2DFromFile::Texture2DFromFile(
	const ImageFile& file,
	GLenum toTexture,
	GLuint minFilter,
	GLuint magFilter,
	GLuint wrapS,
	GLuint wrapT) :
		file(file),
		toTexture(toTexture),
		minFilter(minFilter),
		magFilter(magFilter),
		wrapS(wrapS),
		wrapT(wrapT)
{
	asyncFutures.push_back(std::async(std::launch::async, INTERNALTextureHelper::loadTexture2DAsync, this, file, -1));

	// Synchronous loading spinning lock
	while (Texture::loadSync && !loaded)
		Texture::INTERNALtriggerCreateGraphicsAPITextureHandles();
}

void Texture2DFromFile::INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data)
{
	glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, wrapS);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, wrapT);
	glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, magFilter);

	glTextureStorage2D(textureHandle, file.generateMipmaps ? glm::floor(glm::log2((float_t)glm::max(data.imgWidth, data.imgHeight))) + 1 : 1, data.internalFormat, data.imgWidth, data.imgHeight);
	glTextureSubImage2D(textureHandle, 0, 0, 0, data.imgWidth, data.imgHeight, toTexture, data.pixelType, data.pixels);

	if (file.generateMipmaps)
		glGenerateTextureMipmap(textureHandle);

	stbi_image_free(data.pixels);

	loaded = true;
}

Texture2D::Texture2D(GLsizei width, GLsizei height, GLsizei levels, GLenum internalFormat, GLenum format, GLenum pixelType, const void* pixels, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT)
{
	glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, wrapS);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, wrapT);
	glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, magFilter);

	glTextureStorage2D(textureHandle, levels, internalFormat, width, height);

	if (pixels != nullptr)
		glTextureSubImage2D(textureHandle, 0, 0, 0, width, height, format, pixelType, pixels);

	loaded = true;
}

Texture3D::Texture3D(GLsizei width, GLsizei height, GLsizei depth, GLsizei levels, GLenum internalFormat, GLenum format, GLenum pixelType, const void* pixels, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT, GLuint wrapR)
{
	glCreateTextures(GL_TEXTURE_3D, 1, &textureHandle);

	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, wrapS);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, wrapT);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_R, wrapR);
	glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, magFilter);

	glTextureStorage3D(textureHandle, levels, internalFormat, width, height, depth);

	if (pixels != nullptr)
		glTextureSubImage3D(textureHandle, 0, 0, 0, 0, width, height, depth, format, pixelType, pixels);

	loaded = true;
}

TextureCubemapFromFile::TextureCubemapFromFile(
	const std::vector<ImageFile>& files,
	GLenum toTexture,
	GLuint minFilter,
	GLuint magFilter,
	GLuint wrapS,
	GLuint wrapT,
	GLuint wrapR) :
		files(files),
		toTexture(toTexture),
		minFilter(minFilter),
		magFilter(magFilter),
		wrapS(wrapS),
		wrapT(wrapT),
		wrapR(wrapR)
{
	assert(files.size() == 6);

	imageDatasCache.resize(6);
	numImagesLoaded = 0;

	for (size_t i = 0; i < 6; i++)
	{
		asyncFutures.push_back(std::async(std::launch::async, INTERNALTextureHelper::loadTexture2DAsync, this, files[i], i));
	}

	// Synchronous loading spinning lock
	while (Texture::loadSync && !loaded)
		Texture::INTERNALtriggerCreateGraphicsAPITextureHandles();
}

void TextureCubemapFromFile::INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data)
{
	if (loaded)		// NOTE: just in case....
		return;

	imageDatasCache[data.optionalId] = data;
	numImagesLoaded++;

	if (numImagesLoaded < 6)
		return;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureHandle);

	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, wrapS);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, wrapT);
	glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_R, wrapR);
	glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, magFilter);

	glTextureStorage2D(textureHandle, 1, data.internalFormat, data.imgWidth, data.imgHeight);

	for (size_t i = 0; i < 6; i++)
	{
		ImageDataLoaded& imgData = imageDatasCache[i];
		glTextureSubImage3D(textureHandle, 0, 0, 0, i, imgData.imgWidth, imgData.imgHeight, 1, toTexture, imgData.pixelType, imgData.pixels);
		stbi_image_free(imgData.pixels);
	}

	loaded = true;
}
