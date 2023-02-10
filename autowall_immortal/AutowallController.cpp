#include "AutowallController.hpp"

namespace imm_autowall {
	bool IsArmored( Player* player, int nHitgroup ) {
		const bool bHasHelmet = player->m_bHasHelmet( );
		const bool bHasHeavyArmor = player->m_bHasHeavyArmor( );
		const float flArmorValue = player->m_ArmorValue( );

		if ( flArmorValue > 0 ) {
			switch ( nHitgroup ) {
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
			case HITGROUP_GEAR:
				return true;
				break;
			case HITGROUP_HEAD:
				return bHasHelmet || bHasHeavyArmor;
				break;
			default:
				return bHasHeavyArmor;
				break;
			}
		}

		return false;
	}

	float ScaleDamage( Player* player, float flDamage, float flArmorRatio, int nHitgroup ) {
		if ( !player )
			return -1.f;

		auto pLocal = g_cl.m_local;
		if ( !pLocal )
			return -1.f;

		auto pWeapon = g_cl.m_weapon;
		if ( !pWeapon )
			return -1.f;

		auto mp_damage_scale_ct_head = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_ct_head" ) );
		auto mp_damage_scale_t_head = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_t_head" ) );
		auto mp_damage_scale_ct_body = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_ct_body" ) );
		auto mp_damage_scale_t_body = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_t_body" ) );


		const int nTeamNum = player->m_iTeamNum( );
		float flHeadDamageScale = nTeamNum == teams_t::TEAM_COUNTERTERRORISTS ? mp_damage_scale_ct_head->GetFloat( ) : mp_damage_scale_t_head->GetFloat( );
		const float flBodyDamageScale = nTeamNum == teams_t::TEAM_COUNTERTERRORISTS ? mp_damage_scale_ct_body->GetFloat( ) : mp_damage_scale_t_body->GetFloat( );

		const bool bIsArmored = IsArmored( player, nHitgroup );
		const bool bHasHeavyArmor = player->m_bHasHeavyArmor( );
		const bool bIsZeus = pWeapon->m_iItemDefinitionIndex( ) == WEAPONTYPE_TASER;

		const float flArmorValue = static_cast< float >( player->m_ArmorValue( ) );

		/*
		if( bHasHeavyArmor )
			flHeadDamageScale *= 0.5f;*/

		if ( !bIsZeus ) {
			switch ( nHitgroup ) {
			case HITGROUP_HEAD:
				flDamage *= 4.0f * flHeadDamageScale;
				break;
			case HITGROUP_STOMACH:
				flDamage *= 1.25f * flBodyDamageScale;
				break;
			case HITGROUP_CHEST:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
			case HITGROUP_GEAR:
				flDamage *= flBodyDamageScale;
				break;
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
				flDamage = ( flDamage * 0.75f ) * flBodyDamageScale;
				break;
			default:
				break;
			}
		}

		// enemy have armor
		if ( bIsArmored ) {
			float flArmorScale = 1.0f;
			float flArmorRatioCalculated = flArmorRatio * 0.5f;
			float flArmorBonusRatio = 0.5f;

			if ( bHasHeavyArmor ) {
				flArmorRatioCalculated *= 0.2f;
				flArmorBonusRatio = 0.33f;
				flArmorScale = 0.25f;
			}

			auto flNewDamage = flDamage * flArmorRatioCalculated;
			auto FlEstimatedDamage = ( flDamage - flDamage * flArmorRatioCalculated ) * flArmorScale * flArmorBonusRatio;

			// Does this use more armor than we have?
			if ( FlEstimatedDamage > flArmorValue )
				flNewDamage = flDamage - flArmorValue / flArmorBonusRatio;

			flDamage = flNewDamage;
		}

		return std::floor( flDamage );
	}

	void TraceLine( const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* ignore, CGameTrace* ptr ) {
		Ray_t ray;
		ray.Init( start, end );
		g_csgo.m_engine_trace->TraceRay( ray, mask, ignore, ptr );
	}

