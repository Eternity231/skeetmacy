#include "GetzeEngineController.hpp"
#include "../getze_pattern/PatternController.hpp"
#include "../backend/config/config_new.h"

namespace Getze {
	namespace EngineController {
		int sub_3437A2D7( ) {
			static auto v1 = *( int** )( Memory::Scan( "engine.dll", "03 05 ? ? ? ? 83 CF 10" ) + 2 );
			if ( v1 )
				return *v1;
			else
				return 1;
		}
		void sub_343BA99F( ) {
			static auto sv_friction = g_csgo.m_cvar->FindVar( HASH( "sv_friction" ) );
			static auto sv_stopspeed = g_csgo.m_cvar->FindVar( HASH( "sv_stopspeed" ) );
			static auto sv_accelerate = g_csgo.m_cvar->FindVar( HASH( "sv_accelerate" ) );

			auto v1 = g_cl.m_weapon;
			auto v2 = g_cl.m_local;
			auto v3 = v1 ? v1->max_speed( true ) : 250.f; // full ??
			
			static auto predict_velocity = [ & ]( vec3_t* velocity ) {
				float speed = velocity->length( );
				if ( speed >= 0.1f ) {
					float friction = fmaxf( sv_friction->GetFloat( ), v2->m_surfaceFriction( ) );
					float stop_speed = std::max< float >( speed, sv_stopspeed->GetFloat( ) );
					float time = std::max< float >( g_csgo.m_globals->m_interval, g_csgo.m_globals->m_frametime );
					*velocity *= std::max< float >( 0.f, speed - friction * stop_speed * time / speed );

					auto accel = sv_accelerate->GetFloat( ) * v3 * time * v2->m_surfaceFriction( );
					*velocity -= accel;
				}
			};

			auto v4 = v2->m_vecVelocity( );
			for ( ticks_to_stop = 0; ticks_to_stop < 15; ticks_to_stop++ ) {
				if ( v4.length_2d( ) < ( v3 * 0.34f ) )
					break;

				predict_velocity( &v4 );
			}

			ticks_to_stop = ticks_to_stop;
		}

		void DetectRubberband_sub21223( ) {
			auto v1 = g_csgo.m_cl;
			auto v2 = g_csgo.m_prediction;
			auto v3 = g_csgo.m_engine;
			auto local = g_cl.m_local;

			auto v4 = sub_3437A2D7( ) + v1->m_choked_commands;
			auto v5 = ( int )settings.lag_limit + 2;
			auto v7 = current_tickcount;
			
			// c7
			v7 = g_csgo.m_input->m_commands[ g_cl.m_cmd->m_command_number % 150 ].m_tick;		

			if ( v1 != nullptr && v2 != nullptr && v3->IsInGame( ) && local && local->alive( ) ) {
				if ( v4 > v5 ) { // detect rubberband
					auto v6 = v4 - v5;

					for ( int i = 0; i < v6; ++i ) {
						// fix ruberband
						g_csgo.m_prediction->Update( v1->m_delta_tick, true, v1->m_last_command_ack, v1->m_last_outgoing_command + i );
					}
				}
			}

			// this gives me warnings
			if ( v1 != nullptr && v2 != nullptr && g_csgo.m_globals->m_tick_count != current_tickcount && v3->IsInGame( ) && local && local->alive( ) ) {
				auto v8 = sub_3437A2D7( );

				// detect rubberband
				if ( v8 > ( settings.lag_limit + 1 ) ) {
					*( DWORD* )( g_csgo.m_prediction + 0xC ) = -1;
					*( DWORD* )( g_csgo.m_prediction + 0x1C ) = 0;

					*g_cl.m_packet = true;
				}

				static int v9 = -1;
				if ( ( v8 + v1->m_choked_commands ) > ( ( int )settings.lag_limit + 2 - v9 / 2 ) ) {
					auto v10 = game::TIME_TO_TICKS( v3->GetNetChannelInfo( )->GetLatency( 0 ) );
					for ( int i = 0; i < ( v8 + v1->m_choked_commands - ( int )settings.lag_limit + 2 - v9 / 2 + v10 ); ++i ) {
						auto v11 = *( &g_csgo.m_input->m_commands[ g_cl.m_cmd->m_command_number % 150 ] );
						CUserCmd v12;

						v12.m_view_angles = local->m_angEyeAngles( );
						v12.m_tick = -657;
						v12.m_command_number = ( v9 + 1 ) % 150;

						g_csgo.m_input->m_commands[ ( v9 + 1 ) % 150 ] = v12;
					}
					
					// fix stuff
					*g_cl.m_packet = true;
					g_csgo.m_prediction->m_bPreviousAckHadErrors = 1;
					g_csgo.m_prediction->m_commands_predicted = 0;
				} // this might be wrong reversed
			}
		}
	}
}