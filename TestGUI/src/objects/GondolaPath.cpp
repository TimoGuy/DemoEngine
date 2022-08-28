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

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#include "../imgui/ImGuizmo.h"
#endif


GondolaPath::GondolaPath()
{
	name = "Gondola Path";

	refreshResources();
	renderComponent = new RenderComponent(this);

	trackModels.clear();
	for (auto& path : trackModelPaths)
	{
		trackModels.push_back((Model*)Resources::getResource(path));
	}

	// TEMP
	trackModelConnectionOffsets.clear();
	trackModelConnectionOffsets.push_back(glm::mat4(1.0f));
	trackModelConnectionOffsets.push_back(glm::mat4(1.0f));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -100)));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-37.2923228578, 0, -90.0316316157)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-43.1501872341, 0, -104.173767239)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(37.2923228578, 0, -90.0316316157)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, -45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(43.1501872341, 0, -104.173767239)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, -45, 0)))));
	trackModelConnectionOffsets.push_back(glm::mat4(1.0f));
	trackModelConnectionOffsets.push_back(glm::mat4(1.0f));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -100)));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-37.2923228578, 0, -90.0316316157)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-43.1501872341, 0, -104.173767239)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(37.2923228578, 0, -90.0316316157)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, -45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(43.1501872341, 0, -104.173767239)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(0, -45, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0, 17.275, -97.983)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(20, 0, 0)))));
	trackModelConnectionOffsets.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0, -15.212, -86.263)) * glm::toMat4(glm::quat(glm::radians(glm::vec3(-20, 0, 0)))));

	trackModelConnectionOffsets.resize(trackModels.size(), glm::mat4(1.0f));
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

	if (object.contains("gondola_path_ids"))
		for (auto segmentId : object["gondola_path_ids"])
			addPieceToGondolaPath((int)segmentId);
	recalculateGondolaPathOffsets();
}

nlohmann::json GondolaPath::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	j["gondola_path_ids"] = nlohmann::json::array();
	for (auto& segment : trackSegments)
		j["gondola_path_ids"].push_back(segment.pieceType);

	return j;
}

void GondolaPath::preRenderUpdate()
{
	//
	// Update positions of the text renderers
	//
	glm::mat4 currentTransform(1.0f);
	for (size_t i = 0; i < trackSegments.size(); i++)
	{
		if (MainLoop::getInstance().playMode)
		{
			// Hide the textrenderers
			trackSegmentTextRenderers[i]->text = "";
			continue;
		}

		// Change the position of the textrenderers
		auto& segment = trackSegments[i];

		*segment.localTransform = currentTransform;

		glm::vec3 p1 = glm::vec3(currentTransform * glm::vec4(0, 0, 0, 1));
		currentTransform *= trackModelConnectionOffsets[segment.pieceType];
		glm::vec3 p2 = glm::vec3(currentTransform * glm::vec4(0, 0, 0, 1));
		glm::vec3 midpt = (p1 + p2) / 2.0f + PhysicsUtils::getPosition(getTransform());
		midpt.y += 25.0f;
		trackSegmentTextRenderers[i]->modelMatrix = glm::translate(glm::mat4(1.0f), midpt - MainLoop::getInstance().camera.position) * glm::inverse(MainLoop::getInstance().camera.calculateViewMatrix()) * glm::scale(glm::mat4(1.0f), glm::vec3(0.6f));		// @COPYPASTA
		trackSegmentTextRenderers[i]->text = std::to_string(i + 1);
	}
}

void GondolaPath::refreshResources()
{
}

