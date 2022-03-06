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

uniform float ditherAlpha;


float ditherTransparency(float transparency)   // https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Dither-Node.html
{
    vec2 uv = gl_FragCoord.xy;
    float DITHER_THRESHOLDS[16] =
    {
        1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
    };
    uint index = (uint(uv.x) % 4) * 4 + uint(uv.y) % 4;
    return transparency - DITHER_THRESHOLDS[index];
}

void main()
{
	if (ditherTransparency(ditherAlpha * 2.0) < 0.5)
        discard;

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
