#pragma once

#include "BaseObject.h"


class GondolaPath : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	GondolaPath();
	~GondolaPath();

	void preRenderUpdate();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void refreshResources();

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	PhysicsComponent* physicsComponent = nullptr;
	RenderComponent* renderComponent = nullptr;

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
	std::vector<glm::mat4> trackModelConnectionOffsets;

	struct TrackSegment
	{
		int pieceType = 0;
		glm::mat4* localTransform = nullptr;	// NOTE: this is the calculated ultimate value, not the defined offset! (see trackModelConnectionOffsets)
	};

	std::vector<TrackSegment> trackSegments;
	std::vector<TextRenderer*> trackSegmentTextRenderers;
	void addPieceToGondolaPath(int pieceType, int index = -1);
	void changePieceOfGondolaPath(size_t index, int pieceType);
	void removePieceOfGondolaPath(size_t index);
	void recalculateGondolaPathOffsets();
};
