#pragma once

#include "ShaderExt.h"

class ShaderExtZBuffer : public ShaderExt
{
public:
	ShaderExtZBuffer(Shader* shader);
	void setupExtension();
};