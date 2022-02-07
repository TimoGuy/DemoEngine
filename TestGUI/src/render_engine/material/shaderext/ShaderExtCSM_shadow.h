#pragma once

#include "ShaderExt.h"

class ShaderExtCSM_shadow : public ShaderExt
{
public:
	ShaderExtCSM_shadow(unsigned int programId);
	void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr);

private:
	int depthBufferUniformLoc;
};