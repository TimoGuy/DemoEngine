#pragma once

#include "ShaderExt.h"

class ShaderExtSSAO : public ShaderExt
{
public:
	ShaderExtSSAO(Shader* shader);
	void setupExtension();
};