#pragma once
#ifndef WARPBALL_H
#define WARPBALL_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "const_render.h"
#include "const_sound.h"
#include "template_property_types.h"
#include "json_config.h"
#include "optional.h"

#define WARPBALL_RED_DEFAULT 77
#define WARPBALL_GREEN_DEFAULT 210
#define WARPBALL_BLUE_DEFAULT 130

#define WARPBALL_BEAM_RED_DEFAULT 20
#define WARPBALL_BEAM_GREEN_DEFAULT 243
#define WARPBALL_BEAM_BLUE_DEFAULT 20

#define ALIEN_TELEPORT_SOUND "debris/alien_teleport.wav"

#define WARPBALL_SPRITE "sprites/fexplo1.spr"
#define WARPBALL_SPRITE2 "sprites/xflare1.spr"
#define WARPBALL_BEAM "sprites/lgtning.spr"
#define WARPBALL_SOUND1 "debris/beamstart2.wav"
#define WARPBALL_SOUND2 "debris/beamstart7.wav"

struct WarpballSound
{
	const char* sound{nullptr};
	FloatRange volume{1.0f};
	float attenuation{ATTN_NORM};
	IntRange pitch{100};
};

struct WarpballSprite
{
	const char* sprite{nullptr};
	Color3 color{};
	int alpha{255};
	float scale{1.0f};
	float framerate{12.0f};
	int rendermode{kRenderGlow};
	int renderfx{kRenderFxNoDissipation};
};

struct WarpballBeam
{
	const char* sprite{nullptr};
	int texture{0};
	Color3 color{};
	int alpha{220};
	int width{30};
	int noise{65};
	FloatRange life{0.5f, 1.6f};
};

struct WarpballLight
{
	optional<Color3> color;
	int radius{192};
	float life{1.5f};
	inline bool IsDefined() const {
		return color.has_value() && life > 0.0 && radius > 0;
	}
};

typedef PlayerShake WarpballShake;

struct WarpballAiSound
{
	int type{0};
	int radius{192};
	float duration{0.3f};
	inline bool IsDefined() const {
		return type != 0 && duration > 0.0f && radius > 0;
	}
};

struct WarpballPosition
{
	float verticalShift{0.0f};
	bool defined{false};
	inline bool IsDefined() const {
		return defined;
	}
};

struct WarpballTemplate
{
	WarpballSound sound1;
	WarpballSound sound2;

	WarpballSprite sprite1;
	WarpballSprite sprite2;

	WarpballBeam beam;
	int beamRadius{192};
	IntRange beamCount{10, 20};

	WarpballLight light;
	WarpballShake shake;

	WarpballAiSound aiSound;
	float spawnDelay{0.0f};
	WarpballPosition position;
};

struct WarpballTemplateCatalog : public JSONConfig
{
protected:
	const char* Schema() const override;
	bool ReadFromDocument(const rapidjson::Document& document, const char* fileName) override;
public:
	const WarpballTemplate* FindWarpballTemplate(const char* warpballName, const char* entityClassname = nullptr);
	void PrecacheWarpballTemplate(const char* name, const char* entityClassname);
	void DumpWarpballTemplates() const;

private:
	WarpballTemplate* GetWarpballTemplateMutable(const char* warpballName, const char* entityClassname);
	WarpballTemplate* GetWarpballTemplateByName(const char* warpballName);
	bool AddWarpballTemplate(const rapidjson::Value& allTemplatesJsonValue, const char* templateName, const rapidjson::Value& templateJsonValue, const char* fileName, std::vector<std::string> inheritanceChain = std::vector<std::string>());

	void AssignWarpballSound(WarpballSound& sound, const rapidjson::Value& soundJson);
	void AssignWarpballSprite(WarpballSprite& sprite, const rapidjson::Value& spriteJson);
	void AssignWarpballBeam(WarpballBeam& beam, const rapidjson::Value& beamJson);

	bool UpdateStringFromJson(const char*& str, const rapidjson::Value& jsonValue, const char* key);
	const char* MakeConstantString(const char* str);

	std::map<std::string, std::map<std::string, std::string> > _entityMappings;
	std::map<std::string, WarpballTemplate> _templates;
	std::set<std::string> _stringSet;
};

extern WarpballTemplateCatalog g_WarpballCatalog;

#if SERVER_DLL
class CBaseEntity;

void PlayWarpballEffect(CBaseEntity* pInitiator, const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt);
void PlayWarpballEffect(const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt);
#endif

#endif
