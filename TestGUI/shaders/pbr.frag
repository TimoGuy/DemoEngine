#version 430

out vec4 FragColor;
in vec2 texCoord;
in vec3 fragPosition;
in vec3 normalVector;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
//uniform sampler2D aoMap;
uniform vec4 tilingAndOffset;       // NOTE: x, y are tiling, and z, w are offset

// PBR stuff        TODO: maybe pack these into a UBO that gets calculated at the beginning of the frame only (light positions and the shadow stuff eh!)
uniform samplerCube irradianceMap;
uniform samplerCube irradianceMap2;
uniform samplerCube prefilterMap;
uniform samplerCube prefilterMap2;
uniform float mapInterpolationAmt;

uniform sampler2D brdfLUT;
uniform mat3 sunSpinAmount;

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

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
/*vec4 layerCalc(vec3 fragPosition)           // @Debug: this is for seeing which csm layer is being used
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
}*/

float sampleCSMShadowMapLinear(vec2 coords, vec2 texelSize, int layer, float compare)           // https://www.youtube.com/watch?v=yn5UJzMqxj0 (29:27)
{
    vec2 pixelPos = coords / texelSize + vec2(0.5);
    vec2 fracPart = fract(pixelPos);
    vec2 startTexel = (pixelPos - fracPart) * texelSize;

    float blTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, 0.0), layer)).r);
    float brTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, 0.0), layer)).r);
    float tlTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, texelSize.y), layer)).r);
    float trTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, texelSize.y), layer)).r);
    
    float mixA = mix(blTexel, tlTexel, fracPart.y);
    float mixB = mix(brTexel, trTexel, fracPart.y);

    return mix(mixA, mixB, fracPart.x);
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
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(normalVector);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * 0.5f);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * 0.5f);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(csmShadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            //float pcfDepth = texture(csmShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
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

    
    // NOTE: Anybody who can figure out the distance fading for the shadows gets 10$ -Timo.
    //float ndc = gl_FragCoord.z * 2.0 - 1.0;
    //float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - ndc * (farPlane - nearPlane));
    //if (layer == cascadeCount)
    //{
    //    //
    //    // Create a fading edge in the far plane
    //    //
    //    //float ndc = gl_FragCoord.z * 2.0 - 1.0;
    //    //float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - ndc * (farPlane - nearPlane));
    //    
    //    float fadingEdge = 20.0;
    //    float visibleAmount = clamp(farPlane - linearDepth, 0.0, fadingEdge) / fadingEdge;
    //    shadow *= visibleAmount;
    //    //shadow = clamp(shadow, 0.0, 1.0);
    //}


    //if (shadow < 0.5)       // NOTE: this is to remove some shadow acne for further cascades
    //    return 0.0;         //              NOTE NOTE: After some thinking, we should do a separate renderbuffer to do all of the shadow rendering into the screen space, and then blur it, and then plop it onto the screen after the fact! After the blurring process we could probs do this shadow<0.5, discard dealio. This could speed up things too, maybe?
        
    return shadow;
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

float shadowCalculationPoint(int lightIndex, vec3 fragPosition)
{
    vec3 fragToLight = fragPosition - lightPositions[lightIndex].xyz;
    float currentDepth = length(fragToLight);

    // PCF modded edition
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPosition - fragPosition);
    float diskRadius = (1.0 + (viewDistance / pointLightShadowFarPlanes[lightIndex])) / 25.0;
    for (int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(pointLightShadowMaps[lightIndex], fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= pointLightShadowFarPlanes[lightIndex];   // undo mapping [0;1]
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / pointLightShadowFarPlanes[lightIndex]), 1.0);    
        
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
void main()
{
    vec2 adjustedTexCoord = texCoord * tilingAndOffset.xy + tilingAndOffset.zw;

    float mipmapLevel   = textureQueryLod(albedoMap, adjustedTexCoord).x;
    vec4 rawAlbedo      = textureLod(albedoMap, adjustedTexCoord, mipmapLevel);

    if (rawAlbedo.a < 0.5)      // NOTE: optimization defecit here... this is just to test out the princess eyebrows texture
        discard;
    
    vec3 albedo         = pow(rawAlbedo.rgb, vec3(2.2));

    mipmapLevel         = textureQueryLod(metallicMap, adjustedTexCoord).x;
    float metallic      = textureLod(metallicMap, adjustedTexCoord, mipmapLevel).r;
    
    mipmapLevel         = textureQueryLod(roughnessMap, adjustedTexCoord).x;
    float roughness     = textureLod(roughnessMap, adjustedTexCoord, mipmapLevel).r;
    
    //mipmapLevel       = textureQueryLod(aoMap, adjustedTexCoord).x;
    //float ao          = textureLod(aoMap, adjustedTexCoord, mipmapLevel).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(viewPosition - fragPosition);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < numLights; ++i)      // NOTE: Change this depending on the number of lights being looped over
    {
        // calculate per-light radiance
        vec3 L, H, radiance;
        float attenuation, shadow = 0.0;

        if (length(lightDirections[i]) == 0.0f)
        {
            // Point light
            L = normalize(lightPositions[i].xyz - fragPosition);
            H = normalize(V + L);
            float distance = length(lightPositions[i].xyz - fragPosition);
            attenuation = 1.0 / (distance * distance);
            radiance = lightColors[i] * attenuation;

            if (hasShadow[i])
                shadow = shadowCalculationPoint(i, fragPosition);
        }
        else if (lightPositions[i].w == 0.0f)
        {
            // Directional light
            L = -lightDirections[i];                    // NOTE: this SHOULD be put into the uniform already normalized so no need to do this step here
            H = normalize(V + L);
            attenuation = 1.0f;
            radiance = lightColors[i] * attenuation;

	        if (hasShadow[i])
                shadow = shadowCalculationCSM(-L, fragPosition);
        }
        else
        {
            // Spot light (TODO)
        }

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

    FragColor = vec4(color, 1.0);

    //FragColor = vec4(vec3(linearDepth), 1.0);

    //FragColor += layerCalc(fragPosition);                         // NOTE: for debugging purposes, this shows the individual cascades
}
