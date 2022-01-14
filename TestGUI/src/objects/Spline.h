#pragma once

#include "BaseObject.h"


//
// NOTE: this is more of an editor tool right here.
//		You use it to assign a moving platform to.
//
class Spline : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	Spline();
	~Spline();

	void refreshResources();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return nullptr; }

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

	void preRenderUpdate();

	glm::vec3 getPositionFromLengthAlongPath(float length);
	inline float getTotalLengthOfPath() { return m_total_distance_cache; }

private:
	std::vector<glm::vec3> m_control_points;
	std::vector<glm::vec3> m_calculated_points_cache;
	float m_total_distance_cache;
	bool m_total_distance_cache_dirty;
	bool m_debug_show_spline;

	float calculateTotalDistance();
};

