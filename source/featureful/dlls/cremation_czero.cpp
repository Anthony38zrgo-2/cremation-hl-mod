// Cremation (rewrite): infiltrador CZ + bomba plantable/defusable.
// v1: el CZ planta al llegar a una zona cz_plant_* (3 s quieto), la bomba
// detona a los 35 s; el jugador la desactiva con +USE a <88 u y con LOS
// durante 5 s acumulados. Sin hooks ABI: clases reales + entidades.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "player.h"
#include "combat.h"
#include "hgrunt.h"

#define CZ_PLANT_TIME 3.0f
#define CZ_DEFUSE_TIME 5.0f
#define CZ_BOMB_TIMER 35.0f
#define CZ_DEFUSE_DIST 88.0f
#define CZ_PLANT_DIST 160.0f
#define CZ_BOMB_DAMAGE 500.0f
#define CZ_BOMB_RADIUS 800.0f

class CCZPlantedBomb : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override { BombThink(); }
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override {}

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	void BombThink();
	float m_detonateAt{ 0.0f };
	float m_defuseProgress{ 0.0f };
	float m_nextBeep{ 0.0f };
	int m_lastBeepSecond{ -1 };
};

LINK_ENTITY_TO_CLASS(cz_planted_bomb, CCZPlantedBomb);

TYPEDESCRIPTION CCZPlantedBomb::m_SaveData[] =
{
	DEFINE_FIELD(CCZPlantedBomb, m_detonateAt, FIELD_TIME),
	DEFINE_FIELD(CCZPlantedBomb, m_defuseProgress, FIELD_FLOAT),
	DEFINE_FIELD(CCZPlantedBomb, m_nextBeep, FIELD_TIME),
	DEFINE_FIELD(CCZPlantedBomb, m_lastBeepSecond, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CCZPlantedBomb, CBaseEntity);

void CCZPlantedBomb::Precache()
{
	PRECACHE_MODEL("models/cremation/czero/w_c4.mdl");
	PRECACHE_SOUND("weapons/grenade_beep.wav");
}

void CCZPlantedBomb::Spawn()
{
	// Sin Precache() aqui: la entidad se crea en runtime (fuera de fase de
	// spawn) y el motor lo rechaza. Los assets los precarga el CZ (abajo).
	SET_MODEL(ENT(pev), "models/cremation/czero/w_c4.mdl");
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, Vector(-8, -8, 0), Vector(8, 8, 12));
	pev->health = 50;
	pev->takedamage = DAMAGE_YES;
	m_detonateAt = gpGlobals->time + CZ_BOMB_TIMER;
	m_defuseProgress = 0.0f;
	m_nextBeep = gpGlobals->time;
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CCZPlantedBomb::BombThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (m_detonateAt <= 0.0f)
		return;

	float remaining = m_detonateAt - gpGlobals->time;
	if (remaining <= 0.0f)
	{
		// Detonacion: dano masivo + muerte garantizada (Game Over).
		RadiusDamageInfo rdi(DamageInfo(CZ_BOMB_DAMAGE, DMG_BLAST), CZ_BOMB_RADIUS);
		::RadiusDamage(pev->origin, pev, pev, rdi);
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		MESSAGE_END();
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/explode3.wav", VOL_NORM, ATTN_NONE);
		CBaseEntity* pPlayer = nullptr;
		while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != nullptr)
		{
			if (pPlayer->pev->deadflag == DEAD_NO)
				pPlayer->pev->health = 0;
		}
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "LA BOMBA DETONO");
		ALERT(at_console, "CZ: bomba detonada\n");
		m_detonateAt = 0.0f;
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 2.0f;
		return;
	}

	// Beep acelerado.
	if (gpGlobals->time >= m_nextBeep)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/grenade_beep.wav", VOL_NORM, ATTN_NORM);
		float interval = remaining > 10.0f ? 1.0f : (remaining > 5.0f ? 0.5f : 0.2f);
		m_nextBeep = gpGlobals->time + interval;
	}

	// Defuse: jugador con +USE cerca, con linea de vista.
	CBaseEntity* pPlayer = nullptr;
	bool defusing = false;
	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != nullptr)
	{
		if (pPlayer->pev->deadflag != DEAD_NO || pPlayer->pev->health <= 0)
			continue;
		if (!(pPlayer->pev->button & IN_USE))
			continue;
		Vector delta = pPlayer->Center() - Center();
		if (delta.Length() > CZ_DEFUSE_DIST)
			continue;
		if (!FVisible(pPlayer))
			continue;
		defusing = true;
		break;
	}

	if (defusing)
	{
		m_defuseProgress += 0.1f;
		int whole = (int)(CZ_DEFUSE_TIME - m_defuseProgress);
		if (whole != m_lastBeepSecond && whole >= 0)
		{
			m_lastBeepSecond = whole;
			char msg[64];
			snprintf(msg, sizeof(msg), "DESACTIVANDO... %d", whole + 1);
			UTIL_ClientPrintAll(HUD_PRINTCENTER, msg);
		}
		if (m_defuseProgress >= CZ_DEFUSE_TIME)
		{
			UTIL_ClientPrintAll(HUD_PRINTCENTER, "BOMBA DESACTIVADA");
			ALERT(at_console, "CZ: bomba desactivada\n");
			m_detonateAt = 0.0f;
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1f;
		}
	}
	else
	{
		m_defuseProgress = 0.0f;
		m_lastBeepSecond = -1;
	}
}

