#pragma once

#include <vector>
#include <string>
#include <future>

typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLsizei;


struct ImageFile
{
	std::string fname;
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
	inline static void setLoadSync(bool flag) { loadSync = flag; }

protected:
	static bool loadSync;
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
	Texture2DFromFile(const ImageFile& file, GLenum toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT);

private:
	void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data);

	const ImageFile file;
	GLenum toTexture;
	GLuint minFilter, magFilter, wrapS, wrapT;
};


class Texture2D : public Texture
{
public:
	Texture2D(GLsizei width, GLsizei height, GLsizei levels, GLenum internalFormat, GLenum format, GLenum pixelType, const void* pixels, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT);

private:
	void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data) { }
};


class TextureCubemapFromFile : public Texture
{
public:
	TextureCubemapFromFile(const std::vector<ImageFile>& files, GLenum toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT, GLuint wrapR);

private:
	void INTERNALgenerateGraphicsAPITextureHandleSync(ImageDataLoaded& data);

	const std::vector<ImageFile> files;
	std::vector<ImageDataLoaded> imageDatasCache;
	int numImagesLoaded;
	GLenum toTexture;
	GLuint minFilter, magFilter, wrapS, wrapT, wrapR;
};
