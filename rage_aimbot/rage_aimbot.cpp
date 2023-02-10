#include "rage_aimbot.hpp"
#include "../backend/config/config_new.h"

// round to multiple
typedef __declspec( align( 16 ) ) union {
	float f[ 4 ];
	__m128 v;
} m128;

auto round_to_multiple = [ & ]( int in, int multiple ) {
	const auto ratio = static_cast< double >( in ) / multiple;
	const auto iratio = std::lround( ratio );
	return static_cast< int >( iratio * multiple );
};


namespace rage_immortal {
	bool GetBoxOption( mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& ps, bool override_hitscan ) {
		if ( !hitbox )
			return false;

		if ( !hitboxSet )
			return false;

		if ( hitboxSet->GetHitbox( HITBOX_PELVIS ) == hitbox ) {
			ps = settings.imm_body_scale;
			return true; // always on htibox
		}

		if ( hitboxSet->GetHitbox( HITBOX_R_FOOT ) == hitbox || hitboxSet->GetHitbox( HITBOX_L_FOOT ) == hitbox ) {
			ps = settings.imm_point_scale;
			return true; // always on htibox
		}

		switch ( hitbox->m_group ) {
		case HITGROUP_HEAD:
			if ( override_hitscan )
				return false;

			ps = settings.imm_point_scale;
			return true; // always on htibox
			break;
		case HITGROUP_NECK:
			if ( override_hitscan )
				return false;

			ps = settings.imm_point_scale;
			return true; // always on htibox
			break;
		case HITGROUP_STOMACH:
			ps = settings.imm_point_scale;
			return true; // always on htibox
			break;
		case HITGROUP_RIGHTLEG:
		case HITGROUP_LEFTLEG:
			if ( hitboxSet->GetHitbox( HITBOX_R_FOOT ) != hitbox && hitboxSet->GetHitbox( HITBOX_L_FOOT ) != hitbox ) {
				ps = settings.imm_point_scale;
				return true; // always on htibox
			}
			break;
		case HITGROUP_RIGHTARM:
		case HITGROUP_LEFTARM:
			ps = settings.imm_point_scale;
			return true; // always on htibox
			break;

		default:
			return false;
			break;
		}
	}; // im not sure i need that there but let it go

	bool Run( CUserCmd* cmd ) {
		if ( !settings.imm_rage )
			return false;

		// there should be random init
		//

		// sanity
		auto weapon = g_cl.m_weapon;
		if ( !weapon )
			return false;

		auto weaponInfo = weapon->GetWpnData( );
		if ( !weaponInfo )
			return false;

		auto local = g_cl.m_local;
		if ( !local )
			return false;

		// run aim on zeus
		if ( weapon->m_iItemDefinitionIndex( ) != WEAPONTYPE_TASER && weaponInfo->m_weapon_type == WEAPONTYPE_KNIFE || weaponInfo->m_weapon_type == WEAPONTYPE_GRENADE || weaponInfo->m_weapon_type == WEAPONTYPE_C4 )
			return false;

		// setup rage options idk plm
		
		// sanity check
		if ( !local || !local->alive( ) ) {
			rage_data::m_pLastTarget = nullptr;
			rage_data::m_bFailedHitchance = false;

			return false;
		}

		// setup stuff
		rage_data::m_pLocal = local;
		rage_data::m_pWeapon = weapon;
		rage_data::m_pWeaponInfo = weaponInfo;
		//rage_data::m_pSendPacket = send_packet;
		rage_data::m_pCmd = cmd;
		rage_data::m_bEarlyStop = false;

		// globals.mindmg......

		// auto r8
		if ( weapon->m_iItemDefinitionIndex( ) == REVOLVER ) {
			if ( !( rage_data::m_pCmd->m_buttons & IN_RELOAD ) && weapon->m_iClip1( ) ) {
				static float cockTime = 0.f;
				float curtime = local->m_nTickBase( ) * g_csgo.m_globals->m_interval;
				rage_data::m_pCmd->m_buttons &= ~IN_ATTACK2;

				if ( g_cl.CanFireWeapon( ) ) {
					if ( cockTime <= curtime ) {
						if ( weapon->m_flNextSecondaryAttack() <= curtime )
							cockTime = curtime + 0.234375f;
						else
							rage_data::m_pCmd->m_buttons |= IN_ATTACK2;
					}
					else
						rage_data::m_pCmd->m_buttons |= IN_ATTACK;
				}
				else {
					cockTime = curtime + 0.234375f;
					rage_data::m_pCmd->m_buttons &= ~IN_ATTACK;
				}
			}
		}
		else if ( !g_cl.CanFireWeapon( ) ) {
			if ( !rage_data::m_pWeaponInfo->m_is_full_auto )
				rage_data::m_pCmd->m_buttons &= ~IN_ATTACK;

			if ( g_csgo.m_globals->m_curtime < rage_data::m_pLocal->m_flNextAttack( ) || rage_data::m_pWeapon->m_iClip1( ) < 1 )
				return false;
		}

		if ( rage_data::m_pLastTarget != nullptr && !rage_data::m_pLastTarget->alive( ) ) {
			rage_data::m_pLastTarget = nullptr;
		}

		bool ret = RunInternal( );
		return ret;
	}

