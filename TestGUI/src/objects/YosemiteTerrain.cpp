#include "YosemiteTerrain.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "components/PhysicsComponents.h"
#include "../mainloop/MainLoop.h"
#include "../render_engine/resources/Resources.h"
#include "../utils/PhysicsUtils.h"

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#endif


YosemiteTerrain::YosemiteTerrain(std::string modelResourceName) : modelResourceName(modelResourceName)
{
	name = "Yosemite Terrain";

	INTERNALrecreatePhysicsComponent(modelResourceName);

	renderComponent = new RenderComponent(this);
	refreshResources();
}

YosemiteTerrain::~YosemiteTerrain()
{
	delete renderComponent;
	delete physicsComponent;
}

void YosemiteTerrain::refreshResources()
{
	bool isNewModel;
	model = (Model*)Resources::getResource(modelResourceName, model, &isNewModel);
	if (isNewModel)
	{
		INTERNALrecreatePhysicsComponent(modelResourceName);
		renderComponent->clearAllModels();
		renderComponent->addModelToRender({ model, true, nullptr });
	}

	materials["Material"] = (Material*)Resources::getResource("material;pbrRustyMetal");
	model->setMaterials(materials);
}

void YosemiteTerrain::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	// NOTE: "Type" is taken care of not here, but at the very beginning when the object is getting created.
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	//
	// Check if there's an explicit model name
	//
	if (object.contains("modelResourceName"))
	{
		modelResourceName = object["modelResourceName"];
	}

	// TODO: make it so that you don't have to retrigger this every time you load
	refreshResources();
}

nlohmann::json YosemiteTerrain::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	// Explicit model resource name
	j["modelResourceName"] = modelResourceName;

	return j;
}

void YosemiteTerrain::physicsUpdate()
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

void YosemiteTerrain::INTERNALrecreatePhysicsComponent(std::string modelResourceName)
{
	if (physicsComponent != nullptr)
		delete physicsComponent;

	if (modelResourceName == "model;cube")
	{
		// TODO: Figure out how to get camera zoom in to work with kinematic types too!!!!
		physicsComponent = new BoxCollider(this, glm::vec3(1.0f)/* @GIANT: Fix this yo!!! bounds->extents*/, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);
	}
	else
	{
		Model* fetchedModel = (Model*)Resources::getResource(modelResourceName);
		physicsComponent = new TriangleMeshCollider(this, fetchedModel, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);
	}
}

void YosemiteTerrain::preRenderUpdate()
{
}

#ifdef _DEVELOP
void YosemiteTerrain::imguiPropertyPanel()
{
	// Changing the model resource name
	static std::string tempModelResourceName = modelResourceName;
	ImGui::InputText("Model Resource", &tempModelResourceName);
	if (tempModelResourceName != modelResourceName)
	{
		if (ImGui::Button("Apply Resource Name"))
		{
			modelResourceName = tempModelResourceName;
		}
	}

	ImGui::Separator();

	ImGui::DragFloat3("Velocity", &velocity[0], 0.01f);
	ImGui::DragFloat3("Ang Velocity", &angularVelocity[0], 0.01f);
}
#endif
