#include "../Objects/BaseObject.h"
#include "../ImGui/imgui.h"

#include "PhysicsUtils.h"

#include "../ImGui/ImGuizmo.h"
#include "../MainLoop/MainLoop.h"


namespace PhysicsUtils
{
#pragma region Factory functions

	physx::PxVec3 toPxVec3(const physx::PxExtendedVec3& in)
	{
		return physx::PxVec3(in.x, in.y, in.z);
	}

	physx::PxVec3 toPxVec3(const glm::vec3& in)
	{
		return physx::PxVec3(in.x, in.y, in.z);
	}

	glm::vec3 toGLMVec3(const physx::PxVec3& in)
	{
		return glm::vec3(in.x, in.y, in.z);
	}

	glm::mat4 createGLMTransform(glm::vec3 position, glm::vec3 eulerAnglesDegrees, glm::vec3 scale)
	{
		return
			glm::translate(glm::mat4(1.0f), position) *
			glm::toMat4(glm::quat(glm::radians(eulerAnglesDegrees))) *
			glm::scale(glm::mat4(1.0f), scale);
	}

	physx::PxTransform createTransform(glm::vec3 position, glm::vec3 eulerAngles)
	{
		//
		// Do this freaky long conversion that I copied off the interwebs
		// @Broken: the transform created from this could possibly be not isSane()
		//
		glm::vec3 angles = glm::radians(eulerAngles);

		physx::PxReal cos_x = cosf(angles.x);
		physx::PxReal cos_y = cosf(angles.y);
		physx::PxReal cos_z = cosf(angles.z);

		physx::PxReal sin_x = sinf(angles.x);
		physx::PxReal sin_y = sinf(angles.y);
		physx::PxReal sin_z = sinf(angles.z);

		physx::PxQuat quat = physx::PxQuat();
		quat.w = cos_x * cos_y * cos_z + sin_x * sin_y * sin_z;
		quat.x = sin_x * cos_y * cos_z - cos_x * sin_y * sin_z;
		quat.y = cos_x * sin_y * cos_z + sin_x * cos_y * sin_z;
		quat.z = cos_x * cos_y * sin_z - sin_x * sin_y * cos_z;

		return physx::PxTransform(position.x, position.y, position.z, quat);
	}

