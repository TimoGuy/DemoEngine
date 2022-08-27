#include "GondolaPath.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"
#include "../utils/GameState.h"
#include "../utils/InputManager.h"
#include "../utils/Messages.h"
#include "components/PhysicsComponents.h"
#include "../render_engine/model/Model.h"
#include "../render_engine/material/Material.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/render_manager/RenderManager.h"


GondolaPath::GondolaPath()
{
	name = "Gondola Path";

	refreshResources();
	renderComponent = new RenderComponent(this);
}

GondolaPath::~GondolaPath()
{
	delete renderComponent;
}

void GondolaPath::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	// if (object.contains("num_water_servings"))
	// 	numWaterServings = object["num_water_servings"];

	refreshResources();
}

nlohmann::json GondolaPath::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	// j["num_water_servings"] = numWaterServings;

	return j;
}

void GondolaPath::preRenderUpdate()
{
	
}

void GondolaPath::refreshResources()
{
	trackModels.clear();
	for (auto& path : trackModelPaths)
	{
		trackModels.push_back((Model*)Resources::getResource(path));
	}

	// TEMP
	trackModelConnectionOffsets.clear();
	trackModelConnectionOffsets.resize(trackModels.size(), glm::mat4(1.0f));
}

#ifdef _DEVELOP
void GondolaPath::imguiPropertyPanel()
{
	for (size_t i = 0; i < trackModelPaths.size(); i++)
	{
		if (ImGui::Button(("Add: " + trackModelPaths[i]).c_str()))
		{
			addPieceToGondolaPath((int)i);
		}
	}
}

void GondolaPath::imguiRender()
{
}
#endif

void GondolaPath::addPieceToGondolaPath(int pieceType)
{
	trackSegments.push_back({ pieceType, glm::mat4(1.0f) });

	size_t currentIndex = trackSegments.size() - 1;
	Model* modelToUse = trackModels[pieceType];

	renderComponent->addModelToRender({ modelToUse, true, nullptr, &trackSegments[currentIndex].localTransform });
	recalculateGondolaPathOffsets();
}

void GondolaPath::recalculateGondolaPathOffsets()
{
	glm::mat4 currentTransform(1.0f);
	for (auto& segment : trackSegments)
	{
		segment.localTransform = currentTransform;
		currentTransform *= trackModelConnectionOffsets[segment.pieceType];
	}
}
