#include "includes.h"
#include "backend/config/config_new.h"
Visuals g_visuals{ };;

Color to_color( int col[ 4 ] ) {
	return Color( col[ 0 ], col[ 1 ], col[ 2 ], 255 );
}

int epoch_time( ) {
	return std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now( ).time_since_epoch( ) ).count( );
}

void Visuals::PlayerHurt( IGameEvent* pEvent ) {
	Player* attacker = g_csgo.m_entlist->GetClientEntity<Player*>( g_csgo.m_engine->GetPlayerForUserID( pEvent->m_keys->FindKey( HASH( "attacker" ) )->GetInt( ) ) );
	Player* victim = g_csgo.m_entlist->GetClientEntity<Player*>( g_csgo.m_engine->GetPlayerForUserID( pEvent->m_keys->FindKey( HASH( "userid" ) )->GetInt( ) ) );

	if ( !attacker || !victim || attacker != g_cl.m_local )
		return;

	vec3_t enemy_pos = victim->m_vecOrigin( );

	ImpactInfo_t best_impact;

	float best_impact_distance = 9999999999.f;

	long long time = epoch_time( );

	std::vector<ImpactInfo_t>::iterator iter;

	for ( iter = m_impacts.begin( ); iter != m_impacts.end( ); ) {
		if ( time > iter->time + 25 ) {
			iter = m_impacts.erase( iter );
			continue;
		}

		vec3_t position = vec3_t( iter->x, iter->y, iter->z );

		float distance = position.dist_to( enemy_pos );

		if ( distance < best_impact_distance ) {
			best_impact_distance = distance;
			best_impact = *iter;
		}
		iter++;
	}

	if ( best_impact_distance == 9999999999.f )
		return;

	HitmarkerInfo_t info;
	info.impact = best_impact;
	info.alpha = 255;
	info.damage = pEvent->m_keys->FindKey( HASH( "dmg_health" ) )->GetInt( );
	info.kill = pEvent->m_keys->FindKey( HASH( "health" ) )->GetInt( ) <= 0;
	info.didhs = pEvent->m_keys->FindKey( HASH( "hitgroup" ) )->GetInt( ) == 1;
	m_hitmarkers.push_back( info );
}

void Visuals::ModulateWorld( ) {
	std::vector< IMaterial* > world, props;

	// iterate material handles.
	for( uint16_t h{ g_csgo.m_material_system->FirstMaterial( ) }; h != g_csgo.m_material_system->InvalidMaterial( ); h = g_csgo.m_material_system->NextMaterial( h ) ) {
		// get material from handle.
		IMaterial* mat = g_csgo.m_material_system->GetMaterial( h );
		if( !mat )
			continue;

		// store world materials.
		if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "World textures" ) )
			world.push_back( mat );

		// store props.
		else if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "StaticProp textures" ) )
			props.push_back( mat );
	}

	// night.
	if( settings.night_mode ) {
		for( const auto& w : world )
			w->ColorModulate( 0.17f, 0.16f, 0.18f );

		// IsUsingStaticPropDebugModes my nigga
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for( const auto& p : props )
			p->ColorModulate( 0.5f, 0.5f, 0.5f );
	}

	// disable night.
	else {
		for( const auto& w : world )
			w->ColorModulate( 1.f, 1.f, 1.f );

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->ColorModulate( 1.f, 1.f, 1.f );
	}

	// transparent props.
	if( settings.transparent_props ) {

		// IsUsingStaticPropDebugModes my nigga
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 0.85f );
	}

	// disable transparent props.
	else {

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 1.0f );
	}
}

void Visuals::ThirdpersonThink( ) {
	ang_t                          offset;
	vec3_t                         origin, forward;
	static CTraceFilterSimple_game filter{ };
	CGameTrace                     tr;

	// for whatever reason overrideview also gets called from the main menu.
	if( !g_csgo.m_engine->IsInGame( ) )
		return;

	// check if we have a local player and he is alive.
	bool alive = g_cl.m_local && g_cl.m_local->alive( );

	// camera should be in thirdperson.
	if( GetKeyState( settings.tp_key ) ) {

		// if alive and not in thirdperson already switch to thirdperson.
		if( alive && !g_csgo.m_input->CAM_IsThirdPerson( ) )
			g_csgo.m_input->CAM_ToThirdPerson( );

		// if dead and spectating in firstperson switch to thirdperson.
		else if( g_cl.m_local->m_iObserverMode( ) == 4 ) {

			// if in thirdperson, switch to firstperson.
			// we need to disable thirdperson to spectate properly.
			if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
				g_csgo.m_input->CAM_ToFirstPerson( );
				g_csgo.m_input->m_camera_offset.z = 0.f;
			}

			g_cl.m_local->m_iObserverMode( ) = 5;
		}
	}

	// camera should be in firstperson.
	else if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		g_csgo.m_input->CAM_ToFirstPerson( );
		g_csgo.m_input->m_camera_offset.z = 0.f;
	}

	// if after all of this we are still in thirdperson.
	if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		// get camera angles.
		g_csgo.m_engine->GetViewAngles( offset );

		// get our viewangle's forward directional vector.
		math::AngleVectors( offset, &forward );

		// cam_idealdist convar.
		offset.z = 150.f;

		// start pos.
		origin = g_cl.m_shoot_pos;

		// setup trace filter and trace.
		filter.SetPassEntity( g_cl.m_local );

		g_csgo.m_engine_trace->TraceRay(
			Ray( origin, origin - ( forward * offset.z ), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f } ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter* ) &filter,
			&tr
		);

		// adapt distance to travel time.
		math::clamp( tr.m_fraction, 0.f, 1.f );
		offset.z *= tr.m_fraction;

		// override camera angles.
		g_csgo.m_input->m_camera_offset = { offset.x, offset.y, offset.z };
	}
}

void Visuals::Hitmarker( ) {
	float complete = ( g_csgo.m_globals->m_curtime - m_hit_start ) / m_hit_duration;
	int x = g_cl.m_width / 2,
		y = g_cl.m_height / 2,
		alpha = ( 1.f - complete ) * 240;

	if( g_csgo.m_globals->m_curtime > m_hit_end )
		return;

	if( m_hit_duration <= 0.f )
		return;

	constexpr int line{ 6 };

	if ( settings.hitmarker ) {
		render::line( x - line, y - line, x - ( line / 4 ), y - ( line / 4 ), { 200, 200, 200, alpha } );
		render::line( x - line, y + line, x - ( line / 4 ), y + ( line / 4 ), { 200, 200, 200, alpha } );
		render::line( x + line, y + line, x + ( line / 4 ), y + ( line / 4 ), { 200, 200, 200, alpha } );
		render::line( x + line, y - line, x + ( line / 4 ), y - ( line / 4 ), { 200, 200, 200, alpha } );
	}

	if ( settings.dhitmarker ) {
		if ( !g_cl.m_processing || !g_csgo.m_engine->IsInGame( ) ) {
			if ( !m_impacts.empty( ) )
				m_impacts.clear( );

			if ( !m_hitmarkers.empty( ) )
				m_hitmarkers.clear( );

			return;
		}

		long long time = epoch_time( );

		std::vector<HitmarkerInfo_t>::iterator iter;

		int m_actual_size = m_hitmarkers.size( ) - 1;

		for ( iter = m_hitmarkers.begin( ); iter != m_hitmarkers.end( ); ) {
			bool expired = time > iter->impact.time + 4000; // 2000

			if ( expired )
				iter->alpha -= 220 / 75;

			if ( expired && iter->alpha <= 0 ) {
				iter = m_hitmarkers.erase( iter );
				m_actual_size--;
				continue;
			}

			auto m_end = m_hitmarkers[ m_actual_size ];

			vec3_t m_end_pos3D = vec3_t( m_end.impact.x, m_end.impact.y, m_end.impact.z );
			vec3_t m_pos3D = vec3_t( iter->impact.x, iter->impact.y, iter->impact.z );

			vec2_t pos2D;

			if ( !render::WorldToScreen( m_pos3D, pos2D ) ) {
				++iter;
				continue;
			}

			int m_line_size = 8;

			std::string damage_text;
			damage_text += std::to_string( iter->damage );

			const auto color = iter->kill ? Color( 255, 0, 0, iter->alpha ) : Color( 255, 255, 255, iter->alpha );

			render::line( vec2_t( int( pos2D.x - m_line_size / 1.8 ), int( pos2D.y - m_line_size / 1.8 ) ), vec2_t( int( pos2D.x - m_line_size ), int( pos2D.y - m_line_size ) ), Color( 255, 255, 255, iter->alpha ) );
			render::line( vec2_t( int( pos2D.x - m_line_size / 1.8 ), int( pos2D.y + m_line_size / 1.8 ) ), vec2_t( int( pos2D.x - m_line_size ), int( pos2D.y + m_line_size ) ), Color( 255, 255, 255, iter->alpha ) );
			render::line( vec2_t( int( pos2D.x + m_line_size / 1.8 ), int( pos2D.y + m_line_size / 1.8 ) ), vec2_t( int( pos2D.x + m_line_size ), int( pos2D.y + m_line_size ) ), Color( 255, 255, 255, iter->alpha ) );
			render::line( vec2_t( int( pos2D.x + m_line_size / 1.8 ), int( pos2D.y - m_line_size / 1.8 ) ), vec2_t( int( pos2D.x + m_line_size ), int( pos2D.y - m_line_size ) ), Color( 255, 255, 255, iter->alpha ) );

			if ( m_pos3D == m_end_pos3D )
				render::esp.string( pos2D.x, pos2D.y - 20, Color( 255, 255, 255, iter->alpha ), damage_text.c_str( ), render::StringFlags_t::ALIGN_CENTER );

			++iter;
		}
	}
}

