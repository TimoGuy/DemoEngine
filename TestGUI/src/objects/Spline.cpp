#include "Spline.h"

#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#endif


Spline::Spline()
{
    name = "Spline (unnamed)";
    m_control_points.push_back(glm::vec3(0, 0, 0));
    m_control_points.push_back(glm::vec3(0, 0, 1));
    m_total_distance_cache = calculateTotalDistance();
    m_debug_show_spline = false;
}

Spline::~Spline()
{
}

void Spline::refreshResources()
{
}

void Spline::loadPropertiesFromJson(nlohmann::json& object)
{
}

nlohmann::json Spline::savePropertiesToJson()
{
    return nlohmann::json();
}

#ifdef _DEVELOP
void Spline::imguiPropertyPanel()
{
    ImGui::Text("Control Points");
    ImGui::Checkbox("Show Debug Visualization", &m_debug_show_spline);
    for (size_t i = 0; i < m_control_points.size(); i++)
    {
        const glm::vec3 savedControlPoint = m_control_points[i];
        ImGui::DragFloat3(("Point " + std::to_string(i) + "##" + guid).c_str(), &m_control_points[i].x);

        if (savedControlPoint != m_control_points[i])
            m_total_distance_cache_dirty = true;
    }
}

void Spline::imguiRender()
{
    for (size_t i = 1; i < m_calculated_points_cache.size(); i++)
    {
        //
        // Convert to screen space
        //
        physx::PxU32 lineColor = 0xFF3300FF;

        bool willBeOnScreen = true;
        glm::vec3 pointsOnScreen[] = {
            MainLoop::getInstance().camera.PositionToClipSpace(m_calculated_points_cache[i - 1]),
            MainLoop::getInstance().camera.PositionToClipSpace(m_calculated_points_cache[i])
        };
        for (size_t ii = 0; ii < 2; ii++)
        {
            if (pointsOnScreen[ii].z < 0.0f)
            {
                // Short circuit bc it won't be on screen anymore
                willBeOnScreen = false;
                break;
            }

            pointsOnScreen[ii] /= pointsOnScreen[ii].z;
            pointsOnScreen[ii].x = ImGui::GetWindowPos().x + pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
            pointsOnScreen[ii].y = ImGui::GetWindowPos().y - pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
        }

        if (!willBeOnScreen)
            continue;

        ImVec2 point1(pointsOnScreen[0].x, pointsOnScreen[0].y);
        ImVec2 point2(pointsOnScreen[1].x, pointsOnScreen[1].y);
        ImGui::GetBackgroundDrawList()->AddLine(point1, point2, lineColor, 1.0f);
    }
}
#endif

void Spline::preRenderUpdate()
{
    if (m_total_distance_cache_dirty)
        calculateTotalDistance();
}

glm::vec3 Spline::getPositionFromLengthAlongPath(float length)
{
    // TODO: Make this, and then it should work just fine eh!
    return glm::vec3();
}

float Spline::calculateTotalDistance()
{
    if (m_control_points.size() < 2)
        return 0.0f;

    m_total_distance_cache = 1.0f;    // Reset length for getPositionFromLengthAlongPath()
    m_calculated_points_cache.clear();

    constexpr float scoochInterval = 1.0f / 1000.0f;        // 1.0 / numSamples
    float currentSamplePosition = scoochInterval;

    float totalDistance = 0.0f;
    glm::vec3 prevPosition = m_control_points[0];
    m_calculated_points_cache.push_back(prevPosition);
    
    while (currentSamplePosition <= 1.0f)
    {
        glm::vec3 nextPosition = getPositionFromLengthAlongPath(currentSamplePosition);     // NOTE: this is normalized now bc we set the distance_cache variable to 1.0
        m_calculated_points_cache.push_back(nextPosition);
        totalDistance += glm::length(nextPosition - prevPosition);

        currentSamplePosition += scoochInterval;
        prevPosition = nextPosition;
    }

    // Spit it out!
    m_total_distance_cache = totalDistance;
    m_total_distance_cache_dirty = false;
    return m_total_distance_cache;
}
