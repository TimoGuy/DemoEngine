#pragma once

#include <assimp/scene.h>
#include <vector>
#include <string>

#include "Mesh.h"


class Model
{
public:
	Model(const char* path) { loadModel(path); }
	void render(unsigned int shaderId);

private:
	std::vector<Texture> loadedTextures;
	std::vector<Mesh> meshes;
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
};

