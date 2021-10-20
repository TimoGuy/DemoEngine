#version 430

out vec4 FragColor;
in vec2 texCoord;
in vec3 fragPosition;
in vec4 fragPositionLightSpace;
in vec3 normalVector;

// material parameters
layout (location=5) uniform vec3 zellyColor;

// PBR stuff
layout (location=11) uniform samplerCube irradianceMap;
layout (location=12) uniform samplerCube prefilterMap;
layout (location=13) uniform sampler2D brdfLUT;

// lights
const int MAX_LIGHTS = 8;
layout (location=14) uniform int numLights;
layout (location=15) uniform vec4 lightPositions[MAX_LIGHTS];            // TODO: make this separate with arrays containing directional lights, point lights, and spot lights (if we even need them), that way there doesn't need to have branching if's and we can save on gpu computation
layout (location=16) uniform vec3 lightDirections[MAX_LIGHTS];
layout (location=17) uniform vec3 lightColors[MAX_LIGHTS];

// Shadow map
layout (location=18) uniform sampler2DArray csmShadowMap;            // NOTE: for some reason the shadow map has to be the very last???? It gets combined with the albedo if it's the first one for some reason
layout (location=19) uniform sampler2D spotLightShadowMaps[MAX_LIGHTS];
layout (location=20) uniform samplerCube pointLightShadowMaps[MAX_LIGHTS];
layout (location=21) uniform float pointLightShadowFarPlanes[MAX_LIGHTS];
layout (location=22) uniform bool hasShadow[MAX_LIGHTS];

// CSM (Limit 1 Cascaded Shadow Map... sad day... couldn't figure out a way to have two or more csm's)
layout (std140, binding = 0) uniform LightSpaceMatrices { mat4 lightSpaceMatrices[16]; };
layout (location=23) uniform float cascadePlaneDistances[16];
layout (location=24) uniform int cascadeCount;   // number of frusta - 1
layout (location=25) uniform mat4 cameraView;
layout (location=26) uniform float nearPlane;
layout (location=27) uniform float farPlane;

// OTHER
layout (location=28) uniform vec3 viewPosition;


void main()
{
    FragColor = vec4(zellyColor, 1.0);
}
