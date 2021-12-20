#include "VoxelGroup.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "components/PhysicsComponents.h"
#include "../mainloop/MainLoop.h"
#include "../render_engine/resources/Resources.h"
#include "../utils/PhysicsUtils.h"

#ifdef _DEBUG
#include "../render_engine/render_manager/RenderManager.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#endif


VoxelGroup::VoxelGroup()
{
	name = "Voxel Group (unnamed)";

	// @TODO: small rant: I need some way of not having to create the voxels and then right after load in the voxel data from loadPropertiesFromJson() !!! It's dumb to do it twice.
	resizeVoxelArea(chunk_increments, chunk_increments, chunk_increments);				// For now default. (Needs loading system to do different branch)
	setVoxelBitAtPosition(voxel_group_offset, true);									// Default: center cube is enabled
	setVoxelBitAtPosition(voxel_group_offset + glm::u64vec3(1, 0, 0), true);
	setVoxelBitAtPosition(voxel_group_offset + glm::u64vec3(1, 1, 0), true);
	setVoxelBitAtPosition(voxel_group_offset + glm::u64vec3(1, 1, 1), true);

	renderComponent = new RenderComponent(this);
	refreshResources();
	//INTERNALrecreatePhysicsComponent();
}

VoxelGroup::~VoxelGroup()
{
	delete renderComponent;
	delete physicsComponent;
}

void VoxelGroup::refreshResources()
{
	if (is_voxel_bit_field_dirty)
	{
		// Create quadmesh
		updateQuadMeshFromBitField();

		//
		// Create cooked collision mesh
		//
		INTERNALrecreatePhysicsComponent();
		renderComponent->clearAllModels();
		renderComponent->addModelToRender({ voxel_model, true, nullptr });

		is_voxel_bit_field_dirty = false;
	}

	materials["Material"] = (Material*)Resources::getResource("material;pbrVoxelGroup");
	voxel_model->setMaterials(materials);
}

void VoxelGroup::resizeVoxelArea(size_t x, size_t y, size_t z, glm::u64vec3 offset)
{
	//
	// Create new voxel_bit_field
	//
	std::vector<bool> zVoxels;
	zVoxels.resize(z, false);
	std::vector<std::vector<bool>> yzVoxels;
	yzVoxels.resize(y, zVoxels);
	std::vector<std::vector<std::vector<bool>>> xyzVoxels;
	xyzVoxels.resize(y, yzVoxels);

	// Fill it!
	if (voxel_bit_field.size() == 0)
	{
		// From scratch
		voxel_group_offset = voxel_group_size / (glm::u64)2;
	}
	else
	{
		// With existing
		voxel_group_offset += offset;

		// 
		// (NOTE: this is an unsafe operation and if offset is incorrectly set, there could be some bad joojoo with out-of-bounds stuff)
		// I trust ya tho ;)
		//
		for (size_t i = 0; i < voxel_bit_field.size(); i++)
		{
			size_t offsetX = offset.x;

			for (size_t j = 0; j < voxel_bit_field[i].size(); j++)
			{
				size_t offsetY = offset.y;

				for (size_t k = 0; k < voxel_bit_field[i][j].size(); k++)
				{
					size_t offsetZ = offset.z;

					xyzVoxels[offsetX + i][offsetY + j][offsetZ + k] = voxel_bit_field[i][j][k];
				}
			}
		}
	}

	// Apply the new voxels
	voxel_bit_field = xyzVoxels;

	// Update size
	voxel_group_size = { x, y, z };
	is_voxel_bit_field_dirty = true;
}

void VoxelGroup::setVoxelBitAtPosition(glm::u64vec3 pos, bool flag)
{
	voxel_bit_field[pos.x][pos.y][pos.z] = true;
	is_voxel_bit_field_dirty = true;
}