class CCZeroZombie : public CHGrunt
{
public:
	const char* DefaultModel() override { return "models/cremation/czero/zombie_knife.mdl"; }
	void Precache() override;
	void MonsterThink() override;

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	bool m_hasBomb{ true };
	float m_plantUntil{ 0.0f };
	float m_nextPlantCheck{ 0.0f };
};

LINK_ENTITY_TO_CLASS(monster_czero_zombie, CCZeroZombie);

TYPEDESCRIPTION CCZeroZombie::m_SaveData[] =
{
	DEFINE_FIELD(CCZeroZombie, m_hasBomb, FIELD_BOOLEAN),
	DEFINE_FIELD(CCZeroZombie, m_plantUntil, FIELD_TIME),
	DEFINE_FIELD(CCZeroZombie, m_nextPlantCheck, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CCZeroZombie, CHGrunt);

void CCZeroZombie::Precache()
{
	CHGrunt::Precache();
	PRECACHE_MODEL("models/cremation/czero/w_c4.mdl");
	PRECACHE_SOUND("weapons/grenade_beep.wav");
}

static CBaseEntity* CZFindPlantZone(CCZeroZombie* pCZ)
{
	float best = CZ_PLANT_DIST;
	CBaseEntity* pBest = nullptr;
	for (const char* name : { "cz_plant_a", "cz_plant_b" })
	{
		CBaseEntity* p = nullptr;
		while ((p = UTIL_FindEntityByTargetname(p, name)) != nullptr)
		{
			float d = (p->pev->origin - pCZ->pev->origin).Length();
			if (d < best)
			{
				best = d;
				pBest = p;
			}
		}
	}
	return pBest;
}

void CCZeroZombie::MonsterThink()
{
	CHGrunt::MonsterThink();

	if (!m_hasBomb || pev->deadflag != DEAD_NO)
		return;
	if (gpGlobals->time < m_nextPlantCheck)
	{
		// Canalizando plantado: quieto.
		if (m_plantUntil > 0.0f)
			pev->velocity = Vector(0, 0, 0);
		return;
	}
	m_nextPlantCheck = gpGlobals->time + 0.5f;

	if (m_plantUntil > 0.0f)
	{
		pev->velocity = Vector(0, 0, 0);
		if (gpGlobals->time >= m_plantUntil)
		{
			m_plantUntil = 0.0f;
			m_hasBomb = false;
			Vector origin = pev->origin;
			origin.z += 8.0f;
			CBaseEntity* pBomb = CBaseEntity::Create("cz_planted_bomb", origin, Vector(0, 0, 0), nullptr);
			if (pBomb)
			{
				UTIL_ClientPrintAll(HUD_PRINTCENTER, "BOMBA PLANTADA - 35 SEGUNDOS");
				ALERT(at_console, "CZ: bomba plantada\n");
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/grenade_beep.wav", VOL_NORM, ATTN_NORM);
			}
		}
		return;
	}

	// Cerca de una zona y sin enemigo encima: empezar a plantar.
	CBaseEntity* pZone = CZFindPlantZone(this);
	if (pZone)
	{
		m_plantUntil = gpGlobals->time + CZ_PLANT_TIME;
		pev->velocity = Vector(0, 0, 0);
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "EL CZ ESTA PLANTANDO...");
	}
}