#ifdef _DEVELOP
void GondolaPath::imguiPropertyPanel()
{
	// Connection Offsets
	ImGui::Separator();
	if (ImGui::TreeNode("Connection offsets"))
	{
		bool refreshPathOffsetsPls = false;
		for (size_t i = 0; i < trackModelConnectionOffsets.size(); i++)
		{
			ImGui::Text(("OffsetPiece #" + std::to_string(i)).c_str());
			bool plsCopy = false;
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(trackModelConnectionOffsets[i]), matrixTranslation, matrixRotation, matrixScale);
			plsCopy |= ImGui::DragFloat3(("Pos##GondolaOffsetPerPiece" + std::to_string(i)).c_str(), matrixTranslation, 0.0025f);
			plsCopy |= ImGui::DragFloat3(("Rot##GondolaOffsetPerPiece" + std::to_string(i)).c_str(), matrixRotation, 0.0025f);
			plsCopy |= ImGui::DragFloat3(("Sca##GondolaOffsetPerPiece" + std::to_string(i)).c_str(), matrixScale, 0.0025f);
			if (plsCopy)
				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(trackModelConnectionOffsets[i]));
			refreshPathOffsetsPls |= plsCopy;
		}
		if (refreshPathOffsetsPls)
			recalculateGondolaPathOffsets();

		ImGui::TreePop();
	}

	// TrackSegments
	ImGui::Separator();
	if (ImGui::TreeNode("Edit track segments"))
	{
		for (size_t i = 0; i < trackSegments.size() + 1; i++)
		{
			// Add section
			static size_t showPopupIndex = 0;
			if (ImGui::Button(("Add track segment...##" + std::to_string(i)).c_str()))
			{
				ImGui::OpenPopup("add_track_segment_popup");
				showPopupIndex = i;
			}
			if (ImGui::BeginPopup("add_track_segment_popup"))
			{
				if (i == showPopupIndex)
				{
					for (size_t pathIndex = 0; pathIndex < trackModelPaths.size(); pathIndex++)
					{
						if (ImGui::Button(("Add: " + trackModelPaths[pathIndex] + "##" + std::to_string(pathIndex)).c_str()))
						{
							addPieceToGondolaPath((int)pathIndex, (i == trackSegments.size()) ? -1 : (int)i);
							ImGui::CloseCurrentPopup();
						}
					}
				}

				ImGui::EndPopup();
			}

			if (i == trackSegments.size())
				break;	// Cancel out of the add section bc this last button's gonna be the actual add button

			// Edit section
			GondolaPath::TrackSegment& trackSegment = trackSegments[i];
			if (ImGui::Button(("X## Delete and remove a certain track segment" + std::to_string(i)).c_str()))
			{
				// DELETE!!!
				removePieceOfGondolaPath(i);
				break;	// @HACK: prevent there being any out of bounds exceptions
			}
			ImGui::SameLine();
			if (ImGui::BeginCombo(("#" + std::to_string(i + 1) + " Track Segment").c_str(), trackModelPaths[trackSegment.pieceType].c_str()))
			{
				for (size_t pieceType = 0; pieceType < trackModelPaths.size(); pieceType++)
				{
					const bool isSelected = (trackSegment.pieceType == pieceType);
					if (ImGui::Selectable(trackModelPaths[pieceType].c_str(), isSelected))
					{
						changePieceOfGondolaPath(i, (int)pieceType);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		ImGui::TreePop();
	}
}

void GondolaPath::imguiRender()
{
}
#endif

void GondolaPath::addPieceToGondolaPath(int pieceType, int index)
{
	size_t currentIndex = index < 0 ? trackSegments.size() : (size_t)index;
	if (index < 0)
		trackSegments.push_back({ pieceType, new glm::mat4(1.0f) });
	else
		trackSegments.insert(trackSegments.begin() + currentIndex, { pieceType, new glm::mat4(1.0f) });

	Model* modelToUse = trackModels[pieceType];

	if (index < 0)
		renderComponent->addModelToRender({ modelToUse, true, nullptr, trackSegments[currentIndex].localTransform });
	else
		renderComponent->insertModelToRender(currentIndex, { modelToUse, true, nullptr, trackSegments[currentIndex].localTransform });

	trackSegmentTextRenderers.push_back(new TextRenderer{ std::to_string(trackSegmentTextRenderers.size() + 1), getTransform(), glm::vec3(0.5, 1, 0), TextAlignment::CENTER, TextAlignment::BOTTOM });
	renderComponent->addTextToRender(trackSegmentTextRenderers[trackSegmentTextRenderers.size() - 1]);

	recalculateGondolaPathOffsets();
}

void GondolaPath::changePieceOfGondolaPath(size_t index, int pieceType)
{
	trackSegments[index].pieceType = pieceType;
	Model* modelToUse = trackModels[pieceType];
	renderComponent->changeModelToRender(index, { modelToUse, true, nullptr, trackSegments[index].localTransform });
	recalculateGondolaPathOffsets();
}

void GondolaPath::removePieceOfGondolaPath(size_t index)
{
	trackSegments.erase(trackSegments.begin() + index);
	renderComponent->removeModelToRender(index);

	renderComponent->removeTextRenderer(trackSegmentTextRenderers[trackSegmentTextRenderers.size() - 1]);
	trackSegmentTextRenderers.erase(trackSegmentTextRenderers.begin() + trackSegmentTextRenderers.size() - 1);

	recalculateGondolaPathOffsets();
}

void GondolaPath::recalculateGondolaPathOffsets()
{
	glm::mat4 currentTransform(1.0f);
	for (size_t i = 0; i < trackSegments.size(); i++)
	{
		auto& segment = trackSegments[i];

		*segment.localTransform = currentTransform;
		currentTransform *= trackModelConnectionOffsets[segment.pieceType];
	}

	if (physicsComponent != nullptr)
		delete physicsComponent;

	std::vector<ModelWithTransform> modelsWithTransform;
	for (auto& segment : trackSegments)
		modelsWithTransform.push_back({ trackModels[segment.pieceType], *segment.localTransform });
	physicsComponent = new TriangleMeshCollider(this, modelsWithTransform, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);

	setTransform(getTransform());		// @NOTE: if the physicscomponent was created inside loadPropertiesFromJson after BaseObject::loadPropertiesFromJson(), then a bug would appear where the position would get reset to 0,0,0 bc the transform wasn't propagated to the physicscomponent, hence this piece of code. (aka the fix)
}
