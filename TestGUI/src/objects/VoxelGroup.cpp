#include "VoxelGroup.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/quaternion.hpp>
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
	setVoxelBitAtPosition(voxel_group_offset + glm::i64vec3(1, 0, 0), true);
	setVoxelBitAtPosition(voxel_group_offset + glm::i64vec3(1, 1, 0), true);
	setVoxelBitAtPosition(voxel_group_offset + glm::i64vec3(1, 1, 1), true);

	renderComponent = new RenderComponent(this);
	refreshResources();
}

VoxelGroup::~VoxelGroup()
{
	delete renderComponent;
	delete physicsComponent;
}

void VoxelGroup::loadPropertiesFromJson(nlohmann::json& object)
{
	// NOTE: "Type" is taken care of not here, but at the very beginning when the object is getting created.
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	//
	// Load bitfield
	//
	if (object.contains("voxel_bit_field"))
	{
		std::string bigBitfield = object["voxel_bit_field"];
		for (size_t i = 0; i < (size_t)voxel_group_size.x; i++)
		{
			for (size_t j = 0; j < (size_t)voxel_group_size.y; j++)
			{
				for (size_t k = 0; k < (size_t)voxel_group_size.z; k++)
				{
					char voxel_bit = bigBitfield[i * voxel_group_size.y * voxel_group_size.z + j * voxel_group_size.z + k];
					setVoxelBitAtPosition(
						{ i, j, k },
						voxel_bit != '0'
					);
				}
			}
		}
	}

	// TODO: make it so that you don't have to retrigger this every time you load
	refreshResources();
}

nlohmann::json VoxelGroup::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	//
	// Save bitfield
	//
	std::string bigBitfield;
	bigBitfield.resize(voxel_group_size.x * voxel_group_size.y * voxel_group_size.z, '0');
	for (size_t i = 0; i < (size_t)voxel_group_size.x; i++)
	{
		for (size_t j = 0; j < (size_t)voxel_group_size.y; j++)
		{
			for (size_t k = 0; k < (size_t)voxel_group_size.z; k++)
			{
				bigBitfield[i * voxel_group_size.y * voxel_group_size.z + j * voxel_group_size.z + k] =
					getVoxelBitAtPosition({ i, j, k }) ? '1' : '0';
			}
		}
	}
	j["voxel_bit_field"] = bigBitfield;

	return j;
}

void VoxelGroup::refreshResources()
{
	if (is_voxel_bit_field_dirty)
	{
		// Create quadmesh
		updateQuadMeshFromBitField();

		if (voxel_model != nullptr)
		{
			//
			// Create cooked collision mesh
			//
			INTERNALrecreatePhysicsComponent();
			renderComponent->clearAllModels();
			renderComponent->addModelToRender({ voxel_model, true, nullptr });
		}


		is_voxel_bit_field_dirty = false;
	}

	if (voxel_model != nullptr)
	{
		materials["Material"] = (Material*)Resources::getResource("material;pbrVoxelGroup");
		voxel_model->setMaterials(materials);
	}
}

void VoxelGroup::resizeVoxelArea(size_t x, size_t y, size_t z, glm::i64vec3 offset)
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
		voxel_group_offset = glm::i64vec3(x, y, z) / (glm::i64)2;		// TODO: figure this out, bc this line breaks the voxel creation system (and probs the deletion system too)
		//voxel_group_offset = glm::i64vec3(0);
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

void VoxelGroup::setVoxelBitAtPosition(glm::i64vec3 pos, bool flag)
{
	if (pos.x < 0 ||
		pos.y < 0 ||
		pos.z < 0 ||
		pos.x >= voxel_group_size.x ||
		pos.y >= voxel_group_size.y ||
		pos.z >= voxel_group_size.z)
		return;

	voxel_bit_field[pos.x][pos.y][pos.z] = flag;
	is_voxel_bit_field_dirty = true;
}

bool VoxelGroup::getVoxelBitAtPosition(glm::i64vec3 pos)
{
	if (pos.x < 0 ||
		pos.y < 0 ||
		pos.z < 0 ||
		pos.x >= voxel_group_size.x ||
		pos.y >= voxel_group_size.y ||
		pos.z >= voxel_group_size.z)
		return false;

	return voxel_bit_field[pos.x][pos.y][pos.z];
}

