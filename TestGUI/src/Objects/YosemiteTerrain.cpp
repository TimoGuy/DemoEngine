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


YosemiteTerrain::YosemiteTerrain()
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new YosemiteTerrainImGui(this, bounds);
	physicsComponent = new BoxCollider(this, bounds, RigidActorTypes::STATIC);
	renderComponent = new YosemiteTerrainRender(this, bounds);
}

YosemiteTerrainRender::YosemiteTerrainRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
	refreshResources();
}

void YosemiteTerrainRender::refreshResources()
{
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");
	shadowPassProgramId = *(GLuint*)Resources::getResource("shader;shadowPass");

	model = (Model*)Resources::getResource("model;yosemiteTerrain");

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
	// I'll take the leftover tokens then
	//
}

nlohmann::json YosemiteTerrain::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	return j;
}

void YosemiteTerrainRender::preRenderUpdate()
{
	renderTransform = baseObject->getTransform();
}


void YosemiteTerrainRender::insertMeshesIntoSortedRenderQueue(std::map<GLuint, std::vector<Mesh*>>& sortedRenderQueue)
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

	//model->render(renderTransform, nullptr);
	model->insertMeshesIntoSortedRenderQueue(sortedRenderQueue, &renderTransform, nullptr);
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

	model->renderShadow(&renderTransform, nullptr);
}

void YosemiteTerrainImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->getTransform()));
}
