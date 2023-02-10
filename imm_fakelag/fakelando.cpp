#include "fakelando.hpp"
#include "../backend/config/config_new.h"
#include "../imm_animfix/imm_animfix.hpp"
namespace FakelagSys {
	inline int OutgoingTickcount{};

	struct FakelagData {
		int m_iChoke, m_iMaxChoke;
		int m_iWillChoke = 0;
		int m_iLatency = 0;

		bool m_bAlternative = false;
		bool m_bLanding = false;
	};

	static FakelagData _fakelag_data;

	class C_FakeLag : public FakeLag {
	public:
		C_FakeLag( ) : fakelagData( &_fakelag_data ) {

		}

		virtual void Main( bool* bSendPacket, Encrypted_t<CUserCmd> cmd );
		virtual int GetFakelagChoke( ) const {
			return fakelagData.Xor( )->m_iWillChoke;
		}

		virtual int GetMaxFakelagChoke( ) const {
			return fakelagData.Xor( )->m_iMaxChoke;
		}

		virtual bool IsPeeking( Encrypted_t<CUserCmd> cmd );

	private:
		__forceinline bool AlternativeChoke( bool* bSendPacket );
		bool VelocityChange( Encrypted_t<CUserCmd> cmd );
		bool BreakLagComp( int trigger_limit );
		bool AlternativeCondition( Encrypted_t<CUserCmd> cmd, bool* bSendPacket );
		bool MainCondition( Encrypted_t<CUserCmd> cmd, bool* bSendPacket );

		Encrypted_t<FakelagData> fakelagData;
	};

	Encrypted_t<FakeLag> FakeLag::Get( ) {
		static C_FakeLag instance;
		return &instance;
	}

