#include "YosemiteTerrain.h"

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


YosemiteTerrain::YosemiteTerrain(std::string modelResourceName)
{
	name = "Yosemite Terrain";

	bounds = new RenderAABB();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	INTERNALrecreatePhysicsComponent(modelResourceName);
	renderComponent = new YosemiteTerrainRender(this, bounds, modelResourceName);
}

YosemiteTerrainRender::YosemiteTerrainRender(BaseObject* bo, RenderAABB* bounds, std::string modelResourceName) : RenderComponent(bo, bounds), modelResourceName(modelResourceName)
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
		physicsComponent = new BoxCollider(this, bounds->extents, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);
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

#ifdef _DEBUG
void YosemiteTerrain::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(getTransform()));

	ImGui::Separator();

	// Changing the model resource name
	static std::string tempModelResourceName = ((YosemiteTerrainRender*)getRenderComponent())->modelResourceName;
	ImGui::InputText("Model Resource", &tempModelResourceName);
	if (tempModelResourceName != ((YosemiteTerrainRender*)getRenderComponent())->modelResourceName)
	{
		if (ImGui::Button("Apply Resource Name"))
		{
			((YosemiteTerrainRender*)getRenderComponent())->modelResourceName = tempModelResourceName;
		}
	}

	ImGui::Separator();

	ImGui::DragFloat3("Velocity", &velocity[0], 0.01f);
	ImGui::DragFloat3("Ang Velocity", &angularVelocity[0], 0.01f);
}
#endif
