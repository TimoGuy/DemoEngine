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
	static std::vector<Spline*> m_all_splines;
	static Spline* getSplineFromGUID(const std::string& guid);

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

	glm::vec3 getPositionFromLengthAlongPath(float length, bool inWorldSpace = true);
	float getTotalLengthOfPath(bool inWorldSpace = true);

private:
	bool m_closed_loop;
	std::vector<SplineControlModule> m_control_modules;
	std::vector<glm::vec3> m_calculated_spline_curve_cache;
	std::vector<glm::vec3> m_calculated_spline_curve_distances_cache;
	glm::vec3 m_total_distance_cache;
	bool m_cache_dirty;

	static bool m_debug_show_spline;
	bool m_debug_edit_spline;
	bool m_debug_still_selected;

	int m_imguizmo_using_index;
	bool m_imguizmo_using_is_local_ctrl_pt;

	glm::vec3 calculateTotalDistance();
	float vec3DistanceToFloatDistance(const glm::vec3& in, bool inWorldSpace);
};
