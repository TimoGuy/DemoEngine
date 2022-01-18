// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_SSAO.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D depthTexture;
layout(binding = 1) uniform sampler2D rotationTexture;

uniform float zNear;
uniform float zFar;
uniform float radius;
uniform float attenScale;
uniform float distScale;


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

void main()
{
	float size = 1.0 / float(textureSize(depthTexture, 0).x);
    
	float Z     = (zFar * zNear) / (texture(depthTexture, texCoord).x * (zFar - zNear) - zFar);		// NOTE: this linearizes depth  -Timo
	float att   = 0.0;
	vec3  plane = texture(rotationTexture, texCoord * size / 4.0).xyz - vec3(1.0);
  
	for (int i = 0; i < 8; i++)
	{
		vec3  rSample = reflect(offsets[i], plane);
		float zSample = texture(depthTexture, texCoord + radius * rSample.xy / Z).x;

		zSample = (zFar * zNear) / (zSample * (zFar - zNear) - zFar);
        
		float dist = max(zSample - Z, 0.0) / distScale;
		float occl = 15.0 * max(dist * (2.0 - dist), 0.0);
        
		att += 1.0 / (1.0 + occl*occl);
	}
    
	att = clamp(att * att / 64.0 + 0.45, 0.0, 1.0) * attenScale;
	fragColor = vec4(vec3(att), 1.0);
}