	__forceinline auto DotProduct( const vec3_t& a, const vec3_t& b ) -> float {
		return ( a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ] );
	}

	__forceinline float DistanceToRay( const vec3_t& vecPosition, const vec3_t& vecRayStart, const vec3_t& vecRayEnd, float* flAlong = NULL, vec3_t* vecPointOnRay = NULL ) {
		vec3_t vecTo = vecPosition - vecRayStart;
		vec3_t vecDir = vecRayEnd - vecRayStart;
		float flLength = vecDir.normalize( );

		float flRangeAlong = DotProduct( vecDir, vecTo );
		if ( flAlong ) {
			*flAlong = flRangeAlong;
		}

		float flRange;

		if ( flRangeAlong < 0.0f ) {
			// off start point
			flRange = -vecTo.length ( );

			if ( vecPointOnRay ) {
				*vecPointOnRay = vecRayStart;
			}
		}
		else if ( flRangeAlong > flLength ) {
			// off end point
			flRange = -( vecPosition - vecRayEnd ).length( );

			if ( vecPointOnRay ) {
				*vecPointOnRay = vecRayEnd;
			}
		}
		else { // within ray bounds
			vec3_t vecOnRay = vecRayStart + vecDir * flRangeAlong;
			flRange = ( vecPosition - vecOnRay ).length( );

			if ( vecPointOnRay ) {
				*vecPointOnRay = vecOnRay;
			}
		}

		return flRange;
	}

	void ClipTraceToPlayer( const vec3_t vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, CGameTrace* pGameTrace, Player* player, C_FireBulletData* pData ) {
		CGameTrace playerTrace;
		//ICollideable* pCollideble = pData->m_TargetPlayer->GetCollideable( );

		//if ( !pCollideble )
			//return;

		// get bounding box
		//const Vector vecObbMins = pCollideble->OBBMins( );
		//const Vector vecObbMaxs = pCollideble->OBBMaxs( );
		//const Vector vecObbCenter = ( vecObbMaxs + vecObbMins ) / 2.f;

		const vec3_t vecPosition = player->m_vecOrigin( ) + ( ( player->m_vecMins( ) + player->m_vecMaxs( ) ) * 0.5f );

		float range = DistanceToRay( vecPosition, vecAbsStart, vecAbsEnd );

		Ray_t Ray;
		Ray.Init( vecAbsStart, vecAbsEnd );

		if ( range <= 60.f ) {
			g_csgo.m_engine_trace->ClipRayToEntity( Ray, iMask, player, &playerTrace );

			if ( pData->m_EnterTrace.m_fraction > playerTrace.m_fraction )
				pData->m_EnterTrace = playerTrace;
		}

	}

	void ClipTraceToPlayers( const vec3_t& vecAbsStart, const vec3_t& vecAbsEnd, uint32_t iMask, ITraceFilter* pFilter, CGameTrace* pGameTrace ) {
		float flSmallestFraction = pGameTrace->m_fraction;

		CGameTrace playerTrace;

		for ( int i = 0; i <= g_csgo.m_globals->m_max_clients; ++i ) {
			Player* pPlayer = g_csgo.m_entlist->GetClientEntity< Player* >( i );

			if ( !pPlayer || pPlayer->dormant( ) || !pPlayer->alive( ) )
				continue;

			if ( pFilter && !pFilter->ShouldHitEntity( pPlayer, iMask ) )
				continue;

			//ICollideable* pCollideble = pPlayer->GetCollideable( );
			//if ( !pCollideble )
			//	continue;

			// get bounding box
			//const Vector vecObbMins = pCollideble->OBBMins( );
			//const Vector vecObbMaxs = pCollideble->OBBMaxs( );
			//const Vector vecObbCenter = ( vecObbMaxs + vecObbMins ) / 2.f;

			// calculate world space center
			const vec3_t vecPosition = pPlayer->m_vecOrigin( ) + ( ( pPlayer->m_vecMins( ) + pPlayer->m_vecMaxs( ) ) * 0.5f );

			// calculate distance to ray
			const float range = DistanceToRay( vecPosition, vecAbsStart, vecAbsEnd );

			Ray_t Ray;
			Ray.Init( vecAbsStart, vecAbsEnd );

			if ( range <= 60.f ) {
				g_csgo.m_engine_trace->ClipRayToEntity( Ray, iMask, pPlayer, &playerTrace );
				if ( playerTrace.m_fraction < flSmallestFraction ) {
					// we shortened the ray - save off the trace
					*pGameTrace = playerTrace;
					flSmallestFraction = playerTrace.m_fraction;
				}
			}
		}
	}


	bool TraceToExit( CGameTrace* pEnterTrace, vec3_t vecStartPos, vec3_t vecDirection, CGameTrace* pExitTrace ) {
		constexpr float flMaxDistance = 90.f, flStepSize = 4.f;
		float flCurrentDistance = 0.f;

		static CTraceFilterSimple filter{};

		int iFirstContents = 0;

		bool bIsWindow = 0;
		auto v23 = 0;

		do {
			// Add extra distance to our ray
			flCurrentDistance += flStepSize;

			// Multiply the direction vector to the distance so we go outwards, add our position to it.
			vec3_t vecEnd = vecStartPos + ( vecDirection * flCurrentDistance );

			if ( !iFirstContents )
				iFirstContents = g_csgo.m_engine_trace->GetPointContents( vecEnd, MASK_SHOT );

			int iPointContents = g_csgo.m_engine_trace->GetPointContents( vecEnd, MASK_SHOT );

			if ( !( iPointContents & MASK_SHOT_HULL ) || ( ( iPointContents & CONTENTS_HITBOX ) && iPointContents != iFirstContents ) ) {

				// Let's setup our end position by deducting the direction by the extra added distance
				vec3_t vecStart = vecEnd - ( vecDirection * flStepSize );

				// this gets a bit more complicated and expensive when we have to deal with displacements
				TraceLine( vecEnd, vecStart, MASK_SHOT, nullptr, pExitTrace );

				//// note - dex; this is some new stuff added sometime around late 2017 ( 10.31.2017 update? ).
				//if ( g_Vars.sv_clip_penetration_traces_to_players->GetInt( ) )
					//ClipTraceToPlayers( vecEnd, vecStart, MASK_SHOT, nullptr, pExitTrace );

				// we hit an ent's hitbox, do another trace.
				if ( pExitTrace->m_startsolid && ( pExitTrace->m_surface.m_flags & SURF_HITBOX ) ) {
					filter.SetPassEntity( pExitTrace->m_entity );

					// do another trace, but skip the player to get the actual exit surface 
					TraceLine( vecEnd, vecStartPos, MASK_SHOT_HULL, ( CTraceFilter* )&filter, pExitTrace );

					if ( pExitTrace->hit( ) && !pExitTrace->m_startsolid ) {
						vecEnd = pExitTrace->m_endpos;
						return true;
					}

					continue;
				}

				//Can we hit? Is the wall solid?
				if ( pExitTrace->hit( ) && !pExitTrace->m_startsolid ) {
					if ( game::IsBreakable( pEnterTrace->m_entity ) && game::IsBreakable( pExitTrace->m_entity ) )
						return true;

					if ( pEnterTrace->m_surface.m_flags & SURF_NODRAW ||
						( !( pExitTrace->m_surface.m_flags & SURF_NODRAW ) && pExitTrace->m_plane.m_normal.Dot( vecDirection ) <= 1.f ) ) {
						const float flMultAmount = pExitTrace->m_fraction * 4.f;

						// get the real end pos
						vecStart -= vecDirection * flMultAmount;
						return true;
					}

					continue;
				}

				if ( !pExitTrace->hit( ) || pExitTrace->m_startsolid ) {
					if ( pEnterTrace->did_not_hit_entity( ) && game::IsBreakable( pEnterTrace->m_entity ) ) {
						// if we hit a breakable, make the assumption that we broke it if we can't find an exit (hopefully..)
						// fake the end pos
						pExitTrace = pEnterTrace;
						pExitTrace->m_endpos = vecStartPos + vecDirection;
						return true;
					}
				}
			}
			// max pen distance is 90 units.
		} while ( flCurrentDistance <= flMaxDistance );

		return false;
	}

	bool HandleBulletPenetration( C_FireBulletData* data ) {
		int iEnterMaterial = data->m_EnterSurfaceData->m_game.m_material;
		bool bContentsGrate = ( data->m_EnterTrace.m_contents & CONTENTS_GRATE );
		bool bNoDrawSurf = ( data->m_EnterTrace.m_surface.m_flags & SURF_NODRAW ); // this is valve code :D!
		bool bCannotPenetrate = false;


		if ( !data->m_iPenetrationCount )
		{
			if ( !bNoDrawSurf && bContentsGrate )
			{
				if ( iEnterMaterial != CHAR_TEX_GLASS )
					bCannotPenetrate = iEnterMaterial != CHAR_TEX_GRATE;
			}
		}

		// if we hit a grate with iPenetration == 0, stop on the next thing we hit
		if ( data->m_WeaponData->m_penetration <= 0.f || data->m_iPenetrationCount <= 0 )
			bCannotPenetrate = true;

		// find exact penetration exit
		CGameTrace ExitTrace = { };
		if ( !TraceToExit( &data->m_EnterTrace, data->m_EnterTrace.m_endpos, data->m_vecDirection, &ExitTrace ) ) {
			// ended in solid
			if ( ( g_csgo.m_engine_trace->GetPointContents( data->m_EnterTrace.m_endpos, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) == 0 || bCannotPenetrate )
				return false;
		}

		const surfacedata_t* pExitSurfaceData = g_csgo.m_phys_props->GetSurfaceData( ExitTrace.m_surface.m_surface_props );

		if ( !pExitSurfaceData )
			return false;

		const float flEnterPenetrationModifier = data->m_EnterSurfaceData->m_game.m_penetration_modifier;
		const float flExitPenetrationModifier = pExitSurfaceData->m_game.m_penetration_modifier;
		const float flExitDamageModifier = pExitSurfaceData->m_game.m_damage_modifier;

		const int iExitMaterial = pExitSurfaceData->m_game.m_material;

		float flDamageModifier = 0.f;
		float flPenetrationModifier = 0.f;

		/*
		// percent of total damage lost automatically on impacting a surface
		flDamageModifier = 0.16f;
		flPenetrationModifier = ( flEnterPenetrationModifier + flExitPenetrationModifier ) * 0.5f;*/

		// new penetration method

		if ( iEnterMaterial == CHAR_TEX_GRATE || iEnterMaterial == CHAR_TEX_GLASS ) {
			flPenetrationModifier = 3.0f;
			flDamageModifier = 0.05f;
		}

		else if ( bContentsGrate || bNoDrawSurf ) {
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.16f;
		}

		else {
			flPenetrationModifier = ( flEnterPenetrationModifier + flExitPenetrationModifier ) * 0.5f;
			flDamageModifier = 0.16f;
		}

		// if enter & exit point is wood we assume this is 
		// a hollow crate and give a penetration bonus
		if ( iEnterMaterial == iExitMaterial ) {
			if ( iExitMaterial == CHAR_TEX_WOOD || iExitMaterial == CHAR_TEX_CARDBOARD )
				flPenetrationModifier = 3.0f;
			else if ( iExitMaterial == CHAR_TEX_PLASTIC )
				flPenetrationModifier = 2.0f;
		}

		// calculate damage  
		const float flTraceDistance = ( ExitTrace.m_endpos - data->m_EnterTrace.m_endpos ).length( );
		const float flPenetrationMod = std::max( 0.f, 1.f / flPenetrationModifier );
		const float flTotalLostDamage = ( std::max( 3.f / data->m_WeaponData->m_penetration, 0.f ) *
			( flPenetrationMod * 3.f ) + ( data->m_flCurrentDamage * flDamageModifier ) ) + ( ( ( flTraceDistance * flTraceDistance ) * flPenetrationMod ) / 24 );

		// reduce damage power each time we hit something other than a grate
		data->m_flCurrentDamage -= std::max( 0.f, flTotalLostDamage );

		// do we still have enough damage to deal?
		if ( data->m_flCurrentDamage < 1.f )
			return false;

		// penetration was successful
		// setup new start end parameters for successive trace
		data->m_vecStart = ExitTrace.m_endpos;
		--data->m_iPenetrationCount;

		return true;
	}

	__forceinline bool IsValidHitgroup( int index ) {
		if ( ( index >= HITGROUP_HEAD && index <= HITGROUP_RIGHTLEG ) || index == HITGROUP_GEAR )
			return true;

		return false;
	}

	Player* ToCSPlayer( Player* pEnt ) {
		if ( !pEnt || !pEnt->index() || !pEnt->IsPlayer( ) )
			return nullptr;

		return ( Player* )( pEnt );
	}

	float CanHit( vec3_t& vecEyePos, vec3_t& point, Player* ignore_ent, Player* to_who, int target_hitbox )
	{
		if ( ignore_ent == nullptr || to_who == nullptr )
			return 0;

		vec3_t angles, direction;
		vec3_t tmp = point - vecEyePos;
		float currentDamage = 0;

		Getze::L3D_Math::VectorAngles( tmp, angles );
		Getze::L3D_Math::AngleVectors( angles, &direction );
		direction.normalize( );

		auto local_weapon = g_cl.m_weapon;
		if ( local_weapon != nullptr )
		{
			//if ( FireBullets(  ) )
			//	return currentDamage;
			//else
				//return -1;
		}

		return -1; //That wall is just a bit too thick buddy
	}

	float FireBullets( C_FireBulletData* data ) {
		constexpr float rayExtension = 40.f;

		//This gets set in FX_Firebullets to 4 as a pass-through value.
		//CS:GO has a maximum of 4 surfaces a bullet can pass-through before it 100% stops.
		//Excerpt from Valve: https://steamcommunity.com/sharedfiles/filedetails/?id=275573090
		//"The total number of surfaces any bullet can penetrate in a single flight is capped at 4." -CS:GO Official

		if ( !data->m_Weapon ) {
			data->m_Weapon = g_cl.m_weapon;
			if ( data->m_Weapon ) {
				data->m_WeaponData = g_cl.m_weapon_info;
			}
		}

		data->m_flTraceLength = 0.f;
		data->m_flCurrentDamage = static_cast< float >( data->m_WeaponData->m_damage );

		auto pLocal = g_cl.m_local;
		static CTraceFilterSimple filter{};
		filter.pSkip = data->m_Player;

		data->m_flMaxLength = data->m_WeaponData->m_range;

	//	g_Vars.globals.m_InHBP = true;

		while ( data->m_flCurrentDamage >= 1.f ) {
			// calculate max bullet range
			data->m_flMaxLength -= data->m_flTraceLength;

			// create end point of bullet
			vec3_t vecEnd = data->m_vecStart + data->m_vecDirection * data->m_flMaxLength;

			// create extended end point
			vec3_t vecEndExtended = vecEnd + data->m_vecDirection * rayExtension;

			// ignore local player
			filter.SetPassEntity( pLocal );

			// first trace
			TraceLine( data->m_vecStart, vecEndExtended, MASK_SHOT_HULL | CONTENTS_HITBOX, ( ITraceFilter* )&filter, &data->m_EnterTrace );

			// NOTICE: can remove valve`s hack aka bounding box fix
			// Check for player hitboxes extending outside their collision bounds
			if ( data->m_TargetPlayer ) {
				// clip trace to one player
				ClipTraceToPlayer( data->m_vecStart, vecEndExtended, MASK_SHOT, &data->m_EnterTrace, data->m_TargetPlayer, data );
			}
			else
				ClipTraceToPlayers( data->m_vecStart, vecEndExtended, MASK_SHOT, ( ITraceFilter* )&filter, &data->m_EnterTrace );

			if ( data->m_EnterTrace.m_fraction == 1.f )
				return false;  // we didn't hit anything, stop tracing shoot

			// calculate the damage based on the distance the bullet traveled.
			data->m_flTraceLength += data->m_EnterTrace.m_fraction * data->m_flMaxLength;

			// let's make our damage drops off the further away the bullet is.
			if ( !data->m_bShouldIgnoreDistance )
				data->m_flCurrentDamage *= pow( data->m_WeaponData->m_range, data->m_flTraceLength / 500.f );

		//	C_CSPlayer* pHittedPlayer = ToCSPlayer( ( C_BasePlayer* )data->m_EnterTrace.hit_entity );
			auto pHittedPlayer = ToCSPlayer( (Player*)data->m_EnterTrace.m_entity );

			const int nHitGroup = data->m_EnterTrace.m_hitgroup;
			const bool bHitgroupIsValid = data->m_Weapon->m_iItemDefinitionIndex( ) == WEAPONTYPE_TASER ? ( nHitGroup >= HITGROUP_GENERIC && nHitGroup < HITGROUP_NECK ) : IsValidHitgroup( nHitGroup );
			const bool bTargetIsValid = !data->m_TargetPlayer || ( pHittedPlayer != nullptr && pHittedPlayer->index( ) == data->m_TargetPlayer->index( ) );
			if ( pHittedPlayer != nullptr ) {
				if ( bTargetIsValid && bHitgroupIsValid && pHittedPlayer->IsPlayer( ) && pHittedPlayer->index() <= g_csgo.m_globals->m_max_clients && pHittedPlayer->index( ) > 0 ) {
					data->m_flCurrentDamage = ScaleDamage( pHittedPlayer, data->m_flCurrentDamage, data->m_WeaponData->m_armor_ratio, data->m_Weapon->m_iItemDefinitionIndex( ) == WEAPONTYPE_TASER ? HITGROUP_GENERIC : nHitGroup );
					data->m_iHitgroup = nHitGroup;


					//g_Vars.globals.m_InHBP = false;
					return data->m_flCurrentDamage;
				}
			}

			if ( !data->m_TargetPlayer )
				return -1;

			bool bCanPenetrate = data->m_bPenetration;
			if ( !data->m_bPenetration )
				bCanPenetrate = data->m_EnterTrace.m_contents & CONTENTS_WINDOW;

			if ( !bCanPenetrate )
				break;

			data->m_EnterSurfaceData = g_csgo.m_phys_props->GetSurfaceData( data->m_EnterTrace.m_surface.m_surface_props );

			if ( !data->m_EnterSurfaceData )
				break;

			// check if we reach penetration distance, no more penetrations after that
			// or if our modifier is super low, just stop the bullet
			if ( ( data->m_flTraceLength > 3000.f && data->m_WeaponData->m_penetration < 0.1f ) ||
				data->m_EnterSurfaceData->m_game.m_penetration_modifier < 0.1f ) {
				data->m_iPenetrationCount = 0;
				return -1;
			}

			bool bIsBulletStopped = !HandleBulletPenetration( data );

			if ( bIsBulletStopped )
				return -1;
		}

		//g_Vars.globals.m_InHBP = false;
		return -1.f;
	}
}