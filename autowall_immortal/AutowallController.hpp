#pragma once
#include "../includes.h"

namespace imm_autowall {
	class C_FireBulletData {
	public:
		vec3_t m_vecStart = vec3_t( 0, 0, 0 );
		vec3_t m_vecDirection = vec3_t( 0, 0, 0 );
		vec3_t m_vecPos = vec3_t( 0, 0, 0 );

		CGameTrace m_EnterTrace;

		int m_iPenetrationCount = 4;
		int m_iHitgroup = -1;

		float m_flTraceLength;
		float m_flCurrentDamage;

		// input data

		// distance to point
		float m_flMaxLength = 0.0f;
		float m_flPenetrationDistance = 0.0f;

		// should penetrate walls? 
		bool m_bPenetration = false;

		bool m_bShouldDrawImpacts = false;

		bool m_bShouldIgnoreDistance = false;

		CTraceFilter* m_bTraceFilter = nullptr;
		Player* m_Player = nullptr; // attacker
		Player* m_TargetPlayer = nullptr;  // autowall target ( could be nullptr if just trace attack )
		Weapon* m_Weapon = nullptr; // attacker weapon
		WeaponInfo* m_WeaponData = nullptr;

		surfacedata_t* m_EnterSurfaceData;
	};

	bool IsBreakable( Player* entity );
	bool IsArmored( Player* player, int hitgroup );
	float ScaleDamage( Player* player, float damage, float armor_ratio, int hitgroup );
	void TraceLine( const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* ignore, CGameTrace* ptr );
	void ClipTraceToPlayers( const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* filter, CGameTrace* tr );
	void ClipTraceToPlayer( const vec3_t vecAbsStart, const vec3_t& vecAbsEnd, uint32_t mask, CGameTrace* tr, Player* player, C_FireBulletData* data );
	bool TraceToExit( CGameTrace* enter_trace, vec3_t start, vec3_t direction, CGameTrace* exit_trace );
	bool HandleBulletPenetration( C_FireBulletData* data );
	float FireBullets( C_FireBulletData* data );
	float CanHit( vec3_t& vecEyePos, vec3_t& point, Player* ignore_ent, Player* to_who, int target_hitbox );
	bool FireBullet( vec3_t eyepos, Weapon* pWeapon, vec3_t& direction, float& currentDamage, Player* ignore, Player* to_who, int target_hitbox );

}