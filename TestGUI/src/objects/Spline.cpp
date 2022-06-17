#include "Spline.h"

#include "../mainloop/MainLoop.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../utils/PhysicsUtils.h"

#include <glm/gtc/type_ptr.hpp>

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#include "../imgui/ImGuizmo.h"
#endif


std::vector<Spline*> Spline::m_all_splines;
bool Spline::m_debug_show_spline = false;  // true;

Spline* Spline::getSplineFromGUID(const std::string& guid)
{
    for (size_t i = 0; i < m_all_splines.size(); i++)
    {
        if (m_all_splines[i]->guid == guid)
            return m_all_splines[i];
    }
    std::cout << "ERROR: SPLINE: spline with guid " << guid << " not found." << std::endl;
    assert(false);
    return nullptr;
}

Spline::Spline()
{
    name = "Spline (unnamed)";

    m_closed_loop = false;
    m_control_modules.push_back({ glm::vec3(0, 0, 0), glm::vec3(10, 0, 0) });
    m_control_modules.push_back({ glm::vec3(0, 0, 10), glm::vec3(10, 0, 0) });
    m_total_distance_cache = calculateTotalDistance();
    m_debug_edit_spline = false;
    m_debug_still_selected = false;
    m_imguizmo_using_index = -1;
    m_imguizmo_using_is_local_ctrl_pt = false;

    // Add to all_splines
    m_all_splines.push_back(this);
}

Spline::~Spline()
{
    // Remove from all_splines
    m_all_splines.erase(
        std::remove(
            m_all_splines.begin(),
            m_all_splines.end(),
            this
        ),
        m_all_splines.end()
    );
}

void Spline::refreshResources()
{
}

void Spline::loadPropertiesFromJson(nlohmann::json& object)
{
    // NOTE: "Type" is taken care of not here, but at the very beginning when the object is getting created.
    BaseObject::loadPropertiesFromJson(object["baseObject"]);

    //
    // Load Props
    //
    m_control_modules.clear();

    nlohmann::json& splineControlPoints = object["spline_control_points"];
    for (auto& splineControlPoint : splineControlPoints)
    {
        SplineControlModule scm;
        scm.position = { splineControlPoint["position"][0], splineControlPoint["position"][1], splineControlPoint["position"][2] };
        scm.localControlPoint = { splineControlPoint["local_control_point"][0], splineControlPoint["local_control_point"][1], splineControlPoint["local_control_point"][2] };
        m_control_modules.push_back(scm);
    }
    m_closed_loop = object["closed_loop"];

    // Rerun the initialization
    calculateTotalDistance();
}

nlohmann::json Spline::savePropertiesToJson()
{
    nlohmann::json j;
    j["type"] = TYPE_NAME;
    j["baseObject"] = BaseObject::savePropertiesToJson();

    //
    // Save Props
    //
    std::vector<nlohmann::json> splineControlPoints;
    for (size_t i = 0; i < m_control_modules.size(); i++)
    {
        const glm::vec3 position = m_control_modules[i].position;
        const glm::vec3 ctrlpt = m_control_modules[i].localControlPoint;
        nlohmann::json controlPoint;
        controlPoint["position"] = { position.x, position.y, position.z };
        controlPoint["local_control_point"] = { ctrlpt.x, ctrlpt.y, ctrlpt.z };
        splineControlPoints.push_back(controlPoint);
    }
    j["spline_control_points"] = splineControlPoints;
    j["closed_loop"] = m_closed_loop;

    return j;
}

