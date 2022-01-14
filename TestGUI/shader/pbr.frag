#version 430

out vec4 FragColor;
in vec2 texCoord;
in vec3 fragPosition;
in vec3 normalVector;

// material parameters
uniform sampler2D albedoMap;
uniform vec3 color;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
//uniform sampler2D aoMap;
uniform float ditherAlpha;
uniform float fadeAlpha;
uniform vec4 tilingAndOffset;       // NOTE: x, y are tiling, and z, w are offset

// PBR stuff        TODO: maybe pack these into a UBO that gets calculated at the beginning of the frame only (light positions and the shadow stuff eh!)
uniform samplerCube irradianceMap;
uniform samplerCube irradianceMap2;
uniform samplerCube prefilterMap;
uniform samplerCube prefilterMap2;
uniform float mapInterpolationAmt;

uniform sampler2D brdfLUT;
uniform mat3 sunSpinAmount;

// Lights
const int MAX_LIGHTS = 1024;
layout (std140, binding = 2) uniform LightInformation
{
    vec4 lightPositions[MAX_LIGHTS];
	vec4 lightDirections[MAX_LIGHTS];       // .a contains if has shadow or not
	vec4 lightColors[MAX_LIGHTS];
    vec4 viewPosition;
    ivec4 numLightsToRender;
};

// Shadow map
const int MAX_SHADOWS = 8;
uniform sampler2DArray csmShadowMap;            // NOTE: for some reason the shadow map has to be the very last???? It gets combined with the albedo if it's the first one for some reason
uniform sampler2D spotLightShadowMaps[MAX_SHADOWS];
uniform samplerCube pointLightShadowMaps[MAX_SHADOWS];
uniform float pointLightShadowFarPlanes[MAX_SHADOWS];

// CSM (Limit 1 Cascaded Shadow Map... sad day... couldn't figure out a way to have two or more csm's)
layout (std140, binding = 0) uniform LightSpaceMatrices { mat4 lightSpaceMatrices[16]; };
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1
uniform mat4 cameraView;
uniform float nearPlane;
uniform float farPlane;


const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
vec4 layerCalc(vec3 fragPosition)           // @Debug: this is for seeing which csm layer is being used
{
    // select cascade layer
    vec4 fragPosViewSpace = cameraView * vec4(fragPosition, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    if(depthValue > farPlane)
    {
        return vec4(0);
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosition, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    vec4 retColor = vec4(0);
    if (projCoords.x >= 0.0f && projCoords.x <= 1.0f && projCoords.y >= 0.0f && projCoords.y <= 1.0f)
    {
        if (layer == 0)
            retColor = vec4(.5, 0, 0, 1);
        else if (layer == 1)
            retColor = vec4(0, .5, 0, 1);
        else if (layer == 2)
            retColor = vec4(0, 0, .5, 1);
        else if (layer == 3)
            retColor = vec4(0, .3, .3, 1);
        else if (layer == 4)
            retColor = vec4(.3, .3, 0, 1);
    }

    return retColor;
}

float sampleCSMShadowMapLinear(vec2 coords, vec2 texelSize, int layer, float compare)           // https://www.youtube.com/watch?v=yn5UJzMqxj0 (29:27)
{
    vec2 pixelPos = coords / texelSize + vec2(0.5);
    vec2 fracPart = fract(pixelPos);
    vec2 startTexel = (pixelPos - fracPart - vec2(0.5)) * texelSize;                // NOTE: -vec2(0.5) fixes problem of one cascade being off from another.

    float blTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, 0.0), layer)).r);
    float brTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, 0.0), layer)).r);
    float tlTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, texelSize.y), layer)).r);
    float trTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, texelSize.y), layer)).r);
    
    float mixA = mix(blTexel, tlTexel, fracPart.y);
    float mixB = mix(brTexel, trTexel, fracPart.y);

    return mix(mixA, mixB, fracPart.x);
}

