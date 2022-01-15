#pragma once

#include "BaseObject.h"


struct SplineControlModule
{
	glm::vec3 position;
	glm::vec3 localControlPoint;		// NOTE: to get the mirrored version of this, multiply by -1
};

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
	std::vector<SplineControlModule> m_control_modules;
	std::vector<glm::vec3> m_calculated_spline_curve_cache;
	std::vector<float_t> m_calculated_spline_curve_distances_cache;
	float m_total_distance_cache;
	bool m_cache_dirty;
	bool m_debug_show_spline;

	float calculateTotalDistance();
};

