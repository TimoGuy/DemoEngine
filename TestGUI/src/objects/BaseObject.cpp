#include "BaseObject.h"

#include <sstream>
#include <random>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glad/glad.h>

#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"
#include "../render_engine/camera/Camera.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../render_engine/model/Model.h"
#include "../render_engine/model/animation/Animator.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/material/Shader.h"


//
// TODO: move this into a Create GUID util code
//
uint32_t random_char()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);
	return dis(gen);
}

std::string generate_hex(const uint32_t len)
{
	std::stringstream ss;
	for (uint32_t i = 0; i < len; i++)
	{
		const auto rc = random_char();
		std::stringstream hexstream;
		hexstream << std::hex << rc;
		auto hex = hexstream.str();
		ss << (hex.length() < 2 ? '0' + hex : hex);
	}
	return ss.str();
}

void PhysicsTransformState::updateTransform(glm::mat4 newTransform)
{
	previousTransform = currentTransform;
	currentTransform = newTransform;
}

glm::mat4 PhysicsTransformState::getInterpolatedTransform(float alpha)
{
	// Easy out
	if (currentTransform == previousTransform)
		return currentTransform;

	// Easy out2
	if (alpha >= 1.0f)
		return currentTransform;
	if (alpha <= 0.0f)
		return previousTransform;

	glm::vec3 scale1;
	glm::quat rotation1;
	glm::vec3 translation1;
	glm::vec3 skew1;
	glm::vec4 perspective1;
	glm::decompose(currentTransform, scale1, rotation1, translation1, skew1, perspective1);				// NOTE: I'm banking that this is faster than using my algorithms bc of SIMD
	glm::vec3 scale2;
	glm::quat rotation2;
	glm::vec3 translation2;
	glm::vec3 skew2;
	glm::vec4 perspective2;
	glm::decompose(previousTransform, scale2, rotation2, translation2, skew2, perspective2);

	glm::vec3 translation = glm::mix(translation2, translation1, alpha);
	glm::quat rotation = glm::slerp(rotation2, rotation1, alpha);
	glm::vec3 scale = glm::mix(scale2, scale1, alpha);

	// Compose all these together (trans * rot * scale)
	return glm::scale(glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation), scale);
}

BaseObject::BaseObject()
{
	transform = glm::mat4(1.0f);
	guid = generate_hex(32);

	// This is for physics realm of baseobject
	physicsTransformState.updateTransform(getTransform());
	physicsTransformState.updateTransform(getTransform());

	MainLoop::getInstance().objects.push_back(this);
}

BaseObject::~BaseObject()
{
	MainLoop::getInstance().objects.erase(
		std::remove(
			MainLoop::getInstance().objects.begin(),
			MainLoop::getInstance().objects.end(),
			this
		),
		MainLoop::getInstance().objects.end()
	);
}

void BaseObject::loadPropertiesFromJson(nlohmann::json& object)
{
	// Pick up the guid
	if (object.contains("guid"))
		guid = object["guid"];

	// Pick up the name
	if (object.contains("name"))
		name = object["name"];

	glm::vec3 position = glm::vec3(object["position"][0], object["position"][1], object["position"][2]);
	glm::vec3 eulerAngles = glm::vec3(object["rotation"][0], object["rotation"][1], object["rotation"][2]);
	glm::vec3 scale = glm::vec3(object["scale"][0], object["scale"][1], object["scale"][2]);

	setTransform(
		glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(glm::radians(eulerAngles))) * glm::scale(glm::mat4(1.0f), scale)
	);
}

nlohmann::json BaseObject::savePropertiesToJson()
{
	nlohmann::json j;
	j["guid"] = guid;
	j["name"] = name;

	glm::vec3 position = PhysicsUtils::getPosition(transform);
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(PhysicsUtils::getRotation(transform)));
	glm::vec3 scale = PhysicsUtils::getScale(transform);
	j["position"] = { position.x, position.y, position.z };
	j["rotation"] = { eulerAngles.x, eulerAngles.y, eulerAngles.z };
	j["scale"] = { scale.x, scale.y, scale.z };
	return j;
}

glm::mat4& BaseObject::getTransform()
{
	return transform;
}

glm::mat4 BaseObject::getTransformWithoutScale()
{
	glm::vec3 pos = PhysicsUtils::getPosition(transform);
	glm::quat rot = PhysicsUtils::getRotation(transform);
	return glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot);
}

void BaseObject::setTransform(glm::mat4 newTransform)
{
	transform = newTransform;

	PhysicsComponent* pc = getPhysicsComponent();
	if (pc != nullptr)
	{
		// Reset the transform and propagate it
		physicsTransformState.updateTransform(getTransform());
		physicsTransformState.updateTransform(getTransform());
		pc->propagateNewTransform(newTransform);
	}
}

void BaseObject::INTERNALsubmitPhysicsCalculation(glm::mat4 newTransform)
{
	physicsTransformState.updateTransform(newTransform);
}

void BaseObject::INTERNALfetchInterpolatedPhysicsTransform(float alpha)
{
	transform = physicsTransformState.getInterpolatedTransform(alpha);		// Do this without propagating transforms
}

LightComponent::LightComponent(BaseObject* baseObject, bool castsShadows) : baseObject(baseObject), castsShadows(castsShadows)
{
	MainLoop::getInstance().lightObjects.push_back(this);
}

LightComponent::~LightComponent()
{
	MainLoop::getInstance().lightObjects.erase(
		std::remove(
			MainLoop::getInstance().lightObjects.begin(),
			MainLoop::getInstance().lightObjects.end(),
			this
		),
		MainLoop::getInstance().lightObjects.end()
	);
}

void LightComponent::loadPropertiesFromJson(nlohmann::json& object)
{
	castsShadows = object["casts_shadows"];
}

