// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_SSAO.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D depthTexture;
layout(binding = 1) uniform sampler2D ssNormalTexture;
layout(binding = 2) uniform sampler2D rotationTexture;

uniform float zNear;
uniform float zFar;
uniform float radius;
uniform float bias;
//uniform float attenScale;
//uniform float distScale;
uniform mat4 projection;

const int KERNEL_SIZE = 64;
vec3 samples[KERNEL_SIZE];


const vec3 offsets[8] = vec3[8]
(
	vec3(-0.5, -0.5, -0.5),
	vec3( 0.5, -0.5, -0.5),
	vec3(-0.5,  0.5, -0.5),
	vec3( 0.5,  0.5, -0.5),
	vec3(-0.5, -0.5,  0.5),
	vec3( 0.5, -0.5,  0.5),
	vec3(-0.5,  0.5,  0.5),
	vec3( 0.5,  0.5,  0.5)
);

vec3 getPositionFromDepth()
{
	float z = texture(depthTexture, texCoord).r;
	vec4 pos = vec4(gl_FragCoord.xy / vec2(1920.0, 1080.0) * 2.0 - 1.0, z, 1.0);
	pos = inverse(projection) * pos;
	return pos.xyz / pos.w;
}

void main()
{
	float size = 1.0 / float(textureSize(depthTexture, 0).x);

	vec3 fragPos = getPositionFromDepth();
	float Z     = (zFar * zNear) / (texture(depthTexture, texCoord).x * (zFar - zNear) - zFar);
	//fragColor = vec4(vec3(fragPos.x), 1.0);
	//return;

	vec3 normal = texture(ssNormalTexture, texCoord).rgb;
	vec3 randomVec = texture(rotationTexture, texCoord * vec2(1920.0/4.0, 1080.0/4.0)).xyz;

	vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < KERNEL_SIZE; i++)
	{
		vec3 samplePos = TBN * samples[i];
		samplePos = fragPos + samplePos * radius;

		vec4 offset = vec4(samplePos, 1.0);
		offset      = projection * offset;    // from view to clip-space
		offset.xyz /= offset.w;               // perspective divide
		offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		float sampleDepth = texture(depthTexture, offset.xy).r;
		sampleDepth = (zFar * zNear) / (sampleDepth * (zFar - zNear) - zFar);
		occlusion += (sampleDepth >= Z + bias ? 1.0 : 0.0);
	}
    
	occlusion = 1.0 - (occlusion / KERNEL_SIZE);
	fragColor = vec4(vec3(occlusion), 1.0);

	//float Z     = (zFar * zNear) / (texture(depthTexture, texCoord).x * (zFar - zNear) - zFar);		// NOTE: this linearizes depth  -Timo
	//vec3 normal = texture(ssNormalTexture, texCoord).rgb;
	//float att   = 0.0;
	//vec3  plane = texture(rotationTexture, texCoord * size / 4.0).xyz - vec3(1.0);
	//
	//vec3 tangent	= normalize(plane - normal * dot(plane, normal));
	//vec3 bitangent	= cross(normal, tangent);
	//mat3 TBN		= mat3(tangent, bitangent, normal);
	//
	//for (int i = 0; i < 8; i++)
	//{
	//	vec3 rSample = TBN * offsets[i];
	//
	//
	//
	//	//vec3  rSample = reflect(offsets[i], plane);
	//	float zSample = texture(depthTexture, texCoord + radius * rSample.xy / Z).x;
	//	
	//	zSample = (zFar * zNear) / (zSample * (zFar - zNear) - zFar);
    //    
	//	float dist = max(zSample - Z, 0.0) / distScale;
	//	float occl = 15.0 * max(dist * (2.0 - dist), 0.0);
    //    
	//	att += 1.0 / (1.0 + occl * occl);
	//}
    //
	//att = clamp(att * att / 64.0 + 0.45, 0.0, 1.0) * attenScale;
	//fragColor = vec4(vec3(att), 1.0);
}