	void C_FakeLag::Main( bool* bSendPacket, Encrypted_t<CUserCmd> cmd ) {
		fakelagData->m_iWillChoke = 0;
		fakelagData->m_iMaxChoke = 0;

		auto LocalPlayer = g_cl.m_local;// C_CSPlayer* LocalPlayer = g_cl.m_local;
		if ( !LocalPlayer || !LocalPlayer->alive( ) ) {
			return;
		}

		if ( !( *bSendPacket ) ) {
			return;
		}

		auto a = settings;

		if ( !a.immfal )
			return;

		auto g_GameRules = *( uintptr_t** )( g_csgo.m_gamerules );
		if ( *( bool* )( *( uintptr_t* )g_GameRules + 0x20 ) )
			return;

		// fakewalk check

		if ( LocalPlayer->m_fFlags( ) & 0x40 )
			return;

		auto weapon = g_cl.m_weapon;// reinterpret_cast< C_WeaponCSBaseGun* >( LocalPlayer->m_hActiveWeapon( ).Get( ) );
		if ( !weapon )
			return;

		auto pWeaponData = g_cl.m_weapon_info;// weapon->GetCSWeaponData( );
		if ( !pWeaponData || !pWeaponData )
			return;

		if ( pWeaponData->m_weapon_type == WEAPONTYPE_GRENADE ) {
			if ( !weapon->m_bPinPulled( ) || ( cmd->m_buttons & ( IN_ATTACK | IN_ATTACK2 ) ) ) {
				float throwTime = weapon->m_fThrowTime( );
				if ( throwTime > 0.f ) {
					*bSendPacket = true;
					return;
				}
			}
		}
		else {
			if ( cmd->m_buttons & IN_ATTACK ) {
				//if( !g_TickbaseController.bInRapidFire ) {
				if ( g_cl.CanFireWeapon() ) {
					if ( !a.fakewalkke ) {
						*bSendPacket = false;
						return;
					}
				}
			}
		}

		// lag  limit
		///ezs
		fakelagData->m_bLanding = true;

		if ( fakelagData->m_bLanding ) {
			if ( AlternativeChoke( bSendPacket ) )
				fakelagData->m_bLanding = false;
			return;
		}

		auto net_channel = Encrypted_t<INetChannel>( Interfac::m_pEngine->GetNetChannelInfo( ) );
		if ( !net_channel.IsValid( ) )
			return;

		fakelagData->m_iMaxChoke = 0;
		if ( !AlternativeCondition( cmd, bSendPacket ) && !MainCondition( cmd, bSendPacket ) ) {
			return;
		}

		if ( fakelagData->m_iMaxChoke < 1 )
			return;

		g_csgo.RandomSeed( cmd->m_command_number );

		// skeet.cc
		int variance = 0;
		if ( a.immlimit > 0.f && !a.fakewalkke ) {
			variance = int( float( a.varianceimm * 0.01f ) * float( fakelagData->m_iMaxChoke ) );
		}

		auto apply_variance = [ this ]( int variance, int fakelag_amount ) {
			if ( variance > 0 && fakelag_amount > 0 ) {
				auto max = math::Clamp( variance, 0, ( fakelagData->m_iMaxChoke ) - fakelag_amount );
				auto min = math::Clamp( -variance, -fakelag_amount, 0 );
				fakelag_amount += g_csgo.RandomInt( min, max );
				if ( fakelag_amount == g_cl.m_old_packet ) {
					if ( fakelag_amount >= ( fakelagData->m_iMaxChoke ) || fakelag_amount > 2 && !g_csgo.RandomInt( 0, 1 ) ) {
						--fakelag_amount;
					}
					else {
						++fakelag_amount;
					}
				}
			}

			return fakelag_amount;
		};

		auto apply_choketype = [ & ]( int fakelag_amount ) {

			float extrapolated_speed = LocalPlayer->m_vecVelocity( ).length( ) * Interfac::m_pGlobalVars->m_interval;
			switch ( a.immmodefl ) {
			case 0: // max
				break;
			case 1: // dyn
				fakelag_amount = std::min< int >( static_cast< int >( std::ceilf( 64 / extrapolated_speed ) ), static_cast< int >( fakelag_amount ) );
				fakelag_amount = std::clamp( fakelag_amount, 2, fakelagData->m_iMaxChoke );
				break;
			case 2: // fluc
				if ( cmd->m_tick % 40 < 20 ) {
					fakelag_amount = fakelag_amount;
				}
				else {
					fakelag_amount = 2;
				}
				break;
			}

			return fakelag_amount;
		};

		//if ( **( int** )Engine::Displacement.Data.m_uHostFrameTicks > 1 )
		//	fakelagData->m_iMaxChoke += **( int** )Engine::Displacement.Data.m_uHostFrameTicks - 1;

		fakelagData->m_iMaxChoke = math::Clamp( fakelagData->m_iMaxChoke, 0, a.immlimit );

		auto fakelag_amount = fakelagData->m_iMaxChoke;
		fakelag_amount = apply_variance( variance, fakelag_amount );
		fakelag_amount = apply_choketype( fakelag_amount );
		*bSendPacket = Interfac::m_pClientState->m_choked_commands > fakelag_amount;

		if ( Interfac::m_pClientState->m_choked_commands > 16 )
			*bSendPacket = true;

		fakelagData->m_iWillChoke = fakelag_amount - Interfac::m_pClientState->m_choked_commands;
		auto diff_too_large = abs( Interfac::m_pGlobalVars->m_tick_count - OutgoingTickcount ) > fakelagData->m_iMaxChoke;
		if ( Interfac::m_pClientState->m_choked_commands > 0 && diff_too_large ) {
			*bSendPacket = true;
			fakelagData->m_bAlternative = false;
			fakelagData->m_iWillChoke = 0;
			return;
		}
	}

