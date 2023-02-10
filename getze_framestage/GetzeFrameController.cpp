#include "GetzeFrameController.hpp"
#include "../getze_pattern/PatternController.hpp"
#include "../backend/config/config_new.h"

namespace Getze {
	namespace FrameController {
		void UpdateFlashLight( CFlashLightEffect* pFlashLight, const vec3_t& vecPos, const vec3_t& vecForward, const vec3_t& vecRight, const vec3_t& vecUp ) {
			typedef void( __thiscall* UpdateLight_t )( void*, int, const vec3_t&, const vec3_t&, const vec3_t&, const vec3_t&, float, float, float, bool, const char* );

			static UpdateLight_t oUpdateLight = NULL;
			if ( !oUpdateLight ) {
				DWORD callInstruction = pattern::find( g_csgo.m_client_dll, "E8 ? ? ? ? 8B 06 F3 0F 10 46" ); // get the instruction address // 

				DWORD relativeAddress = *( DWORD* )( callInstruction + 1 ); // read the rel32
				DWORD nextInstruction = callInstruction + 5; // get the address of next instruction
				oUpdateLight = ( UpdateLight_t )( nextInstruction + relativeAddress ); // our function address will be nextInstruction + relativeAddress
			}

			oUpdateLight( pFlashLight, pFlashLight->m_nEntIndex, vecPos, vecForward, vecRight, vecUp, pFlashLight->m_flFov, pFlashLight->m_flFarZ, pFlashLight->m_flLinearAtten, pFlashLight->m_bCastsShadows, pFlashLight->m_szTextureName );
		}

		CFlashLightEffect* CreateFlashLight( int nEntIndex, const char* pszTextureName, float flFov, float flFarZ, float flLinearAtten ) {
			static DWORD oConstructor = pattern::find( g_csgo.m_client_dll, "55 8B EC F3 0F 10 45 ? B8" );

			CFlashLightEffect* pFlashLight = ( CFlashLightEffect* )g_csgo.m_mem_alloc->Alloc( sizeof( CFlashLightEffect ) );
			if ( !pFlashLight ) {
				//std::cout << "Mem Alloc for FL failed" << std::endl;
				return NULL;
			}

			__asm
			{
				movss xmm3, flFov
				mov ecx, pFlashLight
				push flLinearAtten
				push flFarZ
				push pszTextureName
				push nEntIndex
				call oConstructor
			}

			return pFlashLight;
		}

		void DestroyFlashLight( CFlashLightEffect* pFlashLight ) {
			static auto oDestroyFlashLight = reinterpret_cast< void( __thiscall* )( void*, void* ) >( Memory::Scan( "client.dll", "56 8B F1 E8 ? ? ? ? 8B 4E 28" ) );
			oDestroyFlashLight( pFlashLight, pFlashLight );
		}

		void RunFrame( ) {
			static CFlashLightEffect* pFlashLight = NULL;
			if ( GetAsyncKeyState( settings.flashkey ) ) {
				if ( !pFlashLight )
					pFlashLight = CreateFlashLight( g_cl.m_local->index( ), "effects/flashlight001", 40, 1000, 2000 );
				else
				{
					DestroyFlashLight( pFlashLight );
					pFlashLight = NULL;
				}

				g_csgo.m_surface->PlaySound( "items\\flashlight1.wav" );
			}

			if ( pFlashLight && g_cl.m_local->alive() && g_csgo.m_engine->IsConnected( ) && settings.lanterna ) {
				ang_t viewangle;
				g_csgo.m_engine->GetViewAngles( viewangle );
				
				vec3_t f, r, u;
				auto viewAngles = viewangle; // this might work
				math::AngleVectors( viewAngles, &f, &r, &u );

				pFlashLight->m_bIsOn = true;
				pFlashLight->m_bCastsShadows = false;
				pFlashLight->m_flFov = 40;/*fabsf(13 + 37 * cosf(x += 0.002f))*/;
				pFlashLight->m_flLinearAtten = 3000;
				UpdateFlashLight( pFlashLight, g_cl.m_local->get_eye_pos(), f, r, u ); // i guess its right
			}
			else if ( pFlashLight && ( !g_csgo.m_engine->IsConnected( ) || !g_cl.m_local->alive( ) || !settings.lanterna ) )
			{
				DestroyFlashLight( pFlashLight );
				pFlashLight = NULL;
			}
		}