	bool RunInternal( ) {
		auto cmd_backup = *rage_data::m_pCmd;

		rage_data::m_bRePredict = false;
		rage_data::m_bPredictedScope = false;
		rage_data::m_bNoNeededScope = true;

		rage_data::m_flSpread = rage_data::m_pWeapon->GetSpread( );
		rage_data::m_flInaccuracy = rage_data::m_pWeapon->GetInaccuracy( );

		auto succes = RunHitscan( );
		const bool bOnLand = !( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) && rage_data::m_pLocal->m_fFlags( ) & FL_ONGROUND;

		float feet = 0.f;
		if ( ( rage_data::m_pWeapon->m_iItemDefinitionIndex( ) == SCAR20 || rage_data::m_pWeapon->m_iItemDefinitionIndex( ) == G3SG1 ) && succes.second.target && succes.second.target->player ) {
			auto dist = ( rage_data::m_pLocal->m_vecOrigin( ).dist_to( succes.second.target->player->m_vecOrigin( ) ) );
			auto meters = dist * 0.0254f;
			feet = round_to_multiple( meters * 3.281f, 5 );
		}

		bool htcFailed = rage_data::m_bFailedHitchance;
		if ( rage_data::m_bFailedHitchance ) {
			rage_data::m_bFailedHitchance = false;
			rage_data::m_bNoNeededScope = false;
		}

		if ( feet > 0.f ) {
			if ( feet <= 60.f ) {
				rage_data::m_bNoNeededScope = true;
			}
			else {
				rage_data::m_bNoNeededScope = false;
			}

			// fakeduck no need scope false;
		}

		if ( settings.ascope && rage_data::m_pWeaponInfo->m_weapon_type == WEAPONTYPE_SNIPER_RIFLE && rage_data::m_pWeapon->m_zoomLevel( ) <= 0 && rage_data::m_pLocal->m_fFlags( ) & FL_ONGROUND && !rage_data::m_bNoNeededScope ) {
			rage_data::m_pCmd->m_buttons |= IN_ATTACK2;
			rage_data::m_pCmd->m_buttons &= ~IN_ATTACK;
			rage_data::m_pWeapon->m_zoomLevel( ) = 1;
			rage_data::m_bPredictedScope = true;
			rage_data::m_bRePredict = true;
		}

		auto correction = rage_data::m_pLocal->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( );
		if ( rage_data::m_pCmd->m_buttons & IN_ATTACK ) {
			rage_data::m_pCmd->m_view_angles -= correction;
			rage_data::m_pCmd->m_view_angles.normalize( );
		}

		return succes.first;
	}

	void AddPoint( Player* player, LagRecord* record, int side, std::vector<std::pair<vec3_t, bool>>& points, const vec3_t& point, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, bool isMultipoint ) {
		auto pointTransformed = point;
		if ( !hitbox )
			return;

		if ( !hitboxSet )
			return;

		points.push_back( std::make_pair( pointTransformed, isMultipoint ) );
	}

	/*
		static int ClipRayToHitbox( const Ray& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, CGameTrace& trace ) {
			static auto fn = pattern::find( g_csgo.m_client_dll, "55 8B EC 83 E4 F8 F3 0F 10 42" );

			if ( !fn || !hitbox )
				return -1;

			trace.m_fraction = 1.0f;
			trace.m_startsolid = false;

			return reinterpret_cast < int( __fastcall* )( const Ray&, mstudiobbox_t*, matrix3x4_t&, CGameTrace& ) > ( fn )( ray, hitbox, matrix, trace );
		}
	*/

