#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uvCoordinate;
layout (location=3) in ivec4 boneIds;
layout (location=4) in vec4 boneWeights;

uniform mat4 modelMatrix;
uniform mat3 normalsModelMatrix;
uniform mat4 cameraMatrix;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBoneMatrices[MAX_BONES];

out vec2 texCoord;
out vec3 fragPosition;		// For lighting in frag shader
out vec3 normalVector;

void main()
{
	//
	// Do Bone Transformations
	//
	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		if (boneIds[i] == -1)
			continue;
		if (boneIds[i] >= MAX_BONES)
		{
			totalPosition = vec4(vertexPosition, 1.0f);
			break;
		}

		// Apply bone transformation since valid bone!
		vec4 localPosition = finalBoneMatrices[boneIds[i]] * vec4(vertexPosition, 1.0f);
		totalPosition += localPosition * boneWeights[i];

		vec3 localNormal = mat3(finalBoneMatrices[boneIds[i]]) * normal;
		totalNormal += localNormal * boneWeights[i];		// NOTE: I don't know if this is correct!
	}

	//
	// Prepare for Fragment shader
	//
	fragPosition = vec3(modelMatrix * totalPosition);
	normalVector = normalsModelMatrix * normalize(totalNormal);

	gl_Position = cameraMatrix * vec4(fragPosition, 1.0);
	texCoord = uvCoordinate;
}