#include "GetzeVisuals.hpp"
#include "../backend/config/config_new.h"
#include "../backend/Menu/Menu.h"

int indicators = 0;

// pasted from getze
#define c_green Color(173, 255, 47)
#define c_yellow Color(219, 219, 50)
#define c_red Color(191, 0, 10)
#define c_white Color(255, 250, 250)
#define global_color Color(18, 18, 18, 200)

void gain_mouse_pos( vec2_t& last, vec2_t& new_ ) {
	POINT p_mouse_pos;
	GetCursorPos( &p_mouse_pos );
	ScreenToClient( FindWindow( 0, XOR( "Counter-Strike: Global Offensive" ) ), &p_mouse_pos );
	last = new_;
	new_ = vec2_t( static_cast< int >( p_mouse_pos.x ), static_cast< int >( p_mouse_pos.y ) );
}

Color gain_color( int col[ 4 ] ) {
	return Color( col[ 0 ], col[ 1 ], col[ 2 ], 255 );
}

float lerp( float t, float a, float b ) {
	return ( 1 - t ) * a + t * b;
}

const float ind_speed = 0.1f;

namespace Getze {
	namespace VisualsControll {
		void fakelag_indicator( ) {
			if ( !settings.fl_indic )
				return;

			if ( !settings.lag_enable )
				return;

			// we re going to do dymanic position
			vec2_t position = vec2_t( 100, 600 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool is_dragging;
			auto x = position.x;
			auto y = position.y;

			gain_mouse_pos( main_mouse, last_mouse );

			// dragging logic.
			if ( main_mouse.x > position.x - s_drag.x && main_mouse.y > position.y - s_drag.y && main_mouse.x < ( position.x - s_drag.x ) + 200 && main_mouse.y < ( position.y - s_drag.y ) + 20 && GetAsyncKeyState( VK_LBUTTON ) ) {
				s_drag.x += main_mouse.x - last_mouse.x;
				s_drag.y += main_mouse.y - last_mouse.y;
				is_dragging = true;
			}

			x -= s_drag.x;
			y -= s_drag.y;

			auto chocked_command = g_csgo.m_cl->m_choked_commands;
			auto fl_value = settings.lag_limit;

			auto red = std::max( std::min( 255 - int( chocked_command * fl_value ), 255 ), 0 );
			auto green = std::max( std::min( int( chocked_command * fl_value ), 255 ), 0 );

			auto ind_h = g_cl.m_width / 2 - 380;

			std::string choke = "choke";
			std::transform( choke.begin( ), choke.end( ), choke.begin( ), ::toupper );

			Drawing::DrawRect( x, y, 100, 15, { 0, 0, 0, 130 } );
			Drawing::DrawString( render::esp_small, x + 50, y + 2, { 255, 255, 255, 240 }, render::ALIGN_CENTER, choke );

			if ( chocked_command > 0 ) {
				Drawing::DrawRect( x, y + 15, (98.f * ((float)chocked_command / fl_value)), 4, { red, green, 0, 130 });
				Drawing::DrawOutlinedRect( x, y + 15, ( 98.f * ( ( float )chocked_command / fl_value ) ), 4, { red, green, 0, 130 } );
			}
		}
		void bomb_indicator( Player* entity ) {
			auto local = g_cl.m_local;
			if ( !local || !local->alive( ) )
				return;

			//if ( settings.bomb_type_ind != 1 )
				//return;

			// we re going to do dymanic position
			vec2_t position = vec2_t( 200, 600 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool is_dragging;
			auto x = position.x;
			auto y = position.y;

			if ( g_visuals.m_c4_planted ) {
				auto bomb_blow_timer = entity->get_bomb_blow_timer( );;
				auto bomb_defuse_timer = entity->get_bomb_defuse_timer( );
				auto can_defuse = ( bomb_defuse_timer <= bomb_blow_timer && local->m_iTeamNum( ) == 3 ) || ( bomb_defuse_timer > bomb_blow_timer && local->m_iTeamNum( ) == 2 || bomb_defuse_timer <= bomb_blow_timer );

				if ( local->m_iHealth( ) > 0 ) {
					float flArmor = local->m_ArmorValue( );
					float flDistance = local->m_vecOrigin( ).dist_to( entity->m_vecOrigin( ) );

					// l3d math guy
					float a = 450.7f;
					float b = 75.68f;
					float c = 789.2f;
					float d = ( ( flDistance - b ) / c );

					float flDmg = a * exp( -d * d );
					float flDamage = flDmg;

					if ( flArmor > 0 ) {
						float flNew = flDmg * 0.5f;
						float flArmor = ( flDmg - flNew ) * 0.5f;

						if ( flArmor > static_cast< float >( flArmor ) )
						{
							flArmor = static_cast< float >( flArmor ) * ( 1.f / 0.5f );
							flNew = flDmg - flArmor;
						}

						flDamage = flNew;
					}

					float dmg = 100 - ( local->m_iHealth( ) - flDamage );

					if ( dmg > 100 )
						dmg = 100;
					if ( dmg < 0 )
						dmg = 0;

					int Red = dmg;
					int Green = 255 - dmg * 2.55;

					std::string format_dmg;
					std::string format_dmg1;

					format_dmg = tfm::format( XOR( "FATAL -%.1fHP" ), flDamage );
					format_dmg1 = tfm::format( XOR( "-%.1fHP" ), flDamage );

					if ( flDamage > 1 ) {
						if ( flDamage >= local->m_iHealth( ) )
							Drawing::DrawString( render::indicator, 10, 150, Color( Red, Green, 0, 255 ), render::ALIGN_LEFT, format_dmg );
						else
							Drawing::DrawString( render::indicator, 10, 150, Color( Red, Green, 0, 255 ), render::ALIGN_LEFT, format_dmg1 );
					}
				}

				if ( bomb_blow_timer > 0.f ) {
					static auto max_bombtime = g_csgo.m_cvar->FindVar( HASH( "mp_c4timer" ) );
					auto max_bombtimer = max_bombtime->GetFloat( );
					auto btime = bomb_blow_timer * ( 100.f / max_bombtimer );

					if ( btime > 100.f )
						btime = 100.f;
					else if ( btime < 0.f )
						btime = 0.f;

					int blow_percent = x - ( int )( ( 98 * btime ) / 100 );
					int b_red = 255 - ( btime * 2.55 );
					int b_green = btime * 2.55;

					Drawing::DrawRect( x, y, 100, 15, { 0, 0, 0, 130 } );

					std::string choke = "bomb";
					std::transform( choke.begin( ), choke.end( ), choke.begin( ), ::toupper );

					Drawing::DrawString( render::esp_small, x + 50, y + 2, can_defuse ? Color( 0, 255, 0, 240) : Color( 255, 0, 0, 240 ), render::ALIGN_CENTER, choke );

					Drawing::DrawRect( x, y + 15, ( 98.f * float( bomb_blow_timer / max_bombtimer ) ), 4, { b_red,b_green ,0,130 } );
					Drawing::DrawOutlinedRect( x, y + 15, ( 98.f * float( bomb_blow_timer / max_bombtimer ) ), 4, { b_red,b_green ,0,130 } );

					// this is not finished and im bored of it

					/*
					 if ( bomb_defuse_timer > 0.f ) {
						auto max_defuse_timer = g_visuals.has_defuser ? 5.f : 10.f;
						auto dtime = bomb_defuse_timer * ( 100.f / max_defuse_timer );

						if ( dtime > 100.f )
							dtime = 100.f;
						else if ( dtime < 0.f )
							dtime = 0.f;

						int defuse_percent = y - ( int )( ( 98 * dtime ) / 100 );
						int d_red = 255 - ( dtime * 2.55 );
						int d_green = dtime * 2.55;

						Drawing::DrawRect( x, y, 100, 15, { 0, 0, 0, 130 } );

						std::string choke2 = "defuse";
						std::transform( choke2.begin( ), choke2.end( ), choke2.begin( ), ::toupper );

						Drawing::DrawString( render::esp_small, x + 50, y + 2, can_defuse ? Color( 0, 255, 0, 240 ) : Color( 255, 0, 0, 240 ), render::ALIGN_CENTER, choke2 );

						Drawing::DrawRect( x, y + 15, ( 98.f * float( bomb_defuse_timer / max_defuse_timer ) ), 4, { b_red, b_green, 0,130 } );
						Drawing::DrawOutlinedRect( x, y + 15, ( 98.f * float( bomb_defuse_timer / max_defuse_timer ) ), 4, { b_red, b_green, 0,130 } );
					}
					*/
				}
			}
		}

		void indicator( ) {
			if ( settings.indicator_style != 0 || ( !settings.keybindddds && !settings.indicatttors ) )
				return;

			vec2_t position = vec2_t( 300, 500 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool dragging = false;

			auto x = position.x;
			auto y = position.y;

			gain_mouse_pos( main_mouse, last_mouse );

			if ( main_mouse.x > position.x - s_drag.x && main_mouse.y > position.y - s_drag.y && main_mouse.x < ( position.x - s_drag.x ) + 200 && main_mouse.y < ( position.y - s_drag.y ) + 20 && GetAsyncKeyState( VK_LBUTTON ) ) {
				s_drag.x += main_mouse.x - last_mouse.x;
				s_drag.y += main_mouse.y - last_mouse.y;
				dragging = true;
			}

			position.x -= s_drag.x;
			position.y -= s_drag.y;

			std::string spec_text = "indicators / keybinds"; // those kids overthinked this, my brain lose braincells fast when i look at that code

			if ( settings.keybindddds && !settings.indicatttors )
				spec_text = "keybinds";
			if ( !settings.keybindddds && settings.indicatttors )
				spec_text = "indicators";

			if ( CMenu::get( ).IsMenuOpened( ) )
				Drawing::DrawString( render::esp , position.x, position.y + 6, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );


			auto local = g_cl.m_local;
			if ( !local )
				return;

			if ( !local->alive() )
				return;

			if ( CMenu::get( ).IsMenuOpened( ) )
				return;

			if ( settings.indicatttors ) {
				if ( settings.enable ) {
					static float value = 0.f;
					auto chocked_command = g_csgo.m_cl->m_choked_commands;
					auto fl_value = settings.lag_limit;

					static int choked = 0;
					if ( chocked_command )
						choked++;
					else
						choked = 0;

					choked = std::clamp( choked, 0, fl_value );
					value = lerp( ind_speed, value, float( choked / fl_value ) );


					float change = std::abs( math::NormalizedAngle( g_cl.m_body - g_cl.m_angle.y ) );
					float lc = g_cl.m_local->m_vecVelocity( ).length_2d( ) > 270.f || g_cl.m_lagcomp;


					SurfaceEngine::draw_progresstext( position, "LAG", value );
					SurfaceEngine::draw_progresstext( position, "LBY", change );
					SurfaceEngine::draw_progresstext( position, "LC", lc );
					SurfaceEngine::draw_progresstext( position, "DT", TickbaseControll::m_double_tap );
					SurfaceEngine::draw_progresstext( position, "DMG", g_aimbot.m_damage_override );
					SurfaceEngine::draw_progresstext( position, "BAIM", g_aimbot.m_force_baim );
				}
			}
		}
		void indicator_ui( ) {
			if ( settings.indicatttors || settings.indicator_style == 0 )
				return;

			vec2_t position = vec2_t( 0, 500 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool dragging = false;
			
			auto c = gain_color( settings.menu_color );

			auto x = position.x;
			auto y = position.y;

			gain_mouse_pos( main_mouse, last_mouse );

			if ( main_mouse.x > position.x - s_drag.x && main_mouse.y > position.y - s_drag.y && main_mouse.x < ( position.x - s_drag.x ) + 200 && main_mouse.y < ( position.y - s_drag.y ) + 20 && GetAsyncKeyState( VK_LBUTTON ) ) {
				s_drag.x += main_mouse.x - last_mouse.x;
				s_drag.y += main_mouse.y - last_mouse.y;
				dragging = true;
			}

			int amount = 0;
			if ( g_cl.m_local && g_cl.m_local->alive( ) ) {
				if ( settings.enable )
					amount++;
				amount++;
			}

			position.x -= s_drag.x;
			position.y -= s_drag.y;

			float dheight = ( 6 + amount ) * 20 + 5;
			std::string spec_text = "indicators";

			Drawing::DrawRect( position.x + 5, position.y + 5, 205 - 10, dheight - 10 - 5, global_color );
			Drawing::DrawString( render::esp , position.x + 100, position.y + 6, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );
			Drawing::DrawRect( position.x + 8, position.y + 23, 205 - 16, 3, gain_color( settings.menu_color ) );

			auto local = g_cl.m_local;
			if ( !local )
				return;

			if ( !local->alive( ) )
				return;

			if ( CMenu::get( ).IsMenuOpened( ) )
				return;


			position.y += 30;

			{ // local velocity
				static float value = 0.f;
				static auto maxvel = g_csgo.m_cvar->FindVar( HASH( "sv_maxvelocity" ) );

				if ( maxvel ) {

					float newvalue = std::clamp( g_cl.m_local->m_vecVelocity( ).length_2d( ), 0.f, 300.f ) / 300.f; /* FOR NOW cause more retarded servers do max vel at 3500*/
					value = lerp( ind_speed, value, newvalue );
					SurfaceEngine::draw_progressbar( position, "velocity", c, value );
				}
			}

			{ // doubletap
				static float value = 0.f;
				float new_value = std::clamp( ( float )TickbaseControll::m_charged_ticks, 0.f, ( float )TickbaseControll::m_charged_ticks / 16.f );
				value = lerp( ind_speed, value, new_value );

				SurfaceEngine::draw_progressbar( position, "doubletap", c, value );
			}

			{ // lby
				static float value = 0.f;
				float change = std::abs( math::NormalizedAngle( g_cl.m_body - g_cl.m_angle.y ) );
				float new_value = std::clamp( change, 0.f, change / 180.f); // ugh
				value = lerp( ind_speed, value, new_value );

				SurfaceEngine::draw_progressbar( position, "lby update", c, value );
			}

			{ // damage override
				static float value = 0.f;
				float new_value = std::clamp( ( float )g_aimbot.m_damage_override, 0.f, ( float )g_aimbot.m_damage_override / 1.f );
				value = lerp( ind_speed, value, new_value );

				SurfaceEngine::draw_progressbar( position, "dmg override", c, value );
			}

			{ // forcebaim
				static float value = 0.f;
				float new_value = std::clamp( ( float )g_aimbot.m_force_baim, 0.f, ( float )g_aimbot.m_force_baim / 1.f );
				value = lerp( ind_speed, value, new_value );

				SurfaceEngine::draw_progressbar( position, "force baim", c, value );
			}

			{ // packets
				if ( settings.lag_enable ) {
					static float value = 0.f;
					auto chocked_command = g_csgo.m_cl->m_choked_commands;
					auto fl_value = settings.lag_limit;

					static int choked = 0;
					if ( chocked_command )
						choked++;
					else
						choked = 0;

					choked = std::clamp( choked, 0, fl_value );
					value = lerp( ind_speed, value, float( choked / fl_value ) );

					SurfaceEngine::draw_progressbar( position, "packets", c, value );
				}
			}
		}
		void indicator_spect( ) {
			if ( !settings.speclissst )
				return;

			auto people_spec = 0;
			auto spec_modes = 0;
			std::string current_spectators[ 25 ];
			std::string current_modes[ 65 ];
			const auto local = g_cl.m_local;


			vec2_t position = vec2_t( 300, 500 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool dragging = false;

			auto x = position.x;
			auto y = position.y;

			gain_mouse_pos( main_mouse, last_mouse );

			if ( main_mouse.x > position.x - s_drag.x && main_mouse.y > position.y - s_drag.y && main_mouse.x < ( position.x - s_drag.x ) + 200 && main_mouse.y < ( position.y - s_drag.y ) + 20 && GetAsyncKeyState( VK_LBUTTON ) ) {
				s_drag.x += main_mouse.x - last_mouse.x;
				s_drag.y += main_mouse.y - last_mouse.y;
				dragging = true;
			}

			position.x -= s_drag.x;
			position.y -= s_drag.y;

			for ( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
				Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
				if ( !player )
					continue;

				if ( player->m_bIsLocalPlayer( ) )
					continue;

				if ( player->dormant( ) )
					continue;

				if ( player->m_lifeState( ) == LIFE_ALIVE )
					continue;

				if ( player->GetObserverTarget( ) != g_cl.m_local )
					continue;

				player_info_t info;
				if ( !g_csgo.m_engine->GetPlayerInfo( i, &info ) )
					continue;
				
				current_spectators[ people_spec ] = info.m_name;

				switch ( player->m_iObserverMode( ) ) {
					case obeserver_modes::OBS_MODE_IN_EYE:
						current_modes[ people_spec ] = "person"; break;
					case obeserver_modes::OBS_MODE_CHASE:
						current_modes[ people_spec ] = "chase"; break;
					case obeserver_modes::OBS_MODE_ROAMING:
						current_modes[ people_spec ] = "roaming"; break;
					case obeserver_modes::OBS_MODE_DEATHCAM:
						current_modes[ people_spec ] = "death"; break;
					case obeserver_modes::OBS_MODE_FREEZECAM:
						current_modes[ people_spec ] = "freeze"; break;
					case obeserver_modes::OBS_MODE_FIXED:
						current_modes[ people_spec ] = "fixed"; break;
				}

				people_spec++;
			}

			float height = ( people_spec + 2 ) * 20;

			if ( settings.indicator_style == 1 )
				Drawing::DrawRect( position.x + 5, position.y + 5, 200 - 10, height - 10, global_color );

			std::string spec_text = "spectators";
			if ( settings.indicator_style == 0 && CMenu::get().IsMenuOpened() ) {
				auto re_size = Drawing::GetTextSize( render::esp, spec_text.c_str( ) );
				Drawing::DrawString( render::esp, position.x + 100, position.y + 6, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );
			}
			else if ( settings.indicator_style == 1 ) {
				Drawing::DrawString( render::esp, position.x + 100, position.y + 6, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );
			}

			if ( settings.indicator_style == 1 ) {
				Drawing::DrawRect( position.x + 8, position.y + 23, 200 - 16, 3, gain_color(settings.menu_color) );
			}

			if ( !local || !local->alive( ) )
				return;

			if ( settings.indicator_style == 1 )
				position.y += 30;

			if ( CMenu::get( ).IsMenuOpened( ) )
				return;

			for ( int i{  }; i < people_spec; ++i ) {
				if ( settings.indicator_style == 1 ) {
					Drawing::DrawRect( position.x + 9, position.y, 1, 20, gain_color( settings.menu_color ) );
				}

				auto size = Drawing::GetTextSize( render::esp, current_spectators[ i ].c_str( ) );
				int width = size.m_width;
				static int i_size = 10;
				if ( width > 50 ) {
					i_size = 5;
				}

				if ( settings.indicator_style == 1 ) {
					Drawing::DrawString(render::esp, position.x + 14, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, current_spectators[ i ].substr( 0, i_size ).c_str( ) );
					Drawing::DrawString(render::esp, position.x + 110, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, current_modes[ i ].c_str( ) );
					Drawing::DrawString(render::esp, position.x + 175, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, std::to_string( i + 1 ).c_str( ) );
				}
				else {
					auto re_size = Drawing::GetTextSize( render::esp, current_spectators[ i ].substr( 0, i_size ).c_str( ) );
					Drawing::DrawString( render::esp, position.x - ( re_size.m_width / 2 ) + 100, position.y + 6, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, current_spectators[ i ].substr( 0, i_size ).c_str( ) );

				}
				position.y += 20;
			}
		}

		// mythic pasting
		struct damage_infos {
			Player* player{};
			char name[ 25 ]{};
			int health_before{};
			int damage{};
			int health_after{};
			int last{};
			int shots{};
			int miss{};
			int lag{};
			bool killed{};
			std::string method;
			std::string hitbox;
		};
		std::vector<damage_infos> dmg_info;

		void clear_dmg( ) {
			if ( !settings.round_summary )
				return;

			auto local = g_cl.m_local;
			if ( !local || !local->alive( ) )
				return;

			if ( dmg_info.size( ) > 0 )
				dmg_info.clear( );

			dmg_info.clear( );
		}
		void add_dmg( IGameEvent* event ) {
			if ( !event || !settings.round_summary )
				return;

			auto local = g_cl.m_local;
			if ( !local || !local->alive( ) )
				return;

			const auto attacker = g_csgo.m_entlist->GetClientEntity( g_csgo.m_engine->GetPlayerForUserID( event->GetInt( "attacker" ) ) );
			const auto target = reinterpret_cast< Player* >( g_csgo.m_entlist->GetClientEntity( g_csgo.m_engine->GetPlayerForUserID( event->GetInt( "userid" ) ) ) );
			if ( attacker && target && attacker == local && target->m_iTeamNum( ) != local->m_iTeamNum( ) ) {
				damage_infos toadd;
				toadd.player = target;

				// get player info
				player_info_t info;
				g_csgo.m_engine->GetPlayerInfo( toadd.player->index( ), &info );

				memcpy( toadd.name, info.m_name, 9 );
				toadd.name[ 8 ] = '\0';

				auto size = Drawing::GetTextSize( render::esp, toadd.name );
				int width = size.m_width;

				if ( width > 50 ) {
					memcpy( toadd.name, info.m_name, 5 );
					toadd.name[ 4 ] = '\0';
				}

				toadd.health_before = toadd.player->m_iHealth( );
				toadd.damage = event->GetInt( "dmg_health" );
				const auto hitgroup = event->GetInt( "hitgroup" );

				switch ( hitgroup ) {
					case HITGROUP_GENERIC:
						toadd.hitbox = "generic";
						break;
					case HITGROUP_HEAD:
						toadd.hitbox = "head";
						break;
					case HITGROUP_CHEST:
						toadd.hitbox = "chest";
						break;
					case HITGROUP_STOMACH:
						toadd.hitbox = "stomach";
						break;
					case HITGROUP_LEFTARM:
						toadd.hitbox = "left arm";
						break;
					case HITGROUP_RIGHTARM:
						toadd.hitbox = "right arm";
						break;
					case HITGROUP_LEFTLEG:
						toadd.hitbox = "left leg";
					case HITGROUP_RIGHTLEG:
						toadd.hitbox = "right leg";
						break;
					case HITGROUP_GEAR:
						toadd.hitbox = "gear";
						break;
					default:
						toadd.hitbox = "body";
						break;
				}

				dmg_info.push_back( toadd );
			}
		}
		void draw_dmg( ) {
			if ( !settings.round_summary )
				return;

			auto draw_text = [ ]( const char* text, vec2_t pos, int x_offset, Color Color ) {
				Drawing::DrawString( render::esp, pos.x + x_offset, pos.y + 2, Color, render::ALIGN_CENTER, text );
			};

			vec2_t position = vec2_t( 500, 500 );
			static vec2_t main_mouse, last_mouse, s_drag;
			bool dragging = false;

			auto x = position.x;
			auto y = position.y;

			gain_mouse_pos( main_mouse, last_mouse );

			if ( main_mouse.x > position.x - s_drag.x && main_mouse.y > position.y - s_drag.y && main_mouse.x < ( position.x - s_drag.x ) + 200 && main_mouse.y < ( position.y - s_drag.y ) + 20 && GetAsyncKeyState( VK_LBUTTON ) ) {
				s_drag.x += main_mouse.x - last_mouse.x;
				s_drag.y += main_mouse.y - last_mouse.y;
				dragging = true;
			}

			position.x -= s_drag.x;
			position.y -= s_drag.y;

			std::string gap = " | ";
			std::string showcase = "name" + gap + "damage" + gap + "hitbox" + gap + "ticks"; // + gap + method;


			auto size = Drawing::GetTextSize( render::esp, showcase.c_str( ) );
			int width = size.m_width, height = size.m_height;
			float dheight = ( ( dmg_info.size( ) > 5 ? 5 : dmg_info.size( ) ) + 2 ) * 20 + 5;

			if ( settings.indicator_style == 1 ) {
				if ( !CMenu::get().IsMenuOpened() )
					Drawing::DrawRect( position.x + 5, position.y + 5, 200 - 10, dheight - 10 - 5, global_color );
				else
					Drawing::DrawRect( position.x + 5, position.y + 5, 200 - 10, 30, global_color );
			}

			std::string spec_text = "round summary";
			if ( settings.indicator_style == 0 && CMenu::get( ).IsMenuOpened( ) ) {
				Drawing::DrawString( render::esp, position.x + 100, position.y + 6, dragging ? gain_color(settings.menu_color) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );
				Drawing::DrawString( render::esp, position.x + 100, position.y + 30, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, showcase.c_str( ) );

			}
			else if ( settings.indicator_style == 1 )
				Drawing::DrawString( render::esp, position.x + 100, position.y + 6, dragging ? gain_color( settings.menu_color ) : Color( 255, 255, 255, 155 ), render::ALIGN_CENTER, spec_text.c_str( ) );

			if ( settings.indicator_style == 1 )
				Drawing::DrawRect( position.x + 8, position.y + 23, 200 - 16, 3, gain_color( settings.menu_color ) );

			if ( dmg_info.empty( ) || dmg_info.size( ) <= 0 ) {
				return;
			}

			auto local = g_cl.m_local;
			if ( !local || g_csgo.m_gamerules == nullptr || g_csgo.m_gamerules->m_bFreezePeriod( ) ) {
				clear_dmg( );
				return;
			}

			if ( CMenu::get( ).IsMenuOpened( ) )
				return;

			position.y += 30;
			auto it = dmg_info.rbegin( );

			for ( int i = 0; it != dmg_info.rend( ) && i < 5; i++, it++ ) {
				const char* name = it->name;
				std::string damage = std::to_string( it->damage );
				std::string hitbox = it->hitbox;
				std::string method = it->method;
				int ticks = it->lag;

				if ( settings.indicator_style == 1 ) {
					Drawing::DrawRect( position.x + 9, position.y, 1, 20, gain_color( settings.menu_color ) );
				}

				Drawing::DrawString( render::esp, position.x + 14, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, name );
				Drawing::DrawString( render::esp, position.x + 80, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, damage.c_str( ) );
				Drawing::DrawString( render::esp, position.x + 115, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, hitbox.c_str( ) );
				Drawing::DrawString( render::esp, position.x + 165, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, std::to_string( ticks ).c_str( ) );

				position.y += 20;

			}
		}

		void draw_beam( vec3_t start, vec3_t end, Color color, float Width ) {
			BeamInfo_t info;

			info.m_nType = 0;
			info.m_pszModelName = "sprites/purplelaser1.vmt";
			info.m_nModelIndex = -1;
			info.m_flHaloScale = 0.f;
			info.m_flLife = 2.5f;
			info.m_flWidth = Width;
			info.m_flEndWidth = 1;
			info.m_flFadeLength = 0.f;
			info.m_flAmplitude = 2.f;
			info.m_flBrightness = float( color.a( ) );
			info.m_flSpeed = .2f;
			info.m_nStartFrame = 0;
			info.m_flFrameRate = 0.f;
			info.m_flRed = float( color.r( ) );
			info.m_flGreen = float( color.g( ) );
			info.m_flBlue = float( color.b( ) );
			info.m_nSegments = 2;
			info.m_bRenderable = true;
			info.m_nFlags = 0;
			info.m_vecStart = start;
			info.m_vecEnd = end;

			auto myBeam = g_csgo.m_beams->CreateBeamPoints( info );
			if ( myBeam )
				g_csgo.m_beams->DrawBeam( myBeam );
		}

		void add_trail( ) {
			auto local = g_cl.m_local;
			if ( !settings.localyra || !local )
				return;

			static vec3_t old_origin = local->m_vecOrigin( );
			auto origin = local->m_vecOrigin( ) + vec3_t( 0, 0, 5 );

			auto start = old_origin, end = origin;
			Color color = gain_color( settings.traiillcolor );
			constexpr auto width = 1.6f;

			if ( old_origin.length_2d( ) != origin.length_2d( ) )
				draw_beam( start, end, color, width );

			old_origin = origin;
		}

		void sunset_control( ) {
			if ( !settings.suneset )
				return;

			static auto cl_csm_rot_override = g_csgo.m_cvar->FindVar( HASH( "cl_csm_rot_override" ) );

			if ( !settings.suneset ) {
				if ( cl_csm_rot_override->GetBool( ) ) {
					cl_csm_rot_override->SetValue( FALSE );
				}

				return;
			}

			if ( !cl_csm_rot_override->GetBool( ) ) {
				cl_csm_rot_override->SetValue( TRUE );
			}

			/* set the rotation x */
			static auto cl_csm_rot_x = g_csgo.m_cvar->FindVar( HASH( "cl_csm_rot_x" ) );
			if ( cl_csm_rot_x->GetInt( ) != 0 )
				cl_csm_rot_x->SetValue( 0 );

			/* set the rotation y */
			static auto cl_csm_rot_y = g_csgo.m_cvar->FindVar( HASH( "cl_csm_rot_y" ) );
			if ( cl_csm_rot_y->GetInt( ) != 0 )
				cl_csm_rot_y->SetValue( 0 );
		}

		void manual_arrows( ) {
			if ( !settings.manualaaaa )
				return;

			auto rotate_point = [ ]( vec2_t& point, vec2_t origin, bool clockwise, float angle ) -> void {
				vec2_t delta = point - origin;
				vec2_t rotated;

				if ( clockwise ) {
					rotated = vec2_t( delta.x * cos( angle ) - delta.y * sin( angle ), delta.x * sin( angle ) + delta.y * cos( angle ) );
				}
				else {
					rotated = vec2_t( delta.x * sin( angle ) - delta.y * cos( angle ), delta.x * cos( angle ) + delta.y * sin( angle ) );
				}

				point = rotated + origin;
			};

			int centerX = g_cl.m_width / 2, centerY = g_cl.m_height / 2;

			auto left_pos = vec2_t( centerX - 45, centerY ),
				right_pos = vec2_t( centerX + 45, centerY ),
				down_pos = vec2_t( centerX, centerY + 45 ), siz = vec2_t( 8, 8 );

			static std::vector< Vertex > vertices_left =
			{
				Vertex{ vec2_t( left_pos.x - siz.x, left_pos.y + siz.y ), vec2_t( ) },
				Vertex{ vec2_t( left_pos.x, left_pos.y - siz.y ), vec2_t( ) },
				Vertex{ left_pos + siz, vec2_t( ) }
			};

			static std::vector< Vertex > vertices_right =
			{
				Vertex{ vec2_t( right_pos.x - siz.x, right_pos.y + siz.y ), vec2_t( ) },
				Vertex{ vec2_t( right_pos.x, right_pos.y - siz.y ), vec2_t( ) },
				Vertex{ right_pos + siz, vec2_t( ) }
			};

			static std::vector< Vertex > vertices_down =
			{
				Vertex{ vec2_t( down_pos.x - siz.x, down_pos.y + siz.y ), vec2_t( ) },
				Vertex{ vec2_t( down_pos.x, down_pos.y - siz.y ), vec2_t( ) },
				Vertex{ down_pos + siz, vec2_t( ) }
			};

			static bool filled[ 3 ] = { false,false,false };
			if ( !filled[ 0 ] ) {
				for ( unsigned int p = 0; p < vertices_left.size( ); p++ ) {
					rotate_point( vertices_left[ p ].m_pos, left_pos, false, 3.15f );
				}
				filled[ 0 ] = true;
			}
			if ( !filled[ 1 ] ) {
				for ( unsigned int p = 0; p < vertices_right.size( ); p++ ) {
					rotate_point( vertices_right[ p ].m_pos, right_pos, false, 0 );
				}
				filled[ 1 ] = true;
			}

			if ( !filled[ 2 ] ) {
				for ( unsigned int p = 0; p < vertices_down.size( ); p++ ) {
					rotate_point( vertices_down[ p ].m_pos, down_pos, false, 61.25f );
				}
				filled[ 2 ] = true;
			}

			render::textured_polygon( vertices_left.size( ), vertices_left, ( g_hvh.m_left ? gain_color(settings.menu_color).alpha( 160 ) : colors::white.alpha( 130 ) ) );
			render::textured_polygon( vertices_right.size( ), vertices_right, ( g_hvh.m_right ? gain_color( settings.menu_color ).alpha( 160 ) : colors::white.alpha( 130 ) ) );
			render::textured_polygon( vertices_down.size( ), vertices_down, ( g_hvh.m_back ? gain_color( settings.menu_color ).alpha( 160 ) : colors::white.alpha( 130 ) ) );
		}
	}

	namespace SurfaceEngine {
		void draw_progresstext( vec2_t& position, const char* name, const float progress ) {
			auto size = Drawing::GetTextSize( render::indicator, name );
			int width = size.m_width, height = size.m_height;

			auto color = Color( 255, 255, 255 );
			if ( progress >= 0.70f )
				color = c_green;
			else if ( progress < 0.70f && progress > 0.50f )
				color = c_yellow;
			else if ( progress == -1.f )
				color = c_white;
			else
				color = c_red;

			Drawing::DrawString( render::indicator, position.x - 25, position.y, color, render::ALIGN_LEFT, name );
			position.y -= height;
		}

		void draw_progressbar( vec2_t& position, const char* name, const Color color, const float progress ) {
			auto size = Drawing::GetTextSize( render::esp , name );
			int width = size.m_width, height = size.m_height;

			Drawing::DrawRect( position.x + 9, position.y, 1, 20, gain_color( settings.menu_color ) );
			Drawing::DrawString( render::esp, position.x + 14, position.y + 2, Color( 255, 255, 255, 155 ), render::ALIGN_LEFT, name );

			Drawing::DrawRect( position.x + width + 22, position.y + 8, 100, 3, Color( 25, 25, 25, 255 ) );
			Drawing::DrawRect( position.x + width + 22, position.y + 8, progress * 100, 3, gain_color( settings.menu_color ) );

			// im not sure why those faggots did this but eh
			if ( settings.enable )
				position.y += settings.indicator_style == 1 ? 20 : -20; // - 20 depends on indic style
		}

		void draw_progressbar( vec2_t& position, const char* name, const Color color, const float progress, render::Font font ) {
			float progress_alpha = 155 + progress * 100.f;
			Drawing::DrawRect( position.x, position.y, 100, 20, Color( 24, 19, 28, progress_alpha ) );
			Drawing::DrawRect( position.x, position.y + 17, progress * 100.f, 3, gain_color( settings.menu_color ) );
			Drawing::DrawString( font, position.x, position.y, Color( 255, 255, 255, 240 ), render::ALIGN_LEFT, name );

			position.y += settings.indicator_style == 1 ? 20 : -20; //config->Visuals.indicators_styles == 1 ? 20 : -20;
		}
	}
}