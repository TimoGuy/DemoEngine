#version 430

out vec4 fragColor;

in vec3 texCoord;

uniform samplerCube skyboxTex;

void main()
{
	vec3 envColor = texture(skyboxTex, texCoord).rgb;
    
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    fragColor = vec4(envColor, 1.0);
}