nlohmann::json LightComponent::savePropertiesToJson()
{
	nlohmann::json j;
	j["casts_shadows"] = castsShadows;
	return j;
}

void LightComponent::renderPassShadowMap() {}

PhysicsComponent::PhysicsComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	// Populate the transformstate struct
	MainLoop::getInstance().physicsObjects.push_back(this);
}

PhysicsComponent::~PhysicsComponent()
{
	MainLoop::getInstance().physicsObjects.erase(
		std::remove(
			MainLoop::getInstance().physicsObjects.begin(),
			MainLoop::getInstance().physicsObjects.end(),
			this
		),
		MainLoop::getInstance().physicsObjects.end()
	);
}

physx::PxRigidActor* PhysicsComponent::getActor()
{
	return body;
}

void PhysicsComponent::INTERNALonTrigger(const physx::PxTriggerPair& pair)
{
	baseObject->onTrigger(pair);
}

RenderComponent::RenderComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	MainLoop::getInstance().renderObjects.push_back(this);
}

RenderComponent::~RenderComponent()
{
	MainLoop::getInstance().renderObjects.erase(
		std::remove(
			MainLoop::getInstance().renderObjects.begin(),
			MainLoop::getInstance().renderObjects.end(),
			this
		),
		MainLoop::getInstance().renderObjects.end()
	);

	for (size_t i = 0; i < textRenderers.size(); i++)
	{
		MainLoop::getInstance().renderManager->removeTextRenderer(textRenderers[i]);
	}
}

void RenderComponent::addModelToRender(const ModelWithMetadata& modelWithMetadata)
{
	modelsWithMetadata.push_back(modelWithMetadata);
}

void RenderComponent::insertModelToRender(size_t index, const ModelWithMetadata& modelWithMetadata)
{
	modelsWithMetadata.insert(modelsWithMetadata.begin() + index, modelWithMetadata);
}

void RenderComponent::changeModelToRender(size_t index, const ModelWithMetadata& modelWithMetadata)
{
	modelsWithMetadata[index] = modelWithMetadata;
}

void RenderComponent::removeModelToRender(size_t index)
{
	modelsWithMetadata.erase(modelsWithMetadata.begin() + index);
}

ModelWithMetadata RenderComponent::getModelFromIndex(size_t index)
{
	return modelsWithMetadata[index];
}

void RenderComponent::clearAllModels()
{
	modelsWithMetadata.clear();
}

void RenderComponent::addTextToRender(TextRenderer* textRenderer)
{
	textRenderers.push_back(textRenderer);
	MainLoop::getInstance().renderManager->addTextRenderer(textRenderer);
}

void RenderComponent::removeTextRenderer(TextRenderer* textRenderer)
{
	textRenderers.erase(std::remove(textRenderers.begin(), textRenderers.end(), textRenderer), textRenderers.end());
	MainLoop::getInstance().renderManager->removeTextRenderer(textRenderer);
}

void RenderComponent::render(const ViewFrustum* viewFrustum, Shader* zPassShader)								// @Copypasta
{
#ifdef _DEVELOP
	refreshResources();
#endif

	for (size_t i = 0; i < modelsWithMetadata.size(); i++)
	{
		const ModelWithMetadata& mwmd = modelsWithMetadata[i];

		// Short circuit out of loop if none of meshes are in view frustum.
		// Also creates a reference to the meshes of which ones are in the view frustum.
		std::vector<bool> whichMeshesInView;
		if (!mwmd.model->getIfInViewFrustum(baseObject->getTransform() * *mwmd.localTransform, viewFrustum, whichMeshesInView))
			continue;
		
		std::vector<glm::mat4>* boneTransforms = nullptr;
		if (mwmd.modelAnimator != nullptr)
			boneTransforms = mwmd.modelAnimator->getFinalBoneMatrices();

		mwmd.model->render(baseObject->getTransform() * *mwmd.localTransform, zPassShader, &whichMeshesInView, boneTransforms, RenderStage::Z_PASS);
	}
}

void RenderComponent::renderShadow(Shader* shader)		// @Copypasta
{
#ifdef _DEVELOP
	refreshResources();
#endif

	for (size_t i = 0; i < modelsWithMetadata.size(); i++)
	{
		const ModelWithMetadata& mwmd = modelsWithMetadata[i];
		std::vector<glm::mat4>* boneTransforms = nullptr;
		if (mwmd.modelAnimator != nullptr)
			boneTransforms = mwmd.modelAnimator->getFinalBoneMatrices();

		mwmd.model->render(baseObject->getTransform() * *mwmd.localTransform, shader, nullptr, boneTransforms, RenderStage::OVERRIDE);
	}
}

#ifdef _DEVELOP
std::vector<std::string> RenderComponent::getMaterialNameList()
{
	std::vector<std::string> materialNameList;
	for (size_t i = 0; i < modelsWithMetadata.size(); i++)
	{
		std::vector<std::string> currentNameList = modelsWithMetadata[i].model->getMaterialNameList();
		for (size_t j = 0; j < currentNameList.size(); j++)
		{
			if (std::find(materialNameList.begin(), materialNameList.end(), currentNameList[j]) != materialNameList.end())
				continue;	// Uniques only pls
			materialNameList.push_back(currentNameList[j]);
		}
	}
	return materialNameList;
}

void RenderComponent::TEMPrenderImguiModelBounds()
{
	for (size_t i = 0; i < modelsWithMetadata.size(); i++)
	{
		modelsWithMetadata[i].model->TEMPrenderImguiModelBounds(baseObject->getTransform() * *modelsWithMetadata[i].localTransform);
	}
}
#endif

#ifdef _DEVELOP
void RenderComponent::refreshResources()
{
	baseObject->refreshResources();
}
#endif