	physx::PxTransform createTransform(glm::mat4 transform)
	{
		glm::vec3 position = getPosition(transform);
		glm::quat rotation = getRotation(transform);
		
		return physx::PxTransform(
			physx::PxVec3(position.x, position.y, position.z),
			physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)
		);
	}

	glm::mat4 physxTransformToGlmMatrix(physx::PxTransform transform)
	{
		physx::PxMat44 mat4 = physx::PxMat44(transform);
		glm::mat4 newMat;
		newMat[0][0] = mat4[0][0];
		newMat[0][1] = mat4[0][1];
		newMat[0][2] = mat4[0][2];
		newMat[0][3] = mat4[0][3];

		newMat[1][0] = mat4[1][0];
		newMat[1][1] = mat4[1][1];
		newMat[1][2] = mat4[1][2];
		newMat[1][3] = mat4[1][3];

		newMat[2][0] = mat4[2][0];
		newMat[2][1] = mat4[2][1];
		newMat[2][2] = mat4[2][2];
		newMat[2][3] = mat4[2][3];

		newMat[3][0] = mat4[3][0];
		newMat[3][1] = mat4[3][1];
		newMat[3][2] = mat4[3][2];
		newMat[3][3] = mat4[3][3];

		return newMat;
	}

	physx::PxRigidActor* createRigidActor(physx::PxPhysics* physics, physx::PxTransform transform, RigidActorTypes rigidActorType)
	{
		if (rigidActorType == RigidActorTypes::STATIC)
			return physics->createRigidStatic(transform);
		
		if (rigidActorType == RigidActorTypes::KINEMATIC)
		{
			physx::PxRigidDynamic* body = physics->createRigidDynamic(transform);
			body->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			return body;
		}

		if (rigidActorType == RigidActorTypes::DYNAMIC)
			return physics->createRigidDynamic(transform);

		assert(false);
		return nullptr;
	}

	//physx::PxBoxGeometry createBoxCollider;				// TODO: Idk if these functions would be worth it to build... let's just keep going and see if they are
	//physx::PxSphereGeometry sphereCollider;
	//physx::PxCapsuleGeometry capsuleCollider;

	physx::PxCapsuleController* createCapsuleController(
		physx::PxControllerManager* controllerManager,
		physx::PxMaterial* physicsMaterial,
		physx::PxExtendedVec3 position,
		float radius,
		float height,
		physx::PxUserControllerHitReport* hitReport,
		physx::PxControllerBehaviorCallback* behaviorReport,
		float slopeLimit,
		physx::PxVec3 upDirection)
	{
		physx::PxCapsuleControllerDesc desc;
		desc.material = physicsMaterial;
		desc.position = position;
		desc.radius = radius;
		desc.height = height;
		desc.slopeLimit = slopeLimit;
		desc.stepOffset = 0.3f;
		desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;	// @NOTE: This is better... and that is because force sliding prevents input to move side-to-side. @TODO: perhaps in the future, making a "sliding down" state would be good. This is mainly because of me adding a raycast downward to check if the controller is standing on too steep of a slope. When the controller is on a lip, the -y velocity builds up for the automatic sliding down algorithm. Another reason why, is bc if the character brushes against a steep slope in ePREVENT_ANDFORCE_SLIDING mode, then the character cannot move except in one single direction.    //ePREVENT_CLIMBING_AND_FORCE_SLIDING;
		desc.upDirection = upDirection;
		desc.reportCallback = hitReport;
		desc.behaviorCallback = behaviorReport;

		return (physx::PxCapsuleController*)controllerManager->createController(desc);
	}

	float moveTowards(float current, float target, float maxDistanceDelta)
	{
		float delta = target - current;
		return (maxDistanceDelta >= std::abs(delta)) ? target : (current + std::copysignf(1.0f, delta) * maxDistanceDelta);
	}

	float moveTowardsAngle(float currentAngle, float targetAngle, float maxTurnDelta)
	{
		float result;
		float diff = targetAngle - currentAngle;
		if (diff < -180.0f)
		{
			// Move upwards past 360
			targetAngle += 360.0f;
			result = moveTowards(currentAngle, targetAngle, maxTurnDelta);
			if (result >= 360.0f)
			{
				result -= 360.0f;
			}
		}
		else if (diff > 180.0f)
		{
			// Move downwards past 0
			targetAngle -= 360.0f;
			result = moveTowards(currentAngle, targetAngle, maxTurnDelta);
			if (result < 0.0f)
			{
				result += 360.0f;
			}
		}
		else
		{
			// Straight move
			result = moveTowards(currentAngle, targetAngle, maxTurnDelta);
		}

		return result;
	}

	glm::vec2 clampVector(glm::vec2 vector, float min, float max)
	{
		float magnitude = glm::length(vector);

		assert(std::abs(magnitude) > 0.00001f);

		return glm::normalize(vector) * std::clamp(magnitude, min, max);
	}

	bool raycast(physx::PxVec3 origin, physx::PxVec3 unitDirection, physx::PxReal distance, physx::PxRaycastBuffer& hitInfo)
	{		
		// Raycast against all static & dynamic objects (no filtering)
		// The main result from this call is the closest hit, stored in the 'hit.block' structure
		const physx::PxQueryFilterData filterData(physx::PxQueryFlag::eSTATIC);
		return MainLoop::getInstance().physicsScene->raycast(origin, unitDirection, distance, hitInfo, physx::PxHitFlag::eDEFAULT, filterData);
	}

#pragma endregion

#pragma region simple glm decomposition functions

	glm::vec3 getPosition(glm::mat4 transform)
	{
		return glm::vec3(transform[3]);
	}

	glm::quat getRotation(glm::mat4 transform)
	{
		// NOTE: when the scale gets larger, the quaternion will rotate up to however many dimensions there are, thus we have to scale down/normalize this transform to unit scale before extracting the quaternion
		glm::vec3 scale = getScale(transform);
		const glm::mat3 unitScaledRotationMatrix(
			glm::vec3(transform[0]) / scale[0],
			glm::vec3(transform[1]) / scale[1],
			glm::vec3(transform[2]) / scale[2]
		);
		return glm::quat_cast(unitScaledRotationMatrix);
	}

	glm::vec3 getScale(glm::mat4 transform)
	{
		glm::vec3 scale;
		for (int i = 0; i < 3; i++)
			scale[i] = glm::length(glm::vec3(transform[i]));
		return scale;
	}

