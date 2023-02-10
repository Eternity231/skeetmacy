#include "GetzeAimbotController.hpp"

#define FLOW_OUTGOING 0		
#define FLOW_INCOMING 1

namespace Getze {
	namespace AimbotController {
		bool TimeDeltaLarger( Player* player ) {
			const auto net = g_csgo.m_engine->GetNetChannelInfo( );
			static auto sv_maxunlag = g_csgo.m_cvar->FindVar( HASH( "sv_maxunlag" ) );
			static auto cl_interp = g_csgo.m_cvar->FindVar( HASH( "cl_interp" ) );
			static auto cl_interp_ratio = g_csgo.m_cvar->FindVar( HASH( "cl_interp_ratio" ) );
			static auto sv_client_min_interp_ratio = g_csgo.m_cvar->FindVar( HASH( "sv_client_min_interp_ratio" ) );
			static auto sv_client_max_interp_ratio = g_csgo.m_cvar->FindVar( HASH( "sv_client_max_interp_ratio" ) );
			static auto cl_updaterate = g_csgo.m_cvar->FindVar( HASH( "cl_updaterate" ) );
			static auto sv_minupdaterate = g_csgo.m_cvar->FindVar( HASH( "sv_minupdaterate" ) );
			static auto sv_maxupdaterate = g_csgo.m_cvar->FindVar( HASH( "sv_maxupdaterate" ) );

			auto updaterate = std::clamp( cl_updaterate->GetFloat( ), sv_minupdaterate->GetFloat( ), sv_maxupdaterate->GetFloat( ) );
			auto lerp_ratio = std::clamp( cl_interp_ratio->GetFloat( ), sv_client_min_interp_ratio->GetFloat( ), sv_client_max_interp_ratio->GetFloat( ) );

			const auto outgoing = net->GetLatency( FLOW_OUTGOING );
			const auto incoming = net->GetLatency( FLOW_INCOMING );

			float correct = 0.f;
			correct += outgoing;
			correct += incoming;
			correct += std::clamp( lerp_ratio / updaterate, cl_interp->GetFloat( ), 1.0f );
			correct = std::clamp( correct, 0.f, sv_maxunlag->GetFloat( ) );

			// get the newest record
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
			LagRecord* current = data->m_records.front( ).get( );

			float deltaTime = std::abs( correct - ( g_csgo.m_globals->m_curtime - current->m_sim_time ) );
			return deltaTime > 0.2f;

		}

		bool delay_shot( Player* player, CUserCmd* cmd, Weapon* local_weapon ) {
			if ( player == nullptr )
				return false;

			if ( TimeDeltaLarger( player ) )
				return true;

			//auto lul = record.m_Tickrecords;
			auto animstate = player->m_PlayerAnimState( );
			auto player_weapon = g_cl.m_weapon;
			auto net = g_csgo.m_engine->GetNetChannelInfo( );

			if (/*lul.size() <= 0 || */net == nullptr || animstate == nullptr || player_weapon == nullptr || local_weapon->m_iItemDefinitionIndex( ) == WEAPONTYPE_TASER )
				return false;

			auto unlag = ( ( ( ( ( cmd->m_tick + 1 ) * g_csgo.m_globals->m_interval ) + net->GetLatency( FLOW_OUTGOING ) ) - net->GetLatency( FLOW_OUTGOING ) ) <= player->m_flSimulationTime( ) );
			auto unlag2 = ( ( ( ( ( cmd->m_tick + 2 ) * g_csgo.m_globals->m_interval ) + net->GetLatency( FLOW_OUTGOING ) ) - net->GetLatency( FLOW_OUTGOING ) ) <= player->m_flSimulationTime( ) );
			return ( unlag2 && unlag );
		}
	}
}