void Visuals::NoSmoke( ) {
	if( !smoke1 )
		smoke1 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_fire" ), XOR( "Other textures" ) );

	if( !smoke2 )
		smoke2 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_smokegrenade" ), XOR( "Other textures" ) );

	if( !smoke3 )
		smoke3 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods" ), XOR( "Other textures" ) );

	if( !smoke4 )
		smoke4 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods_impactdust" ), XOR( "Other textures" ) );

	if( settings.nosmoke ) {
		if( !smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, true );
	}

	else {
		if( smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, false );
	}
}

void Visuals::think( ) {
	// don't run anything if our local player isn't valid.
	if( !g_cl.m_local )
		return;

	if( settings.noscope
		&& g_cl.m_local->alive( )
		&& g_cl.m_local->GetActiveWeapon( )
		&& g_cl.m_local->GetActiveWeapon( )->GetWpnData( )->m_weapon_type == CSWeaponType::WEAPONTYPE_SNIPER_RIFLE
		&& g_cl.m_local->m_bIsScoped( ) ) {

		// rebuild the original scope lines.
		int w = g_cl.m_width,
			h = g_cl.m_height,
			x = w / 2,
			y = h / 2,
			size = g_csgo.cl_crosshair_sniper_width->GetInt( );

		// Here We Use The Euclidean distance To Get The Polar-Rectangular Conversion Formula.
		if( size > 1 ) {
			x -= ( size / 2 );
			y -= ( size / 2 );
		}

		// draw our lines.
		render::rect_filled( 0, y, w, size, colors::black );
		render::rect_filled( x, 0, size, h, colors::black );
	}

	// draw esp on ents.
	for( int i{ 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); ++i ) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if( !ent )
			continue;

		draw( ent );
	}

	if ( settings.indicator_style == 3 ) {
		Getze::VisualsControll::fakelag_indicator( );
		StatusIndicators( );
		Spectators( );
	}

	// draw everything else.
	PenetrationCrosshair( );
	Hitmarker( );
	DrawPlantedC4( );

	Getze::VisualsControll::manual_arrows( );

	Getze::VisualsControll::indicator( );
	Getze::VisualsControll::indicator_ui( );
	Getze::VisualsControll::indicator_spect( );
	Getze::VisualsControll::draw_dmg( );
}

void Visuals::Spectators( ) {
	if( !settings.spectators )
		return;

	std::vector< std::string > spectators{ XOR( "spectators" ) };
	int h = render::menu_shade.m_size.m_height;

	for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
		if( !player )
			continue;

		if( player->m_bIsLocalPlayer( ) )
			continue;

		if( player->dormant( ) )
			continue;

		if( player->m_lifeState( ) == LIFE_ALIVE )
			continue;

		if( player->GetObserverTarget( ) != g_cl.m_local )
			continue;

		player_info_t info;
		if( !g_csgo.m_engine->GetPlayerInfo( i, &info ) )
			continue;

		spectators.push_back( std::string( info.m_name ).substr( 0, 24 ) );
	}

	size_t total_size = spectators.size( ) * ( h - 1 );

	for( size_t i{ }; i < spectators.size( ); ++i ) {
		const std::string& name = spectators[ i ];

		render::menu_shade.string( g_cl.m_width - 20, ( g_cl.m_height / 2 ) - ( total_size / 2 ) + ( i * ( h - 1 ) ),
			{ 255, 255, 255, 179 }, name, render::ALIGN_RIGHT );
	}
}

void Visuals::StatusIndicators( ) {
	// dont do if dead.
	if( !g_cl.m_processing )
		return;

	// compute hud size.
	// int size = ( int )std::round( ( g_cl.m_height / 17.5f ) * g_csgo.hud_scaling->GetFloat( ) );

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators{ };

	auto indicator_enable = settings.indicator_enable;

	// LC
	if( indicator_enable ) {
		if( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 270.f || g_cl.m_lagcomp ) {
			Indicator_t ind{ };
			ind.color = 0xff15c27b;
			ind.text = XOR( "LC" );

			indicators.push_back( ind );
		}
	}

	// LBY
	if( indicator_enable ) {
		// get the absolute change between current lby and animated angle.
		float change = std::abs( math::NormalizedAngle( g_cl.m_body - g_cl.m_angle.y ) );

		Indicator_t ind{ };
		ind.color = change > 35.f ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR( "LBY" );
		indicators.push_back( ind );
	}

	// PING
	if( g_aimbot.m_fake_latency ) {
		Indicator_t ind{ };
		ind.color = 0xff15c27b;
		ind.text = XOR( "PING" );

		indicators.push_back( ind );
	}

	if ( g_aimbot.m_damage_override ) {
		Indicator_t ind{ };
		ind.color = 0xff15c27b;
		ind.text = XOR( "DMG" );

		indicators.push_back( ind );
	}

	if ( TickbaseControll::m_double_tap ) {
		Indicator_t ind{ };
		ind.color = 0xff15c27b;
		ind.text = XOR( "DT" );

		indicators.push_back( ind );
	}

	if ( g_aimbot.m_force_baim ) {
		Indicator_t ind{ };
		ind.color = 0xff15c27b;
		ind.text = XOR( "BAIM" );

		indicators.push_back( ind );
	}

	if( indicators.empty( ) )
		return;

	// iterate and draw indicators.
	for( size_t i{ }; i < indicators.size( ); ++i ) {
		auto& indicator = indicators[ i ];

		render::indicator.string( 20, g_cl.m_height - 80 - ( 30 * i ), indicator.color, indicator.text );
	}
}

void Visuals::PenetrationCrosshair( ) {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if( !settings.pen_crosshair || !g_cl.m_processing )
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;


	valid_player_hit = ( g_cl.m_pen_data.m_target && g_cl.m_pen_data.m_target->enemy( g_cl.m_local ) );
	if( valid_player_hit )
		final_color = colors::transparent_yellow;

	else if( g_cl.m_pen_data.m_pen )
		final_color = colors::transparent_green;

	else
		final_color = colors::transparent_red;

	// todo - dex; use fmt library to get damage string here?
	//             draw damage string?

	// draw small square in center of screen.
	render::rect_filled( x - 1, y - 1, 3, 3, final_color );
}

void Visuals::draw( Entity* ent ) {
	// draw the bomb over
	//Player* entity = ent->as< Player* >( );
	//Getze::VisualsControll::bomb_indicator( entity );

	if( ent->IsPlayer( ) ) {
		Player* player = ent->as< Player* >( );

		// dont draw dead players.
		if( !player->alive( ) )
			return;

		if( player->m_bIsLocalPlayer( ) )
			return;

		// draw player esp.
		DrawPlayer( player );
		
	}

	else if( settings.items && ent->IsBaseCombatWeapon( ) && !ent->dormant( ) )
		DrawItem( ent->as< Weapon* >( ) );

	else if( settings.proj )
		DrawProjectile( ent->as< Weapon* >( ) );
}

void getze_nade( int x, int y, double factor, Color color ) {
	render::rect_filled( x - 49, y + 10, 98.f, 4, { 80, 80, 80, 125 } );
	render::rect_filled( x - 49, y + 10, 98.f * factor, 4, color );

	render::rect( x - 49, y + 10, 98.f * factor, 4, Color( 10, 10, 10 ) );
}

void DrawPlate( std::string text, std::string sub_text, int x, int y, bool centered, int barpos, float barpercent, Color barclr, int alpha ) {
	int dst = false;
	float a = alpha / 255.f;
	int bar_size = 8;
	render::FontSize_t txts = render::bold.size( text );
	bar_size += txts.m_width;

	if ( sub_text != "" ) {
		dst = true;
		render::FontSize_t mini_txts = render::norm.size( sub_text );
		bar_size += 2 + mini_txts.m_width;
	}

	int x_pos = x;
	if ( centered )
		x_pos = floor( x - ( bar_size / 2 ) );

	render::rect_filled( x_pos, y, bar_size, 16, Color( 0, 0, 0, 150 * a ) );
	render::bold.string( x_pos + 4, y + 2, Color( 255, 255, 255, 255 * a ), text );

	if ( dst )
		render::norm.string( x_pos + txts.m_width + 6, y + 2, Color( 200, 200, 200, 255 * a ), sub_text );

	if ( barpos != 0 ) {
		if ( barpos == 1 ) {
			render::rect_filled( x_pos, y + 16, bar_size, 1, Color( 0, 0, 0, 255 * a ) );
			render::rect_filled( x_pos + ( bar_size / 2 - barpercent / 2 * bar_size ), y + 16, barpercent * bar_size, 1, Color( barclr.r( ), barclr.g( ), barclr.b( ), 255 * a ) );
		}
		else if ( barpos == 2 ) {
			render::rect_filled( x_pos, y - 1, bar_size, 1, Color( 0, 0, 0, 255 * a ) );
			render::rect_filled( x_pos + ( bar_size / 2 - barpercent / 2 * bar_size ), y - 1, barpercent * bar_size, 1, Color( barclr.r( ), barclr.g( ), barclr.b( ), 255 * a ) );
		}
	}

}

