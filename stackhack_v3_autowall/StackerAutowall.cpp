#include "StackerAutowall.hpp"

Autowall* g_Autowall = new Autowall( );

void TraceLine( vec3_t& vecAbsStart, vec3_t& vecAbsEnd, unsigned int mask, Player* ignore, CGameTrace* ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSimple filter;
	filter.pSkip = ignore;

	g_csgo.m_engine_trace->TraceRay( ray, mask, &filter, ptr );
}

bool VectortoVectorVisible( vec3_t src, vec3_t point )
{
	CGameTrace Trace;
	TraceLine( src, point, MASK_SOLID, g_cl.m_local, &Trace );

	if ( Trace.m_fraction == 1.0f )
	{
		return true;
	}

	return false;

}

bool TraceToExit( vec3_t& vecEnd, CGameTrace* pEnterTrace, vec3_t vecStart, vec3_t vecDir, CGameTrace* pExitTrace )
{
	auto distance = 0.0f;

	while ( distance < 90.0f )
	{
		distance += 4.0f;
		vecEnd = vecStart + vecDir * distance;

		auto point_contents = g_csgo.m_engine_trace->GetPointContents( vecEnd, MASK_SHOT_HULL | CONTENTS_HITBOX, NULL );

		if ( point_contents & MASK_SHOT_HULL && ( !( point_contents & CONTENTS_HITBOX ) ) )
			continue;

		auto new_end = vecEnd - ( vecDir * 4.0f );

		TraceLine( vecEnd, new_end, 0x4600400B, 0, pExitTrace );

		if ( pExitTrace->m_startsolid && pExitTrace->m_surface.m_flags & SURF_HITBOX )
		{
			TraceLine( vecEnd, vecStart, 0x600400B, (Player*)pExitTrace->m_entity, pExitTrace ); // this might crash

			if ( ( pExitTrace->m_fraction < 1.0f || pExitTrace->m_allsolid ) && !pExitTrace->m_startsolid )
			{
				vecEnd = pExitTrace->m_endpos;
				return true;
			}

			continue;
		}

		if ( !( pExitTrace->m_fraction < 1.0 || pExitTrace->m_allsolid || pExitTrace->m_startsolid ) || pExitTrace->m_startsolid )
		{
			if ( pExitTrace->m_entity )
			{
				if ( !g_Autowall->DidHitNonWorldEntity( ( Player* )pExitTrace->m_entity ) )
					return true;
			}

			continue;
		}

		if ( ( ( pExitTrace->m_surface.m_flags >> 7 ) & 1 ) && !( ( pExitTrace->m_surface.m_flags >> 7 ) & 1 ) )
			continue;

		if ( pExitTrace->m_plane.m_normal.Dot( vecDir ) < 1.0f )
		{
			float fraction = pExitTrace->m_fraction * 4.0f;
			vecEnd = vecEnd - ( vecDir * fraction );

			return true;
		}
	}

	return false;
}

const vec3_t& world_space_center( Player* player )
{
	vec3_t vecOrigin = player->m_vecOrigin( );
	//auto collide = ( ICollideable* )player->GetCollideable( );

	//vec3_t min = collide->OBBMins( ) + vecOrigin;
	//vec3_t max = collide->OBBMaxs( ) + vecOrigin;

	vec3_t size = player->m_vecOrigin( ) + ( ( player->m_vecMins( ) + player->m_vecMaxs( ) ) * 0.5f );
	//vec3_t size = max - min;
	size /= 2.f;
	//size += min;

	return size;
}

float VectorNormalize( vec3_t& v )
{
	float l = v.length( );
	if ( l != 0.0f )
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; v.z = 1.0f;
	}

	return l;
}

float DistanceToRay( const vec3_t& pos, const vec3_t& ray_start, const vec3_t& ray_end )
{
	vec3_t to = pos - ray_start;
	vec3_t dir = ray_end - ray_start;
	float length = VectorNormalize( dir );

	float rangeAlong = dir.Dot( to );

	float range;

	if ( rangeAlong < 0.0f )
		range = -( pos - ray_start ).length( );
	else if ( rangeAlong > length )
		range = -( pos - ray_end ).length( );
	else
	{
		vec3_t onRay = ray_start + rangeAlong * dir;
		range = ( pos - onRay ).length( );
	}

	return range;
}

