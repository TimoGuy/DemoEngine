#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D cloudEffectBuffer;
uniform sampler2D cloudEffectHistoryBuffer;
uniform sampler2D cloudEffectDepthBuffer;
uniform vec2 invFullResolution;

uniform float cameraZNear;
uniform float cameraZFar;
uniform highp vec3 cameraDeltaPosition;
uniform mat4 currentInverseCameraProjection;
uniform mat4 currentInverseCameraView;
uniform mat4 prevCameraProjectionView;


void main()
{
	//
	// Calculate velocity from the clouds depth buffer
	//
	float z = texture(cloudEffectDepthBuffer, texCoord).x;
    if (z < 0.1 || z > cameraZFar)		// @NOTE: <0.1 cuts out the traditional geometry (maybe) and keeps the cloud depth information.... @NOTE: But we need to cut out the failed raymarches that shouldn't have a depth  -Timo
	{
		fragmentColor = vec4(0, 0, 0, 1);
		return;
	}
	z = (z - cameraZNear) / (cameraZFar - cameraZNear) * 2.0 - 1.0;		// Transform into [-1,1)

    highp vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    highp vec4 viewSpacePosition = currentInverseCameraProjection * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;   // Perspective division
    highp vec3 currentWorldSpaceFragPosition = vec3(currentInverseCameraView * viewSpacePosition);
	highp vec4 prevPosition = prevCameraProjectionView * vec4(currentWorldSpaceFragPosition - cameraDeltaPosition, 1.0);

	highp vec2 a = clipSpacePosition.xy / clipSpacePosition.w;
	a.xy = a.xy * 0.5 + 0.5;
	//a.y = 1.0 - a.y;
	highp vec2 b = prevPosition.xy / prevPosition.w;
	b.xy = b.xy * 0.5 + 0.5;
	//b.y = 1.0 - b.y;
	vec2 velocity = vec2(a - b);		// This would be on the velocity buffer if we had one for the clouds lol

	//fragmentColor = vec4(velocity * 100, 0, 1);
	//return;


	//
	// Sample the @TAA buffers
	//
	vec4 cloudEffectColor = texture(cloudEffectBuffer, texCoord).rgba;				// GL_NEAREST

	vec2 historyBufferTexCoord = texCoord - velocity;
	if (historyBufferTexCoord.x >= 0.0 && historyBufferTexCoord.y >= 0.0 &&
		historyBufferTexCoord.x <= 1.0 && historyBufferTexCoord.y <= 1.0)
	{
		vec4 cloudEffectHistory = texture(cloudEffectHistoryBuffer, historyBufferTexCoord).rgba;		// GL_LINEAR
		cloudEffectColor = mix(cloudEffectColor, cloudEffectHistory, 0.9);
	
		// Apply clamping on the history color.		@NOTE: this looks like the S word with 'dog' in front of it.
		vec3 nearColor0 = texture(cloudEffectBuffer, texCoord + vec2( 1.0,  0.0) * invFullResolution).rgb;
		vec3 nearColor1 = texture(cloudEffectBuffer, texCoord + vec2( 0.0,  1.0) * invFullResolution).rgb;
		vec3 nearColor2 = texture(cloudEffectBuffer, texCoord + vec2(-1.0,  0.0) * invFullResolution).rgb;
		vec3 nearColor3 = texture(cloudEffectBuffer, texCoord + vec2( 0.0, -1.0) * invFullResolution).rgb;
		
		vec3 boxMin = min(cloudEffectColor.rgb, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
		vec3 boxMax = max(cloudEffectColor.rgb, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));
		
		cloudEffectHistory.rgb = clamp(cloudEffectHistory.rgb, boxMin, boxMax);
	}

	fragmentColor = cloudEffectColor;
}
