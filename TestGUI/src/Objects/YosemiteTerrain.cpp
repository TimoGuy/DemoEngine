#include "YosemiteTerrain.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "Components/PhysicsComponents.h"
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"


YosemiteTerrain::YosemiteTerrain(std::string modelResourceName)
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new YosemiteTerrainImGui(this, bounds);
	INTERNALrecreatePhysicsComponent(modelResourceName);
	renderComponent = new YosemiteTerrainRender(this, bounds, modelResourceName);
}

YosemiteTerrainRender::YosemiteTerrainRender(BaseObject* bo, Bounds* bounds, std::string modelResourceName) : RenderComponent(bo, bounds), modelResourceName(modelResourceName)
{
	refreshResources();
}

void YosemiteTerrainRender::refreshResources()
{
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");
	shadowPassProgramId = *(GLuint*)Resources::getResource("shader;shadowPass");

	bool isNewModel;
	model = (Model*)Resources::getResource(modelResourceName, model, &isNewModel);
	if (isNewModel)
	{
		((YosemiteTerrain*)baseObject)->INTERNALrecreatePhysicsComponent(modelResourceName);
	}

	materials["Material"] = (Material*)Resources::getResource("material;pbrRustyMetal");
	model->setMaterials(materials);
}

YosemiteTerrain::~YosemiteTerrain()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;
}

void YosemiteTerrain::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	// NOTE: "Type" is taken care of not here, but at the very beginning when the object is getting created.
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);

	//
	// Check if there's an explicit model name
	//
	if (object.contains("modelResourceName"))
	{
		((YosemiteTerrainRender*)getRenderComponent())->modelResourceName = object["modelResourceName"];
	}

	// TODO: make it so that you don't have to retrigger this every time you load
	((YosemiteTerrainRender*)getRenderComponent())->refreshResources();
}

nlohmann::json YosemiteTerrain::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	// Explicit model resource name
	j["modelResourceName"] = ((YosemiteTerrainRender*)getRenderComponent())->modelResourceName;

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
		physicsComponent = new BoxCollider(this, bounds, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);
	}
	else
	{
		Model* fetchedModel = (Model*)Resources::getResource(modelResourceName);
		physicsComponent = new TriangleMeshCollider(this, fetchedModel, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);
	}
}

void YosemiteTerrainRender::preRenderUpdate()
{
	renderTransform = baseObject->getTransform();
}


void YosemiteTerrainRender::render()
{
#ifdef _DEBUG
	refreshResources();
#endif

	//
	// Setup the transformation matrices and lights
	//
	//glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));
	//glUniformMatrix3fv(glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(renderTransform)))));
	//model->render(pbrShaderProgramId);

	model->render(renderTransform, true);
}

void YosemiteTerrainRender::renderShadow(GLuint programId)
{
	//glUniformMatrix4fv(
	//	glGetUniformLocation(programId, "modelMatrix"),
	//	1,
	//	GL_FALSE,
	//	glm::value_ptr(renderTransform)
	//);
	//model->render(programId);

	glUniformMatrix4fv(glGetUniformLocation(programId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));
	model->render(renderTransform, false);
}

void YosemiteTerrainImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->getTransform()));

	ImGui::Separator();

	// Changing the model resource name
	static std::string tempModelResourceName = ((YosemiteTerrainRender*)baseObject->getRenderComponent())->modelResourceName;
	ImGui::InputText("Model Resource", &tempModelResourceName);
	if (tempModelResourceName != ((YosemiteTerrainRender*)baseObject->getRenderComponent())->modelResourceName)
	{
		if (ImGui::Button("Apply Resource Name"))
		{
			((YosemiteTerrainRender*)baseObject->getRenderComponent())->modelResourceName = tempModelResourceName;
		}
	}

	ImGui::Separator();

	ImGui::DragFloat3("Velocity", &((YosemiteTerrain*)baseObject)->velocity[0], 0.01f);
	ImGui::DragFloat3("Ang Velocity", &((YosemiteTerrain*)baseObject)->angularVelocity[0], 0.01f);
}
