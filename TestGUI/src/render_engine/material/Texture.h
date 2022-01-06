#pragma once

#include <vector>
#include <string>
#include <future>


typedef unsigned int GLuint;
typedef unsigned int GLenum;

struct ImageFile
{
	const std::string fname;
	bool flipVertical;
	bool isHDR;
	bool generateMipmaps;
};


struct ImageDataLoaded
{
	int imgWidth, imgHeight;
	GLenum internalFormat, pixelType;
	void* pixels;
	int optionalId;
};


class Texture
{
public:
	Texture();
	virtual ~Texture();

	inline GLuint getHandle() { return textureHandle; }

	static void INTERNALtriggerCreateGraphicsAPITextureHandles();
	static void INTERNALaddTextureToLoadSynchronously(Texture* tex, const ImageDataLoaded& idl);

protected:
	bool loaded;
	GLuint textureHandle;

	static std::mutex textureMutex;
	static std::vector<std::future<void>> asyncFutures;
	virtual void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data) = 0;

private:
	static std::vector<Texture*> syncTexturesToLoad;
	static std::vector<ImageDataLoaded> syncTexturesToLoadParams;
};


class Texture2DFromFile : public Texture
{
public:
	Texture2DFromFile(const ImageFile file, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT);

private:
	void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data);

	const ImageFile file;
	GLuint toTexture, minFilter, magFilter, wrapS, wrapT;
};


class TextureCubemapFromFile : public Texture
{
public:
	TextureCubemapFromFile(const std::vector<const ImageFile> files, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT, GLuint wrapR);

private:
	void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data);

	const std::vector<const ImageFile> files;
	std::vector<ImageDataLoaded> imageDatasCache;
	int numImagesLoaded;
	GLuint toTexture, minFilter, magFilter, wrapS, wrapT, wrapR;
};
