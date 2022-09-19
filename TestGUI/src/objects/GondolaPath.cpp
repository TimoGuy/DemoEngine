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


std::vector<std::string> GondolaPath::trackModelPaths = {
	"model;gondola_rails_bottom_enter",
	"model;gondola_rails_bottom_exit",
	"model;gondola_rails_bottom_straight",
	"model;gondola_rails_bottom_curve_l",
	"model;gondola_rails_bottom_curve_l_xl",
	"model;gondola_rails_bottom_curve_r",
	"model;gondola_rails_bottom_curve_r_xl",
	"model;gondola_rails_top_enter",
	"model;gondola_rails_top_exit",
	"model;gondola_rails_top_straight",
	"model;gondola_rails_top_curve_left",
	"model;gondola_rails_top_curve_left_xl",
	"model;gondola_rails_top_curve_right",
	"model;gondola_rails_top_curve_right_xl",
	"model;gondola_rails_top_curve_up",
	"model;gondola_rails_top_curve_down",
	"model;gondola_sign_stop_mark",
};
std::vector<int> GondolaPath::trackModelTypes = {
	0,		// Normal Track
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	1,		// Stop at this Track for @HARDCODE amount of time
};
std::vector<glm::mat4> GondolaPath::trackModelConnectionOffsets;
std::vector<glm::vec3*> GondolaPath::trackPathQuadraticBezierPoints;
std::vector<float> GondolaPath::_trackPathLengths_cached;
std::vector<std::vector<glm::vec3>> GondolaPath::_trackPathBezierCurvePoints_cached;