template <typename T>
std::string to_string_with_precision( const T a_value, const int n = 3 )
{
	std::ostringstream out;
	out.precision( n );
	out << std::fixed << a_value;
	return out.str( );
}


void Visuals::DrawProjectile( Weapon* ent ) {
	vec2_t screen;
	vec3_t origin = ent->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	Color col = to_color( settings.proj_color ); // fuck this out
	col.a( ) = 0xb4;

	auto moly_color = Color( 255, 0, 0 );
	auto smoke_color = Color( 58, 214, 252 );
	Color col_safe_icon = Color( 255, 255, 255, 255 );
	Color col_lethal_icon = Color( 255, 255, 255, 255 );
	int dist = g_cl.m_local->m_vecOrigin( ).dist_to( origin ) / 12;

	switch ( settings.projectile_tiple ) {
		case 0: { // default
			// draw decoy.
			if ( ent->is( HASH( "CDecoyProjectile" ) ) )
				render::esp_small.string( screen.x, screen.y, col, XOR( "DECOY" ), render::ALIGN_CENTER );

			// draw molotov.
			else if ( ent->is( HASH( "CMolotovProjectile" ) ) )
				render::esp_small.string( screen.x, screen.y, col, XOR( "MOLLY" ), render::ALIGN_CENTER );

			else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
				const model_t* model = ent->GetModel( );

				if ( model ) {
					// grab modelname.
					std::string name{ ent->GetModel( )->m_name };

					if ( name.find( XOR( "flashbang" ) ) != std::string::npos )
						render::esp_small.string( screen.x, screen.y, col, XOR( "FLASH" ), render::ALIGN_CENTER );

					else if ( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {

						// grenade range.
						if ( settings.proj_ranages )
							render::sphere( origin, 50.f, 5.f, 1.f, to_color( settings.proj_range_color ) );

						render::esp_small.string( screen.x, screen.y, col, XOR( "FRAG" ), render::ALIGN_CENTER );
					}
				}
			}

			// find classes.
			else if ( ent->is( HASH( "CInferno" ) ) ) {
				// fire range.
				if ( settings.proj_ranages )
					render::sphere( origin, 50.f, 5.f, 1.f, to_color( settings.proj_range_color ) );

				render::esp_small.string( screen.x, screen.y, col, XOR( "FIRE" ), render::ALIGN_CENTER );
			}

			else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) )
				render::esp_small.string( screen.x, screen.y, col, XOR( "SMOKE" ), render::ALIGN_CENTER );
		} break;
		case 1: {
			// draw decoy.
			if ( ent->is( HASH( "CDecoyProjectile" ) ) )
				render::esp_small.string( screen.x, screen.y, col, XOR( "DECOY" ), render::ALIGN_CENTER );

			// draw molotov.
			else if ( ent->is( HASH( "CMolotovProjectile" ) ) )
				render::esp_small.string( screen.x, screen.y, col, XOR( "MOLLY" ), render::ALIGN_CENTER );

			else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
				const model_t* model = ent->GetModel( );

				if ( model ) {
					// grab modelname.
					std::string name{ ent->GetModel( )->m_name };

					if ( name.find( XOR( "flashbang" ) ) != std::string::npos )
						render::esp_small.string( screen.x, screen.y, col, XOR( "FLASH" ), render::ALIGN_CENTER );

					else if ( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {
						render::esp_small.string( screen.x, screen.y, col, XOR( "FRAG" ), render::ALIGN_CENTER );
					}
				}
			}

			// find classes.
			else if ( ent->is( HASH( "CInferno" ) ) ) {
				Weapon* grenade_entity = reinterpret_cast< Weapon* >( ent );
				const auto spawn_time = *( float* )( uintptr_t( grenade_entity ) + 0x20 );
				const auto factor = ( ( spawn_time + 7.03125f ) - g_csgo.m_globals->m_curtime ) / 7.03125f;

				int red = fmax( fmin( 255 * factor, 255 ), 0 );
				int green = fmax( fmin( 255 * ( 1.f - factor ), 255 ), 0 );

				if ( spawn_time > 0.f ) { // l3d saves me
					/* this avoid rendering when nade is not on the ground */
					getze_nade( screen.x, screen.y, factor, Color( red, green, 0, 245 ) );
				}

				render::esp_small.string( screen.x, screen.y, col, XOR( "FIRE" ), render::ALIGN_CENTER );
			}

			else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) ) {
				Weapon* pSmokeEffect = reinterpret_cast< Weapon* >( ent );
				const float spawn_time = game::TICKS_TO_TIME( pSmokeEffect->m_nSmokeEffectTickBegin( ) );
				const double reltime = ( ( spawn_time + 17.441 ) - g_csgo.m_globals->m_curtime );
				const double factor = reltime / 17.441;

				int red = std::fmax( std::fmin( 255 * factor, 255 ), 0 );
				int green = std::fmax( std::fmin( 255 * ( 1.f - factor ), 255 ), 0 );

				if ( spawn_time > 0.f ) { // l3d saves me
					/* this avoid rendering when nade is not on the ground */
					getze_nade( screen.x, screen.y, factor, Color( red, green, 0, 245 ) );
				}

				render::esp_small.string( screen.x, screen.y, col, XOR( "SMOKE" ), render::ALIGN_CENTER );
			}
		} break;
		case 2: {
			// draw molotov.
			if ( ent->is( HASH( "CMolotovProjectile" ) ) ) {
				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "fire", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
				} break;
				case 3: { // pandorel
					render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
					render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
					render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "l", render::ALIGN_LEFT );
				} break;
				}
			}

			else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
				const model_t* model = ent->GetModel( );

				if ( model ) {
					// grab modelname.
					std::string name{ ent->GetModel( )->m_name };

					if ( name.find( XOR( "flashbang" ) ) != std::string::npos ) {
						switch ( settings.projectile_tiple ) {
						case 2: { // hyperbius
							DrawPlate( "flash", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
						} break;
						case 3: { // pandorel
							render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
							render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
							render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "i", render::ALIGN_LEFT );
						} break;
						}
					}

					else if ( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {
						switch ( settings.projectile_tiple ) {
						case 2: { // hyperbius
							DrawPlate( "frag", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
						} break;
						case 3: { // pandorel
							render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
							render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
							render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "j", render::ALIGN_LEFT );
						} break;
						}
					}
				}
			}

			// find classes.
			else if ( ent->is( HASH( "CInferno" ) ) ) {
				const double spawn_time = *( float* )( uintptr_t( ent ) + 0x20 );
				const double reltime = ( ( spawn_time + 7.031 ) - g_csgo.m_globals->m_curtime );
				const double factor = reltime / 7.031;

				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "fire", "[ " + to_string_with_precision( ( spawn_time + 7.03125f ) - g_csgo.m_globals->m_curtime, 1 ) + " ]", screen.x, screen.y, true, 1, factor, moly_color, 155 );
				} break;
				case 3: { // pandorel
					if ( dist <= 85 ) { // we render the circle like a boss
						float radius = 144.f;
						render::WorldCircleOutline( origin, radius, 1.f, moly_color );
					}

					render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
					render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
					render::csgh.semi_filled_text_v( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "l", render::ALIGN_LEFT, factor );

				} break;
				}
			}

			else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) ) {
				Weapon* pSmokeEffect = reinterpret_cast< Weapon* >( ent );
				const float spawn_time = game::TICKS_TO_TIME( pSmokeEffect->m_nSmokeEffectTickBegin( ) );
				const double reltime = ( ( spawn_time + 17.441 ) - g_csgo.m_globals->m_curtime );
				const double factor = reltime / 17.441;

				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "smoke", "[ " + to_string_with_precision( ( ( spawn_time + 17.441 ) - g_csgo.m_globals->m_curtime ), 1 ) + " ]", screen.x, screen.y, true, 1, factor, smoke_color, 155 );
				} break;
				case 3: { // pandorel
					if ( spawn_time > 0.f ) { // l3d saves me
						if ( dist <= 85 ) { // we render the circle like a boss
							float radius = 144.f;
							render::WorldCircleOutline( origin, radius, 1.f, smoke_color );
						}

						render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
						render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
						render::csgh.semi_filled_text_v( screen.x - 5, screen.y - 23, { 255,255,255,255 }, "k", render::ALIGN_LEFT, factor );
					}
				} break;
				}
			}
		} break;
		case 3: {
			// draw molotov.
			if ( ent->is( HASH( "CMolotovProjectile" ) ) ) {
				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "fire", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
				} break;
				case 3: { // pandorel
					render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
					render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
					render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "l", render::ALIGN_LEFT );
				} break;
				}
			}

			else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
				const model_t* model = ent->GetModel( );

				if ( model ) {
					// grab modelname.
					std::string name{ ent->GetModel( )->m_name };

					if ( name.find( XOR( "flashbang" ) ) != std::string::npos ) {
						switch ( settings.projectile_tiple ) {
						case 2: { // hyperbius
							DrawPlate( "flash", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
						} break;
						case 3: { // pandorel
							render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
							render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
							render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "i", render::ALIGN_LEFT );
						} break;
						}
					}

					else if ( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {
						switch ( settings.projectile_tiple ) {
						case 2: { // hyperbius
							DrawPlate( "frag", "[ " + std::to_string( dist ) + " ft" + " ]", screen.x, screen.y, true, 1, 0, moly_color, 155 );
						} break;
						case 3: { // pandorel
							render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
							render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
							render::csgh.string( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "j", render::ALIGN_LEFT );
						} break;
						}
					}
				}
			}

			// find classes.
			else if ( ent->is( HASH( "CInferno" ) ) ) {
				const double spawn_time = *( float* )( uintptr_t( ent ) + 0x20 );
				const double reltime = ( ( spawn_time + 7.031 ) - g_csgo.m_globals->m_curtime );
				const double factor = reltime / 7.031;

				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "fire", "[ " + to_string_with_precision( ( spawn_time + 7.03125f ) - g_csgo.m_globals->m_curtime, 1 ) + " ]", screen.x, screen.y, true, 1, factor, moly_color, 155 );
				} break;
				case 3: { // pandorel
					if ( dist <= 85 ) { // we render the circle like a boss
						float radius = 144.f;
						render::WorldCircleOutline( origin, radius, 1.f, moly_color );
					}

					render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
					render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
					render::csgh.semi_filled_text_v( screen.x - 9, screen.y - 23, { 255,255,255,255 }, "l", render::ALIGN_LEFT, factor );

				} break;
				}
			}

			else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) ) {
				Weapon* pSmokeEffect = reinterpret_cast< Weapon* >( ent );
				const float spawn_time = game::TICKS_TO_TIME( pSmokeEffect->m_nSmokeEffectTickBegin( ) );
				const double reltime = ( ( spawn_time + 17.441 ) - g_csgo.m_globals->m_curtime );
				const double factor = reltime / 17.441;

				switch ( settings.projectile_tiple ) {
				case 2: { // hyperbius
					DrawPlate( "smoke", "[ " + to_string_with_precision( ( ( spawn_time + 17.441 ) - g_csgo.m_globals->m_curtime ), 1 ) + " ]", screen.x, screen.y, true, 1, factor, smoke_color, 155 );
				} break;
				case 3: { // pandorel
					if ( spawn_time > 0.f ) { // l3d saves me
						if ( dist <= 85 ) { // we render the circle like a boss
							float radius = 144.f;
							render::WorldCircleOutline( origin, radius, 1.f, smoke_color );
						}

						render::circle( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 120 ) );
						render::circle_outline( screen.x, screen.y - 10, 20, 360, Color( 0, 0, 0, 255 ) );
						render::csgh.semi_filled_text_v( screen.x - 5, screen.y - 23, { 255,255,255,255 }, "k", render::ALIGN_LEFT, factor );
					}
				} break;
				}
			}
		} break;
	}
}