	int FindPeekTarget( bool dormant_peek ) {
		static auto LastPeekTime = 0;
		static auto LastPeekTarget = 0;
		static auto InPeek = false;

		if ( LastPeekTime == Interfac::m_pGlobalVars->m_tick_count ) {
			if ( dormant_peek || InPeek )
				return LastPeekTarget;
			return 0;
		}

		InPeek = false;

		auto local = g_cl.m_local;

		if ( !local || !local->alive( ) )
			return 0;

		bool any_alive_players = false;
		AnimationFixx::C_AnimationData* players[ 64 ];
		int player_count = 0;
		for ( int i = 1; i <= Interfac::m_pGlobalVars->m_max_clients; ++i ) {
			if ( i > 63 )
				break;

			auto target = AnimationFixx::AnimationSystem::Get( )->GetAnimationData( i );
			if ( !target )
				continue;

			// todo: check for round count
			auto player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
			if ( !player || player->m_iTeamNum(  ) )
				continue;

			if ( target->m_bIsAlive )
				any_alive_players = true;

			if ( !player->dormant( ) )
				InPeek = true;

			players[ player_count ] = target;
			player_count++;
		}

		if ( !player_count ) {
			LastPeekTime = Interfac::m_pGlobalVars->m_tick_count;
			LastPeekTarget = 0;
			return 0;
		}

		auto eye_pos = local->m_vecOrigin( ) + vec3_t( 0.0f, 0.0f, 64.0f );

		auto best_target = 0;
		auto best_fov = 9999.0f;
		for ( int i = 0; i < player_count; ++i ) {
			auto animation = players[ i ];
			auto player = g_csgo.m_entlist->GetClientEntity< Player* >( animation->ent_index );
			if ( !player )
				continue;

			if ( InPeek && player->dormant( ) || any_alive_players && !animation->m_bIsAlive )
				continue;

			auto origin = player->m_vecOrigin( );
			if ( player->dormant( ) ) {
				origin = animation->m_vecOrigin;
			}

			origin.z += 64.0f;

			Ray_t ray;
			ray.Init( eye_pos, origin );

			CTraceFilterSimple filter;
			filter.pSkip = local;

			CGameTrace tr;
			g_csgo.m_engine_trace->TraceRay( ray, 0x4600400B, &filter, &tr );
			if ( tr.m_fraction >= 0.99f || tr.m_entity == player ) {
				LastPeekTime = Interfac::m_pGlobalVars->m_tick_count;
				LastPeekTarget = player->index();
				return player->index( );
			}

			ang_t viewangles;
			Interfac::m_pEngine->GetViewAngles( viewangles );

			auto delta = origin - eye_pos;
			auto angles = delta.ToEulerAngles( );
			auto view_delta = angles - viewangles;
			view_delta.normalize( );

			auto fov = std::sqrtf( view_delta.x * view_delta.x + view_delta.y * view_delta.y );

			if ( fov >= best_fov )
				continue;

			best_fov = fov;
			best_target = player->index( );
		}

		LastPeekTime = Interfac::m_pGlobalVars->m_tick_count;
		LastPeekTarget = best_target;
		if ( dormant_peek || InPeek )
			return best_target;

		return 0;
	}

