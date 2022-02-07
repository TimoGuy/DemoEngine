#pragma once

namespace nlohmann { class json; }

class ShaderExt
{
public:
	virtual void setupExtension(unsigned int& tex, nlohmann::json* params = nullptr) = 0;
};