void Visuals::DrawItem( Weapon* item ) {
	// we only want to draw shit without owner.
	Entity* owner = g_csgo.m_entlist->GetClientEntityFromHandle( item->m_hOwnerEntity( ) );
	if( owner )
		return;

	// is the fucker even on the screen?
	vec2_t screen;
	vec3_t origin = item->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	WeaponInfo* data = item->GetWpnData( );
	if( !data )
		return;

	Color col = to_color(settings.item_color); // fuckit
	col.a( ) = 0xb4;

	// render bomb in green.
	if( item->is( HASH( "CC4" ) ) )
		render::esp_small.string( screen.x, screen.y, { 150, 200, 60, 0xb4 }, XOR( "BOMB" ), render::ALIGN_CENTER );

	// if not bomb
	// normal item, get its name.
	else {
		std::string name{ item->GetLocalizedName( ) };

		// smallfonts needs uppercase.
		std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

		render::esp_small.string( screen.x, screen.y, col, name, render::ALIGN_CENTER );
	}

	if( !settings.ammo_dropped ) // weapoin ammo
		return;

	// nades do not have ammo.
	if( data->m_weapon_type == WEAPONTYPE_GRENADE || data->m_weapon_type == WEAPONTYPE_KNIFE )
		return;

	if( item->m_iItemDefinitionIndex( ) == 0 || item->m_iItemDefinitionIndex( ) == C4 )
		return;

	std::string ammo = tfm::format( XOR( "(%i/%i)" ), item->m_iClip1( ), item->m_iPrimaryReserveAmmoCount( ) );
	render::esp_small.string( screen.x, screen.y - render::esp_small.m_size.m_height - 1, col, ammo, render::ALIGN_CENTER );
}

void Visuals::OffScreen( Player* player, int alpha ) {
	vec3_t view_origin, target_pos, delta;
	vec2_t screen_pos, offscreen_pos;
	float  leeway_x, leeway_y, radius, offscreen_rotation;
	bool   is_on_screen;
	Vertex verts[ 3 ], verts_outline[ 3 ];
	Color  color;

	// todo - dex; move this?
	static auto get_offscreen_data = [ ] ( const vec3_t& delta, float radius, vec2_t& out_offscreen_pos, float& out_rotation ) {
		ang_t  view_angles( g_csgo.m_view_render->m_view.m_angles );
		vec3_t fwd, right, up( 0.f, 0.f, 1.f );
		float  front, side, yaw_rad, sa, ca;

		// get viewport angles forward directional vector.
		math::AngleVectors( view_angles, &fwd );

		// convert viewangles forward directional vector to a unit vector.
		fwd.z = 0.f;
		fwd.normalize( );

		// calculate front / side positions.
		right = up.cross( fwd );
		front = delta.dot( fwd );
		side = delta.dot( right );

		// setup offscreen position.
		out_offscreen_pos.x = radius * -side;
		out_offscreen_pos.y = radius * -front;

		// get the rotation ( yaw, 0 - 360 ).
		out_rotation = math::rad_to_deg( std::atan2( out_offscreen_pos.x, out_offscreen_pos.y ) + math::pi );

		// get needed sine / cosine values.
		yaw_rad = math::deg_to_rad( -out_rotation );
		sa = std::sin( yaw_rad );
		ca = std::cos( yaw_rad );

		// rotate offscreen position around.
		out_offscreen_pos.x = ( int ) ( ( g_cl.m_width / 2.f ) + ( radius * sa ) );
		out_offscreen_pos.y = ( int ) ( ( g_cl.m_height / 2.f ) - ( radius * ca ) );
	};

	if( !settings.offscreen )
		return;

	if( !g_cl.m_processing || !g_cl.m_local->enemy( player ) )
		return;

	// get the player's center screen position.
	target_pos = player->WorldSpaceCenter( );
	is_on_screen = render::WorldToScreen( target_pos, screen_pos );

	// give some extra room for screen position to be off screen.
	leeway_x = g_cl.m_width / 18.f;
	leeway_y = g_cl.m_height / 18.f;

	// origin is not on the screen at all, get offscreen position data and start rendering.
	if( !is_on_screen
		|| screen_pos.x < -leeway_x
		|| screen_pos.x >( g_cl.m_width + leeway_x )
		|| screen_pos.y < -leeway_y
		|| screen_pos.y >( g_cl.m_height + leeway_y ) ) {

		// get viewport origin.
		view_origin = g_csgo.m_view_render->m_view.m_origin;

		// get direction to target.
		delta = ( target_pos - view_origin ).normalized( );

		// note - dex; this is the 'YRES' macro from the source sdk.
		radius = 200.f * ( g_cl.m_height / 480.f );

		// get the data we need for rendering.
		get_offscreen_data( delta, radius, offscreen_pos, offscreen_rotation );

		// bring rotation back into range... before rotating verts, sine and cosine needs this value inverted.
		// note - dex; reference: 
		// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/client/tf/tf_hud_damageindicator.cpp#L182
		offscreen_rotation = -offscreen_rotation;

		// setup vertices for the triangle.
		verts[ 0 ] = { offscreen_pos.x, offscreen_pos.y };        // 0,  0
		verts[ 1 ] = { offscreen_pos.x - 12.f, offscreen_pos.y + 24.f }; // -1, 1
		verts[ 2 ] = { offscreen_pos.x + 12.f, offscreen_pos.y + 24.f }; // 1,  1

		// setup verts for the triangle's outline.
		verts_outline[ 0 ] = { verts[ 0 ].m_pos.x - 1.f, verts[ 0 ].m_pos.y - 1.f };
		verts_outline[ 1 ] = { verts[ 1 ].m_pos.x - 1.f, verts[ 1 ].m_pos.y + 1.f };
		verts_outline[ 2 ] = { verts[ 2 ].m_pos.x + 1.f, verts[ 2 ].m_pos.y + 1.f };

		// rotate all vertices to point towards our target.
		verts[ 0 ] = render::RotateVertex( offscreen_pos, verts[ 0 ], offscreen_rotation );
		verts[ 1 ] = render::RotateVertex( offscreen_pos, verts[ 1 ], offscreen_rotation );
		verts[ 2 ] = render::RotateVertex( offscreen_pos, verts[ 2 ], offscreen_rotation );
		// verts_outline[ 0 ] = render::RotateVertex( offscreen_pos, verts_outline[ 0 ], offscreen_rotation );
		// verts_outline[ 1 ] = render::RotateVertex( offscreen_pos, verts_outline[ 1 ], offscreen_rotation );
		// verts_outline[ 2 ] = render::RotateVertex( offscreen_pos, verts_outline[ 2 ], offscreen_rotation );

		// todo - dex; finish this, i want it.
		// auto &damage_data = m_offscreen_damage[ player->index( ) ];
		// 
		// // the local player was damaged by another player recently.
		// if( damage_data.m_time > 0.f ) {
		//     // // only a small amount of time left, start fading into white again.
		//     // if( damage_data.m_time < 1.f ) {
		//     //     // calculate step needed to reach 255 in 1 second.
		//     //     // float step = UINT8_MAX / ( 1000.f * g_csgo.m_globals->m_frametime );
		//     //     float step = ( 1.f / g_csgo.m_globals->m_frametime ) / UINT8_MAX;
		//     //     
		//     //     // increment the new value for the color.
		//     //     // if( damage_data.m_color_step < 255.f )
		//     //         damage_data.m_color_step += step;
		//     // 
		//     //     math::clamp( damage_data.m_color_step, 0.f, 255.f );
		//     // 
		//     //     damage_data.m_color.g( ) = (uint8_t)damage_data.m_color_step;
		//     //     damage_data.m_color.b( ) = (uint8_t)damage_data.m_color_step;
		//     // }
		//     // 
		//     // g_cl.print( "%f %f %u %u %u\n", damage_data.m_time, damage_data.m_color_step, damage_data.m_color.r( ), damage_data.m_color.g( ), damage_data.m_color.b( ) );
		//     
		//     // decrement timer.
		//     damage_data.m_time -= g_csgo.m_globals->m_frametime;
		// }
		// 
		// else
		//     damage_data.m_color = colors::white;

		// render!
		color = g_menu.main.players.offscreen_color.get( ); // damage_data.m_color;
		color.a( ) = ( alpha == 255 ) ? alpha : alpha / 2;

		g_csgo.m_surface->DrawSetColor( color );
		g_csgo.m_surface->DrawTexturedPolygon( 3, verts );

		// g_csgo.m_surface->DrawSetColor( colors::black );
		// g_csgo.m_surface->DrawTexturedPolyLine( 3, verts_outline );
	}
}

