#pragma once

class Shader;

class ShaderExt
{
public:
	ShaderExt(Shader* shader);
	virtual void setupExtension() = 0;

protected:
	Shader* shader;
};
