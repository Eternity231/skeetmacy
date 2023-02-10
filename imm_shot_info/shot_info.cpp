#include "shot_info.hpp"
#include "../imm_animfix/imm_animfix.hpp"
#include "../hasers/hasher.hpp"
using namespace LagController;
namespace ShotInformations {
	struct C_TraceData {
		bool is_resolver_issue;
		bool is_correct;
	};

	typedef __declspec( align( 16 ) ) union {
		float f[ 4 ];
		__m128 v;
	} m128;

	__forceinline __m128 sqrt_ps( const __m128 squared ) {
		return _mm_sqrt_ps( squared );
	}

	bool IntersectSegmentToSegment( Vector s1, Vector s2, Vector k1, Vector k2, float radius ) {
		static auto constexpr epsilon = 0.00000001;

		auto u = s2 - s1;
		auto v = k2 - k1;
		const auto w = s1 - k1;

		const auto a = u.Dot( u );
		const auto b = u.Dot( v );
		const auto c = v.Dot( v );
		const auto d = u.Dot( w );
		const auto e = v.Dot( w );
		const auto D = a * c - b * b;
		float sn, sd = D;
		float tn, td = D;

		if ( D < epsilon ) {
			sn = 0.0;
			sd = 1.0;
			tn = e;
			td = c;
		}
		else {
			sn = b * e - c * d;
			tn = a * e - b * d;

			if ( sn < 0.0 ) {
				sn = 0.0;
				tn = e;
				td = c;
			}
			else if ( sn > sd ) {
				sn = sd;
				tn = e + b;
				td = c;
			}
		}

		if ( tn < 0.0 ) {
			tn = 0.0;

			if ( -d < 0.0 )
				sn = 0.0;
			else if ( -d > a )
				sn = sd;
			else {
				sn = -d;
				sd = a;
			}
		}
		else if ( tn > td ) {
			tn = td;

			if ( -d + b < 0.0 )
				sn = 0;
			else if ( -d + b > a )
				sn = sd;
			else {
				sn = -d + b;
				sd = a;
			}
		}

		const float sc = abs( sn ) < epsilon ? 0.0 : sn / sd;
		const float tc = abs( tn ) < epsilon ? 0.0 : tn / td;

		m128 n;
		auto dp = w + u * sc - v * tc;
		n.f[ 0 ] = dp.Dot( dp );
		const auto calc = sqrt_ps( n.v );
		auto shit = reinterpret_cast< const m128* >( &calc )->f[ 0 ];
		//printf( "shit %f | rad %f\n", shit, radius );
		return shit < radius;
	}

	bool IntersectionBoundingBox( const Vector& src, const Vector& dir, const Vector& min, const Vector& max, Vector* hit_point ) {
		/*
			 Fast Ray-Box Intersection
			 by Andrew Woo
			 from "Graphics Gems", Academic Press, 1990
		 */

		constexpr auto NUMDIM = 3;
		constexpr auto RIGHT = 0;
		constexpr auto LEFT = 1;
		constexpr auto MIDDLE = 2;

		bool inside = true;
		char quadrant[ NUMDIM ];
		int i;

		// Rind candidate planes; this loop can be avoided if
		// rays cast all from the eye(assume perpsective view)
		Vector candidatePlane;
		for ( i = 0; i < NUMDIM; i++ ) {
			if ( src[ i ] < min[ i ] ) {
				quadrant[ i ] = LEFT;
				candidatePlane[ i ] = min[ i ];
				inside = false;
			}
			else if ( src[ i ] > max[ i ] ) {
				quadrant[ i ] = RIGHT;
				candidatePlane[ i ] = max[ i ];
				inside = false;
			}
			else {
				quadrant[ i ] = MIDDLE;
			}
		}

		// Ray origin inside bounding box
		if ( inside ) {
			if ( hit_point )
				*hit_point = src;
			return true;
		}

		// Calculate T distances to candidate planes
		Vector maxT;
		for ( i = 0; i < NUMDIM; i++ ) {
			if ( quadrant[ i ] != MIDDLE && dir[ i ] != 0.f )
				maxT[ i ] = ( candidatePlane[ i ] - src[ i ] ) / dir[ i ];
			else
				maxT[ i ] = -1.f;
		}

		// Get largest of the maxT's for final choice of intersection
		int whichPlane = 0;
		for ( i = 1; i < NUMDIM; i++ ) {
			if ( maxT[ whichPlane ] < maxT[ i ] )
				whichPlane = i;
		}

		// Check final candidate actually inside box
		if ( maxT[ whichPlane ] < 0.f )
			return false;

		for ( i = 0; i < NUMDIM; i++ ) {
			if ( whichPlane != i ) {
				float temp = src[ i ] + maxT[ whichPlane ] * dir[ i ];
				if ( temp < min[ i ] || temp > max[ i ] ) {
					return false;
				}
				else if ( hit_point ) {
					( *hit_point )[ i ] = temp;
				}
			}
			else if ( hit_point ) {
				( *hit_point )[ i ] = candidatePlane[ i ];
			}
		}

		// ray hits box
		return true;
	}


