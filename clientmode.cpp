#include "includes.h"
#include "backend/config/config_new.h"
#include "backend/Framework/MenuFramework.h"
#include "backend/Menu/Menu.h"
bool Hooks::ShouldDrawParticles( ) {
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( ) {
	// remove fog.
	if( settings.removefog )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// g_grenades.think( );
	g_visuals.ThirdpersonThink( );

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );

    // remove scope edge blur.
	if( settings.noscope ) {
		if( g_cl.m_local && g_cl.m_local->m_bIsScoped( ) )
            view->m_edge_blur = 0;
	}
}

void air_stuck( ) {
	auto local = g_cl.m_local;
	if ( !local )
		return;

	if ( GetKeyState( settings.air_stuck_reverse ) )
		g_cl.m_cmd->m_command_number = g_cl.m_cmd->m_tick = INT_MAX;
}

bool Hooks::CreateMove( float time, CUserCmd* cmd ) {
	g_aimbot.m_damage_override = GetKeyState( settings.override_keu );
	g_aimbot.m_hc_override = GetKeyState( settings.override_hc_key );
	g_aimbot.m_force_baim = GetKeyState( settings.force_baim_key );

	auto set = settings;
	auto left = set.aa_left;
	auto right = set.aa_right;
	auto back = set.aa_back;

	g_hvh.m_left = KeyHandler::CheckKey( left, settings.keystyle_left );
	g_hvh.m_right = KeyHandler::CheckKey( right, settings.key_right_style );
	g_hvh.m_back = KeyHandler::CheckKey( back, settings.keystyle_manu_bacl );

	Getze::VisualsControll::add_trail( );

	/*
	// this is a semi fix for now
	if ( KeyHandler::CheckKey( left, settings.keystyle_left ) ) {
		g_hvh.m_left = !g_hvh.m_left;
		g_hvh.m_right = false;
		g_hvh.m_back = false;
	}
	else if ( KeyHandler::CheckKey( right, settings.key_right_style ) ) {
		g_hvh.m_left = false;
		g_hvh.m_right = !g_hvh.m_right;
		g_hvh.m_back = false;
	}
	else if ( KeyHandler::CheckKey( back, settings.keystyle_manu_bacl ) ) {
		g_hvh.m_left = false;
		g_hvh.m_right = false;
		g_hvh.m_back = !g_hvh.m_back;
	}
	*/

	/* eploit??
	
		if ( GetKeyState( left ) )
			g_hvh.m_left = !g_hvh.m_left;

		if ( GetKeyState( right ) )
			g_hvh.m_right = !g_hvh.m_right;

		if ( GetKeyState( back ) )
			g_hvh.m_back = !g_hvh.m_back;
	*/

	//g_hvh.m_left = GetAsyncKeyState( set.aa_left );
	//g_hvh.m_right = GetAsyncKeyState( set.aa_right );
	//g_hvh.m_back = GetAsyncKeyState( set.aa_back );

	Stack   stack;
	bool    ret;

	// let original run first.
	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// called from CInput::ExtraMouseSample -> return original.
	if( !cmd->m_command_number )
		return ret;

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if( ret )
		g_csgo.m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	g_cl.m_packet = stack.next( ).local( 0x1c ).as< bool* >( );

	// get bFinalTick off the stack.
	g_cl.m_final_packet = stack.next( ).local( 0x1b ).as< bool* >( );

	if ( CMenu::get( ).IsMenuOpened() ) {
		// are we IN_ATTACK?
		if ( cmd->m_buttons & IN_ATTACK )
			cmd->m_buttons &= ~IN_ATTACK;

		// are we IN_ATTACK2?
		if ( cmd->m_buttons & IN_ATTACK2 )
			cmd->m_buttons &= ~IN_ATTACK2;
	}

	if ( settings.detect_ruberband ) {
		Getze::EngineController::DetectRubberband_sub21223( );
	}

	if ( settings.predict_velocity ) {
		Getze::EngineController::sub_343BA99F( );
	}

	air_stuck( );

	// invoke move function.
	g_cl.OnTick( cmd );

	// fuk it
	//NetworkControll::KeepComunication( );

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup ) {
	g_visuals.RenderGlow( );

	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}