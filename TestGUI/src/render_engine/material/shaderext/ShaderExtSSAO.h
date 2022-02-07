#pragma once

#include "ShaderExt.h"

class ShaderExtSSAO : public ShaderExt
{
public:
	ShaderExtSSAO(unsigned int programId);
	void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr);

private:
	int ssaoTextureUL, invFullResUL;
};