#ifdef _DEVELOP
void Spline::imguiPropertyPanel()
{
    m_debug_still_selected = true;

    ImGui::Text("Control Points");

    if (ImGui::Checkbox("Is Closed Loop", &m_closed_loop))
        m_cache_dirty = true;

    ImGui::Checkbox("Show Debug Visualization", &m_debug_show_spline);
    if (m_debug_show_spline)
        ImGui::Checkbox("Edit Control Points Visually", &m_debug_edit_spline);

    int deleteRequest = -1;
    for (size_t i = 0; i < m_control_modules.size(); i++)
    {
        std::string pointName = "Point " + std::to_string(i);// +"##" + guid;
        ImGui::Text(pointName.c_str());
        ImGui::SameLine();

        if (ImGui::Button(("Delete##" + std::to_string(i)).c_str()))
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
            MainLoop::getInstance().camera.PositionToClipSpace(glm::vec3(getTransform() * glm::vec4(m_calculated_spline_curve_cache[i], 1.0f)));

        if (pointOnScreen.z < 0.0f)
            continue;   // Short circuit bc it won't be on screen anymore

        pointOnScreen /= pointOnScreen.z;
        pointOnScreen.x = ImGui::GetWindowPos().x + pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
        pointOnScreen.y = ImGui::GetWindowPos().y - pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

        pointsToRender.push_back({ pointOnScreen.x, pointOnScreen.y });
    }
    ImGui::GetBackgroundDrawList()->AddPolyline(&pointsToRender[0], pointsToRender.size(), lineColor, ImDrawFlags_None, 1.0f);

    // Draw all the spline control lines
    physx::PxU32 controlLinesColors[] = { 0xFF3DB80A, 0xFFC74B0D };
    for (size_t i = 0; i < m_control_modules.size(); i++)
    {
        //
        // Convert to screen space
        //
        glm::vec3 pointsOnScreen[] = {
            MainLoop::getInstance().camera.PositionToClipSpace(glm::vec3(getTransform() * glm::vec4(m_control_modules[i].position + m_control_modules[i].localControlPoint, 1.0f))),
            MainLoop::getInstance().camera.PositionToClipSpace(glm::vec3(getTransform() * glm::vec4(m_control_modules[i].position, 1.0f))),
            MainLoop::getInstance().camera.PositionToClipSpace(glm::vec3(getTransform() * glm::vec4(m_control_modules[i].position - m_control_modules[i].localControlPoint, 1.0f))),
        };
        ImVec2 pointsInScreenSpace[3];

        for (size_t j = 0; j < 3; j++)
        {
            if (pointsOnScreen[j].z < 0.0f)
                continue;   // Short circuit bc it won't be on screen anymore

            pointsOnScreen[j] /= pointsOnScreen[j].z;
            pointsOnScreen[j].x = ImGui::GetWindowPos().x + pointsOnScreen[j].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
            pointsOnScreen[j].y = ImGui::GetWindowPos().y - pointsOnScreen[j].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

            pointsInScreenSpace[j] = { pointsOnScreen[j].x, pointsOnScreen[j].y };
        }
        ImGui::GetBackgroundDrawList()->AddLine(pointsInScreenSpace[0], pointsInScreenSpace[1], controlLinesColors[0], 3.0f);
        ImGui::GetBackgroundDrawList()->AddLine(pointsInScreenSpace[1], pointsInScreenSpace[2], controlLinesColors[1], 3.0f);

        //
        // Manipulate the spline positions and whatnot
        //
        if (!m_debug_edit_spline || (ImGuizmo::IsUsing() && m_imguizmo_using_index != (int)i))
            continue;

        MainLoop::getInstance().renderManager->tempDisableImGuizmoManipulateForOneFrame = true;

        for (bool isLocalCtrlPt = false;; isLocalCtrlPt = true)
        {
            if (ImGuizmo::IsUsing() && m_imguizmo_using_index == (int)i && isLocalCtrlPt != m_imguizmo_using_is_local_ctrl_pt)
                if (isLocalCtrlPt)
                    break;
                else
                    continue;       // When you're using the manipulate function, only show either the objectspace position or the control point position.

            glm::mat4 transformCopy = getTransform() * glm::translate(glm::mat4(1.0f), m_control_modules[i].position + (isLocalCtrlPt ? m_control_modules[i].localControlPoint : glm::vec3(0)));
            bool changed =
                ImGuizmo::Manipulate(
                    glm::value_ptr(MainLoop::getInstance().camera.calculateViewMatrix()),
                    glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix()),
                    ImGuizmo::OPERATION::TRANSLATE,
                    ImGuizmo::MODE::LOCAL,
                    glm::value_ptr(transformCopy)
                );

            if (ImGuizmo::IsUsing())
            {
                m_imguizmo_using_index = (int)i;
                m_imguizmo_using_is_local_ctrl_pt = isLocalCtrlPt;
            }
            else
            {
                m_imguizmo_using_index = -1;
            }

            if (changed)
            {
                transformCopy = glm::inverse(getTransform()) * transformCopy;
                glm::vec3 newPosition = PhysicsUtils::getPosition(transformCopy);
                
                if (isLocalCtrlPt)
                    newPosition -= m_control_modules[i].position;

                if (isLocalCtrlPt)
                    m_control_modules[i].localControlPoint = newPosition;
                else
                    m_control_modules[i].position = newPosition;

                m_cache_dirty = true;
            }

            // Break out
            if (isLocalCtrlPt)
                break;
        }
    }
}
#endif

