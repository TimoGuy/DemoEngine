#version 430

out vec4 FragColor;
in vec2 texCoord;
in vec3 fragPosition;
in vec3 normalVector;

// material parameters
uniform vec3 zellyColor;

// PBR stuff
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// lights
const int MAX_LIGHTS = 8;
uniform int numLights;
uniform vec4 lightPositions[MAX_LIGHTS];            // TODO: make this separate with arrays containing directional lights, point lights, and spot lights (if we even need them), that way there doesn't need to have branching if's and we can save on gpu computation
uniform vec3 lightDirections[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];

// Shadow map
uniform sampler2DArray csmShadowMap;            // NOTE: for some reason the shadow map has to be the very last???? It gets combined with the albedo if it's the first one for some reason
uniform sampler2D spotLightShadowMaps[MAX_LIGHTS];
uniform samplerCube pointLightShadowMaps[MAX_LIGHTS];
uniform float pointLightShadowFarPlanes[MAX_LIGHTS];
uniform bool hasShadow[MAX_LIGHTS];

// CSM (Limit 1 Cascaded Shadow Map... sad day... couldn't figure out a way to have two or more csm's)
layout (std140, binding = 0) uniform LightSpaceMatrices { mat4 lightSpaceMatrices[16]; };
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1
uniform mat4 cameraView;
uniform float nearPlane;
uniform float farPlane;

// OTHER
layout (location=28) uniform vec3 viewPosition;


void main()
{
    FragColor = vec4(zellyColor, 1.0);
}