std::string str_toupper( std::string s ) {
	std::transform( s.begin( ), s.end( ), s.begin( ),
		[ ]( unsigned char c ) { return std::toupper( c ); }
	);
	return s;
}

void Visuals::DrawPlayer( Player* player ) {
	if ( !player->enemy( g_cl.m_local ) )
		return;

	constexpr float MAX_DORMANT_TIME = 10.f;
	constexpr float DORMANT_FADE_TIME = MAX_DORMANT_TIME / 2.f;

	Rect		  box;
	player_info_t info;
	Color		  color;

	// get player index.
	int index = player->index( );

	// get reference to array variable.
	float& opacity = m_opacities[ index - 1 ];
	bool& draw = m_draw[ index - 1 ];

	// opacity should reach 1 in 300 milliseconds.
	constexpr int frequency = 1.f / 0.3f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy( g_cl.m_local );
	bool dormant = player->dormant( );

	if( settings.enemy_radar && enemy && !dormant )
		player->m_bSpotted( ) = true;

	// we can draw this player again.
	if( !dormant )
		draw = true;

	if( !draw )
		return;

	// if non-dormant	-> increment
	// if dormant		-> decrement
	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy && settings.dormant;

	// clamp the opacity.
	math::clamp( opacity, 0.f, 1.f );
	if( !opacity && !dormant_esp )
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_curtime - player->m_flSimulationTime( );
	if( dormant && dt > MAX_DORMANT_TIME )
		return;

	// calculate alpha channels.
	int alpha = ( int ) ( 255.f * opacity );
	int low_alpha = ( int ) ( 179.f * opacity );

	// get color based on enemy or not.
	color = g_menu.main.players.box_enemy.get( );

	auto box_enemy = settings.box_enemy;

	if( dormant && dormant_esp ) {
		alpha = 112;
		low_alpha = 80;

		// fade.
		if( dt > DORMANT_FADE_TIME ) {
			// for how long have we been fading?
			float faded = ( dt - DORMANT_FADE_TIME );
			float scale = 1.f - ( faded / DORMANT_FADE_TIME );

			alpha *= scale;
			low_alpha *= scale;
		}

		// override color.
		color = { 112, 112, 112 };
	}

	// override alpha.
	color.a( ) = alpha;

	// get player info.
	if( !g_csgo.m_engine->GetPlayerInfo( index, &info ) )
		return;

	// run offscreen ESP.
	OffScreen( player, alpha );

	// attempt to get player box.
	if( !GetPlayerBoxRect( player, box ) ) {
		// OffScreen( player );
		return;
	}

	// DebugAimbotPoints( player );

	bool bone_esp = settings.skeleton_esp;
	if( bone_esp )
		DrawSkeleton( player, alpha );

	// is box esp enabled for this player.
	bool box_esp = box_enemy;

	// render box if specified.
	if ( box_esp ) {
		if ( !settings.custom_box ) {
			render::rect_outlined( box.x, box.y, box.w, box.h, to_color( settings.box_color ).alpha( low_alpha ), { 10, 10, 10, low_alpha } );
		}
		else {
			const float calcHorizontal = 0.5f * ( (float)settings.box_horizontal * 0.01f );
			const float calcVertical = 0.5f * ( ( float )settings.box_vertical * 0.01f );
			const float offsetHorizontal = ( float )settings.box_horizontal >= 99.f ? 1 : 0;
			const float offsetVertical = ( float )settings.box_vertical >= 99.f ? 1 : 0;
			const Color outline_color = { 10, 10, 10, low_alpha };


			render::line( box.x, box.y, offsetHorizontal + box.x + box.w * calcHorizontal, box.y, color );
			render::line( box.x + box.w - 1, box.y , box.x + box.w * ( 1.f - calcHorizontal ) - 1, box.y , color );
			render::line( box.x, box.y + box.h - 1, offsetHorizontal + box.x + box.w * calcHorizontal, box.y + box.h - 1, color );
			render::line( box.x + box.w - 1, box.y + box.h - 1, box.x + box.w * ( 1.f - calcHorizontal ) - 1, box.y + box.h - 1, color );

			render::line( box.x - 1, box.y - 1, offsetHorizontal + box.x + box.w * calcHorizontal, box.y - 1, outline_color );
			render::line( box.x + box.w, box.y - 1, box.x + box.w * ( 1.f - calcHorizontal ) - 1, box.y - 1, outline_color );
			render::line( box.x - 1, box.y + box.h, offsetHorizontal + box.x + box.w * calcHorizontal, box.y + box.h, outline_color );
			render::line( box.x + box.w, box.y + box.h, box.x + box.w * ( 1.f - calcHorizontal ) - 1, box.y + box.h, outline_color );

			render::line( box.x, box.y, box.x, offsetVertical + box.y + box.h * calcVertical, color );
			render::line( box.x, box.y + box.h - 1, box.x, box.y + box.h * ( 1.f - calcVertical ), color );
			render::line( box.x + box.w - 1, box.y, box.x + box.w - 1, offsetVertical + box.y + box.h * calcVertical, color );
			render::line( box.x + box.w - 1, box.y + box.h - 1, box.x + box.w - 1, box.y + box.h * ( 1.f - calcVertical ), color );
		
			render::line( box.x - 1, box.y, box.x - 1, offsetVertical + box.y + box.h * calcVertical, outline_color );
			render::line( box.x - 1, box.y + box.h - 1, box.x - 1, box.y + box.h * ( 1.f - calcVertical ), outline_color );
			render::line( box.x + box.w, box.y, box.x + box.w, offsetVertical + box.y + box.h * calcVertical, outline_color );
			render::line( box.x + box.w, box.y + box.h - 1, box.x + box.w, box.y + box.h * ( 1.f - calcVertical ), outline_color );
		}
	}

	// is name esp enabled for this player.
	bool name_esp = settings.name_esp;

	// draw name.
	if( name_esp ) {
		// fix retards with their namechange meme 
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name{ std::string( info.m_name ).substr( 0, 24 ) };

		Color clr = g_menu.main.players.name_color.get( );
		// override alpha.
		clr.a( ) = low_alpha;

		render::esp.string( box.x + box.w / 2, box.y - render::esp.m_size.m_height, to_color( settings.name_color ).alpha( low_alpha ), name, render::ALIGN_CENTER );
	}

	// is health esp enabled for this player.
	bool health_esp = settings.health_esp;

	if( health_esp ) {
		/* we setup the positions
		we do full size healthbar to be same size as box and else to be original supremacy
		that was a nice ideea to add some customiaztion to healthbar */
		/* we setup the ints */
		int y;
		int h;

		/* now the design stuff*/
		if ( settings.fullzise_health ) {
			y = box.y;
			h = box.h;
		}
		else {
			y = box.y + 1;
			h = box.h - 2;
		}

		// retarded servers that go above 100 hp..
		int hp = std::min( 100, player->m_iHealth( ) );

		/* run the healthbar animation
		add custom animation time */
		/* we make a int so we have the main value and custom value stored
		we make a int for health animationtime value from menu cfg stuff so we dont have 12173128312831 words on a line */
		int health_animation_time_menu = settings.animation_health;
		int animation_time = settings.custom_animate ? health_animation_time_menu : 200.f;

		if ( settings.health_animation ) {
			static float player_hp[ 64 ];
			if ( player_hp[ player->index( ) ] > hp )
				player_hp[ player->index( ) ] -= animation_time * g_csgo.m_globals->m_frametime;
			else
				player_hp[ player->index( ) ] = hp;

			/* set the hp to the anim and sort the enemy */
			hp = player_hp[ player->index( ) ];
		}

		// calculate hp bar color.
		int r = std::min( ( 510 * ( 100 - hp ) ) / 100, 255 );
		int g = std::min( ( 510 * hp ) / 100, 255 );

		// get hp bar height.
		int fill = ( int ) std::round( hp * h / 100.f );

		Color health_color;
		if ( settings.custom_health ) {
			health_color = to_color( settings.health_color ).alpha( alpha );
		}
		else {
			health_color = { r, g, 0, alpha };
		}

		Color backround_color;
		Color outline_color;
		if ( settings.getze_backround_h ) {
			backround_color = Color( 80, 80, 80, low_alpha );
			outline_color = Color( 10, 10, 10, low_alpha );
		}
		else {
			backround_color = Color( 10, 10, 10, low_alpha );
			outline_color = Color( 5, 5, 5, low_alpha );
		}

		if ( settings.outline_healthbar && !settings.outtline_health_pri ) {
			// render background.
			render::rect_filled( box.x - 6, y - 1, 4, h + 2, backround_color );
			render::rect( box.x - 6, y - 1, 4, h + 2, outline_color );

			// render actual bar.
			render::rect( box.x - 5, y + h - fill, 2, fill, health_color );
		}
		else if ( settings.outline_healthbar && settings.outtline_health_pri ) {
			// render background.
			render::rect_filled( box.x - 6, y - 1, 4, h + 2, backround_color );

			// render actual bar.
			render::rect( box.x - 5, y + h - fill, 2, fill, health_color );
			render::rect( box.x - 6, y - 1, 4, h + 2, outline_color );
		}
		else if ( !settings.outline_healthbar && !settings.outtline_health_pri ){ // normal
			// render background.
			render::rect_filled( box.x - 6, y - 1, 4, h + 2, backround_color );

			// render actual bar.
			render::rect( box.x - 5, y + h - fill, 2, fill, health_color );
		}

		// if hp is below max, draw a string.
		if ( hp < 100 ) {
			if ( !settings.getze_hp_value )
				render::esp_small.string( box.x - 5, y + ( h - fill ) - 5, { 255, 255, 255, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
			else {
				std::string value;
				value += std::to_string( hp ) + "HP";

				// get size.
				auto hp_value = render::esp_small.size( value ).m_width;
				render::esp_small.string( box.x - hp_value - 8, y + 1, { 255, 255, 255, low_alpha }, value, render::ALIGN_LEFT );
			}

		}
	}

	// draw flags.
	{
		std::vector< std::pair< std::string, Color > > flags;

		if ( settings.main_flags ) {
			if ( settings.money_flag ) {
				flags.push_back( { tfm::format( XOR( "$%i" ), player->m_iAccount( ) ), { 150, 200, 60, low_alpha } } );
			}
			
			if ( settings.armor_flag ) {
				// helmet and kevlar.
				if ( player->m_bHasHelmet( ) && player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "HK" ), { 255, 255, 255, low_alpha } } );

				// only helmet.
				else if ( player->m_bHasHelmet( ) )
					flags.push_back( { XOR( "H" ), { 255, 255, 255, low_alpha } } );

				// only kevlar.
				else if ( player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "K" ), { 255, 255, 255, low_alpha } } );
			}

			if ( settings.scope_flag ) {
				std::string getzezus;
				if ( settings.getzeus_scoped )
					getzezus = "SCOPED";
				else
					getzezus = "ZOOM";

				// scoped.
				if ( player->m_bIsScoped( ) )
					flags.push_back( { getzezus, { 60, 180, 225, low_alpha } } );
			}

			if ( settings.reload_flag ) {
				// get ptr to layer 1.
				C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

				// check if reload animation is going on.
				if ( layer1->m_weight != 0.f && player->GetSequenceActivity( layer1->m_sequence ) == 967 /* ACT_CSGO_RELOAD */ )
					flags.push_back( { XOR( "RELOAD" ), { 60, 180, 225, low_alpha } } );
			}

			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
			if ( settings.resolver_flag ) {
				if ( data && data->m_records.size( ) ) {
					LagRecord* current = data->m_records.front( ).get( );
					if ( current ) {
						if ( settings.resolver_flag && !current->resolver_text[ current->m_player->index( ) ].empty( ) ) { // double pointing
							flags.push_back( { current->resolver_text[ current->m_player->index( ) ], {252, 165, 3, low_alpha} } );
						}
					}
				}
			}

			if ( settings.distance_flag ) {
				// pasted from immortal
				auto local = g_cl.m_local;
				if ( local && local->alive( ) ) {
					auto round_to_multiple = [ & ]( int in, int multiple ) {
						const auto ratio = static_cast< double >( in ) / multiple;
						const auto iratio = std::lround( ratio );
						return static_cast< int >( iratio * multiple );
					};

					float distance = local->m_vecOrigin( ).dist_to( player->m_vecOrigin( ) );

					auto meters = distance * 0.0254f;
					auto feet = meters * 3.281f;

					std::string str = std::to_string( round_to_multiple( feet, 5 ) ) + XorStr( " FT" );
					flags.push_back( { str, {255, 255, 255, low_alpha} } );
				}
			}

			/*
						if ( settings.shift_flag  ) {
							LagRecord* current = data->m_records.front( ).get( );
							if ( current->is_shifting ) {
								flags.push_back( { "SHIFT", { 255, 255, 255, low_alpha } } );
							}
						}
			*/

			if ( settings.vulerable && ( player->m_iHealth( ) < 50 ) ) {
				flags.push_back( { "VULNERABLE", { 255, 255, 255, low_alpha } } );
			}

			if ( settings.location_flag ) {
				auto info_string = str_toupper( player->m_szLastPlaceName( ) );
				flags.push_back( { info_string, {255, 255, 255, low_alpha} } );
			}

			if ( settings.hitbox_flag ) {
				if ( data && data->m_records.size( ) ) {
					LagRecord* current = data->m_records.front( ).get( );
					if ( current ) {
						flags.push_back( { current->hitbox_prioritize_type[ player->index() ], { 252, 165, 3, low_alpha } } );
					}
				}
			}
			
		}
		for( size_t i{ }; i < flags.size( ); ++i ) {
			// get flag job (pair).
			const auto& f = flags[ i ];

			int offset = i * ( render::esp_small.m_size.m_height - 1 );

			// draw flag.
			render::esp_small.string( box.x + box.w + 2, box.y + offset, f.second, f.first );
		}
	}

	// draw bottom bars.
	{
		int  offset{ 0 };

		// draw lby update bar.
		if( enemy && settings.lby_update ) {
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

			// make sure everything is valid.
			if( data && data->m_moved &&data->m_records.size( ) ) {
				// grab lag record.
				LagRecord* current = data->m_records.front( ).get( );

				if( current ) {
					if( !( current->m_velocity.length_2d( ) > 0.1 && !current->m_fake_walk ) && data->m_body_index <= 3 ) {
						// calculate box width
						float cycle = std::clamp<float>( data->m_body_update - current->m_anim_time, 0.f, 1.0f );
						float width = ( box.w * cycle ) / 1.1f;

						if( width > 0.f ) {
							// draw.
							render::rect_filled( box.x, box.y + box.h + 2, box.w, 4, { 10, 10, 10, low_alpha } );

							Color clr = g_menu.main.players.lby_update_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3, width, 2, to_color( settings.lbu_update_color ).alpha(low_alpha) );

							// move down the offset to make room for the next bar.
							offset += 5;
						}
					}
				}
			}
		}

		// draw weapon.
		if( settings.weapon_esp	) {
			Weapon* weapon = player->GetActiveWeapon( );
			if( weapon ) {
				WeaponInfo* data = weapon->GetWpnData( );
				if( data ) {
					int bar;
					float scale;

					// the maxclip1 in the weaponinfo
					int max = data->m_max_clip1;
					int current = weapon->m_iClip1( );

					C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

					// set reload state.
					bool reload = ( layer1->m_weight != 0.f ) && ( player->GetSequenceActivity( layer1->m_sequence ) == 967 );

					// ammo bar.
					if( max != -1 && settings.ammo_esp ) {
						// check for reload.
						if( reload )
							scale = layer1->m_cycle;
						else
							scale = ( float ) current / max;

						Color backround_color;
						Color outline_color;
						if ( settings.getze_backround_a ) {
							backround_color = Color( 80, 80, 80, low_alpha );
							outline_color = Color( 10, 10, 10, low_alpha );
						}
						else {
							backround_color = Color( 10, 10, 10, low_alpha );
							outline_color = Color( 5, 5, 5, low_alpha );
						}

						// relative to bar.
						bar = ( int )std::round( ( box.w - 2 ) * scale );

						if ( settings.outline_healthbar_a && !settings.outtline_health_pri_a ) {
							render::rect_filled( box.x, box.y + box.h + 2 + offset, box.w, 4, backround_color );
							render::rect( box.x, box.y + box.h + 2 + offset, box.w, 4, outline_color );
							Color clr = g_menu.main.players.ammo_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3 + offset, bar, 2, to_color( settings.ammo_bar_color ).alpha( low_alpha ) );					
						}
						else if ( settings.outline_healthbar_a && settings.outtline_health_pri_a ) {
							render::rect_filled( box.x, box.y + box.h + 2 + offset, box.w, 4, backround_color );

							Color clr = g_menu.main.players.ammo_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3 + offset, bar, 2, to_color( settings.ammo_bar_color ).alpha( low_alpha ) );
							render::rect( box.x, box.y + box.h + 2 + offset, box.w, 4, outline_color );
						}
						else if ( !settings.outline_healthbar_a && !settings.outtline_health_pri_a ) { // normal
							render::rect_filled( box.x, box.y + box.h + 2 + offset, box.w, 4, backround_color );

							Color clr = g_menu.main.players.ammo_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3 + offset, bar, 2, to_color( settings.ammo_bar_color ).alpha( low_alpha ) );
						}

						if ( !settings.ammo_show_value_fast ) {
							if ( current <= ( int )std::round( max / 5 ) && !reload )
								render::esp_small.string( box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );
						}
						else {
							if ( current <= ( int )std::round( max - 1 ) && !reload )
								render::esp_small.string( box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );
						}
						offset += 6;
					}

					Color weapon_color = to_color( settings.weapon_color ).alpha( low_alpha );
					Color weapon_color_icon = to_color( settings.weapon_color_aa ).alpha( low_alpha );

					// text.
					if( settings.weapon_esp ) {
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName( ) };

						// smallfonts needs upper case.
						std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

						render::esp_small.string( box.x + box.w / 2, box.y + box.h + offset, weapon_color, name, render::ALIGN_CENTER );
					}

					// icons. -> fuck it
					if( settings.esp_icon_weapon ) {
						// icons are super fat..
						// move them back up. -> this might work
						if ( !settings.weapon_esp )
							offset -= 5;
						else if ( settings.weapon_esp )
							offset += 5;

						std::string icon = tfm::format( XOR( "%c" ), m_weapon_icons[ weapon->m_iItemDefinitionIndex( ) ] );
						render::cs.string( box.x + box.w / 2, box.y + box.h + offset, weapon_color_icon, icon, render::ALIGN_CENTER );
					}
				}
			}
		}
	}
}

