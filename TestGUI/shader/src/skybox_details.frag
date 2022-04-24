#version 430
out vec4 fragColor;

in vec3 localPos;

uniform vec3 mainCameraPosition;
uniform vec3 sunOrientation;
uniform float sunRadius;
uniform float sunAlpha;

uniform samplerCube nightSkybox;
uniform mat3 nightSkyTransform;

const vec3 sunBaseColor = vec3(1.0f,0.79f,0.43f);


vec2 rsi(vec3 r0, vec3 rd, float sr)
{
    // RSI from https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code
    float b = dot(r0, rd);
    float c = dot(r0, r0) - sr * sr;

    // Exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0 && b > 0)
        return vec2(1e5, -1e5);

    float discr = b * b - c;

    // Neg discriminant corresponds to ray missing sphere
    if (discr < 0.0)
        return vec2(1e5, -1e5);

    // Ray now found to intersect sphere, compute smallest t value of intersection
    float discrRoot = sqrt(discr);
    return vec2(max(0.0, -b - discrRoot), -b + discrRoot);
}


void main()
{
    vec3 ray_world = normalize(localPos);           // Does this work??? I guess so...  -Timo

    // TIMO
    const vec3 r0 = mainCameraPosition + vec3(0, 6376e2, 0);
    const float rPlanet = 6361e2;  // 6365e2;
    const float rAtmos  = 6465e2;
    vec3 v_Sun = -sunOrientation;

    //
    // Render the night sky
    //
    vec2 isectPlanet = rsi(r0, ray_world, rPlanet);

    // @COPYPASTA:
    // Apply the night sky texture too (reflect off the planet's ground sphere)  -Timo
    vec3 nightSkyboxSampleRay = ray_world;
    if (isectPlanet.x < isectPlanet.y)
    {
        vec3 collisionPoint = r0 + ray_world * isectPlanet.x;
        nightSkyboxSampleRay = reflect(nightSkyboxSampleRay, normalize(collisionPoint));        // @NOTE: sphere.CENTER is 0,0,0 in this case!
    }
    vec3 out_Color = texture(nightSkybox, nightSkyTransform * nightSkyboxSampleRay).rgb * 0.005;
	
	// Render sun (use sunAlpha to change if sun appears or not)
	float _sunRadius = length(normalize(ray_world) - normalize(v_Sun));
	if(_sunRadius < sunRadius)
	{
		_sunRadius /= sunRadius;
		float smoothRadius = smoothstep(0,1,0.1/_sunRadius-0.1);
		out_Color = mix(out_Color, sunBaseColor * 4, smoothRadius * sunAlpha);
	}

    fragColor = vec4(out_Color, 1);
}