void Autowall::ClipTraceToPlayers( vec3_t& absStart, vec3_t absEnd, unsigned int mask, CTraceFilter* filter, CGameTrace* tr )
{
	CGameTrace playerTrace;
	Ray_t ray;
	float smallestFraction = tr->m_fraction;
	const float maxRange = 60.0f;

	ray.Init( absStart, absEnd );

	for ( int i = 1; i <= g_csgo.m_globals->m_max_clients; i++ )
	{
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );

		if ( !player || !player->alive( ) || player->dormant( ) )
			continue;

		if ( filter && filter->ShouldHitEntity( player, mask ) == false )
			continue;

		float range = DistanceToRay( world_space_center( player ), absStart, absEnd );
		if ( range < 0.0f || range > maxRange )
			continue;

		g_csgo.m_engine_trace->ClipRayToEntity( ray, mask | CONTENTS_HITBOX, player, &playerTrace );
		if ( playerTrace.m_fraction < smallestFraction )
		{
			*tr = playerTrace;
			smallestFraction = playerTrace.m_fraction;
		}
	}
}

void Autowall::GetBulletTypeParameters( float& maxRange, float& maxDistance, char* bulletType, bool sv_penetration_type )
{
	if ( sv_penetration_type )
	{
		maxRange = 35.0;
		maxDistance = 3000.0;
	}
	else
	{
		//Play tribune to framerate. Thanks, stringcompare
		//Regardless I doubt anyone will use the old penetration system anyway; so it won't matter much.
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_338MAG" ) ) )
		{
			maxRange = 45.0;
			maxDistance = 8000.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_762MM" ) ) )
		{
			maxRange = 39.0;
			maxDistance = 5000.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_556MM" ) ) || !strcmp( bulletType, ( "BULLET_PLAYER_556MM_SMALL" ) ) || !strcmp( bulletType, ( "BULLET_PLAYER_556MM_BOX" ) ) )
		{
			maxRange = 35.0;
			maxDistance = 4000.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_57MM" ) ) )
		{
			maxRange = 30.0;
			maxDistance = 2000.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_50AE" ) ) )
		{
			maxRange = 30.0;
			maxDistance = 1000.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_357SIG" ) ) || !strcmp( bulletType, ( "BULLET_PLAYER_357SIG_SMALL" ) ) || !strcmp( bulletType, ( "BULLET_PLAYER_357SIG_P250" ) ) || !strcmp( bulletType, ( "BULLET_PLAYER_357SIG_MIN" ) ) )
		{
			maxRange = 25.0;
			maxDistance = 800.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_9MM" ) ) )
		{
			maxRange = 21.0;
			maxDistance = 800.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_45ACP" ) ) )
		{
			maxRange = 15.0;
			maxDistance = 500.0;
		}
		if ( !strcmp( bulletType, ( "BULLET_PLAYER_BUCKSHOT" ) ) )
		{
			maxRange = 0.0;
			maxDistance = 0.0;
		}
	}
}

#include "../getze_pattern/PatternController.hpp"

bool Autowall::IsBreakableEntity( Player* entity )
{
	typedef bool( __thiscall* isBreakbaleEntityFn )( Player* );
	static auto is_breakable_entityFn = reinterpret_cast< isBreakbaleEntityFn >( Memory::Scan( "client.dll", "55 8B EC 83 E4 F8 83 EC 70 6A 58" ) ); // mutiny

	if ( is_breakable_entityFn )
	{
		// 0x27C = m_takedamage

		auto backupval = *reinterpret_cast< int* >( ( uint32_t )entity + 0x27C );
		auto className = entity->GetClientClass( )->m_pNetworkName;

		if ( entity != g_csgo.m_entlist->GetClientEntity( 0 ) )
		{
			// CFuncBrush:
			// (className[1] != 'F' || className[4] != 'c' || className[5] != 'B' || className[9] != 'h')
			if ( ( className[ 1 ] == 'B' && className[ 9 ] == 'e' && className[ 10 ] == 'S' && className[ 16 ] == 'e' ) // CBreakableSurface
				|| ( className[ 1 ] != 'B' || className[ 5 ] != 'D' ) ) // CBaseDoor because fuck doors
			{
				*reinterpret_cast< int* >( ( uint32_t )entity + 0x27C ) = 2;
			}
		}

		bool retn = is_breakable_entityFn( entity );

		*reinterpret_cast< int* >( ( uint32_t )entity + 0x27C ) = backupval;

		return retn;
	}
	else
		return false;
}

void Autowall::ScaleDamage( CGameTrace& enterTrace, WeaponInfo* weaponData, float& currentDamage )
{
	//Cred. to N0xius for reversing this.
	//TODO: _xAE^; look into reversing this yourself sometime

	bool hasHeavyArmor = ( ( Player* )enterTrace.m_entity )->m_bHasHeavyArmor( );
	int armorValue = ( ( Player* )enterTrace.m_entity )->m_ArmorValue( );

	//Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
	auto IsArmored = [ &enterTrace ]( )->bool
	{
		Player* targetEntity = ( Player* )enterTrace.m_entity;
		switch ( enterTrace.m_hitgroup )
		{
		case HITGROUP_HEAD:
			return !!( Player* )targetEntity->m_bHasHelmet( ); //Fuck compiler errors - force-convert it to a bool via (!!)
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
		default:
			return false;
		}
	};

	switch ( enterTrace.m_hitgroup )
	{
	case HITGROUP_HEAD:
		currentDamage *= hasHeavyArmor ? 2.f : 4.f; //Heavy Armor does 1/2 damage
		break;
	case HITGROUP_STOMACH:
		currentDamage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		currentDamage *= 0.75f;
		break;
	default:
		break;
	}

	if ( armorValue > 0 && IsArmored( ) )
	{
		float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weaponData->m_armor_ratio / 2.f;

		//Damage gets modified for heavy armor users
		if ( hasHeavyArmor )
		{
			armorBonusRatio = 0.33f;
			armorRatio *= 0.5f;
			bonusValue = 0.33f;
		}

		auto NewDamage = currentDamage * armorRatio;

		if ( hasHeavyArmor )
			NewDamage *= 0.85f;

		if ( ( ( currentDamage - ( currentDamage * armorRatio ) ) * ( bonusValue * armorBonusRatio ) ) > armorValue )
			NewDamage = currentDamage - ( armorValue / armorBonusRatio );

		currentDamage = NewDamage;
	}
}

bool Autowall::DidHitWorld( Player* ent )
{
	return ent == g_csgo.m_entlist->GetClientEntity( 0 );
}

bool Autowall::DidHitNonWorldEntity( Player* ent )
{
	return ent != nullptr && !DidHitWorld( ent );
}

bool Autowall::TraceToExit( CGameTrace& enterTrace, CGameTrace& exitTrace, vec3_t startPosition, vec3_t direction )
{
	/*
	Masks used:
	MASK_SHOT_HULL					 = 0x600400B
	CONTENTS_HITBOX					 = 0x40000000
	MASK_SHOT_HULL | CONTENTS_HITBOX = 0x4600400B
	*/

	vec3_t start, end;
	float maxDistance = 90.f, rayExtension = 4.f, currentDistance = 0;
	int firstContents = 0;

	while ( currentDistance <= maxDistance )
	{
		//Add extra distance to our ray
		currentDistance += rayExtension;

		//Multiply the direction vector to the distance so we go outwards, add our position to it.
		start = startPosition + direction * currentDistance;

		if ( !firstContents )
			firstContents = g_csgo.m_engine_trace->GetPointContents( start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr ); /*0x4600400B*/
		int pointContents = g_csgo.m_engine_trace->GetPointContents( start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr );

		if ( !( pointContents & MASK_SHOT_HULL ) || pointContents & CONTENTS_HITBOX && pointContents != firstContents ) /*0x600400B, *0x40000000*/
		{
			//Let's setup our end position by deducting the direction by the extra added distance
			end = start - ( direction * rayExtension );

			//Let's cast a ray from our start pos to the end pos
			TraceLine( start, end, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr, &exitTrace );

			//Let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
			if ( exitTrace.m_startsolid && exitTrace.m_surface.m_flags & SURF_HITBOX )
			{
				TraceLine( start, startPosition, MASK_SHOT_HULL, (Player*)exitTrace.m_entity, &exitTrace );

				if ( exitTrace.m_hitgroup != 0 && !exitTrace.m_startsolid )
				{
					start = exitTrace.m_endpos;
					return true;
				}

				continue;
			}

			//Can we hit? Is the wall solid?
			if ( exitTrace.hit( ) && !exitTrace.m_startsolid )
			{
				//Is the wall a breakable? If so, let's shoot through it.
				if ( game::IsBreakable( enterTrace.m_entity ) && game::IsBreakable( exitTrace.m_entity ) )
					return true;

				if ( enterTrace.m_surface.m_flags & SURF_NODRAW || !( exitTrace.m_surface.m_flags & SURF_NODRAW ) && ( exitTrace.m_plane.m_normal.Dot( direction ) <= 1.f ) )
				{
					float multAmount = exitTrace.m_fraction * 4.f;
					start -= direction * multAmount;
					return true;
				}

				continue;
			}

			if ( !exitTrace.hit( ) || exitTrace.m_startsolid )
			{
				if ( DidHitNonWorldEntity( (Player*)enterTrace.m_entity ) && IsBreakableEntity( ( Player* )enterTrace.m_entity ) )
				{
					exitTrace = enterTrace;
					exitTrace.m_endpos = start + direction;
					return true;
				}

				continue;
			}
		}
	}
	return false;
}

#define CHAR_TEX_FLESH		'F'
#define CHAR_TEX_GRATE		'G'
#define CHAR_TEX_PLASTIC	'L'
#define CHAR_TEX_METAL		'M'
#define CHAR_TEX_CARDBOARD	'U'
#define CHAR_TEX_WOOD		'W'
#define CHAR_TEX_GLASS		'Y'

bool Autowall::HandleBulletPenetration( WeaponInfo* weaponData, CGameTrace& enterTrace, vec3_t& eyePosition, vec3_t direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration )
{
	//Because there's been issues regarding this- putting this here.
	if ( &currentDamage == nullptr )
		throw std::invalid_argument( "currentDamage is null!\n" );

	CGameTrace exitTrace;
	Player* pEnemy = ( Player* )enterTrace.m_entity;
	surfacedata_t* enterSurfaceData = g_csgo.m_phys_props->GetSurfaceData( enterTrace.m_surface.m_surface_props );
	int enterMaterial = enterSurfaceData->m_game.m_material;

	float enterSurfPenetrationModifier = enterSurfaceData->m_game.m_penetration_modifier;
	float enterDamageModifier = enterSurfaceData->m_game.m_damage_modifier;
	float thickness, modifier, lostDamage, finalDamageModifier, combinedPenetrationModifier;
	bool isSolidSurf = ( ( enterTrace.m_contents >> 3 ) & CONTENTS_SOLID );
	bool isLightSurf = ( ( enterTrace.m_surface.m_flags >> 7 ) & SURF_LIGHT );

	if ( possibleHitsRemaining <= 0
		//Test for "DE_CACHE/DE_CACHE_TELA_03" as the entering surface and "CS_ITALY/CR_MISCWOOD2B" as the exiting surface.
		//Fixes a wall in de_cache which seems to be broken in some way. Although bullet penetration is not recorded to go through this wall
		//Decals can be seen of bullets within the glass behind of the enemy. Hacky method, but works.
		//You might want to have a check for this to only be activated on de_cache.
		|| ( enterTrace.m_surface.m_name == ( const char* )0x2227c261 && exitTrace.m_surface.m_name == ( const char* )0x2227c868 )
		|| ( !possibleHitsRemaining && !isLightSurf && !isSolidSurf && enterMaterial != CHAR_TEX_GRATE && enterMaterial != CHAR_TEX_GLASS )
		|| weaponData->m_penetration <= 0.f
		|| !TraceToExit( enterTrace, exitTrace, enterTrace.m_endpos, direction )
		&& !( g_csgo.m_engine_trace->GetPointContents( enterTrace.m_endpos, MASK_SHOT_HULL, nullptr ) & MASK_SHOT_HULL ) )
		return false;

	surfacedata_t* exitSurfaceData = g_csgo.m_phys_props->GetSurfaceData( exitTrace.m_surface.m_surface_props );
	int exitMaterial = exitSurfaceData->m_game.m_material;
	float exitSurfPenetrationModifier = exitSurfaceData->m_game.m_penetration_modifier;
	float exitDamageModifier = exitSurfaceData->m_game.m_damage_modifier;

	//Are we using the newer penetration system?
	if ( sv_penetration_type )
	{
		if ( enterMaterial == CHAR_TEX_GRATE || enterMaterial == CHAR_TEX_GLASS )
		{
			combinedPenetrationModifier = 3.f;
			finalDamageModifier = 0.05f;
		}
		else if ( isSolidSurf || isLightSurf )
		{
			combinedPenetrationModifier = 1.f;
			finalDamageModifier = 0.16f;
		}
		else if ( enterMaterial == CHAR_TEX_FLESH && ( g_cl.m_local->m_iTeamNum( ) == pEnemy->m_iTeamNum( ) && ff_damage_reduction_bullets == 0.f ) ) //TODO: Team check config
		{
			//Look's like you aren't shooting through your teammate today
			if ( ff_damage_bullet_penetration == 0.f )
				return false;

			//Let's shoot through teammates and get kicked for teamdmg! Whatever, atleast we did damage to the enemy. I call that a win.
			combinedPenetrationModifier = ff_damage_bullet_penetration;
			finalDamageModifier = 0.16f;
		}
		else
		{
			combinedPenetrationModifier = ( enterSurfPenetrationModifier + exitSurfPenetrationModifier ) / 2.f;
			finalDamageModifier = 0.16f;
		}

		//Do our materials line up?
		if ( enterMaterial == exitMaterial )
		{
			if ( exitMaterial == CHAR_TEX_CARDBOARD || exitMaterial == CHAR_TEX_WOOD )
				combinedPenetrationModifier = 3.f;
			else if ( exitMaterial == CHAR_TEX_PLASTIC )
				combinedPenetrationModifier = 2.f;
		}

		//Calculate thickness of the wall by getting the length of the range of the trace and squaring
		thickness = ( exitTrace.m_endpos - enterTrace.m_endpos ).length_sqr( );
		modifier = fmaxf( 1.f / combinedPenetrationModifier, 0.f );

		//This calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
		lostDamage = fmaxf(
			( ( modifier * thickness ) / 24.f )
			+ ( ( currentDamage * finalDamageModifier )
				+ ( fmaxf( 3.75f / penetrationPower, 0.f ) * 3.f * modifier ) ), 0.f );

		//Did we loose too much damage?
		if ( lostDamage > currentDamage )
			return false;

		//We can't use any of the damage that we've lost
		if ( lostDamage > 0.f )
			currentDamage -= lostDamage;

		//Do we still have enough damage to deal?
		if ( currentDamage < 1.f )
			return false;

		eyePosition = exitTrace.m_endpos;
		--possibleHitsRemaining;

		return true;
	}
	else //Legacy penetration system
	{
		combinedPenetrationModifier = 1.f;

		if ( isSolidSurf || isLightSurf )
			finalDamageModifier = 0.99f; //Good meme :^)
		else
		{
			finalDamageModifier = fminf( enterDamageModifier, exitDamageModifier );
			combinedPenetrationModifier = fminf( enterSurfPenetrationModifier, exitSurfPenetrationModifier );
		}

		if ( enterMaterial == exitMaterial && ( exitMaterial == CHAR_TEX_METAL || exitMaterial == CHAR_TEX_WOOD ) )
			combinedPenetrationModifier += combinedPenetrationModifier;

		thickness = ( exitTrace.m_endpos - enterTrace.m_endpos ).length_sqr( );

		if ( sqrt( thickness ) <= combinedPenetrationModifier * penetrationPower )
		{
			currentDamage *= finalDamageModifier;
			eyePosition = exitTrace.m_endpos;
			--possibleHitsRemaining;

			return true;
		}

		return false;
	}
}

bool Autowall::FireBullet( Weapon* pWeapon, vec3_t& direction, float& currentDamage )
{
	if ( !pWeapon )
		return false;

	bool sv_penetration_type;
	//	  Current bullet travel Power to penetrate Distance to penetrate Range               Player bullet reduction convars			  Amount to extend ray by
	float currentDistance = 0.f, penetrationPower, penetrationDistance, maxRange, ff_damage_reduction_bullets, ff_damage_bullet_penetration, rayExtension = 40.f;
	vec3_t eyePosition = g_cl.m_local->get_eye_pos();

	//For being superiour when the server owners think your autowall isn't well reversed. Imagine a meme HvH server with the old penetration system- pff
	static ConVar* penetrationSystem = g_csgo.m_cvar->FindVar( HASH( "sv_penetration_type" ) );
	static ConVar* damageReductionBullets = g_csgo.m_cvar->FindVar( HASH( "ff_damage_reduction_bullets" ) );
	static ConVar* damageBulletPenetration = g_csgo.m_cvar->FindVar( HASH( "ff_damage_bullet_penetration" ) );

	sv_penetration_type = penetrationSystem->GetInt( );
	ff_damage_reduction_bullets = damageReductionBullets->GetFloat( );
	ff_damage_bullet_penetration = damageBulletPenetration->GetFloat( );

	auto weaponData = g_cl.m_weapon_info;
	CGameTrace enterTrace;

	//We should be skipping g_Globals->LocalPlayer when casting a ray to players.
	CTraceFilterSimple filter;
	filter.pSkip = g_cl.m_local;

	if ( !weaponData )
		return false;

	maxRange = weaponData->m_range;

	GetBulletTypeParameters( penetrationPower, penetrationDistance, "", sv_penetration_type );
	penetrationPower = 35.0;
	penetrationDistance = 3000.0;

	if ( sv_penetration_type )
		penetrationPower = weaponData->m_penetration;

	//This gets set in FX_Firebullets to 4 as a pass-through value.
	//CS:GO has a maximum of 4 surfaces a bullet can pass-through before it 100% stops.
	//Excerpt from Valve: https://steamcommunity.com/sharedfiles/filedetails/?id=275573090
	//"The total number of surfaces any bullet can penetrate in a single flight is capped at 4." -CS:GO Official
	int possibleHitsRemaining = 4;

	//Set our current damage to what our gun's initial damage reports it will do
	currentDamage = weaponData->m_damage;

	//If our damage is greater than (or equal to) 1, and we can shoot, let's shoot.
	while ( possibleHitsRemaining > 0 && currentDamage >= 1.f )
	{
		//Calculate max bullet range
		maxRange -= currentDistance;

		//Create endpoint of bullet
		vec3_t end = eyePosition + direction * maxRange;

		TraceLine( eyePosition, end, MASK_SHOT_HULL | CONTENTS_HITBOX, g_cl.m_local, &enterTrace );
		ClipTraceToPlayers( eyePosition, end + direction * rayExtension, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace );

		//We have to do this *after* tracing to the player.
		surfacedata_t* enterSurfaceData = g_csgo.m_phys_props->GetSurfaceData( enterTrace.m_surface.m_surface_props );
		float enterSurfPenetrationModifier = enterSurfaceData->m_game.m_penetration_modifier;
		int enterMaterial = enterSurfaceData->m_game.m_material;

		//"Fraction == 1" means that we didn't hit anything. We don't want that- so let's break on it.
		if ( enterTrace.m_fraction == 1.f )
			break;

		//calculate the damage based on the distance the bullet traveled.
		currentDistance += enterTrace.m_fraction * maxRange;

		//Let's make our damage drops off the further away the bullet is.
		currentDamage *= pow( weaponData->m_range_modifier, ( currentDistance / 500.f ) );

		//Sanity checking / Can we actually shoot through?
		if ( currentDistance > penetrationDistance && weaponData->m_penetration > 0.f || enterSurfPenetrationModifier < 0.1f )
			break;

		//This looks gay as fuck if we put it into 1 long line of code.
		bool canDoDamage = ( enterTrace.m_hitgroup != HITGROUP_GEAR && enterTrace.m_hitgroup != HITGROUP_GENERIC );
		bool isPlayer = ( ( enterTrace.m_entity )->GetClientClass( )->m_ClassID == 35 );
		bool isEnemy = ( ( ( Player* )g_cl.m_local )->m_iTeamNum( ) != ( ( Player* )enterTrace.m_entity )->m_iTeamNum( ) );
		bool onTeam = ( ( ( Player* )enterTrace.m_entity )->m_iTeamNum( ) == ( int32_t )3 || ( ( Player* )enterTrace.m_entity )->m_iTeamNum( ) == ( int32_t )2 );

		//TODO: Team check config
		if ( ( canDoDamage && isPlayer && isEnemy ) && onTeam )
		{
			ScaleDamage( enterTrace, weaponData, currentDamage );
			return true;
		}

		//Calling HandleBulletPenetration here reduces our penetrationCounter, and if it returns true, we can't shoot through it.
		if ( !HandleBulletPenetration( weaponData, enterTrace, eyePosition, direction, possibleHitsRemaining, currentDamage, penetrationPower, sv_penetration_type, ff_damage_reduction_bullets, ff_damage_bullet_penetration ) )
			break;
	}

	return false;
}

static vec3_t umomaim;

float GetHitgroupDamageMult( int iHitGroup )
{
	switch ( iHitGroup )
	{
	case HITGROUP_GENERIC:
		return 0.5f;
	case HITGROUP_HEAD:
		return 4.0f;
	case HITGROUP_CHEST:
		return 1.f;
	case HITGROUP_STOMACH:
		return 1.25f;
	case HITGROUP_LEFTARM:
		return 0.65f;
	case HITGROUP_RIGHTARM:
		return 0.65f;
	case HITGROUP_LEFTLEG:
		return 0.75f;
	case HITGROUP_RIGHTLEG:
		return 0.75f;
	case HITGROUP_GEAR:
		return 0.5f;
	default:
		return 1.0f;
	}
}

void ScaleDamage( int hitgroup, Player* enemy, float weapon_m_flArmorRatio, float& current_damage )
{
	current_damage *= GetHitgroupDamageMult( hitgroup );

	if ( enemy->m_ArmorValue( ) > 0 )
	{
		if ( hitgroup == HITGROUP_HEAD )
		{
			if ( enemy->m_bHasHelmet( ) )
				current_damage *= ( weapon_m_flArmorRatio * .5f );
		}
		else
		{
			current_damage *= ( weapon_m_flArmorRatio * .5f );
		}
	}
}

#define CHECK_VALID( _v ) 0
inline float VectorLength( const vec3_t& v )
{
	CHECK_VALID( v );
	return ( float )FastSqrt( v.x * v.x + v.y * v.y + v.z * v.z );
}

bool HandleBulletPenetration2( WeaponInfo* wpn_data, FireBulletData_Stacker& data, bool extracheck )
{
	surfacedata_t* enter_surface_data = g_csgo.m_phys_props->GetSurfaceData( data.enter_trace.m_surface.m_surface_props );
	int enter_material = enter_surface_data->m_game.m_material;
	float enter_surf_penetration_mod = enter_surface_data->m_game.m_penetration_modifier;

	data.trace_length += data.enter_trace.m_fraction * data.trace_length_remaining;
	data.current_damage *= pow( wpn_data->m_range_modifier, ( data.trace_length * 0.002f ) );

	if ( ( data.trace_length > 3000.f ) || ( enter_surf_penetration_mod < 0.1f ) )
		data.penetrate_count = 0;

	if ( data.penetrate_count <= 0 )
		return false;

	vec3_t dummy;
	CGameTrace trace_exit;
	if ( !TraceToExit( dummy, &data.enter_trace, data.enter_trace.m_endpos, data.direction, &trace_exit ) )
		return false;

	surfacedata_t* exit_surface_data = g_csgo.m_phys_props->GetSurfaceData( trace_exit.m_surface.m_surface_props );
	int exit_material = exit_surface_data->m_game.m_material;

	float exit_surf_penetration_mod = exit_surface_data->m_game.m_penetration_modifier;
	float final_damage_modifier = 0.16f;
	float combined_penetration_modifier = 0.0f;

	if ( ( ( data.enter_trace.m_contents & CONTENTS_GRATE ) != 0 ) || ( enter_material == 89 ) || ( enter_material == 71 ) )
	{
		combined_penetration_modifier = 3.0f;
		final_damage_modifier = 0.05f;
	}
	else
	{
		combined_penetration_modifier = ( enter_surf_penetration_mod + exit_surf_penetration_mod ) * 0.5f;
	}

	if ( enter_material == exit_material )
	{
		if ( exit_material == 87 || exit_material == 85 )
			combined_penetration_modifier = 3.0f;
		else if ( exit_material == 76 )
			combined_penetration_modifier = 2.0f;
	}

	float v34 = fmaxf( 0.f, 1.0f / combined_penetration_modifier );
	float v35 = ( data.current_damage * final_damage_modifier ) + v34 * 3.0f * fmaxf( 0.0f, ( 3.0f / wpn_data->m_penetration ) * 1.25f );
	float thickness = VectorLength( trace_exit.m_endpos - data.enter_trace.m_endpos );

	if ( extracheck )
	{
		if ( !VectortoVectorVisible( trace_exit.m_endpos, umomaim ) )
			return false;
	}

	thickness *= thickness;
	thickness *= v34;
	thickness /= 24.0f;

	float lost_damage = fmaxf( 0.0f, v35 + thickness );

	if ( lost_damage > data.current_damage )
		return false;

	if ( lost_damage >= 0.0f )
		data.current_damage -= lost_damage;

	if ( data.current_damage < 1.0f )
		return false;

	data.src = trace_exit.m_endpos;
	data.penetrate_count--;

	return true;
}

float Autowall::CanHit( vec3_t& vecEyePos, vec3_t& point )
{
	vec3_t angles, direction;
	vec3_t tmp = point - g_cl.m_local->get_eye_pos( );
	float currentDamage = 0;

	Getze::L3D_Math::VectorAngles( tmp, angles );
	Getze::L3D_Math::AngleVectors( angles, &direction );
	direction.NormalizeInPlace( );

	if ( FireBullet( g_cl.m_weapon, direction, currentDamage ) )
		return currentDamage;

	return -1; //That wall is just a bit too thick buddy
}

bool Autowall::CanHitFloatingPoint( const vec3_t& point, const vec3_t& source, float* damage_given )
{
	umomaim = point;

	if ( !g_cl.m_local )
		return false;

	FireBulletData_Stacker& data = FireBulletData_Stacker( source );
	data.filter = CTraceFilterSimple( );
	data.filter.pSkip = g_cl.m_local;

	vec3_t angles = math::CalcAngle( data.src, point );
	Getze::L3D_Math::AngleVectors( angles, &data.direction );
	VectorNormalize( data.direction );

	auto pWeapon = g_cl.m_weapon;
	//C_BaseCombatWeapon* pWeapon = ( C_BaseCombatWeapon* )g_Globals->LocalPlayer->GetActiveWeapon( );

	if ( !pWeapon )
		return false;

	data.penetrate_count = 1;
	data.trace_length = 0.0f;

	auto weaponData = g_cl.m_weapon_info;
	//CSWeaponInfo* weaponData = pWeapon->GetCSWpnData( );

	if ( !weaponData )
		return false;

	data.current_damage = ( float )weaponData->m_damage;

	data.trace_length_remaining = weaponData->m_range - data.trace_length;

	vec3_t end = data.src + data.direction * data.trace_length_remaining;

	TraceLine( data.src, end, MASK_SHOT | CONTENTS_GRATE, g_cl.m_local, &data.enter_trace );

	if ( VectortoVectorVisible( point, data.src ) )
	{
		*damage_given = data.current_damage;
		return true;
	}

	if ( HandleBulletPenetration2( weaponData, data, true ) )
	{
		*damage_given = data.current_damage;
		return true;
	}

	return false;
}