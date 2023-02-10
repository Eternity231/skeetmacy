#include "StackerLag.hpp"

#define FLOW_OUTGOING	0
#define FLOW_INCOMING	1
#define MAX_FLOWS		2		// in & out

LagComp* g_LagComp = new LagComp( );

float LagComp::LerpTime( ) // GLAD https://github.com/sstokic-tgm/Gladiatorcheatz-v2.1/blob/eaa88bbb4eca71f8aebfed32a5b86300df8ce6a3/features/LagCompensation.cpp
{
	int updaterate = g_csgo.m_cvar->FindVar( HASH( "cl_updaterate" ) )->GetInt( );
	ConVar* minupdate = g_csgo.m_cvar->FindVar( HASH( "sv_minupdaterate" ) );
	ConVar* maxupdate = g_csgo.m_cvar->FindVar( HASH( "sv_maxupdaterate" ) );

	if ( minupdate && maxupdate )
		updaterate = maxupdate->GetInt( );

	float ratio = g_csgo.m_cvar->FindVar( HASH( "cl_interp_ratio" ) )->GetFloat( );

	if ( ratio == 0 )
		ratio = 1.0f;

	float lerp = g_csgo.m_cvar->FindVar( HASH( "cl_interp" ) )->GetFloat( );
	ConVar* cmin = g_csgo.m_cvar->FindVar( HASH( "sv_client_min_interp_ratio" ) );
	ConVar* cmax = g_csgo.m_cvar->FindVar( HASH( "sv_client_max_interp_ratio" ) );

	if ( cmin && cmax && cmin->GetFloat( ) != 1 )
		ratio = clamp( ratio, cmin->GetFloat( ), cmax->GetFloat( ) );

	return std::max( lerp, ( ratio / updaterate ) );
}

bool LagComp::ValidTick( int tick, CUserCmd* cmd ) // gucci i think cant remember
{
	auto nci = g_csgo.m_engine->GetNetChannelInfo( );

	if ( !nci )
		return false;

	auto PredictedCmdArrivalTick = cmd->m_tick + 1 + game::TIME_TO_TICKS( nci->GetAvgLatency( FLOW_INCOMING ) + nci->GetAvgLatency( FLOW_OUTGOING ) );
	auto Correct = clamp( LerpTime( ) + nci->GetLatency( FLOW_OUTGOING ), 0.f, 1.f ) - game::TICKS_TO_TIME( PredictedCmdArrivalTick + game::TIME_TO_TICKS( LerpTime( ) ) - ( tick + game::TIME_TO_TICKS( LerpTime( ) ) ) );

	return ( abs( Correct ) <= 0.2f );
}

#include "../stackhack_v3_aimbot/StackhackController.hpp"


void LagComp::StoreRecord( Player* pEnt, CUserCmd* cmd ) // best lag comp in the world
{
	PlayerRecords Setup;

	static float ShotTime[ 65 ];
	static float OldSimtime[ 65 ];

	if ( pEnt != g_cl.m_local )
		pEnt->FixSetupBones( StackHack::AimbotControll::matrix[ pEnt->index( ) ] );

	if ( PlayerRecord[ pEnt->index( ) ].size( ) == 0 )
	{
		Setup.Velocity = abs( pEnt->m_vecVelocity( ).length_2d( ) );

		Setup.SimTime = pEnt->m_flSimulationTime( );

		memcpy( Setup.Matrix, StackHack::AimbotControll::matrix[ pEnt->index( ) ], ( sizeof( matrix3x4_t ) * 128 ) );

		Setup.Shot = false;

		PlayerRecord[ pEnt->index( ) ].push_back( Setup );
	}

	if ( OldSimtime[ pEnt->index( ) ] != pEnt->m_flSimulationTime( ) )
	{
		Setup.Velocity = abs( pEnt->m_vecVelocity( ).length_2d( ) );

		Setup.SimTime = pEnt->m_flSimulationTime( );

		if ( pEnt == g_cl.m_local )
			pEnt->FixSetupBones( StackHack::AimbotControll::matrix[ pEnt->index( ) ] );

		memcpy( Setup.Matrix, StackHack::AimbotControll::matrix[ pEnt->index( ) ], ( sizeof( matrix3x4_t ) * 128 ) );

		if ( pEnt->GetActiveWeapon( ) && !( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_KNIFE ) )
		{
			if ( ShotTime[ pEnt->index( ) ] != pEnt->GetActiveWeapon( )->m_fLastShotTime( ) )
			{
				Setup.Shot = true;
				ShotTime[ pEnt->index( ) ] = pEnt->GetActiveWeapon( )->m_fLastShotTime( );
			}
			else
				Setup.Shot = false;
		}
		else
		{
			Setup.Shot = false;
			ShotTime[ pEnt->index( ) ] = 0.f;
		}

		PlayerRecord[ pEnt->index( ) ].push_back( Setup );

		OldSimtime[ pEnt->index( ) ] = pEnt->m_flSimulationTime( );
	}

	ShotTick[ pEnt->index( ) ] = -1;

	if ( PlayerRecord[ pEnt->index( ) ].size( ) > 0 )
		for ( int tick = 0; tick < PlayerRecord[ pEnt->index( ) ].size( ); tick++ )
			if ( !ValidTick( game::TIME_TO_TICKS( PlayerRecord[ pEnt->index( ) ].at( tick ).SimTime ), cmd ) )
				PlayerRecord[ pEnt->index( ) ].erase( PlayerRecord[ pEnt->index( ) ].begin( ) + tick );
			else if ( PlayerRecord[ pEnt->index( ) ].at( tick ).Shot )
				ShotTick[ pEnt->index( ) ] = tick; // gets the newest shot tick
}

void LagComp::ClearRecords( int i )
{
	if ( PlayerRecord[ i ].size( ) > 0 )
	{
		for ( int tick = 0; tick < PlayerRecord[ i ].size( ); tick++ )
		{
			PlayerRecord[ i ].erase( PlayerRecord[ i ].begin( ) + tick );
		}
	}
}

template<class T, class U>
T LagComp::clamp( T in, U low, U high )
{
	if ( in <= low )
		return low;

	if ( in >= high )
		return high;

	return in;
}