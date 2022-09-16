#pragma once

#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"
#include "../render_engine/model/animation/AnimatorStateMachine.h"


class GondolaPath : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	GondolaPath();
	~GondolaPath();

	void preRenderUpdate();
	void physicsUpdate();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void refreshResources();

#ifdef _DEVELOP
	int _is_selected = false;	// @HACK
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	PhysicsComponent* physicsComponent = nullptr;
	RenderComponent* renderComponent = nullptr;

	//
	// Gondola Paths
	//
	std::vector<Model*> trackModels;
	static std::vector<std::string> trackModelPaths;
	static std::vector<int> trackModelTypes;
	static std::vector<glm::mat4> trackModelConnectionOffsets;
	static std::vector<glm::vec3*> trackPathQuadraticBezierPoints;
	static std::vector<float> _trackPathLengths_cached;
	static std::vector<std::vector<glm::vec3>> _trackPathBezierCurvePoints_cached;

	struct TrackSegment
	{
		int pieceType = 0;
		glm::mat4* localTransform = nullptr;	// NOTE: this is the calculated ultimate value, not the defined offset! (see trackModelConnectionOffsets)
		float linearPosition = 0.0f;
	};

	std::vector<TrackSegment> trackSegments;
#ifdef _DEVELOP
	std::vector<TextRenderer*> trackSegmentTextRenderers;
#endif
	float totalTrackLinearSpace;
	bool wrapTrackSegments = true;

	struct IndividualGondolaMetadata;

	void addPieceToGondolaPath(int pieceType, int index = -1);
	void changePieceOfGondolaPath(size_t index, int pieceType);
	void removePieceOfGondolaPath(size_t index);
	void recalculateGondolaPathOffsets();
	static void recalculateCachedGondolaBezierCurvePoints();
	physx::PxTransform getBodyTransformFromGondolaPathLinearPosition(float& linearPosition, IndividualGondolaMetadata& gondola);	// The use of a reference is so that auto clamping or wrapping can be done.
	

	//
	// Gondolas
	//
	Model* gondolaModel;
	struct IndividualGondolaMetadata
	{
		Animator* animator;
		AnimatorStateMachine* animatorStateMachine;
		BaseObject* dummyObject;
		RenderComponent* headlessRenderComponent;
		PhysicsComponent* headlessPhysicsComponent;
		float currentLinearPosition;
		float movementSpeed;
		float _movementSpeedDamper = 1.0f;	// Moves from [~0.1 - 1.0]
		float _nextStoppingPointLinearPosition = -1.0f;
		float _stoppingPointWaitTimer = 0.0f;
		PhysicsTransformState bogieBackPTS, bogieFrontPTS;
	};
	std::vector<IndividualGondolaMetadata> gondolasUnderControl;

	float gondolaBogieSpacing = 61.0f;
	void createGondolaUnderControl(float linearPosition, float movementSpeed);
	void recalculateGondolaTransformFromLinearPosition();
	glm::vec4 getGondolaPathPositionAsVec4(float& linearPosition);
	static glm::vec4 getPiecePositionAsVec4(int pieceType, float scaleValue);
};
