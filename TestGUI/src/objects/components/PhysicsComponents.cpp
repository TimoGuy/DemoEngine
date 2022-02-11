#include "PhysicsComponents.h"

#include <glm/gtx/norm.hpp>
#include "../../mainloop/MainLoop.h"
#include "../../utils/PhysicsUtils.h"
#include "../../utils/GameState.h"
#include "../../render_engine/model/Model.h"
#include "../PlayerCharacter.h"

//#ifdef _DEVELOP
#include "../../utils/InputManager.h"
//#endif


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// TriangleMeshCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
TriangleMeshCollider::TriangleMeshCollider(BaseObject* bo, Model* model, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo)
{
	physx::PxU32 nbVerts = 0;
	physx::PxU32 nbIndices = 0;
	uint32_t baseIndex = 0;

	std::vector<physx::PxVec3> verts;
	std::vector<physx::PxU32> indices32;

	//
	// Extract meshes from model and conform to physx trianglemeshdesc
	//
	const std::vector<Mesh>& modelMeshes = model->getMeshes();
	for (size_t i = 0; i < modelMeshes.size(); i++)
	{
		const std::vector<Vertex>& vertices = modelMeshes[i].getVertices();
		const std::vector<uint32_t>& indices = modelMeshes[i].getIndices();

		nbVerts += (physx::PxU32)vertices.size();
		nbIndices += (physx::PxU32)indices.size();

		//
		// Add in vertices
		//
		for (size_t j = 0; j < vertices.size(); j++)
		{
			const glm::vec3 vec = vertices[j].position;
			verts.push_back(physx::PxVec3(vec.x, vec.y, vec.z));
		}

		//
		// Add in indices
		//
		for (size_t j = 0; j < indices.size(); j++)
		{
			indices32.push_back(indices[j] + baseIndex);
		}

		// Bump counter
		baseIndex += (uint32_t)vertices.size();
	}

	physx::PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = nbVerts;
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.points.data = &verts[0];
	
	meshDesc.triangles.count = nbIndices / 3;
	meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
	meshDesc.triangles.data = &indices32[0];

	physx::PxDefaultMemoryOutputStream writeBuffer;
	physx::PxTriangleMeshCookingResult::Enum result;
	bool status = MainLoop::getInstance().physicsCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status)
		return;

	physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	physx::PxTriangleMesh* triMesh = MainLoop::getInstance().physicsPhysics->createTriangleMesh(readBuffer);

	//
	// Create the rigidbody actor!
	// @TODO: start here again, this is where you take the triMesh and connect it to the actor as a shape!!!!
	//
	physx::PxTriangleMeshGeometry triGeom;
	triGeom.triangleMesh = triMesh;
	geom = &triGeom;

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	shape = physx::PxRigidActorExt::createExclusiveShape(*body, triGeom, *MainLoop::getInstance().defaultPhysicsMaterial);			// @NOTE: When the actor gets released, that's when the exclusiveshape gets released too

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	triMesh->release();
}

TriangleMeshCollider::~TriangleMeshCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void TriangleMeshCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void TriangleMeshCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	// NOTE: I guess that it doesn't matter that you recreate the shape geometry huh. (Why do I have to do it for boxCollider and Spherecollider... maybe it's the scale???)
	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);

	//
	// Resize the scale of the triangle mesh
	//
	//body->detachShape(*shape);

	// REDO THE TRI MESH!!!!
}

physx::PxTransform TriangleMeshCollider::getGlobalPose()
{
	return body->getGlobalPose();
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// BoxCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
BoxCollider::BoxCollider(BaseObject* bo, const glm::vec3& extents, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), extents(extents), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	glm::vec3 realExtents = extents * scale;

	geom = new physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	shape->release();
}

BoxCollider::~BoxCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void BoxCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void BoxCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 scale = PhysicsUtils::getScale(newTransform);
	glm::vec3 realExtents = extents * scale;

	//
	// TODO: change to: Get, then move shape
	// NOTE: apparently it doesn't work when trying to set the geometry
	//
	body->detachShape(*shape);

	geom = new physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);
	shape->release();

	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);
}