void VoxelGroup::updateQuadMeshFromBitField()
{
	std::vector<Vertex> quadMesh;
	Vertex vertBuilder;			// Let's call him "Bob" (https://pm1.narvii.com/6734/14e3582d0aa8180bd12d22772631e949ad4e6837v2_hq.jpg)
	const float voxel_size = 2.0f;

	//
	// Cycle thru each bit in the bitfield and see if meshes need to be rendered there
	//
	for (size_t i = 0; i < voxel_bit_field.size(); i++)
	{
		for (size_t j = 0; j < voxel_bit_field[i].size(); j++)
		{
			for (size_t k = 0; k < voxel_bit_field[i][j].size(); k++)
			{
				// Skip if empty
				if (!voxel_bit_field[i][j][k])
					continue;

				// Compute the origin position in the case that quads are added
				// NOTE: the origin is going to be the bottom-left-front
				glm::vec3 voxelOriginPosition = { i, j, k };
				voxelOriginPosition -= voxel_group_offset;
				voxelOriginPosition *= voxel_size;

				//
				// See if on the edge!
				// 
				// x-1 (normal: left)
				if (i == 0 || !voxel_bit_field[i - 1][j][k])
				{
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// x+1 (normal: right)
				if (i == voxel_bit_field.size() - 1 || !voxel_bit_field[i + 1][j][k])
				{
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// y-1 (normal: down)
				if (j == 0 || !voxel_bit_field[i][j - 1][k])
				{
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// y+1 (normal: up)
				if (j == voxel_bit_field[i].size() - 1 || !voxel_bit_field[i][j + 1][k])
				{
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// z-1 (normal: backwards)
				if (k == 0 || !voxel_bit_field[i][j][k - 1])
				{
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// z+1 (normal: forwards)
				if (k == voxel_bit_field[i][j].size() - 1 || !voxel_bit_field[i][j][k + 1])
				{
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}
			}
		}
	}
	voxel_model = new Model(quadMesh);
}

void VoxelGroup::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	// NOTE: "Type" is taken care of not here, but at the very beginning when the object is getting created.
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	// TODO: make it so that you don't have to retrigger this every time you load
	refreshResources();
}

nlohmann::json VoxelGroup::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	return j;
}

void VoxelGroup::physicsUpdate()
{
	if (velocity.isZero() && angularVelocity.isZero())
		return;

	physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)getPhysicsComponent()->getActor();
	physx::PxTransform trans = body->getGlobalPose();
	trans.p += velocity;
	trans.q *= PhysicsUtils::createQuatFromEulerDegrees(PhysicsUtils::toGLMVec3(angularVelocity));
	body->setKinematicTarget(trans);
	INTERNALsubmitPhysicsCalculation(PhysicsUtils::physxTransformToGlmMatrix(trans));
}

void VoxelGroup::INTERNALrecreatePhysicsComponent()
{
	if (physicsComponent != nullptr)
		delete physicsComponent;

	physicsComponent = new TriangleMeshCollider(this, voxel_model, RigidActorTypes::STATIC);
}

void VoxelGroup::preRenderUpdate()
{
#ifdef _DEBUG
	// If in playmode
	// NOTE: turns out even though no physics simulation is happening during !playMode, you can still do raycasts. Nice
	if (MainLoop::getInstance().playMode)
		return;

	// If current selected object is me
	int currentObjIndex = MainLoop::getInstance().renderManager->currentSelectedObjectIndex;
	if (currentObjIndex < 0 ||
		currentObjIndex >= MainLoop::getInstance().objects.size() ||
		MainLoop::getInstance().objects[currentObjIndex] != this)
		return;

	// If press x
	bool yare = false;
	static bool prevIsButtonHeld = GLFW_RELEASE;
	bool isButtonHeld = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_X);
	if (prevIsButtonHeld == GLFW_RELEASE && isButtonHeld == GLFW_PRESS)
	{
		yare = true;
	}
	prevIsButtonHeld = isButtonHeld;


	if (!yare)
		return;

	//
	// Check what position and angle it's clicked on! (@TODO: Figure out why not working)
	//
	double mouseX, mouseY;
	glfwGetCursorPos(MainLoop::getInstance().window, &mouseX, &mouseY);

	physx::PxRaycastBuffer hitInfo;
	const Camera& camera = MainLoop::getInstance().camera;
	bool hit = PhysicsUtils::raycast(
		PhysicsUtils::toPxVec3(camera.position),
		PhysicsUtils::toPxVec3(glm::quat({ 0, 0, 1 }, camera.orientation) * glm::normalize(glm::vec3(mouseX - (camera.width / 2.0f), camera.height - mouseY - 1 - (camera.height / 2.0f), camera.zFar))),
		500,
		hitInfo
	);

	if (!hit || !hitInfo.hasBlock || hitInfo.block.actor != getPhysicsComponent()->getActor())
		return;

	std::cout << "Yolo4" << std::endl;

	glm::vec3 position = glm::inverse(getTransform()) * glm::vec4(PhysicsUtils::toGLMVec3(hitInfo.block.position), 1.0f);
	glm::vec3 normal = glm::mat3(glm::transpose(getTransform())) * PhysicsUtils::toGLMVec3(hitInfo.block.position);			// @CHECK, bc it's based off the normalModelMatrix in mesh.cpp

	std::cout << position.x << ",\t" << position.y << ",\t" << position.z << std::endl;
	std::cout << "\t\t\t" << normal.x << ",\t" << normal.y << ",\t" << normal.z << std::endl;

	//
	// Add onto voxelgroup!
	//
	// @TODO
#endif
}

#ifdef _DEBUG
void VoxelGroup::imguiPropertyPanel()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(getTransform()));

	ImGui::Separator();

	ImGui::DragFloat3("Velocity", &velocity[0], 0.01f);
	ImGui::DragFloat3("Ang Velocity", &angularVelocity[0], 0.01f);
}
#endif
