#pragma once

#include "ShaderExt.h"

class ShaderExtZBuffer : public ShaderExt
{
public:
	ShaderExtZBuffer(unsigned int programId);
	void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr);

private:
	int depthBufferUniformLoc;
};