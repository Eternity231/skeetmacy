#pragma once
#include "../includes.h"

struct FireBulletData_Stacker
{
	FireBulletData_Stacker( const vec3_t& eye_pos )
		: src( eye_pos )
	{
	}

	vec3_t           src;
	CGameTrace          enter_trace;
	vec3_t           direction;
	CTraceFilterSimple filter;
	float           trace_length;
	float           trace_length_remaining;
	float           current_damage;
	int             penetrate_count;
};

class Autowall {
public:

	float CanHit( vec3_t& vecEyePos, vec3_t& point );
	bool FireBullet( Weapon* pWeapon, vec3_t& direction, float& currentDamage );
	bool HandleBulletPenetration( WeaponInfo* weaponData, CGameTrace& enterTrace, vec3_t& eyePosition, vec3_t direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration );
	bool TraceToExit( CGameTrace& enterTrace, CGameTrace& exitTrace, vec3_t startPosition, vec3_t direction );
	void ScaleDamage( CGameTrace& enterTrace, WeaponInfo* weaponData, float& currentDamage );
	bool IsBreakableEntity( Player* entity );
	void GetBulletTypeParameters( float& maxRange, float& maxDistance, char* bulletType, bool sv_penetration_type );
	void ClipTraceToPlayers( vec3_t& absStart, vec3_t absEnd, unsigned int mask, CTraceFilter* filter, CGameTrace* tr );

	bool CanHitFloatingPoint( const vec3_t& point, const vec3_t& source, float* damage_given );

	bool DidHitNonWorldEntity( Player* ent );
	bool DidHitWorld( Player* ent );

}; extern Autowall* g_Autowall;
