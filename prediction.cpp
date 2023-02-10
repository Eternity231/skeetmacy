#include "includes.h"
#include "backend/config/config_new.h"
bool Hooks::InPrediction( ) {
	Stack stack;
	ang_t *angles;

	// note - dex; first 2 'test al, al' instructions in C_BasePlayer::CalcPlayerView.
	static Address CalcPlayerView_ret1{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C" ) ) };
	static Address CalcPlayerView_ret2{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 08 57 8B CE E8 ? ? ? ? 8B 06" ) ) };

	if( g_cl.m_local && settings.novisrecoil ) {
		// note - dex; apparently this calls 'view->DriftPitch()'.
		//             i don't know if this function is crucial for normal gameplay, if it causes issues then comment it out.
		if( stack.ReturnAddress( ) == CalcPlayerView_ret1 )
			return true;

		if( stack.ReturnAddress( ) == CalcPlayerView_ret2 ) {
			// at this point, angles are copied into the CalcPlayerView's eyeAngles argument.
			// (ebp) InPrediction -> (ebp) CalcPlayerView + 0xC = eyeAngles.
			angles = stack.next( ).arg( 0xC ).to< ang_t* >( );

			if( angles ) {
				*angles -= g_cl.m_local->m_viewPunchAngle( )
					     + ( g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( ) )
					     * g_csgo.view_recoil_tracking->GetFloat( );
			}

			return true;
		}
	}

	// that was pasted from getze.us
	static auto MaintainSequenceTransitions = ( void* )pattern::find( g_csgo.m_client_dll, "84 C0 74 17 8B 87" ); //C_BaseAnimating::MaintainSequenceTransitions
	if ( settings.mentain_transition ) {
		if ( settings.enable ) {
			if ( MaintainSequenceTransitions && _ReturnAddress( ) == MaintainSequenceTransitions )
				return true;
		}
	}

	static auto SetupBones_Timing = ( void* )pattern::find( g_csgo.m_client_dll, "84 C0 74 0A F3 0F 10 05 ? ? ? ? EB 05" ); //C_BaseAnimating::SetupBones
	if ( settings.fix_setup_bones_timing ) {
		if ( settings.enable ) {
			if ( SetupBones_Timing && /*cheat::main::updating_resolver_anims &&*/ _ReturnAddress( ) == SetupBones_Timing )
				return false;
		}
	}


	return g_hooks.m_prediction.GetOldMethod< InPrediction_t >( CPrediction::INPREDICTION )( this );
}

void fix_attack_packet( CUserCmd* cmd, bool m_predict ) {
	// fuck llama and his shit code style
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;

	if ( m_predict ) {
		m_bLastAttack = cmd->m_weapon_select || ( cmd->m_buttons & IN_ATTACK2 );
		m_flLastCycle = g_cl.m_local->m_flCycle( );

		if ( settings.pred_logs )
			notify_controller::g_notify.push_event( "predicted cycle", "[prediction]" );
	}
	else if ( m_bLastAttack && !m_bInvalidCycle ) {
		m_bInvalidCycle = g_cl.m_local->m_flCycle( ) == 0.f && m_flLastCycle > 0.f;

		if ( settings.pred_logs )
			notify_controller::g_notify.push_event( "not-predicted cycle", "[prediction]" );
	}
}

void fix_revolver( ) {
	auto weapon = g_cl.m_weapon;
	if ( !weapon )
		return;

	if ( weapon->m_iItemDefinitionIndex( ) == 64 && /*g_pSettings->RevolverShit*/ weapon->m_flNextSecondaryAttack( ) == FLT_MAX )
		weapon->m_flNextSecondaryAttack( ) = weapon->m_flNextSecondaryAttack();

	if ( settings.pred_logs )
		notify_controller::g_notify.push_event( "revolver-chocked", "[prediction]" );
}

void Hooks::RunCommand( Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper ) {
	// airstuck jitter / overpred fix.
	if( cmd->m_tick >= std::numeric_limits< int >::max( ) )
		return;

	// onetap reverse
	if ( settings.violanes_prediction ) {
		fix_attack_packet( cmd, true ); // idk
		fix_revolver( );
	}

	g_hooks.m_prediction.GetOldMethod< RunCommand_t >( CPrediction::RUNCOMMAND )( this, ent, cmd, movehelper );

	// store non compressed netvars.
	g_netdata.store( );

	// onetap reverse
	if ( settings.violanes_prediction ) {
		fix_attack_packet( cmd, false ); // idk
	}
}