#pragma endregion

#pragma region imgui property panel functions

	void imguiTransformMatrixProps(float* matrixPtr)
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(matrixPtr, matrixTranslation, matrixRotation, matrixScale);
		ImGui::DragFloat3("Position", matrixTranslation, 0.025f);
		ImGui::DragFloat3("Rotation", matrixRotation, 0.025f);
		ImGui::DragFloat3("Scale", matrixScale, 0.025f);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrixPtr);
	}

#pragma endregion

#pragma region imgui draw functions

	void imguiRenderBoxCollider(glm::mat4 modelMatrix, physx::PxBoxGeometry& boxGeometry, ImU32 color)
	{
		physx::PxVec3 halfExtents = boxGeometry.halfExtents;
		std::vector<glm::vec4> points = {
			modelMatrix * glm::vec4(halfExtents.x, halfExtents.y, halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(-halfExtents.x, halfExtents.y, halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(halfExtents.x, -halfExtents.y, halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(-halfExtents.x, -halfExtents.y, halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(halfExtents.x, halfExtents.y, -halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(-halfExtents.x, halfExtents.y, -halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f),
			modelMatrix * glm::vec4(-halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f),
		};
		std::vector<glm::vec3> screenSpacePoints;
		unsigned int indices[4][4] = {
			{0, 1, 2, 3},
			{4, 5, 6, 7},
			{0, 4, 2, 6},
			{1, 5, 3, 7}
		};
		for (unsigned int i = 0; i < points.size(); i++)
		{
			//
			// Convert to screen space
			//
			glm::vec3 point = glm::vec3(points[i]);
			glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
			float clipZ = pointOnScreen.z;
			pointOnScreen /= clipZ;
			pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
			pointOnScreen.z = clipZ;		// Reassign for test later
			screenSpacePoints.push_back(pointOnScreen);
		}

		for (unsigned int i = 0; i < 4; i++)
		{
			//
			// Draw quads if in the screen
			//
			glm::vec3 ssPoints[] = {
				screenSpacePoints[indices[i][0]],
				screenSpacePoints[indices[i][1]],
				screenSpacePoints[indices[i][2]],
				screenSpacePoints[indices[i][3]]
			};
			if (ssPoints[0].z < 0.0f || ssPoints[1].z < 0.0f || ssPoints[2].z < 0.0f || ssPoints[3].z < 0.0f) continue;

			ImGui::GetBackgroundDrawList()->AddQuad(
				ImVec2(ssPoints[0].x, ssPoints[0].y),
				ImVec2(ssPoints[1].x, ssPoints[1].y),
				ImVec2(ssPoints[3].x, ssPoints[3].y),
				ImVec2(ssPoints[2].x, ssPoints[2].y),
				color,
				3.0f
			);
		}
	}

	void imguiRenderSphereCollider(glm::mat4 modelMatrix, physx::PxSphereGeometry& sphereGeometry)
	{
		imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), 16);
		imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f), 16);
		imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(0.0f), 16);
	}

	void imguiRenderCapsuleCollider(glm::mat4 modelMatrix, physx::PxCapsuleGeometry& capsuleGeometry)
	{
		// Capsules by default extend along the x axis
		imguiRenderSausage(modelMatrix, capsuleGeometry.radius, capsuleGeometry.halfHeight, glm::vec3(0.0f, 0.0f, 0.0f), 16);
		imguiRenderSausage(modelMatrix, capsuleGeometry.radius, capsuleGeometry.halfHeight, glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), 16);
		imguiRenderCircle(modelMatrix, capsuleGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(-capsuleGeometry.halfHeight, 0.0f, 0.0f), 16);
		imguiRenderCircle(modelMatrix, capsuleGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(capsuleGeometry.halfHeight, 0.0f, 0.0f), 16);
	}

	void imguiRenderCharacterController(glm::mat4 modelMatrix, physx::PxCapsuleController& controller)
	{
		// Like a vertical capsule (default extends along the y axis... or whichever direction is up)
		float halfHeight = controller.getHeight() / 2.0f;
		physx::PxVec3 upDirection = controller.getUpDirection();
		modelMatrix *= glm::toMat4(glm::quat(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(upDirection.x, upDirection.y, upDirection.z)));
		imguiRenderSausage(modelMatrix, controller.getRadius(), halfHeight, glm::vec3(0.0f, 0.0f, glm::radians(90.0f)), 16);
		imguiRenderSausage(modelMatrix, controller.getRadius(), halfHeight, glm::vec3(glm::radians(90.0f), 0.0f, glm::radians(90.0f)), 16);
		imguiRenderCircle(modelMatrix, controller.getRadius(), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f, halfHeight, 0.0f), 16);
		imguiRenderCircle(modelMatrix, controller.getRadius(), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f, -halfHeight, 0.0f), 16);
	}

	void imguiRenderCircle(glm::mat4 modelMatrix, float radius, glm::vec3 eulerAngles, glm::vec3 offset, unsigned int numVertices, ImU32 color)
	{
		std::vector<glm::vec4> ringVertices;
		float angle = 0;
		for (unsigned int i = 0; i < numVertices; i++)
		{
			// Set point
			glm::vec4 point = glm::vec4(std::cosf(angle) * radius, std::sinf(angle) * radius, 0.0f, 1.0f);
			ringVertices.push_back(modelMatrix * (glm::toMat4(glm::quat(eulerAngles)) * point + glm::vec4(offset, 0.0f)));

			// Increment angle
			angle += physx::PxTwoPi / (float)numVertices;
		}

		//
		// Convert to screen space
		//
		std::vector<ImVec2> screenSpacePoints;
		for (unsigned int i = 0; i < ringVertices.size(); i++)
		{
			glm::vec3 point = glm::vec3(ringVertices[i]);
			glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
			float clipZ = pointOnScreen.z;
			if (clipZ < 0.0f) return;							// NOTE: this is to break away from any rings that are intersecting with the near space

			pointOnScreen /= clipZ;
			pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
			screenSpacePoints.push_back(ImVec2(pointOnScreen.x, pointOnScreen.y));
		}

		ImGui::GetBackgroundDrawList()->AddPolyline(
			&screenSpacePoints[0],
			(int)screenSpacePoints.size(),
			color,
			ImDrawFlags_Closed,
			3.0f
		);
	}

	void imguiRenderSausage(glm::mat4 modelMatrix, float radius, float halfHeight, glm::vec3 eulerAngles, unsigned int numVertices)					// NOTE: this is very similar to the function imguiRenderCircle
	{
		assert(numVertices % 2 == 0);		// Needs to be even number of vertices to work

		std::vector<glm::vec4> ringVertices;
		float angle = glm::radians(90.0f);
		for (unsigned int i = 0; i <= numVertices / 2; i++)
		{
			// Set point
			glm::vec4 point = glm::vec4(std::cosf(angle) * radius - halfHeight, std::sinf(angle) * radius, 0.0f, 1.0f);
			ringVertices.push_back(modelMatrix * glm::toMat4(glm::quat(eulerAngles)) * point);

			// Increment angle
			if (i < numVertices / 2)
				angle += physx::PxTwoPi / (float)numVertices;
		}

		for (unsigned int i = 0; i <= numVertices / 2; i++)
		{
			// Set point
			glm::vec4 point = glm::vec4(std::cosf(angle) * radius + halfHeight, std::sinf(angle) * radius, 0.0f, 1.0f);
			ringVertices.push_back(modelMatrix * glm::toMat4(glm::quat(eulerAngles)) * point);

			// Increment angle
			if (i < numVertices / 2)
				angle += physx::PxTwoPi / (float)numVertices;
		}

		//
		// Convert to screen space
		//
		std::vector<ImVec2> screenSpacePoints;
		for (unsigned int i = 0; i < ringVertices.size(); i++)
		{
			glm::vec3 point = glm::vec3(ringVertices[i]);
			glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
			float clipZ = pointOnScreen.z;
			if (clipZ < 0.0f) return;							// NOTE: this is to break away from any rings that are intersecting with the near space

			pointOnScreen /= clipZ;
			pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
			screenSpacePoints.push_back(ImVec2(pointOnScreen.x, pointOnScreen.y));
		}

		ImGui::GetBackgroundDrawList()->AddPolyline(
			&screenSpacePoints[0],
			(int)screenSpacePoints.size(),
			ImColor::HSV(0.39f, 0.88f, 0.92f),
			ImDrawFlags_Closed,
			3.0f
		);
	}

