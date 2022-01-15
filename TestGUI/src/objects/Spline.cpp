#include "Spline.h"

#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#endif


Spline::Spline()
{
    name = "Spline (unnamed)";
    m_control_modules.push_back({ glm::vec3(0, 0, 0), glm::vec3(1, 0, 0) });
    m_control_modules.push_back({ glm::vec3(0, 0, 1), glm::vec3(1, 0, 0) });
    m_total_distance_cache = calculateTotalDistance();
    m_debug_show_spline = true;
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

    int deleteRequest = -1;
    for (size_t i = 0; i < m_control_modules.size(); i++)
    {
        std::string pointName = "Point " + std::to_string(i);// +"##" + guid;
        ImGui::Text(pointName.c_str());
        ImGui::SameLine();

        if (ImGui::Button("Delete"))
            deleteRequest = (int)i;

        const SplineControlModule savedControlPoint = m_control_modules[i];
        ImGui::DragFloat3(("Position (Local-Space)##" + pointName).c_str(), &m_control_modules[i].position.x);
        ImGui::DragFloat3(("Control Point (Relative to Position)##" + pointName).c_str(), &m_control_modules[i].localControlPoint.x);

        if (savedControlPoint.position != m_control_modules[i].position ||
            savedControlPoint.localControlPoint != m_control_modules[i].localControlPoint)
            m_cache_dirty = true;
    }

    if (deleteRequest >= 0)
    {
        m_control_modules.erase(m_control_modules.begin() + deleteRequest);
        m_cache_dirty = true;
    }

    if (ImGui::Button("Add Point"))
    {
        if (m_control_modules.size() > 0)
            m_control_modules.push_back(m_control_modules[m_control_modules.size() - 1]);       // Make a copy of the last one
        else
            m_control_modules.push_back({ glm::vec3(0, 0, 0), glm::vec3(1, 0, 0) });
        m_cache_dirty = true;
    }
}

void Spline::imguiRender()
{
    if (!m_debug_show_spline || m_control_modules.size() < 2)
        return;

    physx::PxU32 lineColor = 0xFF3300FF;
    std::vector<ImVec2> pointsToRender;
    for (size_t i = 0; i < m_calculated_spline_curve_cache.size(); i++)
    {
        //
        // Convert to screen space
        //
        glm::vec3 pointOnScreen =
            MainLoop::getInstance().camera.PositionToClipSpace(glm::vec3(getTransform() * glm::vec4(m_calculated_spline_curve_cache[i - 1], 1.0f)));

        if (pointOnScreen.z < 0.0f)
            continue;   // Short circuit bc it won't be on screen anymore

        pointOnScreen /= pointOnScreen.z;
        pointOnScreen.x = ImGui::GetWindowPos().x + pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
        pointOnScreen.y = ImGui::GetWindowPos().y - pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

        pointsToRender.push_back({ pointOnScreen.x, pointOnScreen.y });
    }
    ImGui::GetBackgroundDrawList()->AddPolyline(&pointsToRender[0], pointsToRender.size(), lineColor, ImDrawFlags_None, 1.0f);
}
#endif

void Spline::preRenderUpdate()
{
    if (m_cache_dirty)
        calculateTotalDistance();
}

glm::vec3 Spline::getPositionFromLengthAlongPath(float length)
{
    if (m_control_modules.size() == 1)
        return m_control_modules[0].position;
    if (m_control_modules.size() == 0)
        return glm::vec3(0.0f);

    // Get the correct length and the curveindex to use
    bool trimmed = false;
    size_t curveIndex = 0;
    while (!trimmed)
    {
        float_t currentCurveLength = 1.0f;
        if (curveIndex >= 0 && curveIndex < m_calculated_spline_curve_distances_cache.size())
            currentCurveLength = m_calculated_spline_curve_distances_cache[curveIndex];

        if (length <= currentCurveLength)
        {
            length /= currentCurveLength;
            trimmed = true;
        }
        else
            length -= currentCurveLength;
    }

    assert(curveIndex + 1 < m_control_modules.size());

    // Calculate the curve position based off length, which is now [0, 1] inclusive
    glm::vec3 controlPoints[] = {
        m_control_modules[curveIndex].position,
        m_control_modules[curveIndex].position + m_control_modules[curveIndex].localControlPoint,
        m_control_modules[curveIndex + 1].position - m_control_modules[curveIndex + 1].localControlPoint,
        m_control_modules[curveIndex + 1].position,
    };

    // Calculate the tertiary, secondary, and primary sample points
    glm::vec3 tertiaryControlPoints[] = {
        PhysicsUtils::lerp(controlPoints[0], controlPoints[1], glm::vec3(length)),
        PhysicsUtils::lerp(controlPoints[1], controlPoints[2], glm::vec3(length)),
        PhysicsUtils::lerp(controlPoints[2], controlPoints[3], glm::vec3(length)),
    };
    glm::vec3 secondaryControlPoints[] = {
        PhysicsUtils::lerp(tertiaryControlPoints[0], tertiaryControlPoints[1], glm::vec3(length)),
        PhysicsUtils::lerp(tertiaryControlPoints[1], tertiaryControlPoints[2], glm::vec3(length)),
    };
    return PhysicsUtils::lerp(secondaryControlPoints[0], secondaryControlPoints[1], glm::vec3(length));
}

float Spline::calculateTotalDistance()
{
    if (m_control_modules.size() < 2)
        return 0.0f;

    std::cout << "BEZIER: Recalculating curve...";

    m_total_distance_cache = 0;
    m_calculated_spline_curve_cache.clear();
    m_calculated_spline_curve_distances_cache.clear();    // Signal to let getPositionFromLengthAlongPath() that length needs to be 1.0 for each curve

    std::vector<float_t> calculatedDistances;
    for (size_t i = 1; i < m_control_modules.size(); i++)
    {
        constexpr float scoochInterval = 1.0f / 100.0f;        // 1.0 / numSamples
        float currentSamplePosition = scoochInterval;

        glm::vec3 prevPosition = m_control_modules[i - 1].position;
        if (i - 1 == 0)
            m_calculated_spline_curve_cache.push_back(prevPosition);     // Only log the first one, or else we get repeats for last point in curve and first point in next curve

        float segmentDistance = 0.0f;
        while (currentSamplePosition <= 1.0f)
        {
            glm::vec3 nextPosition = getPositionFromLengthAlongPath(currentSamplePosition + (float_t)i - 1.0f);     // NOTE: length of each curve is flagged to be 1.0 rn
            m_calculated_spline_curve_cache.push_back(nextPosition);
            segmentDistance += glm::length(nextPosition - prevPosition);

            currentSamplePosition += scoochInterval;
            prevPosition = nextPosition;
        }

        calculatedDistances.push_back(segmentDistance);
        m_total_distance_cache += segmentDistance;
    }

    // Spit it out!
    std::cout << "\t\tDONE!" << std::endl;
    m_calculated_spline_curve_distances_cache = calculatedDistances;
    m_cache_dirty = false;
    return m_total_distance_cache;
}
