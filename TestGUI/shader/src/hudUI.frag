#version 430
out vec4 fragColor;

in vec2 texCoord;
in vec2 fragLocalPosition;

const int NUM_STAMINA_WHEELS = 16;

uniform float staminaAmountFilled[NUM_STAMINA_WHEELS];
uniform float staminaDepleteChaser[NUM_STAMINA_WHEELS];
uniform vec3 staminaBarColor1;
uniform vec3 staminaBarColor2;		// Seemingly unused atm...
uniform vec3 staminaBarColor3;
uniform vec3 staminaBarColor4;
uniform float staminaBarDepleteColorIntensity;

void main()
{
	uint wheelLayer = uint(texCoord.y * NUM_STAMINA_WHEELS);

	vec3 color = staminaBarColor3;
	if (texCoord.x > staminaAmountFilled[wheelLayer])
		color = (texCoord.x < staminaDepleteChaser[wheelLayer]) ?
				staminaBarColor4 * staminaBarDepleteColorIntensity :
				staminaBarColor1;

	fragColor =
		vec4(
			color,
			(staminaDepleteChaser[wheelLayer] == 0.0) ? 0.5 : 1.0
		);
}