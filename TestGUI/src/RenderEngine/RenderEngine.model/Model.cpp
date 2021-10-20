#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>

#include <glad/glad.h>
#include <stb/stb_image.h>

#include "RenderEngine.model.animation/Animation.h"


Model::Model() { scene = nullptr; }

Model::Model(const char* path)
{
	loadModel(path, {});
}


Model::Model(const char* path, std::vector<int> animationIndices)
{
	loadModel(path, animationIndices);
}

void Model::render(const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneMatrices)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].render(modelMatrix, boneMatrices);
	}
}

void Model::renderShadow(const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneMatrices)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].render(modelMatrix, boneMatrices);
	}
}

void Model::setMaterials(std::map<std::string, Material*> materialMap)
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		meshes[i].pickFromMaterialList(materialMap);
	}
}


void Model::loadModel(std::string path, std::vector<int> animationIndices)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights);

	Model::scene = scene;

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}
	std::cout << "Nope everything's all good" << std::endl;

	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);		// This starts the recursive process of loading in the model as vertices!

	//
	// Load in animations
	//
	for (unsigned int i = 0; i < animationIndices.size(); i++)
	{
		int animationIndex = animationIndices[i];
		Animation newAnimation(scene, this, animationIndex);
		animations.push_back(newAnimation);
	}
}


void Model::processNode(aiNode* node, const aiScene* scene)
{
	// process all of the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	// Recursive part: continue going down the children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}


Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		setVertexBoneDataToDefault(vertex);

		//
		// Process vertex positions, normals, and tex coords
		//
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;

		// normal
		glm::vec3 normal;
		normal.x = mesh->mNormals[i].x;
		normal.y = mesh->mNormals[i].y;
		normal.z = mesh->mNormals[i].z;
		vertex.normal = normal;

		// texcoord
		if (mesh->mTextureCoords[0])		// NOTE: sometimes meshes can have more than one texcoords, since there can be up to 8 per vertex!!! Dang!
		{
			glm::vec2 st;
			st.x = mesh->mTextureCoords[0][i].x;
			st.y = mesh->mTextureCoords[0][i].y;
			vertex.texCoords = st;
		}
		else
			vertex.texCoords = glm::vec2(0.0f, 0.0f);

		// Add to array
		vertices.push_back(vertex);
	}

	//
	// Process indices
	//
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	//
	// Process material
	//
	std::string materialName;
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiString name = material->GetName();
		materialName = std::string(name.C_Str());
	}

	// Process Bones
	extractBoneWeightForVertices(vertices, mesh, scene);

	return Mesh(vertices, indices, materialName);
}


unsigned int textureFromFile(const char* path, const std::string& directory);
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName)
{
	size_t numTextures = material->GetTextureCount(type);

	std::vector<Texture> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString str;
		material->GetTexture(type, i, &str);

		// See if texture is already loaded
		bool skip = false;
		for (unsigned int j = 0; j < loadedTextures.size(); j++)
		{
			if (std::strcmp(loadedTextures[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(loadedTextures[j]);
				skip = true;
				break;
			}
		}

		// Load if not found
		if (!skip)
		{
			Texture texture;
			texture.id = textureFromFile(str.C_Str(), directory);
			texture.type = typeName;
			texture.path = str.C_Str();

			textures.push_back(texture);
			loadedTextures.push_back(texture);
		}
	}

	return textures;
}


unsigned int textureFromFile(const char* path, const std::string& directory)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RED;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}


void Model::setVertexBoneDataToDefault(Vertex& vertex)
{
	for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		vertex.boneIds[i] = -1;
		vertex.boneWeights[i] = 0.0f;
	}
}


void Model::addVertexBoneData(Vertex& vertex, int boneId, float boneWeight)
{
	static int highestNumWeights = -1;
	vertex.numWeights++;
	if (vertex.numWeights > highestNumWeights)
	{
		highestNumWeights = vertex.numWeights;
		std::cout << "\t\tHIGHEST NUM WEIGHTS:::" << highestNumWeights << std::endl;
	}

	//boneId = 3;
	int smallestBoneWeightIndex = -1;
	for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		if (vertex.boneIds[i] < 0)
		{
			// Add bone weight to this empty slot for a bone weight
			vertex.boneIds[i] = boneId;
			vertex.boneWeights[i] = boneWeight;
			return;
		}
		//else
		//{
		//	// Find the smallest bone weight in case all slots are filled
		//	if (smallestBoneWeightIndex < 0 ||
		//		vertex.boneWeights[i] < vertex.boneWeights[smallestBoneWeightIndex])
		//	{
		//		smallestBoneWeightIndex = i;
		//	}
		//}
	}

	//
	// Try to override lowest bone weight
	//
	////std::cout << "Max Bone weight exceeded; overriding..." << std::endl;
	//if (vertex.boneWeights[smallestBoneWeightIndex] < boneWeight)
	//{
	//	// Use heavier bone!
	//	vertex.boneIds[smallestBoneWeightIndex] = boneId;
	//	vertex.boneWeights[smallestBoneWeightIndex] = boneWeight;
	//	//std::cout << "\tSuccess" << std::endl;
	//	return;
	//}

	////std::cout << "\tFailed" << std::endl;
}


void Model::extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
	for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
	{
		int boneId = -1;
		std::string boneName(mesh->mBones[boneIndex]->mName.data);
		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			// Create new bone info and add to map
			BoneInfo newBoneInfo;
			newBoneInfo.id = boneCounter;
			{
				glm::mat4 to;
				aiMatrix4x4 from = mesh->mBones[boneIndex]->mOffsetMatrix;
				//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
				to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
				to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
				to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
				to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
				newBoneInfo.offset = to;
			}
			boneInfoMap[boneName] = newBoneInfo;
			std::cout << "Created new bone: " << boneName << "\tID: " << boneCounter << std::endl;
			boneCounter++;

			boneId = newBoneInfo.id;
		}
		else
		{
			boneId = boneInfoMap[boneName].id;
		}

		assert(boneId != -1);
		auto weights = mesh->mBones[boneIndex]->mWeights;
		unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

		for (unsigned int weightIndex = 0; weightIndex < numWeights; weightIndex++)	// Hi
		{
			int vertexId = weights[weightIndex].mVertexId;
			float weight = weights[weightIndex].mWeight;
			assert(vertexId <= vertices.size());
			addVertexBoneData(vertices[vertexId], boneId, weight);
		}
	}
}