physx::PxTransform BoxCollider::getGlobalPose() { return body->getGlobalPose(); }


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// SphereCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
SphereCollider::SphereCollider(BaseObject* bo, float radius, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), radius(radius), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	geom = new physx::PxSphereGeometry(maxScale * radius);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	shape->release();
}

SphereCollider::~SphereCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void SphereCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void SphereCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 scale = PhysicsUtils::getScale(newTransform);
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	//
	// TODO: change to: Get, then move shape
	//
	body->detachShape(*shape);

	geom = new physx::PxSphereGeometry(maxScale * radius);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);
	shape->release();

	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);
}

physx::PxTransform SphereCollider::getGlobalPose() { return body->getGlobalPose(); }


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// PlayerPhysics Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
float playerCapsuleControllerRadius = 1.0f;
PlayerPhysics::PlayerPhysics(BaseObject* bo) : PhysicsComponent(bo)
{
	controller =
		PhysicsUtils::createCapsuleController(
			MainLoop::getInstance().physicsControllerManager,
			MainLoop::getInstance().defaultPhysicsMaterial,
			physx::PxExtendedVec3(0.0f, 100.0f, 0.0f),
			playerCapsuleControllerRadius,
			4.5f,
			this,		// PxUserControllerHitReport
			this//,		// PxBehaviorCallbackWhatevs
			//0.0f		// Undo the slope climbing stuff		@TODO: fix this!!!!!!
		);

	GameState::getInstance().playerActorPointer = controller->getActor();
	body = controller->getActor();

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::ENTITY;

	std::vector<physx::PxShape*> shapes;
	shapes.resize(body->getNbShapes());
	int numShapes = body->getShapes(&shapes[0], shapes.size());
	for (size_t i = 0; i < numShapes; i++)
		shapes[i]->setQueryFilterData(filterData);
}

PlayerPhysics::~PlayerPhysics()
{
	controller->release();
}

void PlayerPhysics::lockVelocity(bool yAlso)
{
	if (yAlso)
		velocity = physx::PxVec3(0);
	else
	{
		velocity.x = 0;
		velocity.z = 0;
	}
}


#ifdef _DEVELOP
bool playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;		// NOTE: just in case first frame may get messed up, this flag is set to true at first.
#endif