void Visuals::DrawPlantedC4( ) {
	bool        mode_2d, mode_3d, is_visible;
	float       explode_time_diff, dist, range_damage;
	vec3_t      dst, to_target;
	int         final_damage;
	std::string time_str, damage_str;
	Color       damage_color;
	vec2_t      screen_pos;

	static auto scale_damage = [ ] ( float damage, int armor_value ) {
		float new_damage, armor;

		if( armor_value > 0 ) {
			new_damage = damage * 0.5f;
			armor = ( damage - new_damage ) * 0.5f;

			if( armor > ( float ) armor_value ) {
				armor = ( float ) armor_value * 2.f;
				new_damage = damage - armor;
			}

			damage = new_damage;
		}

		return std::max( 0, ( int ) std::floor( damage ) );
	};

	auto plant_c4 = settings.planted_c4;

	// store menu vars for later.
	mode_2d = plant_c4;
	mode_3d = plant_c4;
	if( !mode_2d && !mode_3d )
		return;

	// bomb not currently active, do nothing.
	if( !m_c4_planted )
		return;

	// calculate bomb damage.
	// references:
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L271
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L437
	//     https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/sdk/sdk_gamerules.cpp#L173
	{
		// get our distance to the bomb.
		// todo - dex; is dst right? might need to reverse CBasePlayer::BodyTarget...
		dst = g_cl.m_local->WorldSpaceCenter( );
		to_target = m_planted_c4_explosion_origin - dst;
		dist = to_target.length( );

		// calculate the bomb damage based on our distance to the C4's explosion.
		range_damage = m_planted_c4_damage * std::exp( ( dist * dist ) / ( ( m_planted_c4_radius_scaled * -2.f ) * m_planted_c4_radius_scaled ) );

		// now finally, scale the damage based on our armor (if we have any).
		final_damage = scale_damage( range_damage, g_cl.m_local->m_ArmorValue( ) );
	}

	// m_flC4Blow is set to gpGlobals->curtime + m_flTimerLength inside CPlantedC4.
	explode_time_diff = m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;

	// get formatted strings for bomb.
	time_str = tfm::format( XOR( "%.2f" ), explode_time_diff );
	damage_str = tfm::format( XOR( "%i" ), final_damage );

	// get damage color.
	damage_color = ( final_damage < g_cl.m_local->m_iHealth( ) ) ? colors::white : colors::red;

	// finally do all of our rendering.
	is_visible = render::WorldToScreen( m_planted_c4_explosion_origin, screen_pos );

	// 'on screen (2D)'.
	if( mode_2d ) {
		// g_cl.m_height - 80 - ( 30 * i )
		// 80 - ( 30 * 2 ) = 20

		// render::menu_shade.string( 60, g_cl.m_height - 20 - ( render::hud_size.m_height / 2 ), 0xff0000ff, "", render::hud );

		// todo - dex; move this next to indicators?

		if( explode_time_diff > 0.f )
			render::esp.string( 2, 65, colors::white, time_str, render::ALIGN_LEFT );

		if( g_cl.m_local->alive( ) )
			render::esp.string( 2, 65 + render::esp.m_size.m_height, damage_color, damage_str, render::ALIGN_LEFT );
	}

	// 'on bomb (3D)'.
	if( mode_3d && is_visible ) {
		if( explode_time_diff > 0.f )
			render::esp_small.string( screen_pos.x, screen_pos.y, colors::white, time_str, render::ALIGN_CENTER );

		// only render damage string if we're alive.
		if( g_cl.m_local->alive( ) )
			render::esp_small.string( screen_pos.x, ( int ) screen_pos.y + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER );
	}
}

