#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uvCoordinate;
layout (location=3) in ivec4 boneIds;
//layout (location=4) in ivec4 boneIds2;
layout (location=4) in vec4 boneWeights;
//layout (location=6) in vec4 boneWeights2;

uniform mat4 modelMatrix;
uniform mat3 normalsModelMatrix;
uniform mat4 cameraMatrix;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;//8;
uniform mat4 finalBoneMatrices[MAX_BONES];

// JOJOJOOJO
//uniform int boneIndex;

out vec2 texCoord;
out vec3 fragPosition;		// For lighting in frag shader
out vec3 normalVector;

void main()
{
	//
	// Do Bone Transformations
	//
	mat4 boneTransform = mat4(0.0f);
	mat3 normTransform = mat3(0.0f);
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		if (boneIds[i] == -1)
			continue;
		if (boneIds[i] >= MAX_BONES)
		{
			boneTransform = mat4(1.0f);
			break;
		}

		// Apply bone transformation since valid bone!
		boneTransform += finalBoneMatrices[boneIds[i]] * boneWeights[i];
		normTransform += mat3(finalBoneMatrices[boneIds[i]] * boneWeights[i]);		// NOTE: I don't know if this is correct! (But it seems to be so far... maybe with non uniform scales this'll stop working???)
	}

	//
	// Prepare for Fragment shader
	//
	fragPosition = vec3(modelMatrix * boneTransform * vec4(vertexPosition, 1.0));
	normalVector = normalsModelMatrix * normTransform * normal;

	gl_Position = cameraMatrix * vec4(fragPosition, 1.0);
	texCoord = uvCoordinate;
}
