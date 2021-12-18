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
#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#endif


VoxelGroup::VoxelGroup()
{
	name = "Voxel Group (unnamed)";

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
	static bool isNewModel = true;
	if (isNewModel)
	{
		//
		// Create quadmesh
		//
		std::vector<Vertex> quadMesh;
		Vertex vert;
		vert.position = glm::vec3(0);
		quadMesh.push_back(vert);
		vert.position = glm::vec3(0, 0, 1);
		quadMesh.push_back(vert);
		vert.position = glm::vec3(1, 0, 1);
		quadMesh.push_back(vert);
		vert.position = glm::vec3(1, 0, 0);
		quadMesh.push_back(vert);
		model = new Model(quadMesh);

		//
		// Create cooked collision mesh
		//
		INTERNALrecreatePhysicsComponent();
		renderComponent->clearAllModels();
		renderComponent->addModelToRender({ model, true, nullptr });

		isNewModel = false;
	}

	materials["Material"] = (Material*)Resources::getResource("material;pbrVoxelGroup");
	model->setMaterials(materials);
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

	physicsComponent = new TriangleMeshCollider(this, model, RigidActorTypes::STATIC);
}

void VoxelGroup::preRenderUpdate()
{
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