void PlayerPhysics::physicsUpdate()
{
	//
	// Add gravity (or sliding gravity if sliding)
	//
	velocity.y -= 4.9f * MainLoop::getInstance().physicsDeltaTime;

	physx::PxVec3 cookedVelocity = velocity;

	// Add force to go down stairs
	if (isGrounded && !isSliding)
		cookedVelocity.y = -controller->getStepOffset();

	//
	// Do the deed
	//
	physx::PxControllerCollisionFlags collisionFlags = controller->move(cookedVelocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);
	bool prevIsSliding = isSliding;
	isGrounded = false;
	isSliding = false;
	isSlidingCeiling = false;
	isSandwiching = false;
	hasValidStandingOnAngularVelocityY = false;

	// Check if collision on top and bottom
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN &&
		collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		// Move away at the average of the two normals
		glm::vec3 avgHitNormal = (groundHitNormal + ceilingHitNormal) / 2.0f;
		avgHitNormal.y = 0.0f;
		avgHitNormal = glm::normalize(avgHitNormal);

		controller->move(
			PhysicsUtils::toPxVec3(avgHitNormal * 0.1f),
			0.01f,
			MainLoop::getInstance().physicsDeltaTime,
			NULL,
			NULL
		);
	}

	// Collision on bottom
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		isGrounded = true;
		bool isFlatGround = (glm::dot(groundHitNormal, glm::vec3(0, 1, 0)) > 0.69465837f);

		// Try to retrieve grounded info (don't assign to isFlatGround if currently sliding)
		physx::PxRaycastBuffer hitInfo;
		constexpr float padding = 0.05f;
		if (PhysicsUtils::raycast(
				PhysicsUtils::toPxVec3(controller->getFootPosition()) + physx::PxVec3(0, padding, 0),
				physx::PxVec3(0, -1, 0),
				controller->getStepOffset() + padding + padding,
				hitInfo)
			&& !prevIsSliding)
			isFlatGround |= (glm::dot(PhysicsUtils::toGLMVec3(hitInfo.block.normal), glm::vec3(0, 1, 0)) > 0.69465837f);

		if (isFlatGround)
		{
			velocity.y = 0.0f;		// Remove gravity

			if (hitInfo.hasBlock && (hitInfo.block.actor->getType() & physx::PxActorType::eRIGID_DYNAMIC))		// @TODO: @REFACTOR: attach the actor via the onshapecollide() callback instead of a downwards raycast (only use as fallback)
			{
				physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)hitInfo.block.actor;
				standingOnAngularVelocityYRadians = body->getAngularVelocity().y;
				hasValidStandingOnAngularVelocityY = true;
			}
		}
		else if (velocity.y < 0.0f)
		{
			isSliding = true;		// Slide down!
			
			// Slide down normal going downhill (NOTE: This algorithm is different from the slide up normal uphill counterpart. This is bc the way the capsule controller treats these types of collisions is different. This version pushes the controller horizontally away from the wall, the uphill version redirects the upwards velocity along the normal of the wall  -Timo)
			float fallVelocity = velocity.y - offsetFootMovedReconstructed.y;
			const glm::vec3 upXnormal = glm::cross(glm::vec3(0, 1, 0), groundHitNormal);
			const glm::vec3 uxnXnormal = glm::normalize(glm::cross(upXnormal, groundHitNormal));
			const glm::vec2 slidingVectorXZ = glm::vec2(uxnXnormal.x, uxnXnormal.z) * fallVelocity / uxnXnormal.y;

			glm::vec2 pushOffVelocity(0.0f);
			glm::vec2 flatVelocity(velocity.x, velocity.z);
			if (glm::length2(flatVelocity) > 0.001f)
				pushOffVelocity = glm::min(glm::dot(glm::normalize(glm::vec2(groundHitNormal.x, groundHitNormal.z)), glm::normalize(flatVelocity)), 0.0f) * flatVelocity;

			physx::PxVec3 offsetVector =
				physx::PxVec3(
					slidingVectorXZ.x,
					fallVelocity,
					slidingVectorXZ.y
				)
				+ PhysicsUtils::toPxVec3(ceilingHitNormal * 0.1f);
			controller->move(
				offsetVector,
				0.01f,
				MainLoop::getInstance().physicsDeltaTime,
				NULL,
				NULL
			);
		}
	}

	// Collision on sides
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		//std::cout << "\tSide Collision";
	}

	// Collision on head
	if (velocity.y > 0.0f && collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		if (glm::dot(ceilingHitNormal, glm::vec3(0, -1, 0)) > 0.69465837f)
		{
			velocity.y = 0.0f;		// Hit your head on the ceiling
		}
		else if (velocity.y > 0.0f)
		{
			isSlidingCeiling = true;

			// Slide up normal going uphill (https://www.youtube.com/watch?v=GI5LAbP5slE)
			float riseVelocity = velocity.y - offsetHeadMovedReconstructed.y;
			const glm::vec3 slopeDirection = glm::vec3(0, -1, 0) - ceilingHitNormal * glm::dot(glm::vec3(0, -1, 0), ceilingHitNormal);
			const glm::vec3 moveDirection = -riseVelocity * slopeDirection;

			physx::PxVec3 offsetVector = PhysicsUtils::toPxVec3(moveDirection);
			controller->move(
				offsetVector,
				0.01f,
				MainLoop::getInstance().physicsDeltaTime,
				NULL,
				NULL
			);
			playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;		// NOTE: Prevent single-tick bounce off when ceiling is right above you. Since isGrounded's state changes at the same time you do this, then you fly away real quick  -Timo
		}
	}

	//
	// Apply transform
	//
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 trans = baseObject->getTransform();
	trans[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	baseObject->INTERNALsubmitPhysicsCalculation(trans);

	//
	// Set the velocity when leaving or landing the ground
	//
#ifdef _DEVELOP
	if (playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference)		// BLOCKING FLAG
		playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = false;
	else
#endif
	if (prevIsGrounded != isGrounded)
	{
		// Set the velocity!
		velocity = PhysicsUtils::toPxVec3(pos) - prevPositionWhileGrounded;
	
		if (isGrounded)
		{
			// Landing on the ground
			glm::vec2 flatVelocity(velocity.x, velocity.z);
			float newRunSpeed = glm::length(flatVelocity);
	
			if (newRunSpeed > 0.01f)
			{
				glm::vec2 idealFacingDirection = glm::normalize(flatVelocity);
				float facingDotIdeal = glm::dot(((PlayerCharacter*)baseObject)->facingDirection, idealFacingDirection);
				if (facingDotIdeal < 0.0f)
				{
					idealFacingDirection *= -1.0f;
					newRunSpeed *= -1.0f;
				}
	
				((PlayerCharacter*)baseObject)->facingDirection = idealFacingDirection;		// ; TODO: figger out turning speed problems!!! (MAYBE: just have there be an exponential function on turn speed. If moving a certain speed, make turn speed move towards 0 like asymptote)
				((PlayerCharacter*)baseObject)->currentRunSpeed = newRunSpeed;
			}
		}
		else
			// Leaving the ground
			velocity.y = glm::max(velocity.y, 0.0f);
	}
	prevPositionWhileGrounded = PhysicsUtils::toPxVec3(pos);		// Use this to create the velocity
	prevIsGrounded = isGrounded;

// @NOTE: Make this not show up in the release game!!!! @DEBUG only... but it's still kinda glitchy so we need it.
//#ifdef _DEVELOP
	// Set a save point for the player if press pause!
	static physx::PxExtendedVec3 savedPlayerResetPoint{ 0, 4, 0 };
	if (InputManager::getInstance().pausePressed)
	{
		savedPlayerResetPoint = controller->getFootPosition();
		std::cout << "::PLAYER:: set reset point to { " << savedPlayerResetPoint.x << ", " << savedPlayerResetPoint.y << ", " << savedPlayerResetPoint.z << " }" << std::endl;
	}

	// Reset position of player to 0, 0, 0!
	if (InputManager::getInstance().inventoryPressed)
	{
		controller->setFootPosition(savedPlayerResetPoint);
		velocity = { 0, 0, 0 };
		((PlayerCharacter*)baseObject)->currentRunSpeed = 0.0f;
	}
//#endif
}

