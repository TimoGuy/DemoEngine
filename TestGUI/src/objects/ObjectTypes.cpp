#include "PlayerCharacter.h"
#include "YosemiteTerrain.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "WaterPuddle.h"
#include "RiverDropoff.h"
#include "VoxelGroup.h"
#include "Spline.h"

// @Palette: this is where you dictate the json type of the object
const std::string PlayerCharacter::TYPE_NAME =		"player";
const std::string YosemiteTerrain::TYPE_NAME =		"ground";
const std::string DirectionalLight::TYPE_NAME =		"dir_light";
const std::string PointLight::TYPE_NAME =			"pt_light";
const std::string WaterPuddle::TYPE_NAME =			"water_puddle";
const std::string RiverDropoff::TYPE_NAME =			"river_dropoff";
const std::string VoxelGroup::TYPE_NAME =			"voxel_group";
const std::string Spline::TYPE_NAME =				"spline";
