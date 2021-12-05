#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>

#include "animation/Animation.h"
#include "../../utils/PhysicsUtils.h"
#include "../camera/Camera.h"


Model::Model() { scene = nullptr; }

Model::Model(const char* path)
{
	loadModel(path, {});
}


Model::Model(const char* path, std::vector<std::string> animationNames)
{
	loadModel(path, animationNames);
}

bool Model::getIfInViewFrustum(const glm::mat4& modelMatrix, const ViewFrustum* viewFrustum, std::vector<bool>& out_whichMeshesInView)
{
	bool modelIsChottoDakeDemoInViewFrustum = false;

	out_whichMeshesInView.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		const bool withinViewFrustum = viewFrustum->checkIfInViewFrustum(meshes[i].bounds, modelMatrix * localTransform);
		out_whichMeshesInView.push_back(withinViewFrustum);
		if (withinViewFrustum)
			modelIsChottoDakeDemoInViewFrustum = true;
	}

	return modelIsChottoDakeDemoInViewFrustum;
}


void Model::render(const glm::mat4& modelMatrix, GLuint shaderIdOverride, const std::vector<bool>* whichMeshesInView)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		if (whichMeshesInView == nullptr || (*whichMeshesInView)[i])
			meshes[i].render(modelMatrix * localTransform, shaderIdOverride, false);			// NOTE: inside this render function, transparent meshes are extracted and then placed into a queue at the rendermanager level. Then, they're rendered after all opaque materials have fimished!
	}
}

#ifdef _DEBUG
void Model::TEMPrenderImguiModelBounds(glm::mat4 trans)
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		RenderAABB cookedBounds =
			PhysicsUtils::fitAABB(
				meshes[i].bounds,
				trans * localTransform
			);

		physx::PxBoxGeometry boxGeometry(cookedBounds.extents.x, cookedBounds.extents.y, cookedBounds.extents.z);
		PhysicsUtils::imguiRenderBoxCollider(
			glm::translate(glm::mat4(1.0f), cookedBounds.center),
			boxGeometry,
			ImColor(0.9607843137f, 0.8666666667f, 0.1529411765f)
		);
	}
}
#endif

void Model::setMaterials(std::map<std::string, Material*> materialMap)
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		meshes[i].pickFromMaterialList(materialMap);
	}
}


void Model::loadModel(std::string path, std::vector<std::string> animationNames)
{
	std::cout << "MODEL::" << path << "::Import Started" << std::endl;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights);

	Model::scene = scene;

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);		// This starts the recursive process of loading in the model as vertices!

	//
	// Load in animations
	//
	for (unsigned int i = 0; i < animationNames.size(); i++)
	{
		Animation newAnimation(scene, this, animationNames[i]);
		animations.push_back(newAnimation);
	}

	std::cout << "MODEL::" << path << "::Import Fimished" << std::endl;
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

	glm::vec3* minAABBPoint = nullptr;
	glm::vec3* maxAABBPoint = nullptr;

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

		// Mesh AABB
		if (minAABBPoint == nullptr)
			minAABBPoint = new glm::vec3(vector);
		else
		{
			minAABBPoint->x = std::min(minAABBPoint->x, vector.x);
			minAABBPoint->y = std::min(minAABBPoint->y, vector.y);
			minAABBPoint->z = std::min(minAABBPoint->z, vector.z);
		}

		if (maxAABBPoint == nullptr)
			maxAABBPoint = new glm::vec3(vector);
		else
		{
			maxAABBPoint->x = std::max(maxAABBPoint->x, vector.x);
			maxAABBPoint->y = std::max(maxAABBPoint->y, vector.y);
			maxAABBPoint->z = std::max(maxAABBPoint->z, vector.z);
		}

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

	// Convert aiAABB to RenderAABB
	RenderAABB modelRenderAABB;
	if (minAABBPoint != nullptr && maxAABBPoint != nullptr)
	{
		modelRenderAABB.center = (*minAABBPoint + *maxAABBPoint) / 2.0f;
		modelRenderAABB.extents = *maxAABBPoint - modelRenderAABB.center;

		delete minAABBPoint;
		delete maxAABBPoint;
	}

	return Mesh(vertices, indices, modelRenderAABB, materialName);
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