	bool C_FakeLag::IsPeeking( Encrypted_t<CUserCmd> cmd ) {
		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) )
			return false;

		auto pWeapon = g_cl.m_weapon;// reinterpret_cast< C_WeaponCSBaseGun* >( local->m_hActiveWeapon( ).Get( ) );
		if ( !pWeapon )
			return false;

		auto target = FindPeekTarget( true );
		if ( !target )
			return false;

		auto player = g_csgo.m_entlist->GetClientEntity< Player* >( target );
		if ( !player )
			return false;

		auto enemy_weapon = g_cl.m_weapon;// reinterpret_cast< C_WeaponCSBaseGun* >( player->m_hActiveWeapon( ).Get( ) );
		if ( !enemy_weapon )
			return false;

		auto weapon_data = enemy_weapon->GetWpnData( );
		auto enemy_eye_pos = player->get_eye_pos( );

		auto eye_pos = local->get_eye_pos( );
		auto peek_add = local->m_vecVelocity( ) * game::TICKS_TO_TIME( 14 );
		auto second_scan = eye_pos;
		second_scan += peek_add;

		imm_autowall::C_FireBulletData data;

		data.m_bPenetration = true;
		data.m_vecStart = eye_pos;

		data.m_Player = local;
		data.m_TargetPlayer = player;
		data.m_WeaponData = pWeapon->GetWpnData( );
		data.m_Weapon = pWeapon;

		//data.m_vecDirection = ( enemy_eye_pos - eye_pos ).normalized( );
		data.m_flPenetrationDistance = data.m_vecDirection.normalize( );
		float damage = imm_autowall::FireBullets( &data );

		data = imm_autowall::C_FireBulletData{ };
		data.m_bPenetration = true;
		data.m_vecStart = second_scan;

		data.m_Player = local;
		data.m_TargetPlayer = player;
		data.m_WeaponData = pWeapon->GetWpnData( );
		data.m_Weapon = pWeapon;

		//data.m_vecDirection = ( enemy_eye_pos - second_scan ).Normalized( );
		data.m_flPenetrationDistance = data.m_vecDirection.normalize( );

		float damage2 = imm_autowall::FireBullets( &data );

		if ( damage >= 1.0f )
			return false;

		if ( damage2 >= 1.0f )
			return true;

		return false;
	}

	bool C_FakeLag::AlternativeChoke( bool* bSendPacket ) {
		fakelagData->m_iMaxChoke = ( int )settings.alternativechok;

		//if ( **( int** )Engine::Displacement.Data.m_uHostFrameTicks > 1 )
		//	fakelagData->m_iMaxChoke += **( int** )Engine::Displacement.Data.m_uHostFrameTicks - 1;

		fakelagData->m_iMaxChoke = math::Clamp( fakelagData->m_iMaxChoke, 0, settings.immlimit );

		*bSendPacket = Interfac::m_pClientState->m_choked_commands >= fakelagData->m_iMaxChoke;
		fakelagData->m_iWillChoke = fakelagData->m_iMaxChoke - Interfac::m_pClientState->m_choked_commands;

		if ( *bSendPacket ) {
			fakelagData->m_bAlternative = false;
			return true;
		}

		return false;
	}

	#define RAD2DEG math::rad_to_deg

	bool C_FakeLag::VelocityChange( Encrypted_t<CUserCmd> cmd ) {
		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) )
			return false;

		static auto init_velocity = false;
		static auto previous_velocity = Vector{ };
		if ( local->m_vecVelocity( ).length( ) < 5.0f ) {
			init_velocity = false;
			return false;
		}

		if ( !init_velocity ) {
			init_velocity = true;
		}

		auto move_dir = RAD2DEG( std::atan2f( previous_velocity.y, previous_velocity.x ) );
		auto current_move_dir = RAD2DEG( std::atan2f( local->m_vecVelocity( ).y, local->m_vecVelocity( ).x ) );
		auto delta = std::remainderf( current_move_dir - move_dir, 360.0f );
		if ( std::fabsf( delta ) >= 30.0f ) {
			previous_velocity = local->m_vecVelocity( );
			return true;
		}

		return false;
	}

	bool C_FakeLag::BreakLagComp( int trigger_limit ) {
		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) )
			return false;

		//if( !g_Vars.fakelag.break_lag_compensation )
		//	return false;

		auto speed = local->m_vecVelocity( ).length_sqr( );
		if ( speed < 5.0f )
			return false;

		auto choke = trigger_limit - Interfac::m_pClientState->m_choked_commands;
		if ( choke < 1 )
			return false;

		auto simulated_origin = local->m_vecOrigin( );
		auto move_per_tick = local->m_vecVelocity( ) * Interfac::m_pGlobalVars->m_interval;
		for ( int i = 0; i < choke; ++i ) {
			simulated_origin += move_per_tick;

			// fix htis -> im too lazy
			//auto distance = g_Vars.globals.m_vecNetworkedOrigin.DistanceSquared( simulated_origin );
			//if ( distance > 4096.0f )
			//	return true;
		}

		return false;
	}

	bool C_FakeLag::AlternativeCondition( Encrypted_t<CUserCmd> cmd, bool* bSendPacket ) {
		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) || ( int )settings.alternativechok <= 0 ) {
			fakelagData->m_bAlternative = false;
			return false;
		}

		fakelagData->m_iMaxChoke = 0;

		//if( g_Vars.fakelag.break_lag_compensation ) {
		//	auto distance = local->m_vecOrigin( ).DistanceSquared( g_Vars.globals.m_vecNetworkedOrigin );
		//	if( distance > 4096.0f )
		//		return true;
		//}

		if ( !fakelagData->m_bAlternative ) {
			bool landing = false;
			fakelagData->m_bAlternative = true;

			if ( fakelagData->m_bAlternative && Interfac::m_pClientState->m_choked_commands > 0 ) {
				*bSendPacket = true;
				return true;
			}
		}

		if ( fakelagData->m_bAlternative ) {
			fakelagData->m_iMaxChoke = ( int )settings.alternativechok;
		}

		return fakelagData->m_bAlternative;
	}

	Vector GetSmoothedVelocity( float min_delta, Vector a, Vector b ) {
		Vector delta = a - b;
		float delta_length = delta.length( );

		if ( delta_length <= min_delta ) {
			Vector result;
			if ( -min_delta <= delta_length ) {
				return a;
			}
			else {
				float iradius = 1.0f / ( delta_length + FLT_EPSILON );
				return b - ( ( delta * iradius ) * min_delta );
			}
		}
		else {
			float iradius = 1.0f / ( delta_length + FLT_EPSILON );
			return b + ( ( delta * iradius ) * min_delta );
		}
	}

	bool C_FakeLag::MainCondition( Encrypted_t<CUserCmd> cmd, bool* bSendPacket ) {
		C_CSPlayer* local = g_cl.m_local;
		if ( !local || !local->alive( ) || ( int )settings.immlimit < 1 )
			return false;

		auto animState = local->m_PlayerAnimState2( );
		auto velocity = GetSmoothedVelocity( Interfac::m_pGlobalVars->m_interval * 2000.0f, local->m_vecVelocity( ), animState->m_vecVelocity );

		bool moving = velocity.length( ) >= 1.2f;
		bool air = !( local->m_fFlags( ) & FL_ONGROUND );

		fakelagData->m_iMaxChoke = !moving ? 3 : ( int )settings.immlimit;
		return true;
	}
}