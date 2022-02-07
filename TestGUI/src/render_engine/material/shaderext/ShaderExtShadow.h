#pragma once

#include "ShaderExt.h"

class ShaderExtShadow : public ShaderExt		// @REFACTOR: for all extensions that use textures, the gluint texture ids should get propagated automatically from within the rendermanager
{
public:
	ShaderExtShadow(unsigned int programId);
	void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr);

	static const int MAX_SHADOWS = 8;

private:
	static unsigned int spotLightShadows[MAX_SHADOWS];
	static unsigned int pointLightShadows[MAX_SHADOWS];
	static float pointLightShadowFarPlanes[MAX_SHADOWS];

	int spotLightShadowMapsUL[MAX_SHADOWS],
		pointLightShadowMapsUL[MAX_SHADOWS],
		pointLightShadowFarPlanesUL[MAX_SHADOWS];
};