bool Visuals::GetPlayerBoxRect( Player* player, Rect& box ) {
	vec3_t origin, mins, maxs;
	vec2_t bottom, top;

	// get interpolated origin.
	origin = player->GetAbsOrigin( );

	// get hitbox bounds.
	player->ComputeHitboxSurroundingBox( &mins, &maxs );

	// correct x and y coordinates.
	mins = { origin.x, origin.y, mins.z };
	maxs = { origin.x, origin.y, maxs.z + 8.f };

	if( !render::WorldToScreen( mins, bottom ) || !render::WorldToScreen( maxs, top ) )
		return false;

	box.h = bottom.y - top.y;
	box.w = box.h / 2.f;
	box.x = bottom.x - ( box.w / 2.f );
	box.y = bottom.y - box.h;

	return true;
}

void Visuals::DrawHistorySkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	AimPlayer* data;
	LagRecord* record;
	int           parent;
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	if( !g_menu.main.misc.fake_latency.get( ) ) // fuck it rn
		return;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	data = &g_aimbot.m_players[ player->index( ) - 1 ];
	if( !data )
		return;

	record = resolve_record::resolve_data::find_last_record( data );//g_resolver.FindLastRecord( data );
	if( !record )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		record->m_bones->get_bone( bone_pos, i );
		record->m_bones->get_bone( parent_pos, parent );

		Color clr = player->enemy( g_cl.m_local ) ? g_menu.main.players.skeleton_enemy.get( ) : g_menu.main.players.skeleton_friendly.get( );
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::DrawSkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	BoneArray     matrix[ 128 ];
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	// get bone matrix.
	if( !player->SetupBones( matrix, 128, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime ) )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		matrix->get_bone( bone_pos, i );
		matrix->get_bone( parent_pos, parent );

		Color clr = to_color(settings.skeleton_color);
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::RenderGlow( ) {
	Color   color;
	Player* player;

	if( !g_cl.m_local )
		return;

	if( !g_csgo.m_glow->m_object_definitions.Count( ) )
		return;

	float blend = 125;

	for( int i{ }; i < g_csgo.m_glow->m_object_definitions.Count( ); ++i ) {
		GlowObjectDefinition_t* obj = &g_csgo.m_glow->m_object_definitions[ i ];

		// skip non-players.
		if( !obj->m_entity || !obj->m_entity->IsPlayer( ) )
			continue;

		// get player ptr.
		player = obj->m_entity->as< Player* >( );

		if( player->m_bIsLocalPlayer( ) )
			continue;

		// get reference to array variable.
		float& opacity = m_opacities[ player->index( ) - 1 ];

		bool enemy = player->enemy( g_cl.m_local );

		if( enemy && !settings.enemy_glow )
			continue;

		// enemy color
		if( enemy )
			color = g_menu.main.players.glow_enemy.get( ); // fuck it

		obj->m_render_occluded = true;
		obj->m_render_unoccluded = false;
		obj->m_render_full_bloom = false;
		obj->m_color = { ( float ) color.r( ) / 255.f, ( float ) color.g( ) / 255.f, ( float ) color.b( ) / 255.f };
		obj->m_alpha = opacity * blend;
	}
}