		void ExtendChokePackets( ) {
			static bool noob = false;
			if ( noob )
				return;

			if ( !noob ) {
				static DWORD lol = Memory::Scan( "engine.dll", "55 8B EC A1 ? ? ? ? 81 EC ? ? ? ? B9 ? ? ? ? 53 8B 98" ) + 0xBC + 1;
				DWORD old;

				VirtualProtect( ( LPVOID )lol, 1, PAGE_READWRITE, &old );
				*( int* )lol = 62;
				VirtualProtect( ( LPVOID )lol, 1, old, &old );

				noob = true;
			}
		}

		void PredictVelocity( vec3_t* velocity ) {
			static auto sv_friction = g_csgo.m_cvar->FindVar( HASH( "sv_friction" ) );
			static auto sv_stopspeed = g_csgo.m_cvar->FindVar( HASH( "sv_stopspeed" ) );

			float speed = velocity->length( );
			if ( speed >= 0.1f ) {
				float friction = sv_friction->GetFloat( );
				float stop_speed = std::max< float >( speed, sv_stopspeed->GetFloat( ) );
				float time = std::max< float >( g_csgo.m_globals->m_interval, g_csgo.m_globals->m_frametime );
				*velocity *= std::max< float >( 0.f, speed - friction * stop_speed * time / speed );
			}
		}

		void QuickStopL3D( CUserCmd* cmd ) {
			auto local = g_cl.m_local;
			if ( !local )
				return;

			vec3_t hvel = local->m_vecVelocity( );
			hvel.z = 0;
			float speed = hvel.length_2d( );

			if ( speed < 1.f ) // Will be clipped to zero anyways
			{
				cmd->m_forward_move = 0.f;
				cmd->m_side_move = 0.f;
				return;
			}

			static float accel = g_csgo.m_cvar->FindVar( HASH( "sv_accelerate" ) )->GetFloat( );
			static float maxSpeed = g_csgo.m_cvar->FindVar( HASH( "sv_maxspeed" ) )->GetFloat( );
			float playerSurfaceFriction = local->m_surfaceFriction( ); // I'm a slimy boi
			float max_accelspeed = accel * g_csgo.m_globals->m_interval * maxSpeed * playerSurfaceFriction;

			float wishspeed{};

			// Only do custom deceleration if it won't end at zero when applying max_accel
			// Gamemovement truncates speed < 1 to 0
			if ( speed - max_accelspeed <= -1.f )
			{
				// We try to solve for speed being zero after acceleration:
				// speed - accelspeed = 0
				// speed - accel*frametime*wishspeed = 0
				// accel*frametime*wishspeed = speed
				// wishspeed = speed / (accel*frametime)
				// ^ Theoretically, that's the right equation, but it doesn't work as nice as 
				//   doing the reciprocal of that times max_accelspeed, so I'm doing that :shrug:
				wishspeed = max_accelspeed / ( speed / ( accel * g_csgo.m_globals->m_interval ) );
			}
			else // Full deceleration, since it won't overshoot
			{
				// Or use max_accelspeed, doesn't matter
				wishspeed = max_accelspeed;
			}

			// Calculate the negative movement of our velocity, relative to our viewangles
			vec3_t ndir = ( hvel * -1.f ); 
			L3D_Math::VectorAngles( ndir, ndir );
			ndir.y = cmd->m_view_angles.y - ndir.y; // Relative to local view
			L3D_Math::AngleVectors( ndir, &ndir );

			cmd->m_forward_move = ndir.x * wishspeed;
			cmd->m_side_move = ndir.y * wishspeed;
		}
	}
}