#pragma endregion

	Bounds fitAABB(Bounds bounds, glm::mat4 modelMatrix)
	{
		const glm::vec3 globalCenter{ modelMatrix * glm::vec4(bounds.center, 1.0f) };

		// Scaled orientation (remove position part of the matrix)
		const glm::vec3 right =		glm::vec3(glm::mat4(glm::mat3(modelMatrix)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) * bounds.extents.x;
		const glm::vec3 up =		glm::vec3(glm::mat4(glm::mat3(modelMatrix)) * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)) * bounds.extents.y;
		const glm::vec3 forward =	glm::vec3(glm::mat4(glm::mat3(modelMatrix)) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)) * bounds.extents.z;

		const float newIi =
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

		const float newIj =
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

		const float newIk =
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

		Bounds newConstructedAABB;
		newConstructedAABB.center = globalCenter;
		newConstructedAABB.extents = glm::vec3(newIi, newIj, newIk);
		return newConstructedAABB;
	}

	RaySegmentHit raySegmentCollideWithAABB(glm::vec3 start, glm::vec3 end, Bounds bounds)
	{
		const glm::vec3 delta = end - start;
		constexpr float paddingX = 0.0f;
		constexpr float paddingY = 0.0f;
		constexpr float paddingZ = 0.0f;

		const float scaleX = 1.0f / delta.x;
		const float scaleY = 1.0f / delta.y;
		const float scaleZ = 1.0f / delta.z;
		const float signX = std::copysignf(1.0f, scaleX);
		const float signY = std::copysignf(1.0f, scaleY);
		const float signZ = std::copysignf(1.0f, scaleZ);
		const float nearTimeX = (bounds.center.x - signX * (bounds.extents.x + paddingX) - start.x) * scaleX;
		const float nearTimeY = (bounds.center.y - signY * (bounds.extents.y + paddingY) - start.y) * scaleY;
		const float nearTimeZ = (bounds.center.z - signZ * (bounds.extents.z + paddingZ) - start.z) * scaleZ;
		const float farTimeX = (bounds.center.x + signX * (bounds.extents.x + paddingX) - start.x) * scaleX;
		const float farTimeY = (bounds.center.y + signY * (bounds.extents.y + paddingY) - start.y) * scaleY;
		const float farTimeZ = (bounds.center.z + signZ * (bounds.extents.z + paddingZ) - start.z) * scaleZ;

		//
		// If the closest time of collision on either axis is further than the far time on the opposite axis, we can’t be colliding
		//
		if (nearTimeX > farTimeY ||
			nearTimeX > farTimeZ ||
			nearTimeY > farTimeX ||
			nearTimeY > farTimeZ ||
			nearTimeZ > farTimeX ||
			nearTimeZ > farTimeY)
		{
			return { false };
		}

		//
		// Find the largest of the nearTimes and smallest of the farTimes
		//
		float nearTime = nearTimeX;
		if (nearTime < nearTimeY)
			nearTime = nearTimeY;
		if (nearTime < nearTimeZ)
			nearTime = nearTimeZ;

		float farTime = farTimeX;
		if (farTime > farTimeY)
			farTime = farTimeY;
		if (farTime > farTimeZ)
			farTime = farTimeZ;

		if (nearTime >= 1 || farTime <= 0)
		{
			return { false };
		}

		//
		// A collision of sorts is happening, we know now...
		// See more at https://noonat.github.io/intersect/#aabb-vs-segment
		//
		nearTime = glm::clamp(nearTime, 0.0f, 1.0f);
		glm::vec3 hitPosition = nearTime * delta + start;
		return { true, hitPosition, nearTime };
	}

	float lerp(float a, float b, float t)
	{
		return ((1.0f - t) * a) + (t * b);
	}

	float lerpAngleDegrees(float a, float b, float t)
	{
		float result;
		float diff = b - a;
		if (diff < -180.f)
		{
			// lerp upwards past 360
			b += 360.f;
			result = lerp(a, b, t);
			if (result >= 360.f)
			{
				result -= 360.f;
			}
		}
		else if (diff > 180.f)
		{
			// lerp downwards past 0
			b -= 360.f;
			result = lerp(a, b, t);
			if (result < 0.f)
			{
				result += 360.f;
			}
		}
		else
		{
			// straight lerp
			result = lerp(a, b, t);
		}

		return result;
	}
}