#pragma once

#include "ShaderExt.h"

class ShaderExtPBR_daynight_cycle : public ShaderExt
{
public:
	ShaderExtPBR_daynight_cycle(unsigned int programId);
	void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr);

private:
	int depthBufferUniformLoc;
};