GondolaPath::GondolaPath()
{
	name = "Gondola Path";

	refreshResources();
	renderComponent = new RenderComponent(this);

	//
	// Load in all the track models (NOTE: this only really has to be done once)
	//
	trackModels.clear();
	for (auto& path : trackModelPaths)
	{
		trackModels.push_back((Model*)Resources::getResource(path));
	}

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
	trackModelConnectionOffsets.push_back(glm::mat4(1.0f));

	trackModelConnectionOffsets.resize(trackModels.size(), glm::mat4(1.0f));

	trackPathQuadraticBezierPoints.clear();
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(-0.5f, 0.0f, -51.6f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(-1.9f, 0.0f, -63.3f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(0.5f, 0.0f, -51.6f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(1.9f, 0.0f, -63.3f));
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(nullptr);
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(-0.5f, 0.0f, -51.6f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(-1.9f, 0.0f, -63.3f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(0.5f, 0.0f, -51.6f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(1.9f, 0.0f, -63.3f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(0.0f, 0.0f, -53.2f));
	trackPathQuadraticBezierPoints.push_back(new glm::vec3(0.0f, 0.0f, -41.8f));
	trackPathQuadraticBezierPoints.push_back(nullptr);

	trackPathQuadraticBezierPoints.resize(trackModels.size(), nullptr);

	recalculateCachedGondolaBezierCurvePoints();

	gondolaModel = (Model*)Resources::getResource("model;gondola");
	gondolasUnderControl.clear();
	recalculateGondolaTransformFromLinearPosition();
}

GondolaPath::~GondolaPath()
{
	if (physicsComponent != nullptr)
		delete physicsComponent;
	delete renderComponent;

	for (auto& guc : gondolasUnderControl)
	{
		delete guc.headlessPhysicsComponent;
		delete guc.headlessRenderComponent;
		delete guc.dummyObject;
		delete guc.animatorStateMachine;
		delete guc.animator;
	}
}

void GondolaPath::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	refreshResources();

	if (object.contains("gondola_path_ids"))
		for (auto segmentId : object["gondola_path_ids"])
			addPieceToGondolaPath((int)segmentId);

	if (object.contains("gondolas_under_control"))
	{
		for (auto guc : object["gondolas_under_control"])
			createGondolaUnderControl(guc["linear_position"], guc["movement_speed"]);
	}

	recalculateGondolaPathOffsets();
	recalculateGondolaTransformFromLinearPosition();
}

nlohmann::json GondolaPath::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	j["gondola_path_ids"] = nlohmann::json::array();
	for (auto& segment : trackSegments)
		j["gondola_path_ids"].push_back(segment.pieceType);

	j["gondolas_under_control"] = nlohmann::json::array();
	for (auto& guc : gondolasUnderControl)
	{
		nlohmann::json guc_json;
		guc_json["linear_position"] = guc.currentLinearPosition;
		guc_json["movement_speed"] = guc.movementSpeed;
		j["gondolas_under_control"].push_back(guc_json);
	}

	return j;
}

void GondolaPath::preRenderUpdate()
{
	// Update all gondola animatorStateMachines
	for (auto& gondola : gondolasUnderControl)
	{
		gondola.animatorStateMachine->updateStateMachine(MainLoop::getInstance().deltaTime);
		
		// @NOTE: very @TEMP
		glm::vec3 bogieFrontPosition = gondola.animator->getBoneTransformation("Bogie.Front").globalTransformation * glm::vec4(0, 0, 0, 1);
		glm::vec3 bogieBackPosition = gondola.animator->getBoneTransformation("Bogie.Back").globalTransformation * glm::vec4(0, 0, 0, 1);

		gondola.animator->setBoneTransformation("Bogie.Back", glm::translate(glm::mat4(1.0f), bogieBackPosition) * glm::toMat4(glm::normalize(glm::lerp(gondola.bogieBackOrientation._calculatedNlerpOrientationA, gondola.bogieBackOrientation._calculatedNlerpOrientationB, PhysicsTransformState::interpolationAlpha))));
		gondola.animator->setBoneTransformation("Bogie.Front", glm::translate(glm::mat4(1.0f), bogieFrontPosition) * glm::toMat4(glm::normalize(glm::lerp(gondola.bogieFrontOrientation._calculatedNlerpOrientationA, gondola.bogieFrontOrientation._calculatedNlerpOrientationB, PhysicsTransformState::interpolationAlpha))));
	}

#ifdef _DEVELOP
	//
	// Update positions of the text renderers
	//
	glm::mat4 currentTransform(1.0f);
	for (size_t i = 0; i < trackSegments.size(); i++)
	{
		if (MainLoop::getInstance().playMode || _is_selected <= 0)
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

	// @HACK: flag to see if imguiPropertyPanel() was executed.
	_is_selected--;
#endif
}

void GondolaPath::physicsUpdate()
{
	//
	// Update all gondola movement paths
	//
	for (auto& gondola : gondolasUnderControl)
	{
		const static float WAIT_AT_STATION_TIME = 5.0f;
		constexpr float STOPPING_POINT_OFFSET = -50.0f;

		// Short circuit wait timer
		if (gondola._stoppingPointWaitTimer > 0.0f)
		{
			if (gondola._stoppingPointWaitTimer < 1.0f)
				gondola.animatorStateMachine->setVariable("isDoorOpen", false);

			gondola._stoppingPointWaitTimer -= MainLoop::getInstance().physicsDeltaTime;
			continue;
		}

		// Find next stopping point linear position
		float movementSpeedSign = (gondola.movementSpeed < 0.0f) ? -1.0f : 1.0f;
		float signedDistanceToNextStoppingPoint = glm::abs(gondola.movementSpeed);		// Maximum value of movement (NOTE: this is init'd to this so that if it's not changed then this will be the movement amount)
		if (gondola._nextStoppingPointLinearPosition < 0.0f)
		{
			float bestLinearDistance = -1.0f;
			while (bestLinearDistance < 0.0f)
			{
				for (auto& trackSegment : trackSegments)
				{
					if (trackModelTypes[trackSegment.pieceType] != 1)
						continue;

					float testingSignedDistance = ((trackSegment.linearPosition + STOPPING_POINT_OFFSET) - (gondola.currentLinearPosition + gondolaBogieSpacing * 0.5f)) * movementSpeedSign;
					if (testingSignedDistance <= 0.0f)
					{
						// @NOTE: @NOTE: this is the section that was intended to use wrapping train lines... but it's buggy and doesn't work... so for now I'm just gonna do simple line like structures  -Timo
						//if (wrapTrackSegments)
						//{
						//	// Retry finding the signed distance
						//	testingSignedDistance = (trackSegment.linearPosition - (gondola.currentLinearPosition - totalTrackLinearSpace)) * movementSpeedSign;		// @COPYPASTA
						//	if (testingSignedDistance <= 0.0f)
						//		continue;		// Stopping point is already behind, throw it out
						//}
						//else
						continue;		// Stopping point is already behind, throw it out
					}

					//
					// Apply passed signedDistance for the stopping track segment!
					//
					if (gondola._nextStoppingPointLinearPosition < 0.0f)
					{
						// First set... comparison fuyou
						bestLinearDistance = testingSignedDistance;
						gondola._nextStoppingPointLinearPosition = trackSegment.linearPosition + STOPPING_POINT_OFFSET;
					}
					else
					{
						// See closest signed distance to 0 without bust (<0.0f)
						if (bestLinearDistance > testingSignedDistance)
						{
							bestLinearDistance = testingSignedDistance;
							gondola._nextStoppingPointLinearPosition = trackSegment.linearPosition + STOPPING_POINT_OFFSET;
						}
					}
				}

				// Break out with the success!
				if (bestLinearDistance > 0.0f)
					break;

				// Try again flipped if couldn't find a next stopping point
				gondola.movementSpeed = -gondola.movementSpeed;
				movementSpeedSign = -movementSpeedSign;
			}
		}
		else
		{
			// Find the right signed distance to the next stopping point and control the speed
			signedDistanceToNextStoppingPoint = (gondola._nextStoppingPointLinearPosition - (gondola.currentLinearPosition + gondolaBogieSpacing * 0.5f)) * movementSpeedSign;

			if (signedDistanceToNextStoppingPoint < 0.0f)
			{
				//
				// Flag as made it to the station
				//
				gondola.currentLinearPosition = gondola._nextStoppingPointLinearPosition - gondolaBogieSpacing * 0.5f;	// Clamp to the station position
				gondola._nextStoppingPointLinearPosition = -1.0f;	// Flag this as needing recalculation
				gondola._stoppingPointWaitTimer = WAIT_AT_STATION_TIME;		// Start wait cycle
				gondola._movementSpeedDamper = 0.1f;
				gondola.animatorStateMachine->setVariable("isDoorOpen", true);
				continue;		// Short circuit
			}

			// Decrease in preparation for actual movement (this @HARDCODE value makes gondola slow down right before the station)
			signedDistanceToNextStoppingPoint = (signedDistanceToNextStoppingPoint + 5.0f) * 0.005f;
		}

		// Re-increase the speed damper if not at full speed yet
		if (gondola._movementSpeedDamper < 1.0f)
			gondola._movementSpeedDamper = glm::min(1.0f, gondola._movementSpeedDamper + MainLoop::getInstance().physicsDeltaTime);

		// Slow down when getting up to the signed distance
		gondola.currentLinearPosition += glm::clamp(signedDistanceToNextStoppingPoint, 0.0f, glm::abs(gondola.movementSpeed)) * gondola._movementSpeedDamper * movementSpeedSign;
	}

	// Convert the new calculated linear position to world transform
	recalculateGondolaTransformFromLinearPosition();
}

void GondolaPath::refreshResources()
{
}

#ifdef _DEVELOP

static bool showGondolaPathDebugPaths = false;

void GondolaPath::imguiPropertyPanel()
{
	_is_selected = 2;

	// Basic Props
	ImGui::Checkbox("Show debug paths", &showGondolaPathDebugPaths);

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

	// Bezier Curves
	ImGui::Separator();
	if (ImGui::TreeNode("Connection Path Bezier Curves"))
	{
		for (size_t i = 0; i < trackPathQuadraticBezierPoints.size(); i++)
		{
			if (trackPathQuadraticBezierPoints[i] == nullptr)
				continue;

			ImGui::Text(trackModelPaths[i].c_str());
			bool changed = ImGui::DragFloat3(("Pos##GondolaPathBezierCurvesConnectionPos" + std::to_string(i)).c_str(), &trackPathQuadraticBezierPoints[i]->x, 0.1f);
			if (changed)
			{
				recalculateCachedGondolaBezierCurvePoints();
			}
		}
	}

	// TrackSegments
	ImGui::Separator();
	if (ImGui::TreeNode("Edit track segments"))
	{
		ImGui::Checkbox("Wrap track segments", &wrapTrackSegments);
		ImGui::Separator();

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

	// Gondolas Under Control
	ImGui::Separator();
	if (ImGui::TreeNode("Edit Gondolas Under Control"))
	{
		ImGui::DragFloat("Gondola Bogie Spacing", &gondolaBogieSpacing);

		ImGui::Separator();
		ImGui::Text("Gondola Linear Positions");

		bool linearPosChanged = false;
		int gondolaId = 1;
		for (auto& gondola : gondolasUnderControl)
		{
			linearPosChanged |=
				ImGui::DragFloat(("Linear Position of Gondola #" + std::to_string(gondolaId)).c_str(), &gondola.currentLinearPosition, 0.1f);
			ImGui::DragFloat(("Speed of Gondola #" + std::to_string(gondolaId)).c_str(), &gondola.movementSpeed, 0.1f);
			gondolaId++;
		}

		if (linearPosChanged)
			recalculateGondolaTransformFromLinearPosition();

		if (ImGui::Button("Create Gondola Under Control"))
		{
			createGondolaUnderControl(0.0f, 2.0f);
			recalculateGondolaTransformFromLinearPosition();
		}


		ImGui::TreePop();
	}
}

void GondolaPath::imguiRender()
{
	if (showGondolaPathDebugPaths && _is_selected > 0)
	{
		// Draw the bezier curves
		for (size_t i = 0; i < trackSegments.size(); i++)
		{
			auto& segment = trackSegments[i];
			glm::vec3 point1 = getTransform() * *segment.localTransform * glm::vec4(0, 0, 0, 1);
			glm::vec3 point2 = getTransform() * *segment.localTransform * trackModelConnectionOffsets[segment.pieceType] * glm::vec4(0, 0, 0, 1);

			glm::vec3* bezierPt = trackPathQuadraticBezierPoints[segment.pieceType];
			if (bezierPt == nullptr)
				PhysicsUtils::imguiRenderLine(point1, point2, ImColor(1.0f, 0.0f, 0.0f));
			else
			{
				for (size_t j = 0; j < _trackPathBezierCurvePoints_cached[segment.pieceType].size() + 1; j++)
				{
					glm::vec3 linePoint1 = (j == 0) ? point1 : getTransform() * *segment.localTransform * glm::vec4(_trackPathBezierCurvePoints_cached[segment.pieceType][j - 1], 1);
					glm::vec3 linePoint2 = (j == _trackPathBezierCurvePoints_cached[segment.pieceType].size()) ? point2 : getTransform() * *segment.localTransform * glm::vec4(_trackPathBezierCurvePoints_cached[segment.pieceType][j], 1);
					PhysicsUtils::imguiRenderLine(linePoint1, linePoint2, ImColor(1.0f, 0.0f, 0.0f));
				}
			}
		}

		// Draw the bogie spacing indicators
		glm::vec3 frontBogiePoint = getTransform() * glm::vec4(0, 0, gondolaBogieSpacing * 0.5f, 1);
		glm::vec3 backBogiePoint = getTransform() * glm::vec4(0, 0, -gondolaBogieSpacing * 0.5f, 1);
		PhysicsUtils::imguiRenderLine(backBogiePoint, frontBogiePoint);
	}
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

#ifdef _DEVELOP
	trackSegmentTextRenderers.push_back(new TextRenderer{ std::to_string(trackSegmentTextRenderers.size() + 1), getTransform(), glm::vec3(0.5, 1, 0), TextAlignment::CENTER, TextAlignment::BOTTOM });
	renderComponent->addTextToRender(trackSegmentTextRenderers[trackSegmentTextRenderers.size() - 1]);
#endif

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

#ifdef _DEVELOP
	renderComponent->removeTextRenderer(trackSegmentTextRenderers[trackSegmentTextRenderers.size() - 1]);
	trackSegmentTextRenderers.erase(trackSegmentTextRenderers.begin() + trackSegmentTextRenderers.size() - 1);
#endif

	recalculateGondolaPathOffsets();
}

void GondolaPath::recalculateGondolaPathOffsets()
{
	glm::mat4 currentTransform(1.0f);
	float linearSpaceCurrentPosition = 0.0f;
	for (size_t i = 0; i < trackSegments.size(); i++)
	{
		auto& segment = trackSegments[i];

		*segment.localTransform = currentTransform;
		segment.linearPosition = linearSpaceCurrentPosition;
		currentTransform *= trackModelConnectionOffsets[segment.pieceType];
		linearSpaceCurrentPosition += _trackPathLengths_cached[segment.pieceType];
	}
	totalTrackLinearSpace = linearSpaceCurrentPosition;

	if (physicsComponent != nullptr)
		delete physicsComponent;

	std::vector<ModelWithTransform> modelsWithTransform;
	for (auto& segment : trackSegments)
		modelsWithTransform.push_back({ trackModels[segment.pieceType], *segment.localTransform });
	physicsComponent = new TriangleMeshCollider(this, modelsWithTransform, RigidActorTypes::STATIC); // RigidActorTypes::KINEMATIC);

	setTransform(getTransform());		// @NOTE: if the physicscomponent was created inside loadPropertiesFromJson after BaseObject::loadPropertiesFromJson(), then a bug would appear where the position would get reset to 0,0,0 bc the transform wasn't propagated to the physicscomponent, hence this piece of code. (aka the fix)
}

void GondolaPath::recalculateCachedGondolaBezierCurvePoints()		// @NOTE: this is really only for the debug view	@NOTE: not anymore!
{
	_trackPathLengths_cached.clear();
	_trackPathBezierCurvePoints_cached.clear();

	for (size_t i = 0; i < trackPathQuadraticBezierPoints.size(); i++)
	{
		if (trackPathQuadraticBezierPoints[i] == nullptr)
		{
			_trackPathLengths_cached.push_back(glm::length(glm::vec3(trackModelConnectionOffsets[i] * glm::vec4(0, 0, 0, 1))));
			_trackPathBezierCurvePoints_cached.push_back({});
			continue;
		}

		constexpr size_t numSlicesBezier = 15;
		
		float t_interval = 1.0f / (float)(numSlicesBezier + 1);
		float t = t_interval;

		std::vector<glm::vec3> calculatedPoints;
		for (size_t slice = 0; slice < numSlicesBezier; slice++, t += t_interval)
		{
			calculatedPoints.push_back(getPiecePositionAsVec4(i, t));
		}
		_trackPathBezierCurvePoints_cached.push_back(calculatedPoints);

		// @NOTE: getting the track length right here cannot be just a debug view thing! I need it for the track length calculations  -Timo
		float bezierCurveLength = 0.0f;
		glm::vec3 point1 = glm::vec3(0, 0, 0);
		glm::vec3 point2 = trackModelConnectionOffsets[i] * glm::vec4(0, 0, 0, 1);
		for (size_t ptInd = 0; ptInd < calculatedPoints.size() + 1; ptInd++)
		{
			glm::vec3 a = (ptInd == 0) ? point1 : calculatedPoints[ptInd - 1];
			glm::vec3 b = (ptInd == calculatedPoints.size()) ? point2 : calculatedPoints[ptInd];
			bezierCurveLength += glm::length(a - b);
		}
		_trackPathLengths_cached.push_back(bezierCurveLength);
	}
}

physx::PxTransform GondolaPath::getBodyTransformFromGondolaPathLinearPosition(float& linearPosition, IndividualGondolaMetadata& gondola)
{
	glm::vec3 thisBogiePosition = getGondolaPathPositionAsVec4(linearPosition);

	// Move the other bogie position until it ~matches~ the gondolaBogieSpacing
	constexpr float itsGoodEnoughThreshold = 0.1f;
	float scoochAmount = gondolaBogieSpacing;	// Start out with this, and then go halvsies
	float otherBogieLinearPosition = linearPosition + scoochAmount;
	float prevSignedDistanceSign = 1.0f;
	glm::vec3 otherBogiePosition;

	for (size_t _ = 0; _ < 100; _++)	// @NOTE: this acts as a timeout if you don't take 100 steps
	{
		// Test to see if the searched position is close enough to the exact answer
		otherBogiePosition = getGondolaPathPositionAsVec4(otherBogieLinearPosition);
		float signedDistanceFromTarget = gondolaBogieSpacing - glm::length(otherBogiePosition - thisBogiePosition);
		if (abs(signedDistanceFromTarget) < itsGoodEnoughThreshold)
		{
			//std::cout << "Num of calculations until good enough: " << _ << std::endl;
			break;
		}

		// Adjust the length of the search
		float newSignedDistanceSign = glm::sign(signedDistanceFromTarget);
		if (prevSignedDistanceSign != newSignedDistanceSign)
		{
			scoochAmount *= -0.5f;
			prevSignedDistanceSign = newSignedDistanceSign;
		}

		// Scooch
		otherBogieLinearPosition += scoochAmount;
	}

	// Calculate the position and orientation of this answer
	glm::vec3 bogiesMidpoint = (thisBogiePosition + otherBogiePosition) * 0.5f;
	glm::vec3 bogiesLookDirection = otherBogiePosition - thisBogiePosition;
	glm::vec3 bogiesLookDirectionXZ = glm::vec3(bogiesLookDirection.x, 0.0f, bogiesLookDirection.z);
	glm::vec3 bogiesLookDirectionXZY = glm::vec3(glm::length(bogiesLookDirectionXZ), 0.0f, bogiesLookDirection.y);
	glm::vec3 gondolaBodyRotationEuler(atan2f(bogiesLookDirectionXZY.x, bogiesLookDirectionXZY.z) - 3.1415926535f * 0.5f, atan2f(bogiesLookDirectionXZ.x, bogiesLookDirectionXZ.z), 0.0f);
	glm::mat4 gondolaBodyTransform = glm::translate(glm::mat4(1.0f), bogiesMidpoint) * glm::toMat4(glm::quat(gondolaBodyRotationEuler));

	// Update the bogie positions for the gondola's transform state
	glm::vec3 gondolaSpaceUp = gondolaBodyTransform * glm::vec4(0, 1, 0, 0);
	glm::mat4 to010WorldUp = glm::toMat4(glm::quat(gondolaSpaceUp, glm::vec3(0, 1, 0)));

	glm::vec3 bogieBackDeltaPosition = to010WorldUp * glm::vec4(thisBogiePosition - gondola.bogieBackOrientation.prevWorldSpacePosition, 1.0f);
	glm::vec3 bogieFrontDeltaPosition = to010WorldUp * glm::vec4(otherBogiePosition - gondola.bogieFrontOrientation.prevWorldSpacePosition, 1.0f);

	gondola.bogieBackOrientation.prevWorldSpacePosition = thisBogiePosition;
	gondola.bogieFrontOrientation.prevWorldSpacePosition = otherBogiePosition;

	glm::vec3 bogieBackRotationEuler = glm::vec3(0.0f, atan2f(bogieBackDeltaPosition.x, bogieBackDeltaPosition.z) - gondolaBodyRotationEuler.y, 0.0f);
	glm::vec3 bogieFrontRotationEuler = glm::vec3(0.0f, atan2f(bogieFrontDeltaPosition.x, bogieFrontDeltaPosition.z) - gondolaBodyRotationEuler.y, 0.0f);

	gondola.bogieBackOrientation._calculatedNlerpOrientationA = gondola.bogieBackOrientation._calculatedNlerpOrientationB;
	if (glm::length2(bogieBackDeltaPosition) > 0.01f)		// Removes flipping when the gondola slows down to a halt
		gondola.bogieBackOrientation._calculatedNlerpOrientationB = glm::quat(bogieBackRotationEuler);

	gondola.bogieFrontOrientation._calculatedNlerpOrientationA = gondola.bogieFrontOrientation._calculatedNlerpOrientationB;
	if (glm::length2(bogieFrontDeltaPosition) > 0.01f)		// Removes flipping when the gondola slows down to a halt
		gondola.bogieFrontOrientation._calculatedNlerpOrientationB = glm::quat(bogieFrontRotationEuler);

	// Create physics transform
	return PhysicsUtils::createTransform(gondolaBodyTransform);
}

void GondolaPath::createGondolaUnderControl(float linearPosition, float movementSpeed)
{
	IndividualGondolaMetadata gmm;
	gmm.animator = new Animator(&gondolaModel->getAnimations(), { "Bogie.Front", "Bogie.Back" });
	gmm.animatorStateMachine = new AnimatorStateMachine("gondola", gmm.animator);
	gmm.dummyObject = new DummyBaseObject();
	gmm.headlessRenderComponent = new RenderComponent(gmm.dummyObject);
	gmm.headlessRenderComponent->addModelToRender({ gondolaModel, true, gmm.animator });		// @FIXME: add in the correct gondola door animation behavior!!!
	gmm.headlessPhysicsComponent = new TriangleMeshCollider(gmm.dummyObject, { { gondolaModel } }, RigidActorTypes::KINEMATIC);
	gmm.currentLinearPosition = linearPosition;
	gmm.movementSpeed = movementSpeed;
	gondolasUnderControl.push_back(gmm);
}

void GondolaPath::recalculateGondolaTransformFromLinearPosition()
{
	for (auto& gondola : gondolasUnderControl)
	{
		physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)gondola.headlessPhysicsComponent->getActor();
		physx::PxTransform trans = getBodyTransformFromGondolaPathLinearPosition(gondola.currentLinearPosition, gondola);
		body->setKinematicTarget(trans);
		gondola.dummyObject->INTERNALsubmitPhysicsCalculation(PhysicsUtils::physxTransformToGlmMatrix(trans));
	}
}

glm::vec4 GondolaPath::getGondolaPathPositionAsVec4(float& linearPosition)
{
	// Auto wrapping/clamping
	if (wrapTrackSegments)
	{
		if (linearPosition < 0.0f)
			linearPosition += totalTrackLinearSpace;
		if (linearPosition >= totalTrackLinearSpace)
			linearPosition -= totalTrackLinearSpace;
	}
	else
		linearPosition = glm::clamp(linearPosition, 0.0f, totalTrackLinearSpace);

	// Find the current segment it's at
	size_t segmentIndex = 0;
	float localLinearPosition = linearPosition;
	while (localLinearPosition >= _trackPathLengths_cached[trackSegments[segmentIndex].pieceType])
	{
		if (segmentIndex == trackSegments.size() - 1)
			break;
		localLinearPosition -= _trackPathLengths_cached[trackSegments[segmentIndex].pieceType];
		segmentIndex++;
	}

	TrackSegment* segment = &trackSegments[segmentIndex];
	float scaleValue = localLinearPosition / _trackPathLengths_cached[segment->pieceType];

	glm::vec3 ret = getTransform() * *segment->localTransform * getPiecePositionAsVec4(segment->pieceType, scaleValue);
	return glm::vec4(ret, 1.0f);
}

glm::vec4 GondolaPath::getPiecePositionAsVec4(int pieceType, float scaleValue)
{
	size_t i = (size_t)pieceType;

	glm::vec3 point1 = glm::vec3(0, 0, 0);
	glm::vec3 point2 = trackModelConnectionOffsets[i] * glm::vec4(0, 0, 0, 1);

	if (trackPathQuadraticBezierPoints[i] == nullptr)
	{
		// Get lerp of just the linear line
		return glm::vec4(PhysicsUtils::lerp(point1, point2, glm::vec3(scaleValue)), 1.0f);
	}
	else
	{
		// This is a bezier curve
		glm::vec3 midPoint = *trackPathQuadraticBezierPoints[i];
		glm::vec3 vec3a = PhysicsUtils::lerp(point1, midPoint, glm::vec3(scaleValue));
		glm::vec3 vec3b = PhysicsUtils::lerp(midPoint, point2, glm::vec3(scaleValue));

		return glm::vec4(PhysicsUtils::lerp(vec3a, vec3b, glm::vec3(scaleValue)), 1.0f);
	}
}