void Spline::preRenderUpdate()
{
    if (m_debug_still_selected)
        m_debug_still_selected = false;
    else
        m_debug_edit_spline = false;        // Release the edit mode if not selected anymore

    if (m_cache_dirty)
        calculateTotalDistance();
}

glm::vec3 Spline::getPositionFromLengthAlongPath(float length, bool inWorldSpace)
{
    if (m_control_modules.size() == 1)
        return m_control_modules[0].position;
    if (m_control_modules.size() == 0)
        return glm::vec3(0.0f);

    // Loop the input
    if (m_total_distance_cache != glm::vec3(0))
        length = glm::mod(length, getTotalLengthOfPath(inWorldSpace));

    // Get the correct length and the curveindex to use
    bool trimmed = false;
    size_t curveIndex = 0;
    while (!trimmed)
    {
        float_t currentCurveLength = 1.0f;
        if (curveIndex >= 0 && curveIndex < m_calculated_spline_curve_distances_cache.size())
            currentCurveLength = vec3DistanceToFloatDistance(m_calculated_spline_curve_distances_cache[curveIndex], inWorldSpace);

        if (length <= currentCurveLength)
        {
            length /= currentCurveLength;
            trimmed = true;
        }
        else
        {
            length -= currentCurveLength;
            curveIndex++;
        }
    }

    size_t nextCurveIndex = curveIndex + 1;
    assert(nextCurveIndex < m_control_modules.size() + (m_closed_loop ? 1 : 0));

    // Calculate the curve position based off length, which is now [0, 1] inclusive
    if (m_closed_loop && nextCurveIndex == m_control_modules.size())
        nextCurveIndex = 0;     // Loop back

    glm::vec3 controlPoints[] = {
        m_control_modules[curveIndex].position,
        m_control_modules[curveIndex].position + m_control_modules[curveIndex].localControlPoint,
        m_control_modules[nextCurveIndex].position - m_control_modules[nextCurveIndex].localControlPoint,
        m_control_modules[nextCurveIndex].position,
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
    glm::vec3 primaryControlPoint = PhysicsUtils::lerp(secondaryControlPoints[0], secondaryControlPoints[1], glm::vec3(length));

    if (inWorldSpace)
        return getTransform() * glm::vec4(primaryControlPoint, 1.0f);
    return primaryControlPoint;
}

float Spline::getTotalLengthOfPath(bool inWorldSpace)
{
    return vec3DistanceToFloatDistance(m_total_distance_cache, inWorldSpace);
}

glm::vec3 Spline::calculateTotalDistance()
{
    if (m_control_modules.size() < 2)
        return glm::vec3(0);

    //std::cout << "BEZIER: Recalculating curve...";

    m_total_distance_cache = glm::vec3(0);
    m_calculated_spline_curve_cache.clear();
    m_calculated_spline_curve_distances_cache.clear();    // Signal to let getPositionFromLengthAlongPath() that length needs to be 1.0 for each curve

    glm::vec3 prevPosition = m_control_modules[0].position;
    m_calculated_spline_curve_cache.push_back(prevPosition);     // Only log the first one, or else we get repeats for last point in curve and first point in next curve

    std::vector<glm::vec3> calculatedDistances;
    glm::vec3 totalDistance(0);
    for (size_t i = 1; i < m_control_modules.size() + (m_closed_loop ? 1 : 0); i++)
    {
        constexpr float scoochInterval = 1.0f / 100.0f;        // 1.0 / numSamples
        float currentSamplePosition = scoochInterval;

        glm::vec3 segmentDistance(0);
        while (currentSamplePosition < 1.0f)
        {
            glm::vec3 nextPosition = getPositionFromLengthAlongPath(currentSamplePosition + (float_t)i - 1.0f, false);     // NOTE: length of each curve is flagged to be 1.0 rn
            m_calculated_spline_curve_cache.push_back(nextPosition);
            segmentDistance += glm::abs(nextPosition - prevPosition);

            currentSamplePosition += scoochInterval;
            prevPosition = nextPosition;
        }

        calculatedDistances.push_back(segmentDistance);
        totalDistance += segmentDistance;
    }

    // Spit it out!
    //std::cout << "\t\tDONE!" << std::endl;
    m_calculated_spline_curve_distances_cache = calculatedDistances;
    m_total_distance_cache = totalDistance;
    m_cache_dirty = false;
    return m_total_distance_cache;
}

float Spline::vec3DistanceToFloatDistance(const glm::vec3& in, bool inWorldSpace)
{
    if (inWorldSpace)
        return glm::dot(PhysicsUtils::getScale(getTransform()), in);
    return in.x + in.y + in.z;
}
