#pragma once

#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


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
	std::vector<std::string> trackModelPaths = {
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
		"model;gondola_rails_top_curve_down"
	};
	std::vector<Model*> trackModels;
	static std::vector<glm::mat4> trackModelConnectionOffsets;
	static std::vector<glm::vec3*> trackPathQuadraticBezierPoints;
	static std::vector<float> _trackPathLengths_cached;
	static std::vector<std::vector<glm::vec3>> _trackPathBezierCurvePoints_cached;

	struct TrackSegment
	{
		int pieceType = 0;
		glm::mat4* localTransform = nullptr;	// NOTE: this is the calculated ultimate value, not the defined offset! (see trackModelConnectionOffsets)
	};

	std::vector<TrackSegment> trackSegments;
#ifdef _DEVELOP
	std::vector<TextRenderer*> trackSegmentTextRenderers;
#endif
	float totalTrackLinearSpace;
	bool wrapTrackSegments = true;

	void addPieceToGondolaPath(int pieceType, int index = -1);
	void changePieceOfGondolaPath(size_t index, int pieceType);
	void removePieceOfGondolaPath(size_t index);
	void recalculateGondolaPathOffsets();
	static void recalculateCachedGondolaBezierCurvePoints();
	physx::PxTransform getBodyTransformFromGondolaPathLinearPosition(float& linearPosition);	// The use of a reference is so that auto clamping or wrapping can be done.
	

	//
	// Gondolas
	//
	Model* gondolaModel;
	struct GondolaModelMetadata
	{
		Animator animator;
		BaseObject* dummyObject;
		RenderComponent* headlessRenderComponent;
		PhysicsComponent* headlessPhysicsComponent;
		float currentLinearPosition;
		float movementSpeed;
	};
	std::vector<GondolaModelMetadata> gondolasUnderControl;

	float gondolaBogieSpacing = 61.0f;
	void createGondolaUnderControl(float linearPosition, float movementSpeed);
	void recalculateGondolaTransformFromLinearPosition();
	glm::vec4 getGondolaPathPositionAsVec4(float& linearPosition);
	static glm::vec4 getPiecePositionAsVec4(int pieceType, float scaleValue);
};