void Visuals::DrawHitboxMatrix( LagRecord* record, Color col, float time ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiohitboxset_t* set;
	mstudiobbox_t* bbox;
	vec3_t             mins, maxs, origin;
	ang_t			   angle;

	model = record->m_player->GetModel( );
	if( !model )
		return;

	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	set = hdr->GetHitboxSet( record->m_player->m_nHitboxSet( ) );
	if( !set )
		return;

	for( int i{ }; i < set->m_hitboxes; ++i ) {
		bbox = set->GetHitbox( i );
		if( !bbox )
			continue;

		// bbox.
		if( bbox->m_radius <= 0.f ) {
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix( bbox->m_angle, rot_matrix );

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], rot_matrix, matrix );

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles( matrix, bbox_angle );

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin( );

			// draw box.
			g_csgo.m_debug_overlay->AddBoxOverlay( origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r( ), col.g( ), col.b( ), 0, time );
		}

		// capsule.
		else {
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix( bbox->m_angle, matrix );

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], matrix, matrix );

			// get world positions from new matrix.
			math::VectorTransform( bbox->m_mins, matrix, mins );
			math::VectorTransform( bbox->m_maxs, matrix, maxs );

			g_csgo.m_debug_overlay->AddCapsuleOverlay( mins, maxs, bbox->m_radius, col.r( ), col.g( ), col.b( ), col.a( ), time, 0, 0 );
		}
	}
}

void Visuals::DrawBeams( ) {
	size_t     impact_count;
	float      curtime, dist;
	bool       is_final_impact;
	vec3_t     va_fwd, start, dir, end;
	BeamInfo_t beam_info;
	Beam_t* beam;

	if( !g_cl.m_local )
		return;

	if( !settings.bullet_tracers )
		return;

	auto vis_impacts = &g_shots.m_vis_impacts;

	// the local player is dead, clear impacts.
	if( !g_cl.m_processing ) {
		if( !vis_impacts->empty( ) )
			vis_impacts->clear( );
	}

	else {
		impact_count = vis_impacts->size( );
		if( !impact_count )
			return;

		curtime = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );

		for( size_t i{ impact_count }; i-- > 0; ) {
			auto impact = &vis_impacts->operator[ ]( i );
			if( !impact )
				continue;

			// impact is too old, erase it.
			if( std::abs( curtime - game::TICKS_TO_TIME( impact->m_tickbase ) ) > 1 /* 1 second */ ) {
				vis_impacts->erase( vis_impacts->begin( ) + i );

				continue;
			}

			// already rendering this impact, skip over it.
			if( impact->m_ignore )
				continue;

			// is this the final impact?
			// last impact in the vector, it's the final impact.
			if( i == ( impact_count - 1 ) )
				is_final_impact = true;

			// the current impact's tickbase is different than the next, it's the final impact.
			else if( ( i + 1 ) < impact_count && impact->m_tickbase != vis_impacts->operator[ ]( i + 1 ).m_tickbase )
				is_final_impact = true;

			else
				is_final_impact = false;

			// is this the final impact?
			// is_final_impact = ( ( i == ( impact_count - 1 ) ) || ( impact->m_tickbase != vis_impacts->at( i + 1 ).m_tickbase ) );

			if( is_final_impact ) {
				// calculate start and end position for beam.
				start = impact->m_shoot_pos;

				dir = ( impact->m_impact_pos - start ).normalized( );
				dist = ( impact->m_impact_pos - start ).length( );

				end = start + ( dir * dist );

				static const char* traceroul;
				switch ( settings.bulettraceroul ) {
					case 0: {
						traceroul = "sprites/purplelaser1.vmt";
					} break;
					case 1: {
						traceroul = "sprites/physbeam.vmt";
					} break;
				}

				// setup beam info.
				// note - dex; possible beam models: sprites/physbeam.vmt | sprites/white.vmt
				beam_info.m_vecStart = start;
				beam_info.m_vecEnd = end;
				beam_info.m_nModelIndex = g_csgo.m_model_info->GetModelIndex( traceroul );
				beam_info.m_pszModelName = traceroul;
				beam_info.m_flHaloScale = 0.f;
				beam_info.m_flLife = 1;
				beam_info.m_flWidth = 2.f;
				beam_info.m_flEndWidth = 2.f;
				beam_info.m_flFadeLength = 0.f;
				beam_info.m_flAmplitude = 0.f;   // beam 'jitter'.
				beam_info.m_flBrightness = 255.f;
				beam_info.m_flSpeed = 0.5f;  // seems to control how fast the 'scrolling' of beam is... once fully spawned.
				beam_info.m_nStartFrame = 0;
				beam_info.m_flFrameRate = 0.f;
				beam_info.m_nSegments = 2;     // controls how much of the beam is 'split up', usually makes m_flAmplitude and m_flSpeed much more noticeable.
				beam_info.m_bRenderable = true;  // must be true or you won't see the beam.
				beam_info.m_nFlags = 0;

				// color plm
				if( !impact->m_hit_player ) {
					beam_info.m_flRed = g_menu.main.visuals.impact_beams_color.get( ).r( );
					beam_info.m_flGreen = g_menu.main.visuals.impact_beams_color.get( ).g( );
					beam_info.m_flBlue = g_menu.main.visuals.impact_beams_color.get( ).b( );
				}

				else {
					beam_info.m_flRed = g_menu.main.visuals.impact_beams_hurt_color.get( ).r( );
					beam_info.m_flGreen = g_menu.main.visuals.impact_beams_hurt_color.get( ).g( );
					beam_info.m_flBlue = g_menu.main.visuals.impact_beams_hurt_color.get( ).b( );
				}

				// attempt to render the beam.
				beam = game::CreateGenericBeam( beam_info );
				if( beam ) {
					g_csgo.m_beams->DrawBeam( beam );

					// we only want to render a beam for this impact once.
					impact->m_ignore = true;
				}
			}
		}
	}
}

void Visuals::DebugAimbotPoints( Player* player ) {
	std::vector< vec3_t > p2{ };

	AimPlayer* data = &g_aimbot.m_players.at( player->index( ) - 1 );
	if( !data || data->m_records.empty( ) )
		return;

	LagRecord* front = data->m_records.front( ).get( );
	if( !front || front->dormant( ) )
		return;

	// get bone matrix.
	BoneArray matrix[ 128 ];
	if( !g_bones.setup( player, matrix, front ) )
		return;

	data->SetupHitboxes( front, false );
	if( data->m_hitboxes.empty( ) )
		return;

	for( const auto& it : data->m_hitboxes ) {
		std::vector< vec3_t > p1{ };

		if( !data->SetupHitboxPoints( front, matrix, it.m_index, p1 ) )
			continue;

		for( auto& p : p1 )
			p2.push_back( p );
	}

	if( p2.empty( ) )
		return;

	for( auto& p : p2 ) {
		vec2_t screen;

		if( render::WorldToScreen( p, screen ) )
			render::rect_filled( screen.x, screen.y, 2, 2, { 0, 255, 255, 255 } );
	}
}