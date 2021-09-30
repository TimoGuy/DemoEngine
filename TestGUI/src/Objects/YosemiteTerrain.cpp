#include "YosemiteTerrain.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"




#include "../Objects/DirectionalLight.h"			// tEMP
#include "../Objects/PointLight.h"					// temp

YosemiteTerrain::YosemiteTerrain()
{
	transform = glm::mat4(1.0f);

	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new YosemiteTerrainImGui(this, bounds);
	physicsComponent = new BoxCollider(this, bounds);
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

	model = *(Model*)Resources::getResource("model;yosemiteTerrain");

	pbrAlbedoTexture = *(GLuint*)Resources::getResource("texture;pbrAlbedo");
	pbrNormalTexture = *(GLuint*)Resources::getResource("texture;pbrNormal");
	pbrMetalnessTexture = *(GLuint*)Resources::getResource("texture;pbrMetalness");
	pbrRoughnessTexture = *(GLuint*)Resources::getResource("texture;pbrRoughness");
}

YosemiteTerrain::~YosemiteTerrain()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;
}

void YosemiteTerrain::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
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
	
}


void YosemiteTerrainRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
#ifdef _DEBUG
	refreshResources();
#endif
	// @Copypasta
	//
	// Load in textures
	//
	glUseProgram(pbrShaderProgramId);
	glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix()));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbrAlbedoTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "albedoMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pbrNormalTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pbrMetalnessTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pbrRoughnessTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE0);

	//
	// Setup the transformation matrices and lights
	//
	glUniformMatrix4fv(
		glGetUniformLocation(pbrShaderProgramId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(baseObject->transform)
	);
	glUniformMatrix3fv(
		glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(baseObject->transform))))
	);

	model.render(pbrShaderProgramId);
}

void YosemiteTerrainRender::renderShadow(GLuint programId)
{
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(baseObject->transform)
	);
	model.render(programId);
}

void YosemiteTerrainImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->transform));
}

void YosemiteTerrainImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	ImGuiComponent::renderImGui();
}

BoxCollider::BoxCollider(BaseObject* bo, Bounds* bounds) : PhysicsComponent(bo, bounds)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->transform);

	physx::PxRigidStatic* boxBody = PhysicsUtils::createRigidbodyStatic(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->transform));
	glm::vec3 realExtents = bounds->extents * scale;
	physx::PxShape* shape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z), *MainLoop::getInstance().defaultPhysicsMaterial);
	boxBody->attachShape(*shape);

	MainLoop::getInstance().physicsScene->addActor(*boxBody);
	shape->release();
}

void BoxCollider::physicsUpdate()
{
	// Do nothing
}
