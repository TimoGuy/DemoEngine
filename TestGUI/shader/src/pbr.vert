#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uvCoordinate;
layout (location=3) in ivec4 boneIds;
layout (location=4) in vec4 boneWeights;

out vec2 texCoord;
out vec3 fragPosition;
out vec3 normalVector;

//layout (std140, binding = 3) uniform RenderingMatrices		// @REFACTOR: this should be in its own extension file yo
//{
//	mat4 modelMatrix;
//	mat3 normalsModelMatrix;
//	mat4 cameraMatrix;
//};
uniform mat4 modelMatrix;  // @REFACTOR: put these in a ubo
uniform mat3 normalsModelMatrix;
uniform mat4 cameraMatrix;  // @REFACTOR: put these in a ubo

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout (std140, binding = 1) uniform FinalBoneMatrices { mat4 finalBoneMatrices[MAX_BONES]; };

void main()
{
	//
	// Do Bone Transformations (NOTE: the branching paths don't do much of a difference in performance)
	//
	mat4 boneTransform = mat4(1.0f);
	mat3 normTransform = mat3(1.0f);
	bool first = true;
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		if (boneIds[i] == -1)
			continue;
		if (boneIds[i] >= MAX_BONES)
		{
			boneTransform = mat4(1.0f);
			break;
		}
		int selectedBone = boneIds[i];

		if (!first)
		{
			// Apply bone transformation since valid bone!
			boneTransform += finalBoneMatrices[selectedBone] * boneWeights[i];
			normTransform += mat3(finalBoneMatrices[selectedBone] * boneWeights[i]);		// NOTE: I don't know if this is correct! (But it seems to be so far... maybe with non uniform scales this'll stop working???)
		}
		else
		{
			first = false;
			boneTransform = finalBoneMatrices[selectedBone] * boneWeights[i];
			normTransform = mat3(finalBoneMatrices[selectedBone] * boneWeights[i]);
		}
	}

	//
	// Prep for frag shader
	//
	normalVector = normalize(normalsModelMatrix * normTransform * normal);
	fragPosition = vec3(modelMatrix * boneTransform * vec4(vertexPosition, 1.0));
	texCoord = uvCoordinate;

	gl_Position = cameraMatrix * modelMatrix * boneTransform * vec4(vertexPosition, 1.0);			// @ShowCaitlin
}
