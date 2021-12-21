#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


typedef unsigned int GLuint;

class VoxelGroup : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	VoxelGroup();
	~VoxelGroup();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	void preRenderUpdate();
	void physicsUpdate();

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void INTERNALrecreatePhysicsComponent();

#ifdef _DEBUG
	void imguiPropertyPanel();
	void imguiRender();
#endif

	// TODO: this should be private, with all the components just referring back to the single main one
	physx::PxVec3 velocity, angularVelocity;

	// TODO: This needs to stop (having a bad loading function) bc this function should be private, but it's not. Kuso.
	void refreshResources();

private:
	//
	// Some voxel props
	//
	const float voxel_render_size = 2.0f;
	size_t chunk_increments = 32;
	glm::u64vec3 voxel_group_size;
	glm::u64vec3 voxel_group_offset;
	std::vector<std::vector<std::vector<bool>>> voxel_bit_field;
	bool is_voxel_bit_field_dirty = false;

	void resizeVoxelArea(size_t x, size_t y, size_t z, glm::u64vec3 offset = glm::u64vec3(0));
	void setVoxelBitAtPosition(glm::u64vec3 pos, bool flag);
	void updateQuadMeshFromBitField();
	Model* voxel_model;
	std::map<std::string, Material*> materials;
};

