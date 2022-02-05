#version 430
out vec4 fragColor;

in vec2 texCoord;
in vec2 fragLocalPosition;

uniform float padding;
uniform vec2 staminaBarExtents;
uniform float staminaAmountFilled;
uniform float staminaDepleteChaser;
uniform vec3 staminaBarColor1;
uniform vec3 staminaBarColor2;
uniform vec3 staminaBarColor3;
uniform vec3 staminaBarColor4;
uniform float staminaBarDepleteColorIntensity;

void main()
{
	if (abs(fragLocalPosition.x) > staminaBarExtents.x || abs(fragLocalPosition.y) > staminaBarExtents.y)
	{
		// Draw border
		fragColor = vec4(staminaBarColor1, 1.0);
		return;
	}
	else
	{
		// Find if in bar or not
		float staminaAmountFilledPosition = (staminaAmountFilled * 2.0 - 1.0) * staminaBarExtents.x;
		float staminaDepleteChaserPosition = (staminaDepleteChaser * 2.0 - 1.0) * staminaBarExtents.x;
		if (fragLocalPosition.x > staminaAmountFilledPosition)
		{
			if (fragLocalPosition.x > staminaDepleteChaserPosition)
			{
				// Not filled part (border color)
				fragColor = vec4(staminaBarColor1, 1.0);
				return;
			}
			else
			{
				// Deplete amount indicator area
				fragColor = vec4(staminaBarColor4 * staminaBarDepleteColorIntensity, 1.0);
				return;
			}
		}
		else
		{
			// Filled part
			fragColor = vec4(staminaBarColor3, 1.0);
			return;
		}
	}
}