	bool CanHitPlayer( LagController::C_LagRecord* pRecord, int iSide, const Vector& vecEyePos, const Vector& vecEnd, int iHitboxIndex ) {
		auto hdr = *( studiohdr_t** )pRecord->player->m_studioHdr( );
		if ( !hdr )
			return false;

		auto pHitboxSet = hdr->GetHitboxSet( pRecord->player->m_nHitboxSet( ) );

		if ( !pHitboxSet )
			return false;

		auto pHitbox = pHitboxSet->GetHitbox( iHitboxIndex );

		if ( !pHitbox )
			return false;

		bool bIsCapsule = pHitbox->m_radius != -1.0f;
		bool bHitIntersection = false;

		CGameTrace tr;

		//Interfac::m_pDebugOverlay->AddLineOverlay( eyePos, end, 255, 0, 0, false, 5.f );

		matrix3x4_t* pBone = pRecord->GetBoneMatrix( );

		vec3_t min, max;

		bHitIntersection = bIsCapsule ?
			IntersectSegmentToSegment( vecEyePos, vecEnd, pHitbox->m_mins, pHitbox->m_maxs, pHitbox->m_radius ) : IntersectionBoundingBox( vecEyePos, vecEnd, pHitbox->m_mins, pHitbox->m_maxs, nullptr );//( tr.hit_entity == pRecord->player && ( tr.hitgroup >= Hitgroup_Head && tr.hitgroup <= Hitgroup_RightLeg ) || tr.hitgroup == Hitgroup_Gear );

		return bHitIntersection;
	};

	void TraceMatrix( const Vector& vecStart, const Vector& vecEnd, LagController::C_LagRecord* pRecord, C_CSPlayer* Player,
		std::vector<C_TraceData>& TracesData, int iSide, bool bDidHit, int iHitboxIndex ) {
		auto& TraceData = TracesData.emplace_back( );

		pRecord->Apply( Player );

		TraceData.is_resolver_issue = CanHitPlayer( pRecord, iSide, vecStart, vecEnd, iHitboxIndex );
		TraceData.is_correct = TraceData.is_resolver_issue == bDidHit;
	}

	Encrypted_t<C_ShotInformation> C_ShotInformation::Get( ) {
		static C_ShotInformation instance;
		return &instance;
	}

	void C_ShotInformation::Start( ) {
		auto netchannel = Encrypted_t<INetChannel>( Interfac::m_pEngine->GetNetChannelInfo( ) );
		if ( !netchannel.IsValid( ) ) {
			return;
		}

		ProcessEvents( );

		auto latency = netchannel->GetAvgLatency( 0 ) * 1000.f;

		const auto pLocal = g_cl.m_local;
		auto it = this->m_Shapshots.begin( );
		while ( it != this->m_Shapshots.end( ) ) {
			if ( it->correctSequence && Interfac::m_pClientState->m_last_command_ack >= it->outSequence + latency ) {
#if defined(DEBUG_MODE) || defined(DEV)
				ILoggerEvent::Get( )->PushEvent( XorStr( "Missed shot due to prediction error" ), FloatColor( 0.5f, 0.5f, 0.5f ), false );
#endif

				it = this->m_Shapshots.erase( it );
			}
			else {
				it++;
			}
		}
	}