void PlayerPhysics::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 pos = PhysicsUtils::getPosition(newTransform);
	controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));

#ifdef _DEVELOP
	playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;
#endif
}

physx::PxTransform PlayerPhysics::getGlobalPose()
{
	physx::PxExtendedVec3 pos = controller->getPosition();
	return PhysicsUtils::createTransform(glm::vec3(pos.x, pos.y, pos.z));
}

void PlayerPhysics::onShapeHit(const physx::PxControllerShapeHit& hit)
{
	if (hit.worldNormal.y >= 0.0f)
	{
		groundHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
		offsetFootMovedReconstructed =
			PhysicsUtils::toPxExtendedVec3(
				(hit.worldPos
					+ PhysicsUtils::toPxExtendedVec3(hit.worldNormal * playerCapsuleControllerRadius)
					+ physx::PxExtendedVec3(0, -playerCapsuleControllerRadius, 0))
					- controller->getFootPosition()
			);		// NOTE: this is probably getting truncated, the data types. for some reason pxextendedvec3-pxextendedvec3=pxvec3???
	}
	else
	{
		ceilingHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
		offsetHeadMovedReconstructed =
			PhysicsUtils::toPxExtendedVec3(
				(hit.worldPos
					+ PhysicsUtils::toPxExtendedVec3(hit.worldNormal * playerCapsuleControllerRadius)
					+ physx::PxExtendedVec3(0, -controller->getHeight() - playerCapsuleControllerRadius, 0))
				- controller->getFootPosition()
			);		// NOTE: this is probably getting truncated, the data types. for some reason pxextendedvec3-pxextendedvec3=pxvec3???
	}
}

void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit) { PX_UNUSED(hit); }
void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit) { PX_UNUSED(hit); }

// https://github.com/NVIDIAGameWorks/PhysX-3.4/blob/master/PhysX_3.4/Samples/SampleBridges/SampleBridgesCCT.cpp
physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor)
{
	return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;		// NOTE: the physx-provided sliding feature doesn't work so well eh.
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxController&)
{
	return physx::PxControllerBehaviorFlags(0);
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxObstacle&)
{
	return physx::PxControllerBehaviorFlags(0);
}
