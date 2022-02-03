/*

// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_SSAO.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D depthTexture;
layout(binding = 1) uniform sampler2D ssNormalTexture;
layout(binding = 2) uniform sampler2D rotationTexture;

uniform float zNear;
uniform float zFar;
uniform float radius;
uniform float bias;
//uniform float attenScale;
//uniform float distScale;
uniform mat4 projection;

const int KERNEL_SIZE = 64;
vec3 samples[KERNEL_SIZE];


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

vec3 getPositionFromDepth()
{
	float z = texture(depthTexture, texCoord).r;
	vec4 pos = vec4(gl_FragCoord.xy / vec2(1920.0, 1080.0) * 2.0 - 1.0, z, 1.0);
	pos = inverse(projection) * pos;
	return pos.xyz / pos.w;
}

void main()
{
	float size = 1.0 / float(textureSize(depthTexture, 0).x);

	vec3 fragPos = getPositionFromDepth();
	float Z     = (zFar * zNear) / (texture(depthTexture, texCoord).x * (zFar - zNear) - zFar);
	//fragColor = vec4(vec3(fragPos.x), 1.0);
	//return;

	vec3 normal = texture(ssNormalTexture, texCoord).rgb;
	vec3 randomVec = texture(rotationTexture, texCoord * vec2(1920.0/4.0, 1080.0/4.0)).xyz;

	vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < KERNEL_SIZE; i++)
	{
		vec3 samplePos = TBN * samples[i];
		samplePos = fragPos + samplePos * radius;

		vec4 offset = vec4(samplePos, 1.0);
		offset      = projection * offset;    // from view to clip-space
		offset.xyz /= offset.w;               // perspective divide
		offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		float sampleDepth = texture(depthTexture, offset.xy).r;
		sampleDepth = (zFar * zNear) / (sampleDepth * (zFar - zNear) - zFar);
		occlusion += (sampleDepth >= Z + bias ? 1.0 : 0.0);
	}
    
	occlusion = 1.0 - (occlusion / KERNEL_SIZE);
	fragColor = vec4(vec3(occlusion), 1.0);

	//float Z     = (zFar * zNear) / (texture(depthTexture, texCoord).x * (zFar - zNear) - zFar);		// NOTE: this linearizes depth  -Timo
	//vec3 normal = texture(ssNormalTexture, texCoord).rgb;
	//float att   = 0.0;
	//vec3  plane = texture(rotationTexture, texCoord * size / 4.0).xyz - vec3(1.0);
	//
	//vec3 tangent	= normalize(plane - normal * dot(plane, normal));
	//vec3 bitangent	= cross(normal, tangent);
	//mat3 TBN		= mat3(tangent, bitangent, normal);
	//
	//for (int i = 0; i < 8; i++)
	//{
	//	vec3 rSample = TBN * offsets[i];
	//
	//
	//
	//	//vec3  rSample = reflect(offsets[i], plane);
	//	float zSample = texture(depthTexture, texCoord + radius * rSample.xy / Z).x;
	//	
	//	zSample = (zFar * zNear) / (zSample * (zFar - zNear) - zFar);
    //    
	//	float dist = max(zSample - Z, 0.0) / distScale;
	//	float occl = 15.0 * max(dist * (2.0 - dist), 0.0);
    //    
	//	att += 1.0 / (1.0 + occl * occl);
	//}
    //
	//att = clamp(att * att / 64.0 + 0.45, 0.0, 1.0) * attenScale;
	//fragColor = vec4(vec3(att), 1.0);
}



*/

































/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
#version 430

//#extension GL_ARB_shading_language_include : enable
//#include "common.h"

// The pragma below is critical for optimal performance
// in this fragment shader to let the shader compiler
// fully optimize the maths and batch the texture fetches
// optimally

#define AO_RANDOMTEX_SIZE 4
#define M_PI 3.14159265

// tweakables
const float  NUM_STEPS = 4;
const float  NUM_DIRECTIONS = 8; // rotationTexture/g_Jitter initialization depends on this

const vec2 InvFullResolution = vec2(1.0 / 1920.0, 1.0 / 1080.0);


layout(binding=0) uniform sampler2D depthTexture;
layout(binding=1) uniform sampler2D rotationTexture;

uniform float zNear;
uniform float zFar;
uniform float powExponent;
uniform float radius;
uniform float bias;
uniform vec4 projInfo;
  
layout(location=0,index=0) out vec4 FragColor;

in vec2 texCoord;

//----------------------------------------------------------------------------------

vec3 UVToView(vec2 uv, float eye_z)
{
  return vec3((uv * projInfo.xy + projInfo.zw) * eye_z, eye_z);
}

vec3 FetchViewPos(vec2 UV)
{
  float ViewDepth = textureLod(depthTexture,UV,0).x;
  ViewDepth = ((zNear * zFar) / ((zNear - zFar) * ViewDepth + zFar));			// LINEARIZE DEPTH
  return UVToView(UV, ViewDepth);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
  vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));
  vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));
  vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));
  vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));
  return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

//----------------------------------------------------------------------------------
float Falloff(float DistanceSquare)
{
  // 1 scalar mad instruction
  float NegInvR2 = 1.0 / (radius * radius);
  return DistanceSquare * NegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(vec3 P, vec3 N, vec3 S)
{
  vec3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

  // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
  float NDotVBias = clamp(bias, 0.0, 1.0);
  return clamp(NdotV - NDotVBias, 0, 1) * clamp(Falloff(VdotV), 0, 1);
}

//----------------------------------------------------------------------------------
vec2 RotateDirection(vec2 Dir, vec2 CosSin)
{
  return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
              Dir.x*CosSin.y + Dir.y*CosSin.x);
}

//----------------------------------------------------------------------------------
vec4 GetJitter()
{
	// (cos(Alpha),sin(Alpha),rand1,rand2)
	return textureLod(rotationTexture, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE), 0);
}

//----------------------------------------------------------------------------------
float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal)
{
	// Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
	float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

	const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;
	float AO = 0;

	for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
	{
		float Angle = Alpha * DirectionIndex;

		// Compute normalized 2D direction
		vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);

		// Jitter starting sample within the first step
		float RayPixels = (Rand.z * StepSizePixels + 1.0);

		for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
		{
			vec2 SnappedUV = round(RayPixels * Direction) * InvFullResolution + FullResUV;
			vec3 S = FetchViewPos(SnappedUV);

			RayPixels += StepSizePixels;

			AO += ComputeAO(ViewPosition, ViewNormal, S);
		}
	}

	float AOMultiplier = 1.0 / (1.0 - bias);		// NOTE: bias was set to 0.1 in the example!!!!
	AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
	return clamp(1.0 - AO * 2.0,0,1);
}

//----------------------------------------------------------------------------------
void main()
{
	vec2 uv = texCoord;
	vec3 ViewPosition = FetchViewPos(uv);

	// Reconstruct view-space normal from nearest neighbors
	vec3 ViewNormal = -ReconstructNormal(uv, ViewPosition);

	// Compute projection of disk of radius control.R into screen space
	float fov = 45.0;
	float height = 1080.0;
	float projScale = float(height) / (tan(fov * 0.5) * 2.0);
	float RadiusToScreen = radius * 0.5 * projScale;
	float RadiusPixels = RadiusToScreen / ViewPosition.z;

	// Get jitter vector for the current full-res pixel
	vec4 Rand = GetJitter();

	float AO = ComputeCoarseAO(uv, RadiusPixels, Rand, ViewPosition, ViewNormal);

	FragColor = vec4(clamp(pow(AO, powExponent), 0.0, 1.0), ViewPosition.z, 0, 1);
}