float shadowSampleCSMLayer(vec3 lightDir, int layer)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosition, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)     // NOTE: This is tuned for 1024x1024 stable shadow maps
    vec3 normal = normalize(normalVector);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float distanceMultiplier = pow(0.45, layer);

    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * distanceMultiplier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * distanceMultiplier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(csmShadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            //float pcfDepth = texture(csmShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;        // @Debug: not filtered shadows
            //shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            shadow += sampleCSMShadowMapLinear(projCoords.xy + vec2(x, y) * texelSize, texelSize, layer, currentDepth - bias);
        }
    }
    shadow /= 9.0;
    shadow = 1.0 - shadow;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
    {
        shadow = 0.0;
    }

    return shadow;
}

float shadowCalculationCSM(vec3 lightDir, vec3 fragPosition)
{
	// select cascade layer
    vec4 fragPosViewSpace = cameraView * vec4(fragPosition, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }
    if (depthValue > farPlane)                  // (TODO: This works, but I don't want it bc it can lop off more than half of the last cascade. This should probs account for the bounds of the shadow cascade (perhaps use projCoords.x and .y too????))
    {
        return 0.0;
    }

    // Fading between shadow cascades
    float fadingEdgeAmount = 2.5 * (layer + 1);
    float visibleAmount = -1;
    if (layer == cascadeCount)
        visibleAmount = clamp(farPlane - depthValue, 0.0, fadingEdgeAmount) / fadingEdgeAmount;
    else
        visibleAmount = clamp(cascadePlaneDistances[layer] - depthValue, 0.0, fadingEdgeAmount) / fadingEdgeAmount;

    // Sample the shadow map(s)
    float shadow1 = 0, shadow2 = 0;
    if (layer != cascadeCount && visibleAmount < 1.0)
        shadow2 = shadowSampleCSMLayer(lightDir, layer + 1);
    shadow1 = shadowSampleCSMLayer(lightDir, layer);

    // Mix sampled shadows       (TODO: This works, but I don't want it bc it can lop off more than half of the last cascade. This should probs account for the bounds of the shadow cascade (perhaps use projCoords.x and .y too????))
    return mix(shadow2, shadow1, visibleAmount);
}


// Array of offset direction for sampling for point light shadows
vec3 gridSamplingDisk[20] = vec3[]          // TODO: optimize this sampling disk... for sure 1,1,1 and -1,-1,-1 don't have to be there
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);


float shadowCalculationPoint(int lightIndex, int shadowIndex, vec3 fragPosition)
{
    vec3 fragToLight = fragPosition - lightPositions[lightIndex].xyz;
    float currentDepth = length(fragToLight);

    // PCF modded edition
    float shadow = 0.0;
    float bias = max((0.00065 * currentDepth * currentDepth + 0.2) * (1.0 - dot(normalize(normalVector), normalize(fragToLight))), 0.005);
    int samples = 20;
    float viewDistance = length(viewPosition.xyz - fragPosition);
    float diskRadius = (1.0 + (viewDistance / pointLightShadowFarPlanes[shadowIndex])) / 25.0;
    for (int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(pointLightShadowMaps[shadowIndex], fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= pointLightShadowFarPlanes[shadowIndex];   // undo mapping [0;1]
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / pointLightShadowFarPlanes[shadowIndex]), 1.0);    
        
    return shadow;
}


// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    float mipmapLevel = textureQueryLod(normalMap, texCoord * tilingAndOffset.xy + tilingAndOffset.zw).x;
    vec3 tangentnormalVector = textureLod(normalMap, texCoord * tilingAndOffset.xy + tilingAndOffset.zw, mipmapLevel).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fragPosition);
    vec3 Q2  = dFdy(fragPosition);
    vec2 st1 = dFdx(texCoord * tilingAndOffset.xy + tilingAndOffset.zw);
    vec2 st2 = dFdy(texCoord * tilingAndOffset.xy + tilingAndOffset.zw);

    vec3 N   = normalize(normalVector);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentnormalVector);
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
void main()
{
    if (ditherTransparency(ditherAlpha * 2.0) < 0.5)
        discard;

    vec2 adjustedTexCoord = texCoord * tilingAndOffset.xy + tilingAndOffset.zw;

    float mipmapLevel   = textureQueryLod(albedoMap, adjustedTexCoord).x;
    vec4 rawAlbedo      = textureLod(albedoMap, adjustedTexCoord, mipmapLevel);

    if (rawAlbedo.a < 0.5)      // NOTE: optimization defecit here... this is just to test out the princess eyebrows texture
        discard;
    
    vec3 albedo         = pow(rawAlbedo.rgb, vec3(2.2)) * color;

    mipmapLevel         = textureQueryLod(metallicMap, adjustedTexCoord).x;
    float metallic      = textureLod(metallicMap, adjustedTexCoord, mipmapLevel).r;
    
    mipmapLevel         = textureQueryLod(roughnessMap, adjustedTexCoord).x;
    float roughness     = textureLod(roughnessMap, adjustedTexCoord, mipmapLevel).r;
    
    //mipmapLevel       = textureQueryLod(aoMap, adjustedTexCoord).x;
    //float ao          = textureLod(aoMap, adjustedTexCoord, mipmapLevel).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(viewPosition.xyz - fragPosition);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    int shadowIndex = 0;
    for(int i = 0; i < numLightsToRender.x; ++i)      // NOTE: Change this depending on the number of lights being looped over
    {
        // calculate per-light radiance
        vec3 L, H, radiance;
        float attenuation, shadow = 0.0;

        if (length(lightDirections[i].xyz) == 0.0f)
        {
            // Point light
            const float lightAttenuationThreshold = 0.025;
            L = normalize(lightPositions[i].xyz - fragPosition);
            H = normalize(V + L);
            float distance = length(lightPositions[i].xyz - fragPosition);
            attenuation = 1.0 / (distance * distance);
            radiance = max(lightColors[i].xyz * attenuation - lightAttenuationThreshold, 0.0);

            if (lightDirections[i].a == 1)
                shadow = shadowCalculationPoint(i, shadowIndex, fragPosition);
        }
        else if (lightPositions[i].w == 0.0f)
        {
            // Directional light
            L = -lightDirections[i].xyz;                    // NOTE: this SHOULD be put into the uniform already normalized so no need to do this step here
            H = normalize(V + L);
            attenuation = 1.0f;
            radiance = lightColors[i].xyz * attenuation;

	        if (lightDirections[i].a == 1)
                shadow = shadowCalculationCSM(-L, fragPosition);
        }
        else
        {
            // Spot light (TODO)
        }

        // Bump up if shadow was used
        if (lightDirections[i].a == 1)
            shadowIndex++;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (Using the IBL tex as the ambient)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = mix(texture(irradianceMap, sunSpinAmount * N).rgb, texture(irradianceMap2, sunSpinAmount * N).rgb, mapInterpolationAmt);
    vec3 diffuse = irradiance * albedo;

    //
    // Sample pre-filter map and BRDF LUT to combine via split-sum approximation to get specular ibl
    //
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = mix(textureLod(prefilterMap, sunSpinAmount * R, roughness * MAX_REFLECTION_LOD).rgb, textureLod(prefilterMap2, sunSpinAmount * R, roughness * MAX_REFLECTION_LOD).rgb, mapInterpolationAmt);
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    //
    // Combine the colors with the shading
    //
    vec3 ambient = (kD * diffuse + specular);// * ao;
    vec3 color = ambient + Lo;

    FragColor = vec4(color, fadeAlpha);

    //FragColor = vec4(vec3(linearDepth), 1.0);

    //if (shadow < 0.5)
    //    FragColor = layerCalc(fragPosition);                         // NOTE: for debugging purposes, this shows the individual cascades
}
