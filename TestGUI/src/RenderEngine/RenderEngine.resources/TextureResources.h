#pragma once

#include <string>
#include <map>


class TextureResources
{
public:
	static TextureResources& getInstance()
	{
		static TextureResources instance;
		return instance;
	}

	unsigned int loadTexture2D(
		const std::string& textureName,
		const std::string& fname,
		unsigned int fromTexture,
		unsigned int toTexture,
		unsigned int minFilter,
		unsigned int magFilter,
		unsigned int wrapS,
		unsigned int wrapT);

	unsigned int loadHDRTexture2D(
		const std::string& textureName,
		const std::string& fname,
		unsigned int fromTexture,
		unsigned int toTexture,
		unsigned int minFilter,
		unsigned int magFilter,
		unsigned int wrapS,
		unsigned int wrapT);

	bool eraseTexture(const std::string& name);

private:
	//
	// Singleton pattern
	//
	TextureResources() {}
	//TextureResources(TextureResources const&);		// NOTE: do not implement!
	//void operator=(TextureResources const&);		// NOTE: do not implement!

	std::map<std::string, unsigned int> textureMap;
	unsigned int findTexture(const std::string& name);
	void registerTexture(const std::string& name, unsigned int textureId);

public:
	/*TextureResources(TextureResources const&) = delete;
	void operator=(TextureResources const&) = delete;*/
	// Note: Scott Meyers mentions in his Effective Modern
	//       C++ book, that deleted functions should generally
	//       be public as it results in better error messages
	//       due to the compilers behavior to check accessibility
	//       before deleted status
};