void VoxelGroup::updateQuadMeshFromBitField()
{
	if (voxel_model != nullptr)
	{
		delete voxel_model;
		voxel_model = nullptr;
	}

	std::vector<Vertex> quadMesh;
	Vertex vertBuilder;			// Let's call him "Bob" (https://pm1.narvii.com/6734/14e3582d0aa8180bd12d22772631e949ad4e6837v2_hq.jpg)

	//
	// Cycle thru each bit in the bitfield and see if meshes need to be rendered there
	//
	bool has_filled_bits = false;
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
				voxelOriginPosition *= voxel_render_size;

				//
				// See if on the edge!
				// 
				// x-1 (normal: left)
				if (i == 0 || !voxel_bit_field[i - 1][j][k])
				{
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// x+1 (normal: right)
				if (i == voxel_bit_field.size() - 1 || !voxel_bit_field[i + 1][j][k])
				{
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// y-1 (normal: down)
				if (j == 0 || !voxel_bit_field[i][j - 1][k])
				{
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// y+1 (normal: up)
				if (j == voxel_bit_field[i].size() - 1 || !voxel_bit_field[i][j + 1][k])
				{
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// z-1 (normal: backwards)
				if (k == 0 || !voxel_bit_field[i][j][k - 1])
				{
					vertBuilder.position = glm::vec3(0, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 0) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}

				// z+1 (normal: forwards)
				if (k == voxel_bit_field[i][j].size() - 1 || !voxel_bit_field[i][j][k + 1])
				{
					vertBuilder.position = glm::vec3(0, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(0, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 0, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
					vertBuilder.position = glm::vec3(1, 1, 1) * voxel_render_size + voxelOriginPosition;
					quadMesh.push_back(vertBuilder);
				}
			}
		}
	}

	if (quadMesh.size() > 0)
		voxel_model = new Model(quadMesh);
	else
		// DELETE IF NO VOXELS
		MainLoop::getInstance().deleteObject(this);
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

bool jfjkjlskdjfkljyes_pls;
glm::vec3 jfodsjfsalkdfjlkasdf;
glm::vec3 jfjdskldfklslkdfnormal_raw;
glm::vec3 jojfjsdkfjksdkfhskdnormal_cooked;
glm::i64vec3 jojokhkohhohokhoh_from_pos;
glm::i64vec3 jojokhkohhohokhoh_to_pos;

void VoxelGroup::preRenderUpdate()
{
#ifdef _DEBUG
	// NOTE: turns out even though no physics simulation is happening during !playMode, you can still do raycasts. Nice
	if (MainLoop::getInstance().playMode)
		return;

	// If current selected object is me
	int currentObjIndex = MainLoop::getInstance().renderManager->currentSelectedObjectIndex;
	if (currentObjIndex < 0 ||
		currentObjIndex >= MainLoop::getInstance().objects.size() ||
		MainLoop::getInstance().objects[currentObjIndex] != this)
		return;

	// MODE: 0: normal. 1: appending a bunch of voxels
	enum VOXEL_EDIT_MODES { NORMAL, APPENDING_VOXELS };
	static int mode = NORMAL;
	static ViewPlane rawPlane;
	static glm::vec3 cookedNormal;
	static glm::i64vec3 fromPosition;
	static glm::i64vec3 toPosition;

	// Do the WS position camera vector.
	Camera& camera = MainLoop::getInstance().camera;

	double xpos, ypos;
	glfwGetCursorPos(MainLoop::getInstance().window, &xpos, &ypos);
	xpos /= MainLoop::getInstance().camera.width;
	ypos /= MainLoop::getInstance().camera.height;
	ypos = 1.0 - ypos;
	xpos = xpos * 2.0f - 1.0f;
	ypos = ypos * 2.0f - 1.0f;
	glm::vec3 clipSpacePosition(xpos, ypos, 1.0f);
	const glm::vec3 worldSpacePosition = camera.clipSpacePositionToWordSpace(clipSpacePosition);

	if (mode == NORMAL)
	{

		// If press x (if held shift, then x is "turbo'd")
		bool yareDelete = false;
		static bool prevIsButtonHeld1 = GLFW_RELEASE;
		bool isButtonHeld1 = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_X);
		bool isShiftHeld = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		if ((prevIsButtonHeld1 == GLFW_RELEASE || isShiftHeld) && isButtonHeld1 == GLFW_PRESS)
		{
			yareDelete = true;
		}
		prevIsButtonHeld1 = isButtonHeld1;

		// OR if press c (if held shift, then c is group append)
		bool yareAdd = false;
		static bool prevIsButtonHeld2 = GLFW_RELEASE;
		bool isButtonHeld2 = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_C);
		if (prevIsButtonHeld2 == GLFW_RELEASE && isButtonHeld2 == GLFW_PRESS)
		{
			yareAdd = true;
		}
		prevIsButtonHeld2 = isButtonHeld2;


		if (!yareDelete && !yareAdd)
			return;

		//
		// Check what position and angle it's clicked on! (@TODO: Figure out why not working)
		//
		physx::PxRaycastBuffer hitInfo;
		bool hit = PhysicsUtils::raycast(
			PhysicsUtils::toPxVec3(camera.position),
			PhysicsUtils::toPxVec3(glm::normalize(worldSpacePosition - camera.position)),
			500,
			hitInfo
		);

		if (!hit || !hitInfo.hasBlock || hitInfo.block.actor != getPhysicsComponent()->getActor())
			return;

		glm::vec3 positionRaw = glm::inverse(getTransform()) * glm::vec4(PhysicsUtils::toGLMVec3(hitInfo.block.position), 1.0f) / voxel_render_size;
		glm::vec3 normal = glm::round(glm::mat3(glm::transpose(getTransform())) * PhysicsUtils::toGLMVec3(hitInfo.block.normal));
		positionRaw -= glm::vec3(0.1f) * normal;		// NOTE: a slight offset occurs especially if rotated, so this is to do some padding for that
		glm::i64vec3 position = glm::i64vec3(glm::floor(positionRaw)) + voxel_group_offset;

		//
		// Edit the voxelgroup!
		//
		if (yareAdd)
		{
			if (isShiftHeld)
			{
				//
				// Take the normal and project the current mouse position to it
				//
				rawPlane.position = PhysicsUtils::toGLMVec3(hitInfo.block.position);
				rawPlane.normal = PhysicsUtils::toGLMVec3(hitInfo.block.normal);
				cookedNormal = normal;

				fromPosition = position;

				mode = APPENDING_VOXELS;
			}
			else
			{
				//
				// Append one voxel at a time (If in range!)
				//
				glm::i64vec3 additionVoxel = position + glm::i64vec3(normal);
				setVoxelBitAtPosition(additionVoxel, true);
			}
		}
		else if (yareDelete)
		{
			// Delete voxel!
			setVoxelBitAtPosition(position, false);
		}
	}

	if (mode == APPENDING_VOXELS)
	{
		bool isButtonHeld = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_C);
		if (isButtonHeld)
		{
			//
			// Keep track of what the position of (cpu side ray-marching)
			//
			glm::vec3 prevRMPos = worldSpacePosition + glm::vec3(1.0f);		// Just to enter the while loop below
			glm::vec3 currRMPos = worldSpacePosition;
			const glm::vec3 targetRMPos = camera.position;		// NOTE: not the goal, but the direction that RM is gonna go

			size_t iterations = 0;
			const size_t max_iterations = 100;
			while (glm::distance(prevRMPos, currRMPos) > 0.01f && iterations < max_iterations)
			{
				prevRMPos = currRMPos;

				const float distance = glm::abs(rawPlane.getSignedDistance(prevRMPos));
				const glm::vec3 raymarchDelta = glm::normalize(targetRMPos - currRMPos) * distance;
				currRMPos += raymarchDelta;

				iterations++;
			}

			if (iterations < max_iterations)
			{
				jfodsjfsalkdfjlkasdf = currRMPos;
				jojokhkohhohokhoh_from_pos = fromPosition;
				jfjdskldfklslkdfnormal_raw = rawPlane.normal;
				jojfjsdkfjksdkfhskdnormal_cooked = cookedNormal;
				jfjkjlskdjfkljyes_pls = true;

				// Convert hovering position to voxelposition
				glm::vec3 toPositionRaw = glm::inverse(getTransform()) * glm::vec4(currRMPos, 1.0f) / voxel_render_size;
				toPositionRaw -= glm::vec3(0.1f) * cookedNormal;		// NOTE: a slight offset occurs especially if rotated, so this is to do some padding for that
				toPosition = glm::i64vec3(glm::floor(toPositionRaw)) + voxel_group_offset;

				jojokhkohhohokhoh_to_pos = toPosition;
			}
		}
		else
		{
			//
			// Fill in all that area specified while holding SHIFT+C
			//
			glm::i64vec3 startPlacementPosition = fromPosition + glm::i64vec3(cookedNormal);
			glm::i64vec3 currentPlacementPosition = fromPosition + glm::i64vec3(cookedNormal);
			glm::i64vec3 targetPlacementPosition = toPosition + glm::i64vec3(cookedNormal);
			bool breakX, breakY, breakZ;
			for (currentPlacementPosition.x = startPlacementPosition.x, breakX = false; !breakX; currentPlacementPosition.x = PhysicsUtils::moveTowards(currentPlacementPosition.x, targetPlacementPosition.x, 1))
			{
				for (currentPlacementPosition.y = startPlacementPosition.y, breakY = false; !breakY; currentPlacementPosition.y = PhysicsUtils::moveTowards(currentPlacementPosition.y, targetPlacementPosition.y, 1))
				{
					for (currentPlacementPosition.z = startPlacementPosition.z, breakZ = false; !breakZ; currentPlacementPosition.z = PhysicsUtils::moveTowards(currentPlacementPosition.z, targetPlacementPosition.z, 1))
					{
						//
						// Fill in current spot, then move towards the next spot
						//
						setVoxelBitAtPosition(currentPlacementPosition, true);

						// Check if finished
						if (currentPlacementPosition.z == targetPlacementPosition.z)
						{
							breakZ = true;
						}
					}

					// Check if finished
					if (currentPlacementPosition.y == targetPlacementPosition.y)
					{
						breakY = true;
					}
				}

				// Check if finished
				if (currentPlacementPosition.x == targetPlacementPosition.x)
				{
					breakX = true;
				}
			}

			mode = NORMAL;
		}
	}
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

void VoxelGroup::imguiRender()
{
	if (jfjkjlskdjfkljyes_pls)
	{
		// Draw the bounds for the creation area of the voxel group
		const float halfSingleBlock = 0.5f * voxel_render_size;
		const glm::vec3 normalMask = glm::abs(jojfjsdkfjksdkfhskdnormal_cooked);

		glm::vec3 deltaPositions(jojokhkohhohokhoh_to_pos - jojokhkohhohokhoh_from_pos);
		glm::vec3 halfExtents = (glm::abs(deltaPositions) + 1.0f) * halfSingleBlock;
		halfExtents *= 1.0f - normalMask;
		halfExtents = glm::max(glm::vec3(halfSingleBlock), halfExtents);
		physx::PxBoxGeometry geom(PhysicsUtils::toPxVec3(halfExtents));

		const glm::vec3 offset = (glm::vec3(jojokhkohhohokhoh_from_pos - voxel_group_offset) + jojfjsdkfjksdkfhskdnormal_cooked) * voxel_render_size + (deltaPositions + 1.0f) * halfSingleBlock;

		glm::vec3 position = PhysicsUtils::getPosition(getTransform());
		glm::quat rotation = PhysicsUtils::getRotation(getTransform());
		glm::vec3 scale = PhysicsUtils::getScale(getTransform());
		PhysicsUtils::imguiRenderBoxCollider(
			glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), scale),
			geom,
			ImColor(0.78f, 0.243f, 0.373f)
		);

		// Draw the guiding circle for appending with SHIFT+C
		PhysicsUtils::imguiRenderCircle(glm::translate(glm::mat4(1.0f), jfodsjfsalkdfjlkasdf), 0.5f, glm::eulerAngles(glm::quat({ 0, 0, 1 }, jfjdskldfklslkdfnormal_raw)), glm::vec3(), 10);
		jfjkjlskdjfkljyes_pls = false;
	}
}
#endif
