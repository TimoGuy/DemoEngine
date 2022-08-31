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


std::vector<glm::mat4> GondolaPath::trackModelConnectionOffsets;
std::vector<glm::vec3*> GondolaPath::trackPathQuadraticBezierPoints;
std::vector<float> GondolaPath::_trackPathLengths_cached;
std::vector<std::vector<glm::vec3>> GondolaPath::_trackPathBezierCurvePoints_cached;

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

	trackPathQuadraticBezierPoints.resize(trackModels.size(), nullptr);

	recalculateCachedGondolaBezierCurvePoints();

	gondolaModel = (Model*)Resources::getResource("model;gondola");
	gondolasUnderControl.clear();
	recalculateGondolaTransformFromLinearPosition();
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
}

void GondolaPath::physicsUpdate()
{
	//
	// Update all gondola movement paths
	//
	for (auto& gondola : gondolasUnderControl)
	{
		gondola.currentLinearPosition += 2.0f;
	}

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
		bool linearPosChanged = false;
		int gondolaId = 1;
		for (auto& gondola : gondolasUnderControl)
		{
			linearPosChanged |=
				ImGui::DragFloat(("Linear Position of Gondola #" + std::to_string(gondolaId)).c_str(), &gondola.currentLinearPosition);
			gondolaId++;
		}

		if (linearPosChanged)
			recalculateGondolaTransformFromLinearPosition();

		if (ImGui::Button("Create Gondola Under Control"))
		{
			GondolaModelMetadata gmm;
			gmm.animator = Animator(&gondolaModel->getAnimations());
			gmm.dummyObject = new DummyBaseObject();
			gmm.headlessRenderComponent = new RenderComponent(gmm.dummyObject);
			gmm.headlessRenderComponent->addModelToRender({ gondolaModel, true, &gmm.animator });
			gmm.headlessPhysicsComponent = new TriangleMeshCollider(gmm.dummyObject, { { gondolaModel } }, RigidActorTypes::KINEMATIC);
			gmm.currentLinearPosition = 0.0f;
			gondolasUnderControl.push_back(gmm);
		}


		ImGui::TreePop();
	}
}

void GondolaPath::imguiRender()
{
	if (showGondolaPathDebugPaths && _is_selected > 0)
	{
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
	float linearSpaceCurrentPosition = 0.0f;
	for (size_t i = 0; i < trackSegments.size(); i++)
	{
		auto& segment = trackSegments[i];

		*segment.localTransform = currentTransform;
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
		glm::vec3 point1 = glm::vec3(0, 0, 0);
		glm::vec3 midPoint = *trackPathQuadraticBezierPoints[i];
		glm::vec3 point2 = trackModelConnectionOffsets[i] * glm::vec4(0, 0, 0, 1);
		
		float t_interval = 1.0f / (float)(numSlicesBezier + 1);
		float t = t_interval;

		std::vector<glm::vec3> calculatedPoints;
		for (size_t slice = 0; slice < numSlicesBezier; slice++, t += t_interval)
		{
			glm::vec3 vec3a = PhysicsUtils::lerp(point1, midPoint, glm::vec3(t));
			glm::vec3 vec3b = PhysicsUtils::lerp(midPoint, point2, glm::vec3(t));

			calculatedPoints.push_back(
				PhysicsUtils::lerp(vec3a, vec3b, glm::vec3(t))
			);
		}
		_trackPathBezierCurvePoints_cached.push_back(calculatedPoints);

		// @NOTE: getting the track length right here cannot be just a debug view thing! I need it for the track length calculations  -Timo
		float bezierCurveLength = 0.0f;
		for (size_t ptInd = 0; ptInd < calculatedPoints.size() + 1; ptInd++)
		{
			glm::vec3 a = (ptInd == 0) ? point1 : calculatedPoints[ptInd - 1];
			glm::vec3 b = (ptInd == calculatedPoints.size()) ? point2 : calculatedPoints[ptInd];
			bezierCurveLength += glm::length(a - b);
		}
		_trackPathLengths_cached.push_back(bezierCurveLength);
	}
}

physx::PxTransform GondolaPath::getBodyTransformFromGondolaPathLinearPosition(float& linearPosition)
{
	// Auto wrapping/clamping
	if (wrapTrackSegments)
		linearPosition = fmodf(linearPosition, totalTrackLinearSpace);
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

	// Get the position and orientation of the segment it's at
	return PhysicsUtils::createTransform(
		glm::translate(getTransform() * *segment->localTransform, glm::mix(glm::vec3(0.0f), PhysicsUtils::getPosition(trackModelConnectionOffsets[segment->pieceType]), scaleValue)) *
			glm::toMat4(glm::slerp(glm::quat(), PhysicsUtils::getRotation(trackModelConnectionOffsets[segment->pieceType]), scaleValue))
	);
}

void GondolaPath::recalculateGondolaTransformFromLinearPosition()
{
	for (auto& gondola : gondolasUnderControl)
	{
		physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)gondola.headlessPhysicsComponent->getActor();
		physx::PxTransform trans = getBodyTransformFromGondolaPathLinearPosition(gondola.currentLinearPosition);
		body->setKinematicTarget(trans);
		gondola.dummyObject->INTERNALsubmitPhysicsCalculation(PhysicsUtils::physxTransformToGlmMatrix(trans));
	}
}
