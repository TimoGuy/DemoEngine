#include "TextureResources.h"

#include <iostream>
#include <stb/stb_image.h>
#include <glad/glad.h>


unsigned int TextureResources::loadTexture2D(
	const std::string& textureName,
	const std::string& fname,
	unsigned int fromTexture = GL_RGB,
	unsigned int toTexture = GL_RGB,
	unsigned int minFilter = GL_NEAREST,
	unsigned int magFilter = GL_NEAREST,
	unsigned int wrapS = GL_REPEAT,
	unsigned int wrapT = GL_REPEAT)
{
	unsigned int textureId = TextureResources::getInstance().findTexture(textureName);
	if (!textureId)
	{
		stbi_set_flip_vertically_on_load(true);
		int imgWidth, imgHeight, numColorChannels;
		unsigned char* bytes = stbi_load(fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

		if (bytes == NULL)
		{
			std::cout << stbi_failure_reason() << std::endl;
		}

		glGenTextures(1, &textureId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

		glTexImage2D(GL_TEXTURE_2D, 0, fromTexture, imgWidth, imgHeight, 0, toTexture, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(bytes);
		glBindTexture(GL_TEXTURE_2D, 0);

		registerTexture(textureName, textureId);
	}

	return textureId;
}

unsigned int TextureResources::loadHDRTexture2D(
	const std::string& textureName,
	const std::string& fname,
	unsigned int fromTexture = GL_RGB16F,
	unsigned int toTexture = GL_RGB,
	unsigned int minFilter = GL_NEAREST,
	unsigned int magFilter = GL_NEAREST,
	unsigned int wrapS = GL_REPEAT,
	unsigned int wrapT = GL_REPEAT)
{
	unsigned int textureId = TextureResources::getInstance().findTexture(textureName);
	if (!textureId)
	{
		stbi_set_flip_vertically_on_load(true);
		int imgWidth, imgHeight, numColorChannels;
		float* bytes = stbi_loadf(fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

		if (bytes == NULL)
		{
			std::cout << stbi_failure_reason() << std::endl;
		}

		glGenTextures(1, &textureId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

		glTexImage2D(GL_TEXTURE_2D, 0, fromTexture, imgWidth, imgHeight, 0, toTexture, GL_FLOAT, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(bytes);
		glBindTexture(GL_TEXTURE_2D, 0);

		registerTexture(textureName, textureId);
	}

	return textureId;
}

bool TextureResources::eraseTexture(const std::string& name)
{
	return false;
}

unsigned int TextureResources::findTexture(const std::string& name)
{
	return 0;
}

void TextureResources::registerTexture(const std::string& name, unsigned int textureId)
{}