	void C_ShotInformation::ProcessEvents( ) {
		auto TranslateHitbox = [ ]( int hitbox ) -> std::string {
			std::string result = { };
			switch ( hitbox ) {
			case HITBOX_HEAD:
				result = XorStr( "head" ); break;
			case HITBOX_NECK:
			case HITBOX_LOWER_NECK:
				result = XorStr( "neck" ); break;
			case HITBOX_CHEST:
			case HITBOX_UPPER_CHEST:
				result = XorStr( "chest" ); break;
			case HITBOX_R_FOOT:
			case HITBOX_R_CALF:
			case HITBOX_R_THIGH:
			case HITBOX_L_FOOT:
			case HITBOX_L_CALF:
			case HITBOX_L_THIGH:
				result = XorStr( "leg" ); break;
			case HITBOX_L_FOREARM:
			case HITBOX_L_HAND:
			case HITBOX_L_UPPER_ARM:
			case HITBOX_R_FOREARM:
			case HITBOX_R_HAND:
			case HITBOX_R_UPPER_ARM:
				result = XorStr( "arm" ); break;
			case HITBOX_PELVIS:
				result = XorStr( "stomach" ); break;
			default:
				result = XorStr( "-" );
			}

			return result;
		};

		auto FixedStrLength = [ ]( std::string str ) -> std::string {
			if ( ( int )str[ 0 ] > 255 )
				return XorStr( "" );

			if ( str.size( ) < 15 )
				return str;

			std::string result;
			for ( size_t i = 0; i < 15u; i++ )
				result.push_back( str.at( i ) );
			return result;
		};

		if ( !this->m_GetEvents ) {
			return;
		}

		this->m_GetEvents = false;

		if ( this->m_Shapshots.empty( ) ) {
			this->m_Weaponfire.clear( );
			return;
		}

		if ( this->m_Weaponfire.empty( ) ) {
			return;
		}

		try {
			auto it = this->m_Weaponfire.begin( );
			while ( it != this->m_Weaponfire.end( ) ) {
				if ( this->m_Shapshots.empty( ) || this->m_Weaponfire.empty( ) ) {
					this->m_Weaponfire.clear( );
					break;
				}

				auto snapshot = it->snapshot;

				if ( !( &it->snapshot ) || !&( *it->snapshot ) ) {
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				if ( snapshot == this->m_Shapshots.end( ) ) {
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				auto player = snapshot->player;
				if ( !player ) {
					this->m_Shapshots.erase( it->snapshot );
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				if ( player != g_csgo.m_entlist->GetClientEntity< Player* >( player->index()  ) ) {
					this->m_Shapshots.erase( it->snapshot );
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				auto anim_data = AnimationFixx::AnimationSystem::Get( )->GetAnimationData( player->index() );
				if ( !anim_data ) {
					this->m_Shapshots.erase( it->snapshot );
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				if ( it->impacts.empty( ) ) {
					this->m_Shapshots.erase( it->snapshot );
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				auto& lag_data = LagController::LagCompensation::Get( )->GetLagData( player->index( ) );
				if ( !player->alive( ) ) {
					this->m_Shapshots.erase( it->snapshot );
					it = this->m_Weaponfire.erase( it );
					continue;
				}

				auto did_hit = it->damage.size( ) > 0;

				// last reseived impact
				auto last_impact = it->impacts.back( );

				C_BaseLagRecord backup;
				backup.Setup( player );

				std::vector<C_TraceData> trace_data;
				TraceMatrix( it->snapshot->eye_pos, last_impact, &it->snapshot->resolve_record, player, trace_data, 0, did_hit, it->snapshot->Hitbox );

				backup.Apply( player );

				// in_attack
				//g_Vars.globals.m_iFiredShots++;

				if ( !did_hit ) {
					auto aimpoint_distance = it->snapshot->eye_pos.dist_to( it->snapshot->AimPoint ) - 32.f;
					auto impact_distance = it->snapshot->eye_pos.dist_to( last_impact );
					float aimpoint_lenght = it->snapshot->AimPoint.length( );
					float impact_lenght = last_impact.length( );

					auto td = &trace_data[ 0 ];

					auto AddMissLog = [ & ]( std::string reason ) -> void {
						std::stringstream msg;

						player_info_t info;
						if ( Interfac::m_pEngine->GetPlayerInfo( it->snapshot->playerIdx, &info ) ) {
#if defined (BETA_MODE) || defined(DEV)
							msg << XorStr( "reason: " ) << reason.data( ) << XorStr( " | " );
							msg << XorStr( "flick: " ) << int( it->snapshot->resolve_record.m_iResolverMode == EResolverModes::RESOLVE_PRED ) << XorStr( " | " );
							msg << XorStr( "dmg: " ) << it->snapshot->m_nSelectedDamage << XorStr( " | " );
							msg << XorStr( "hitgroup: " ) << TranslateHitbox( it->snapshot->Hitbox ).data( ) << XorStr( " | " );
							msg << XorStr( "player: " ) << FixedStrLength( info.szName ).data( );

							ILoggerEvent::Get( )->PushEvent( msg.str( ), FloatColor( 255, 128, 128 ), !g_Vars.misc.undercover_log, XorStr( "missed shot " ) );
#else
							msg << XorStr( "reason: " ) << reason.data( ) << XorStr( " | " );
							msg << XorStr( "hitgroup: " ) << TranslateHitbox( it->snapshot->Hitbox ).data( ) << XorStr( " | " );
							msg << XorStr( "ent: " ) << FixedStrLength( info.m_name ).data( );

							notify_controller::g_notify.push_event( msg.str( ) );
						//	ILoggerEvent::Get( )->PushEvent( msg.str( ), FloatColor( 255, 128, 128 ), false, XorStr( "missed shot " ) );
#endif
						}
					};

					if ( td->is_resolver_issue ) {
						if ( it->snapshot->ResolverType == resolve_record::resolve_data::resolver_modes::resolve_body )
							lag_data->m_iMissedShotsLBY++;
						else
							lag_data->m_iMissedShots++;

						//if ( g_Vars.esp.event_resolver ) {
							AddMissLog( XorStr( "resolver" ) );
						//}
					}
					else if ( aimpoint_distance > impact_distance ) { // occulusion issue
						//if ( g_Vars.esp.event_resolver ) {
							AddMissLog( XorStr( "occlusion" ) );
						//}
					}
					else { // spread issue
						//if ( g_Vars.esp.event_resolver ) {
							AddMissLog( XorStr( "spread" ) );
						//}
					}
				}
				else {
					bool shoud_break = false;
					auto best_damage = it->damage.end( );
					auto dmg = it->damage.begin( );
					while ( dmg != it->damage.end( ) ) {
						shoud_break = true;
						if ( best_damage == it->damage.end( )
							|| dmg->damage > best_damage->damage ) {
							best_damage = dmg;
						}

						dmg++;
					}


					if ( it->snapshot->Hitgroup != best_damage->hitgroup ) {

					}
					else {

					}
				}

				this->m_Shapshots.erase( it->snapshot );
				it = this->m_Weaponfire.erase( it );
			}
		}
		catch ( const std::exception& ) {
			return;
		}
	}

	void C_ShotInformation::EventCallback( IGameEvent* gameEvent, uint32_t hash ) {
		if ( this->m_Shapshots.empty( ) ) {
			return;
		}
		auto net_channel = Encrypted_t<INetChannel>( Interfac::m_pEngine->GetNetChannelInfo( ) );
		if ( !net_channel.IsValid( ) ) {
			this->m_Shapshots.clear( );
			return;
		}

		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) ) {
			this->m_Shapshots.clear( );
			return;
		}

		auto weapon = g_cl.m_weapon;// C_WeaponCSBaseGun* weapon = ( C_WeaponCSBaseGun* )( local->m_hActiveWeapon( ).Get( ) );
		if ( !weapon ) {
			this->m_Shapshots.clear( );
			return;
		}

		auto weapon_data = g_cl.m_weapon_info;// weapon->GetCSWeaponData( );
		if ( !weapon_data ) {
			this->m_Shapshots.clear( );
			return;
		}

		auto it = this->m_Shapshots.begin( );
		while ( it != this->m_Shapshots.end( ) ) {
			// unhandled snapshots
			if ( std::fabsf( it->time - Interfac::m_pGlobalVars->m_realtime ) >= 2.5f ) {
				it = this->m_Shapshots.erase( it );
			}
			else {
				it++;
			}
		}

		auto snapshot = this->m_Shapshots.end( );

		switch ( hash ) {
		case hash_32_fnv1a_const( "player_hurt" ):
		{
			if ( this->m_Weaponfire.empty( ) || Interfac::m_pEngine->GetPlayerForUserID( gameEvent->GetInt( XorStr( "attacker" ) ) ) != local->index( ) )
				return;

			// TODO: check if need backtrack
			Player* target = (Player*)g_csgo.m_engine->GetPlayerForUserID( gameEvent->m_keys->FindKey( HASH( "userid" ) )->GetInt( ) );
			if ( !target || target == local || local->m_iTeamNum( ) || target->dormant( ) )
				return;

			auto& player_damage = this->m_Weaponfire.back( ).damage.emplace_back( );
			player_damage.playerIdx = target->index();
			player_damage.player = target;
			player_damage.damage = gameEvent->GetInt( XorStr( "dmg_health" ) );
			player_damage.hitgroup = gameEvent->GetInt( XorStr( "hitgroup" ) );
			break;
		}
		case hash_32_fnv1a_const( "bullet_impact" ):
		{
			if ( this->m_Weaponfire.empty( ) || Interfac::m_pEngine->GetPlayerForUserID( gameEvent->GetInt( XorStr( "userid" ) ) ) != local->index( ) )
				return;

			this->m_Weaponfire.back( ).impacts.emplace_back( gameEvent->GetFloat( XorStr( "x" ) ), gameEvent->GetFloat( XorStr( "y" ) ), gameEvent->GetFloat( XorStr( "z" ) ) );
			break;
		}
		case hash_32_fnv1a_const( "weapon_fire" ):
		{
			if ( Interfac::m_pEngine->GetPlayerForUserID( gameEvent->GetInt( XorStr( "userid" ) ) ) != local->index( ) )
				return;

			int nElement = this->m_Weaponfire.size( ) / weapon_data->m_bullets;

			// will get iBullets weapon_fire events
			if ( nElement != this->m_Shapshots.size( ) ) {
				snapshot = this->m_Shapshots.begin( ) + nElement;
				auto& fire = this->m_Weaponfire.emplace_back( );
				fire.snapshot = snapshot;
			}
			break;
		}
		case hash_32_fnv1a_const( "player_death" ):
		{
			int id = Interfac::m_pEngine->GetPlayerForUserID( gameEvent->GetInt( XorStr( "userid" ) ) );
			auto player = g_csgo.m_entlist->GetClientEntity< Player* >( id );

			if ( !player || player == local )
				return;

			auto lagData = LagController::LagCompensation::Get( )->GetLagData( id );
			if ( lagData.IsValid( ) ) {
				lagData->m_iMissedShots = 0;
				lagData->m_iMissedShotsLBY = 0;

			}

			break;
		}
		case hash_32_fnv1a_const( "round_start" ):
		{
			for ( int i = 1; i < Interfac::m_pGlobalVars->m_max_clients; ++i ) {
				auto lagData = LagController::LagCompensation::Get( )->GetLagData( i );
				if ( lagData.IsValid( ) ) {
					lagData->m_iMissedShots = 0;
					lagData->m_iMissedShotsLBY = 0;
				}
			}

			break;
		}
		}

		this->m_GetEvents = true;
	}
	void C_ShotInformation::CreateSnapshot( C_CSPlayer* player, const Vector& shootPosition, const Vector& aimPoint, LagController::C_LagRecord* record, int resolverSide, int hitgroup, int hitbox, int nDamage, bool doubleTap ) {
		auto& snapshot = this->m_Shapshots.emplace_back( );

		snapshot.playerIdx = player->index();
		snapshot.player = player;
		snapshot.resolve_record = *record;
		snapshot.eye_pos = shootPosition;
		snapshot.time = Interfac::m_pGlobalVars->m_realtime;
		snapshot.correctSequence = false;
		snapshot.correctEyePos = false;
		snapshot.Hitbox = hitbox;
		snapshot.doubleTap = doubleTap;
		snapshot.ResolverType = resolverSide;

		snapshot.AimPoint = aimPoint;
		snapshot.Hitgroup = hitgroup;
		snapshot.m_nSelectedDamage = nDamage;

		auto data = AnimationFixx::AnimationSystem::Get( )->GetAnimationData( player->index() );
		if ( data ) {
			data->m_flLastScannedYaw = record->m_flEyeYaw;
		}

	}

	void C_ShotInformation::CorrectSnapshots( bool is_sending_packet ) {
		auto local = g_cl.m_local;
		if ( !local )
			return;

		auto netchannel = Encrypted_t<INetChannel>( Interfac::m_pEngine->GetNetChannelInfo( ) );
		if ( !netchannel.IsValid( ) )
			return;

		for ( auto& snapshot : this->m_Shapshots ) {
			if ( is_sending_packet && !snapshot.correctSequence ) {
				snapshot.outSequence = Interfac::m_pClientState->m_last_outgoing_command + Interfac::m_pClientState->m_choked_commands + 1;
				snapshot.correctSequence = true;
			}
		}
	}
}