	bool Hitchance( C_AimPoint* pPoint, const vec3_t& vecStart, float flChance ) {
		if ( flChance <= 0.0f )
			return true;

		if ( !pPoint )
			return false;

		const ang_t& angle{ }; // this might crash

		constexpr float HITCHANCE_MAX = 100.f;
		constexpr int   SEED_MAX = 255;

		vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
		float      inaccuracy, spread;
		CGameTrace tr;
		size_t     total_hits{ }, needed_hits{ ( size_t )std::ceil( ( pPoint->hitchance * SEED_MAX ) / HITCHANCE_MAX ) };

		// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
		inaccuracy = g_cl.m_weapon->GetInaccuracy( );
		spread = g_cl.m_weapon->GetSpread( );

		if ( settings.automatic_hc_increase ) {
			auto weapon = g_cl.m_weapon;
			if ( !weapon )
				return false;

			if ( weapon->m_iItemDefinitionIndex( ) == SCAR20 || weapon->m_iItemDefinitionIndex( ) == G3SG1 ) {
				if ( ( inaccuracy <= 0.0380f ) && weapon->m_zoomLevel( ) == 0 ) {
					needed_hits = 50.f;
					return true;
				}
			}
		}

		// get needed directional vectors.
		math::AngleVectors( angle, &fwd, &right, &up );

		// iterate all possible seeds.
		for ( int i{ }; i <= SEED_MAX; ++i ) {
			// get spread.
			wep_spread = g_cl.m_weapon->CalculateSpread( i, inaccuracy, spread );

			// get spread direction.
			dir = ( fwd + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized( );

			// get end of trace.
			end = start + ( dir * g_cl.m_weapon_info->m_range );

			// setup ray and trace.
			g_csgo.m_engine_trace->ClipRayToEntity( Ray( start, end ), MASK_SHOT, pPoint->target->player, &tr );

			// check if we hit a valid player / hitgroup on the player and increment total hits.
			if ( tr.m_entity == pPoint->target->player && game::IsValidHitgroup( tr.m_hitgroup ) )
				++total_hits;

			// we made it.
			if ( total_hits >= needed_hits )
				return true;

			// we cant make it anymore.
			if ( ( SEED_MAX - i + total_hits ) < needed_hits )
				return false;
		}

		return false;
	}

	
	bool IsPointAccurate( C_AimPoint* point, const vec3_t& start ) {
		if ( !g_cl.CanFireWeapon( ) )
			return false;

		auto Weapon = g_cl.m_weapon;
		auto weaponInfo = g_cl.m_weapon->GetWpnData( );
		auto m_weapon_id = Weapon->m_iItemDefinitionIndex( );

		bool not_auto = m_weapon_id == REVOLVER || m_weapon_id == AWP || m_weapon_id == SSG08 || m_weapon_id == WEAPONTYPE_TASER || weaponInfo->m_weapon_type == WEAPONTYPE_SHOTGUN;

		float percentage = 0.40;
		float v4 = percentage * ( rage_data::m_pLocal->m_bIsScoped( ) ? weaponInfo->m_max_player_speed_alt : weaponInfo->m_max_player_speed );

		// unpredicted velocity
		// duck ammount

		auto dist = ( rage_data::m_pLocal->m_vecOrigin( ).dist_to( point->target->player->m_vecOrigin( ) ) );
		auto meters = dist * 0.0254f;
		float flDistanceInFeet = round_to_multiple( meters * 3.281f, 5 );
		int iHealth = point->target->player->m_iHealth( );
		bool bOnLand = !( rage_data::m_pLocal->m_fFlags( ) & FL_ONGROUND ) && rage_data::m_pLocal->m_fFlags( ) & FL_ONGROUND;

		if ( bOnLand )
			return false;

		float hitchance = settings.imm_hc;
		if ( hitchance > 0.0f ) {
			if ( !Hitchance( point, start, hitchance * 0.01f ) ) {
				rage_data::m_bFailedHitchance = true;
				return false;
			}
		}

		rage_data::m_bFailedHitchance = false;
		return true;
	}

	matrix3x4_t* GetBoneMatrix( ) {
		 // we dont need this shit
	}

	void Multipoint( Player* player, int side, std::vector< vec3_t >& points, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& pointScale, int hitboxIndex ) {
		matrix3x4_t boneMatrix[ 128 ];

		if ( !hitbox || !boneMatrix )
			return;

		if ( !hitboxSet )
			return;

		vec3_t center = ( hitbox->m_maxs + hitbox->m_mins ) * 0.5f;

		// transofrm center
		auto centerTrans = center;
		math::VectorTransform( centerTrans, boneMatrix[ hitbox->m_bone ], centerTrans );

		points.push_back( { centerTrans } );

		auto local = g_cl.m_local;
		if ( !local )
			return; // im trying to do this in another method that i did in my sup where i ported it and it works but it has bugs because of way i did it

		if ( !settings.imm_static_pointscale && rage_data::m_pWeapon && hitbox->m_radius > 0.0f ) {
			pointScale = 0.91f; // we can go high here because the new multipoint is perfect

			auto spreadCone = g_cl.m_weapon->GetSpread( ) + g_cl.m_weapon->GetInaccuracy( );
			auto dist = centerTrans.dist_to( g_cl.m_local->GetShootPosition( ) );

			dist /= sinf( math::deg_to_rad( 90.0f - math::rad_to_deg( spreadCone ) ) );

			spreadCone = sinf( spreadCone );

			float radiusScaled = ( hitbox->m_radius - ( dist * spreadCone ) );
			if ( radiusScaled < 0.0f ) {
				radiusScaled *= -1.f;
			}

			float ps = pointScale;
			pointScale = ( radiusScaled / hitbox->m_radius );
			pointScale = math::Clamp( pointScale, 0.0f, ps );
		}

		if ( hitbox->m_radius <= 0.0f ) {
			if ( hitboxIndex == HITBOX_R_FOOT || hitboxIndex == HITBOX_L_FOOT ) {
				float d1 = ( hitbox->m_mins.z - center.z ) * 0.425f;

				if ( hitboxIndex == HITBOX_L_FOOT )
					d1 *= -1.f;

				// we wont add checkbox to enable it
				points.push_back( { ( hitbox->m_maxs.x - center.x ) * pointScale + center.x, center.y, center.z } );
				points.push_back( { ( hitbox->m_mins.x - center.x ) * pointScale + center.x, center.y, center.z } );
	
			}
		}
		else {
			float r = hitbox->m_radius * pointScale;
			if ( hitboxIndex == HITBOX_HEAD ) {
				pointScale = math::Clamp<float>( pointScale, 0.1f, 0.95f );
				r = hitbox->m_radius * pointScale;

				vec3_t right{ hitbox->m_maxs.x, hitbox->m_maxs.y, hitbox->m_maxs.z + r };
				vec3_t left{ hitbox->m_maxs.x, hitbox->m_maxs.y, hitbox->m_maxs.z - r };

				points.push_back( { right } );
				points.push_back( { left } );

				// we dont

				constexpr float rotation = 0.70710678f;

				// ok, this looks ghetto as shit but we have to clamp these to not have these be off too much
				pointScale = std::clamp<float>( pointScale, 0.1f, 0.95f );
				r = hitbox->m_radius * pointScale;

				points.push_back( { hitbox->m_maxs.x + ( rotation * r ), hitbox->m_maxs.y + ( -rotation * r ), hitbox->m_maxs.z } );

				pointScale = std::clamp<float>( pointScale, 0.1f, 0.95f );
				r = hitbox->m_radius * pointScale;

				vec3_t back{ center.x, hitbox->m_maxs.y - r, center.z };
				points.push_back( { back } );
			}
		}
	}

	bool SetupTargets( ) {
		rage_data::m_targets.clear( );
		rage_data::m_aim_points.clear( );
		rage_data::m_aim_data.clear( );
		rage_data::m_pBestTarget = nullptr;

		for ( int idx = 1; idx <= g_csgo.m_globals->m_max_clients; ++idx ) {
			auto player = g_csgo.m_entlist->GetClientEntity< Player* >( idx );
			if ( !player || player == rage_data::m_pLocal || !player->alive( ) || /*player->m_bGunGameImmunity( ) ||*/ g_cl.m_local->m_iTeamNum( ) /* not sure abt this */ )
				continue;

			
			if ( !player->dormant( ) )
				GeneratePoints( player, rage_data::m_targets, rage_data::m_aim_points );
			
		}

		if ( rage_data::m_targets.empty( ) )
			return false;

		if ( rage_data::m_aim_points.empty( ) ) {

			//for ( auto& target : rage_data::m_targets )
				//target.backup.Apply( target.player );

			return false;
		}

		for ( auto& target : rage_data::m_targets ) {
			std::vector<C_AimPoint> tempPoints;

			for ( auto& point : rage_data::m_aim_points ) {
				if ( point.target->player->index() == target.player->index( ) )
					tempPoints.emplace_back( point );
			}

			if ( !tempPoints.empty( ) ) {
				//target.record->Apply( target.player );

				// scan all valid points
				for ( size_t i = 0u; i < tempPoints.size( ); ++i )
					ScanPoint( &tempPoints.at( i ) );

				//target.backup.Apply( target.player );
				

				std::vector<C_AimPoint> finalPoints;

				if ( !finalPoints.empty( ) )
					finalPoints.clear( );

				int hp = target.player->m_iHealth( );

				for ( auto& p : tempPoints ) {
					if ( p.damage >= 1.0f ) {
						if ( !p.isHead ) {
							if ( p.damage >= hp ) {
								p.isLethal = true;
								target.hasLethal = true;
							}

							target.onlyHead = false;
						}

						if ( p.center ) {
							target.hasCenter = true;
						}

						// is this point valid? fuck yeah, let's push back
						finalPoints.push_back( p );
						continue;
					}
				}

				// don't even bother adding an entry if there are no valid aimpoints
				if ( !finalPoints.empty( ) ) {
					target.points = finalPoints;

					// add a valid entry
					rage_data::m_aim_data.emplace_back( target );
				}
			}
		}

		rage_data::m_aim_points.clear( );

		if ( rage_data::m_aim_data.empty( ) ) {
			//for ( auto& target : rage_data::m_targets )
				//target.backup.Apply( target.player );

			return false;
		}

		SelectBestTarget( );

		if ( rage_data::m_pBestTarget && !rage_data::m_aim_points.empty( ) )
			return true;

		// backup targets
		//for ( auto& target : rage_data::m_targets )
			//target.backup.Apply( target.player );

		return false;
	}

	std::pair<bool, C_AimPoint> RunHitscan( ) {
		rage_data::m_vecEyePos = g_cl.m_local->GetShootPosition();

		if ( !SetupTargets( ) )
			return { false, C_AimPoint( ) };

		C_AimPoint* bestPoint = nullptr;
		for ( auto& p : rage_data::m_aim_points ) {
			if ( p.damage < 1.0f )
				continue;

			// if we got no bestPoint yet, we should always take the first point
			if ( !bestPoint ) {
				bestPoint = &p;
				continue;
			}

			// get vars.
			int iHealth = p.target->player->m_iHealth( );
			float flMaxBodyDamage = penetration::scale( p.target->player, rage_data::m_pWeaponInfo->m_damage, rage_data::m_pWeaponInfo->m_armor_ratio, HITGROUP_STOMACH );
			float flMaxHeadDamage = penetration::scale( p.target->player, rage_data::m_pWeaponInfo->m_damage, rage_data::m_pWeaponInfo->m_armor_ratio, HITGROUP_HEAD );

			if ( p.isLethal ) {
				// don't shoot at head if we can shoot body and kill enemy
				if ( !p.isBody ) {
					continue; // go to next point
				}

				// we always want this point, due to it being either choosen by bShouldBaim or p.is_should_baim
				if ( bestPoint->isHead ) {
					bestPoint = &p;

					continue;
				}

				if ( ( int )p.damage >= int( bestPoint->damage ) || p.center ) {
					bestPoint = &p;
					break;
				}
			}
			else {
				// not sure about these
				auto bestTarget = p.target;
				if ( bestTarget->preferHead ) {
					// oneshot!!
					if ( p.isHead && ( p.damage >= p.target->player->m_iHealth( ) ) ) {
						bestPoint = &p;
						break;
					}

				}

				if ( int( p.damage ) >= int( bestPoint->damage ) ) {
					bestPoint = &p;
					continue;
				}
			}

			auto bestTarget = bestPoint->target;
			if ( bestTarget->preferBody ) {
				if ( bestPoint->isBody != p.isBody ) {
					bestPoint = bestPoint->isBody ? bestPoint : &p;
					continue;
				}
			}
			else {
				if ( bestTarget->preferHead ) {
					if ( bestPoint->isHead != p.isHead ) {
						bestPoint = bestPoint->isHead ? bestPoint : &p;
						continue;
					}
				}
			}
		}

		bool result = false;
		if ( bestPoint ) {
			if ( IsPointAccurate( bestPoint, rage_data::m_vecEyePos ) ) {
				bool bDidDelayHeadShot = rage_data::m_bDelayedHeadAim;
				rage_data::m_bDelayedHeadAim = false;
				//g_Vars.globals.m_bDelayingShot[ bestPoint->target->record->m_player->index( ) ] = false;

				if ( AimAtPoint( bestPoint ) ) {
					if ( rage_data::m_pCmd->m_buttons & IN_ATTACK ) {
						rage_data::m_pLastTarget = rage_data::m_pBestTarget->player;
						result = true;
					}
				}
			}
		}


		return { result, *bestPoint };
	}

	bool AimAtPoint( C_AimPoint* bestPoint ) {
		auto pLocal = g_cl.m_local;
		if ( !pLocal && pLocal->alive( ) )
			return false;

		auto pWeapon = g_cl.m_weapon;
		if ( !pWeapon )
			return false;

		rage_data::m_pCmd->m_buttons &= ~IN_USE;

		// todo: aimstep
		vec3_t delta = bestPoint->position - rage_data::m_vecEyePos;
		delta.normalize( );

		ang_t aimAngles = delta.ToEulerAngles( );
		aimAngles.normalize( );

		//if ( !g_Vars.rage.silent_aim )
		//	Interfaces::m_pEngine->SetViewAngles( aimAngles );

		rage_data::m_pCmd->m_view_angles = aimAngles;
		rage_data::m_pCmd->m_view_angles.normalize( );

	//	g_Vars.globals.CorrectShootPosition = true;
	//	g_Vars.globals.AimPoint = bestPoint->position;
	//	g_Vars.globals.ShootPosition = rage_data::m_vecEyePos;

		rage_data::m_iChokedCommands = -1;
		rage_data::m_bFailedHitchance = false;

		*rage_data::m_pSendPacket = false;
		rage_data::m_pCmd->m_buttons |= IN_ATTACK;

		return true;
	}

	int convert_hitbox_to_hitgroup( int hitbox ) {
		switch ( hitbox ) {
		case HITBOX_HEAD:
		case HITBOX_NECK:
			return HITGROUP_HEAD;
		case HITBOX_LOWER_NECK:
		case HITBOX_UPPER_CHEST:
		case HITBOX_CHEST:
			return HITGROUP_CHEST;
		case HITBOX_PELVIS:
		case HITBOX_L_THIGH:
		case HITBOX_R_THIGH:
			return HITGROUP_STOMACH;
		case HITBOX_L_CALF:
		case HITBOX_L_FOOT:
			return HITGROUP_LEFTLEG;
		case HITBOX_R_CALF:
		case HITBOX_R_FOOT:
			return HITGROUP_RIGHTLEG;
		case HITBOX_L_FOREARM:
		case HITBOX_L_HAND:
			return HITGROUP_LEFTARM;
		case HITBOX_R_FOREARM:
		case HITBOX_R_HAND:
			return HITGROUP_RIGHTARM;
		default:
			return HITGROUP_STOMACH;
		}
	}


	void ScanPoint( C_AimPoint* pPoint ) {
		if ( !pPoint || !pPoint->target || !pPoint->target->player )
			return;

		// no do
		if ( pPoint->hitboxIndex == HITBOX_HEAD ) {
			return;
		}

		imm_autowall::C_FireBulletData fireData;
		fireData.m_bPenetration = true; // mo more checkbox

		auto dir = pPoint->position - rage_data::m_vecEyePos;
		dir.normalize( );

		fireData.m_vecStart = rage_data::m_vecEyePos;
		fireData.m_vecDirection = dir;
		fireData.m_iHitgroup = convert_hitbox_to_hitgroup( pPoint->hitboxIndex );
		fireData.m_Player = rage_data::m_pLocal;
		fireData.m_TargetPlayer = pPoint->target->player;
		fireData.m_WeaponData = rage_data::m_pWeaponInfo;
		fireData.m_Weapon = rage_data::m_pWeapon;

		pPoint->damage = imm_autowall::FireBullets( &fireData );
		pPoint->penetrated = fireData.m_iPenetrationCount < 4;

		int hp = pPoint->target->player->m_iHealth( );
		float mindmg = ( settings.mind_mg_imm > 100 ? hp + ( settings.mind_mg_imm - 100 ) : settings.mind_mg_imm );
		if ( settings.overideeeunimm && settings.min_keyu ) {
			mindmg = settings.mindmgovim > 100 ? hp + ( settings.mindmgovim - 100 ) : settings.mindmgovim;
		}
		else if ( !pPoint->penetrated ) {
			mindmg = settings.mind_mg_imm > 100 ? hp + ( settings.mind_mg_imm - 100 ) : settings.mind_mg_imm;
		}

		// if lethal, we still want to shoot
		if ( pPoint->damage >= 20 && pPoint->damage >= ( hp + 2 ) )
			mindmg = hp;

		if ( ( convert_hitbox_to_hitgroup( pPoint->hitboxIndex ) == HITGROUP_HEAD ) && fireData.m_iHitgroup != HITGROUP_HEAD ) {
			pPoint->damage = 0.f;
			return;
		}

		if ( pPoint->damage >= mindmg ) {
			pPoint->hitgroup = fireData.m_EnterTrace.m_hitgroup;
			pPoint->healthRatio = int( float( pPoint->target->player->m_iHealth( ) ) / pPoint->damage ) + 1;

			auto hitboxSet = ( *( studiohdr_t** )pPoint->target->player->m_studioHdr( ) )->GetHitboxSet( pPoint->target->player->m_nHitboxSet( ) );
			auto hitbox = hitboxSet->GetHitbox( pPoint->hitboxIndex );

			pPoint->isHead = pPoint->hitboxIndex == HITBOX_HEAD || pPoint->hitboxIndex == HITBOX_NECK;
			pPoint->isBody = pPoint->hitboxIndex == HITBOX_PELVIS || pPoint->hitboxIndex == HITGROUP_STOMACH;

			// nospread enabled
			if ( rage_data::m_flSpread != 0.0f && rage_data::m_flInaccuracy != 0.0f ) {	
				int iHits = 0;
				pPoint->hitchance = float( iHits ) / 0.5f;
			}
			else {
				pPoint->hitchance = 0.f;
			}
		}
		else {
			pPoint->healthRatio = 100;
			pPoint->hitchance = 0.0f;
			pPoint->damage = 0.f;
		}
	}

	void SinCos( float a, float* s, float* c ) {
		*s = sin( a );
		*c = cos( a );
	}

	void AngleVectors( const ang_t& angles, vec3_t& forward ) {
		float	sp, sy, cp, cy;

		SinCos( math::deg_to_rad( angles[ 1 ] ), &sy, &cy );
		SinCos( math::deg_to_rad( angles[ 0 ] ), &sp, &cp );

		forward.x = cp * cy;
		forward.y = cp * sy;
		forward.z = -sp;
	}

	float GetFov( const ang_t& viewAngle, const vec3_t& start, const vec3_t& end ) {

		vec3_t dir, fw;

		// get direction and normalize.
		dir = ( end - start ).normalized( );

		// get the forward direction vector of the view angles.
		AngleVectors( viewAngle, fw );

		// get the angle between the view angles forward directional vector and the target location.
		return std::max( math::rad_to_deg( std::acos( fw.Dot( dir ) ) ), 0.f );
	}

	void SelectBestTarget( ) {
		if ( rage_data::m_aim_data.empty( ) )
			return;

		auto CheckTargets = [ & ]( C_AimTarget* a, C_AimTarget* b ) -> bool {
			if ( !a || !b )
				goto fuck_yeah;

			if ( rage_data::m_pLastTarget != nullptr && rage_data::m_pLastTarget->alive( ) && a->player->index( ) == rage_data::m_pLastTarget->index( ) )
				return true;

			switch ( settings.targte_sele ) {
			case SELECT_HIGHEST_DAMAGE: {
				float damageFirstTarget{}, damageSecondTarget{};

				for ( auto& p : a->points )
					damageFirstTarget += p.damage;

				for ( auto& p : b->points )
					damageSecondTarget += p.damage;

				return damageFirstTarget > damageSecondTarget;
				break;
			}
			case SELECT_FOV: {
				float fovToFirstTarget, fovToSecondTarget;

				auto first = a->player->WorldSpaceCenter( );
				auto second = b->player->WorldSpaceCenter( );

				fovToFirstTarget = GetFov( rage_data::m_pCmd->m_view_angles, rage_data::m_vecEyePos, first );
				fovToSecondTarget = GetFov( rage_data::m_pCmd->m_view_angles, rage_data::m_vecEyePos, second );

				return fovToFirstTarget < fovToSecondTarget;
				break;
			}
			case SELECT_LOWEST_HP:
				return a->player->m_iHealth( ) <= b->player->m_iHealth( ); break;
			case SELECT_LOWEST_DISTANCE:
				return a->player->m_vecOrigin( ).dist_to( rage_data::m_pLocal->m_vecOrigin( ) ) <= b->player->m_vecOrigin( ).dist_to( rage_data::m_pLocal->m_vecOrigin( ) ); break;
			}
		fuck_yeah:
			// this might not make sense to you, but it actually does.
			return ( a != nullptr || ( a != nullptr && b != nullptr && a == b ) ) ? true : false;
		};

		for ( auto& data : rage_data::m_aim_data ) {
			// this should never happen, just to be extra safe.
			if ( data.points.empty( ) )
				continue;

			if ( !rage_data::m_pBestTarget ) {
				rage_data::m_pBestTarget = &data;
				rage_data::m_aim_points = data.points;

				// we only have one entry (target)? let's skip target selection..
				if ( rage_data::m_aim_data.size( ) == 1 )
					break;
				else
					continue;
			}

			if ( rage_data::m_pLastTarget != nullptr && rage_data::m_pLastTarget->alive( ) && data.player->index() != rage_data::m_pLastTarget->index( ) && rage_data::m_pLastTarget->index( ) <= 64 ) {
				continue;
			}

			if ( CheckTargets( &data, rage_data::m_pBestTarget ) ) {
				rage_data::m_pBestTarget = &data;
				rage_data::m_aim_points = data.points;
				continue;
			}
		}
	}

	int GeneratePoints( Player* player, std::vector<C_AimTarget>& aim_targets, std::vector<C_AimPoint>& aim_points ) {
		player_info_t info;
		if ( !g_csgo.m_engine->GetPlayerInfo( player->index(), &info ) )
			return 0;

		auto local = g_cl.m_local;
		if ( !local )
			return false;

		auto animState = player->m_PlayerAnimState( );

		if ( !animState )
			return 0;

		const model_t* model = player->GetModel( );
		if ( !model )
			return 0;

		auto hdr = g_csgo.m_model_info->GetStudioModel( model );
		if ( !hdr )
			return 0;

		auto hitboxSet = hdr->GetHitboxSet( player->m_nHitboxSet( ) );
		if ( !hitboxSet )
			return 0;

		auto& aim_target = aim_targets.emplace_back( );
		aim_target.player = player;
		aim_target.overrideHitscan = false;

		bool prefer_body = settings.bodyprefer || !( player->m_fFlags() & FL_ONGROUND );
		aim_target.preferBody = prefer_body;//&& this->OverrideHitscan( player, record );
		aim_target.preferHead = !aim_target.preferBody;

		auto addedPoints = 0;
		for ( int i = 0; i < HITBOX_MAX; i++ ) {
			auto hitbox = hitboxSet->GetHitbox( i );
			float ps = 0.0f;
			if ( !GetBoxOption( hitbox, hitboxSet, ps, aim_target.overrideHitscan ) ) {
				continue;
			}

			bool bDelayLimb = false;
			auto bIsLimb = hitbox->m_group == HITGROUP_LEFTARM || hitbox->m_group == HITGROUP_RIGHTARM || hitbox->m_group == HITGROUP_RIGHTLEG || hitbox->m_group == HITGROUP_LEFTLEG;
			if ( bIsLimb ) {
				// we're not moving, let's delay the limb shot
				if ( rage_data::m_pLocal->m_vecVelocity( ).length_2d( ) < 3.25f ) {
					bDelayLimb = true;
				}
				// we're moving, let's not shoot at limbs at all
				else {
					continue;
				}

			}

			ps *= 0.01f;

			std::vector< vec3_t > points;
			Multipoint( player, 0, points, hitbox, hitboxSet, ps, i );

			if ( !points.size( ) )
				continue;

			for ( const auto& point : points ) {
				C_AimPoint& p = aim_points.emplace_back( );

				//p.position = point.first;
				//p.center = !point.second;
				p.target = &aim_target;
				//p.record = record;
				p.hitboxIndex = i;

			//	if ( point.second )
				//	p.pointscale = ps;
			//	else
					p.pointscale = 0.f;

				++addedPoints;
			}
		}

		return addedPoints;
	}


	// i finished that on my other supremacy, i might add it here too
}