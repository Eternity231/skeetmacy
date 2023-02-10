#include "big_resolve.hpp"
#include "../backend/config/config_new.h"
// vars.
static float next_lby_update[ 65 ];
static float add_lby[ 65 ];

namespace resolve_record {
	namespace resolve_data {
		// aimbot records
		LagRecord* find_ideal_record( AimPlayer* data ) {
			LagRecord* first_valid, * current;

			// dont got for empty records
			if ( data->m_records.empty( ) )
				return nullptr;

			first_valid = nullptr;

			// iritate records.
			for ( const auto& it : data->m_records ) {
				// safety checks
				if ( it->dormant( ) || it->immune( ) || !it->valid( ) )
					continue;

				// get current record.
				current = it.get( );

				// first record that was valid, store it for later.
				if ( !first_valid )
					first_valid = current;

				// try to find a record with a shot, lby update, walking or no anti-aim.
				if ( it->m_shot || it->m_mode == resolver_modes::resolve_body || it->m_mode == resolver_modes::resolve_walk || it->m_mode == resolver_modes::resolve_none )
					return current;
			}

			// no records found
			return ( first_valid ) ? first_valid : nullptr;
		}

		LagRecord* find_last_record( AimPlayer* data ) {
			LagRecord* current;

			if ( data->m_records.empty( ) )
				return nullptr;

			// iterate records in reverse.
			for ( auto it = data->m_records.crbegin( ); it != data->m_records.crend( ); ++it ) {
				current = it->get( );

				// if this record is valid.
				// we are done since we iterated in reverse.
				if ( current->valid( ) && !current->immune( ) && !current->dormant( ) )
					return current;
			}

			return nullptr;
		}

		// lowerbody yaw
		void on_body_update( Player* player, float value ) {
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

			// go set data.
			data->m_old_body = data->m_body;
			data->m_body = value;
		}

		// resolver stuff
		float get_away_angle( LagRecord* record ) {
			// vars.
			float  delta{ std::numeric_limits< float >::max( ) };
			vec3_t pos;
			ang_t  away;

			// clean as a baby ass
			math::VectorAngles( g_cl.m_local->m_vecOrigin( ) - record->m_pred_origin, away );
			return away.y;
		}

		void match_shot( AimPlayer* data, LagRecord* record ) {
			// show time
			float shoot_time = -1.f;

			Weapon* weapon = data->m_player->GetActiveWeapon( );
			if ( weapon ) {
				shoot_time = weapon->m_fLastShotTime( ) + g_csgo.m_globals->m_interval;
			}

			// this record has a shot on it.
			if ( game::TIME_TO_TICKS( shoot_time ) == game::TIME_TO_TICKS( record->m_sim_time ) ) {
				if ( record->m_lag <= 2 )
					record->m_shot = true;

				// more then 1 choke, cant hit pitch, apply prev pitch.
				else if ( data->m_records.size( ) >= 2 ) {
					LagRecord* previous = data->m_records[ 1 ].get( );

					if ( previous && !previous->dormant( ) )
						record->m_eye_angles.x = previous->m_eye_angles.x;
				}
			}
		}

		float get_lby_rotated_yaw( float lby, float yaw ) {
			auto normalized_angle = math::NormalizedAngle;
			float delta = normalized_angle( yaw - lby );
			if ( fabs( delta ) < 25.f )
				return lby;

			if ( delta > 0.f )
				return yaw + 25.f;

			return yaw;
		}

		bool is_yaw_sideways( Player* entity, float yaw ) {
			auto local_player = g_cl.m_local;
			if ( !local_player )
				return false;

			auto calculate_angle = math::CalcAngle;
			auto normalized_angle = math::NormalizedAngle;

			const auto at_target_yaw = calculate_angle( local_player->m_vecOrigin( ), entity->m_vecOrigin( ) ).y;
			const float delta = fabs( normalized_angle( at_target_yaw - yaw ) );

			return delta > 20.f && delta < 160.f;
		}

		void reset_indexes( AimPlayer* data ) {
			// alpha does that, lets do the same
			data->m_upd_index = 0;
			data->m_stand_index = 0;
			data->m_body_index = 0;
			data->m_missed_shots = 0;
			data->m_unknown_move = 0;
			data->m_last_move = 0;

			if ( settings.debug_reset_logs )
				notify_controller::g_notify.push_event( "indexes reseted\n", "[debug]" );
		}
	}

	namespace onetap_reverse {
		void set_final_angle( AimPlayer* data, LagRecord* current, float& out_angle ) {
			if ( std::abs( data->m_move_lby - data->m_stand_lby ) < 35.f
				&& data->m_misses_but_who_tf_cares < 1
				&& data->m_moved ) {
				out_angle = current->m_body;
				return;
			}

			if ( !data->m_stand_ticks ) {
				out_angle = data->m_move_lby;
				return;
			}

			if ( data->m_stand_resolving
				&& data->m_moved ) {
				if ( current->m_anim_time >= data->m_body_update ) {
					out_angle = current->m_body;
					data->m_body_update = current->m_anim_time + 1.1f;
					return;
				}
			}

			if ( !data->m_misses_but_who_tf_cares ) {
				out_angle = data->m_move_lby;
				return;
			}

			const auto away_angle = resolve_data::get_away_angle( current );
			const auto negative_delta = math::angle_diff( current->m_body - 110.f, away_angle );
			const auto positive_delta = math::angle_diff( current->m_body + 110.f, away_angle );

			switch ( data->m_misses_but_who_tf_cares % 4 ) {
			case 0:
				out_angle = data->m_move_lby;
				break;
			case 1:
				out_angle = data->m_anti_fs_angle;
				break;
			case 2:
				out_angle = current->m_body - negative_delta;
				break;
			case 3:
				out_angle = current->m_body - positive_delta;
				break;
			default:
				out_angle = data->m_stand_lby;
				break;
			}
		}

		/* weird shit happens if do it in math */
		__forceinline vec3_t calc_ang( const vec3_t& src, const vec3_t& dst ) {
			constexpr auto k_pi = 3.14159265358979323846f;
			constexpr auto k_pi2 = k_pi * 2.f;
			constexpr auto k_rad_pi = 180.f / k_pi;

			vec3_t angles;

			vec3_t delta = src - dst;
			const float length = delta.length_2d( );

			angles.y = std::atanf( delta.y / delta.x ) * k_rad_pi;
			angles.x = std::atanf( -delta.z / length ) * -k_rad_pi;
			angles.z = 0.0f;

			if ( delta.x >= 0.0f )
				angles.y += 180.0f;

			return angles;
		}

		/* not 100% from onetap cuz onetaps eating shit */
		void anti_freestand( AimPlayer* data, LagRecord* current ) {
			if ( !data->m_player
				|| data->m_records.empty( ) )
				return;

			if ( !data->m_player->alive( )
				|| data->m_player->m_iTeamNum( ) == g_cl.m_local->m_iTeamNum( )
				|| data->m_player->dormant( ) )
				return;

			const auto at_target_angle = calc_ang( g_cl.m_local->m_vecOrigin( ), data->m_player->m_vecOrigin( ) );

			vec3_t left_dir{}, right_dir{};
			math::AngleVectors( ang_t( 0.f, at_target_angle.y - 90.f, 0.f ), &left_dir );
			math::AngleVectors( ang_t( 0.f, at_target_angle.y + 90.f, 0.f ), &right_dir );

			const auto eye_pos = data->m_player->wpn_shoot_pos( );
			auto left_eye_pos = eye_pos + ( left_dir * 16.f );
			auto right_eye_pos = eye_pos + ( right_dir * 16.f );

			auto shoot_pos = g_cl.m_shoot_pos + ( g_cl.m_local->m_vecVelocity( ) * g_csgo.m_globals->m_interval ) * 1;

			auto left_pen_data = g_auto_wall->get( shoot_pos, left_eye_pos, data->m_player );
			auto right_pen_data = g_auto_wall->get( shoot_pos, right_eye_pos, data->m_player );

			if ( left_pen_data.m_dmg == -1
				&& right_pen_data.m_dmg == -1 ) {

				Ray ray{};
				CGameTrace trace{};
				CTraceFilterWorldOnly filter{};

				ray = { left_eye_pos, eye_pos };
				g_csgo.m_engine_trace->TraceRay( ray, 0xFFFFFFFF, &filter, &trace );
				const auto left_frac = trace.m_fraction;

				ray = { right_eye_pos, eye_pos };
				g_csgo.m_engine_trace->TraceRay( ray, 0xFFFFFFFF, &filter, &trace );
				const auto right_frac = trace.m_fraction;

				if ( left_frac > right_frac ) {
					data->m_anti_fs_angle = at_target_angle.y + 90.f;
				}
				else if ( right_frac > left_frac ) {
					data->m_anti_fs_angle = at_target_angle.y - 90.f;
				}
				else
					data->m_anti_fs_angle = at_target_angle.y - 180.f;

				return;
			}

			if ( left_pen_data.m_dmg == right_pen_data.m_dmg ) {
				data->m_anti_fs_angle = at_target_angle.y - 180.f;
				return;
			}

			if ( left_pen_data.m_dmg > right_pen_data.m_dmg ) {
				data->m_anti_fs_angle = at_target_angle.y + 90.f;
			}
			else if ( right_pen_data.m_dmg > left_pen_data.m_dmg ) {
				data->m_anti_fs_angle = at_target_angle.y - 90.f;
			}

		}

		float get_pitch_angle( AimPlayer* data, LagRecord* current ) {
			float pitch{ current->m_eye_angles.x };

			if ( std::abs( data->m_player->m_flThirdPersonRecoil( ) + current->m_eye_angles.x - 180.f ) < 0.1f
				|| data->m_misses_but_who_tf_cares >= 2 )
				pitch = 89.f;

			return pitch;
		}

		void handle_resolver( AimPlayer* data, LagRecord* current, LagRecord* previous ) {
			if ( !previous ) {
				return;
			}

			current->m_eye_angles.x = get_pitch_angle( data, current );

			float final_resolver_angle{};

			const auto on_ground = ( current->m_flags & FL_ONGROUND );
			const auto speed_3d = current->m_anim_velocity.length( );
			if ( on_ground ) {
				if ( speed_3d <= 0.1f ) {
					data->m_stand_lby = current->m_body;
					data->m_stand_ticks += 1;
					data->m_stand_resolving = true;
				}
				else {
					if ( !current->m_fake_walk ) {
						data->m_move_lby = current->m_body;
						data->m_stand_ticks = 0;
						data->m_moved = true;
						data->m_stand_resolving = false;
						data->m_body_update = current->m_anim_time + 0.22f;
					}
					else {
						data->m_body_update = current->m_anim_time + 1.1f; // ayo ot wtf
					}
				}

			on_ground_resolver:

				set_final_angle( data, current, final_resolver_angle );

				/* if you really need you can add override resolver here and change final_resolver_angle value */

				current->m_eye_angles.y = final_resolver_angle;

				return;
			}

			data->m_moved = false;
			goto on_ground_resolver;
		}

	}

	namespace resolve_anti_freestand {
		void resolve_anti_freestanding( LagRecord* record ) {
			record->resolver_text[ record->m_player->index( ) ] = "FREESTAND";

			// constants
			constexpr float STEP{ 4.f };
			constexpr float RANGE{ 32.f };

			// best target.
			vec3_t enemypos = record->m_player->GetShootPosition( );
			float away = resolve_data::get_away_angle( record );

			// construct vector of angles to test.
			std::vector< AdaptiveAngle > angles{ };
			angles.emplace_back( away - 180.f );
			angles.emplace_back( away + 90.f );
			angles.emplace_back( away - 90.f );

			// start the trace at the your shoot pos.
			vec3_t start = g_cl.m_local->GetShootPosition( );

			// see if we got any valid result.
			// if this is false the path was not obstructed with anything.
			bool valid{ false };

			// iterate vector of angles.
			for ( auto it = angles.begin( ); it != angles.end( ); ++it ) {
				vec3_t end{ enemypos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * RANGE,enemypos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * RANGE,enemypos.z };

				// compute the direction.
				vec3_t dir = end - start;
				float len = dir.normalize( );

				// should never happen.
				if ( len <= 0.f )
					continue;

				// step thru the total distance, 4 units per step.
				for ( float i{ 0.f }; i < len; i += STEP ) {
					// get the current step position.
					vec3_t point = start + ( dir * i );

					// get the contents at this point.
					int contents = g_csgo.m_engine_trace->GetPointContents( point, MASK_SHOT_HULL );

					// contains nothing that can stop a bullet.
					if ( !( contents & MASK_SHOT_HULL ) )
						continue;

					float mult = 1.f;

					// over 50% of the total length, prioritize this shit.
					if ( i > ( len * 0.5f ) )
						mult = 1.25f;

					// over 90% of the total length, prioritize this shit.
					if ( i > ( len * 0.75f ) )
						mult = 1.25f;

					// over 90% of the total length, prioritize this shit.
					if ( i > ( len * 0.9f ) )
						mult = 2.f;

					// append 'penetrated distance'.
					it->m_dist += ( STEP * mult );

					// mark that we found anything.
					valid = true;
				}
			}

			if ( !valid ) {
				return;
			}

			// put the most distance at the front of the container.
			std::sort( angles.begin( ), angles.end( ), [ ]( const AdaptiveAngle& a, const AdaptiveAngle& b ) { 
				return a.m_dist > b.m_dist; 
			} );

			// the best angle should be at the front now.
			AdaptiveAngle* best = &angles.front( );
			record->m_eye_angles.y = best->m_yaw;
		}
		void resolve_anti_freestanding_reversed( LagRecord* record ) {
			record->resolver_text[ record->m_player->index( ) ] = "FREESTAND";

			// constants
			constexpr float STEP{ 4.f };
			constexpr float RANGE{ 32.f };

			// best target.
			vec3_t enemypos = record->m_player->GetShootPosition( );
			float away = resolve_data::get_away_angle( record );

			// construct vector of angles to test.
			std::vector< AdaptiveAngle > angles{ };
			angles.emplace_back( away - 180.f );
			angles.emplace_back( away - 90.f );
			angles.emplace_back( away + 90.f );

			// start the trace at the your shoot pos.
			vec3_t start = g_cl.m_local->GetShootPosition( );
			bool valid{ false };

			// iterate vector of angles.
			for ( auto it = angles.begin( ); it != angles.end( ); ++it ) {
				vec3_t end{ enemypos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * RANGE, enemypos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * RANGE, enemypos.z };

				// compute the direction.
				vec3_t dir = end - start;
				float len = dir.normalize( );

				// should never happen.
				if ( len <= 0.f )
					continue;

				// step thru the total distance, 4 units per step.
				for ( float i{ 0.f }; i < len; i += STEP ) {
					// get the current step position.
					vec3_t point = start + ( dir * i );

					// get the contents at this point.
					int contents = g_csgo.m_engine_trace->GetPointContents( point, MASK_SHOT_HULL );

					// contains nothing that can stop a bullet.
					if ( !( contents & MASK_SHOT_HULL ) )
						continue;

					float mult = 1.f;

					// over 50% of the total length, prioritize this shit.
					if ( i > ( len * 0.5f ) )
						mult = 1.25f;

					// over 90% of the total length, prioritize this shit.
					if ( i > ( len * 0.75f ) )
						mult = 1.25f;

					// over 90% of the total length, prioritize this shit.
					if ( i > ( len * 0.9f ) )
						mult = 2.f;

					// append 'penetrated distance'.
					it->m_dist += ( STEP * mult );

					// mark that we found anything.
					valid = true;
				}
			}

			if ( !valid ) {
				return;
			}

			// put the most distance at the front of the container.
			std::sort( angles.begin( ), angles.end( ), [ ]( const AdaptiveAngle& a, const AdaptiveAngle& b ) {
					return a.m_dist > b.m_dist;
				} );

			// the best angle should be at the front now.
			AdaptiveAngle* best = &angles.front( );
			record->m_eye_angles.y = best->m_yaw;
		}
	}

	namespace resolve_stages {
		void set_mode( LagRecord* record ) {
			float speed = record->m_velocity.length_2d( );

			if ( ( record->m_flags & FL_ONGROUND ) && speed > 0.1f && !record->m_fake_walk )
				record->m_mode = resolve_data::resolver_modes::resolve_walk;
			else if ( ( record->m_flags & FL_ONGROUND ) && ( speed <= 0.1f || record->m_fake_walk ) )
				record->m_mode = resolve_data::resolver_modes::resolve_last_move;
			else if ( !( record->m_flags & FL_ONGROUND ) )
				record->m_mode = resolve_data::resolver_modes::resolve_air;
		}

		void resolve_angles( Player* player, LagRecord* record ) {
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
			bool fake = hyperbius::get_chocked_packet( record->m_player ) > 1;

			// run match shot and set modes
			resolve_data::match_shot( data, record );
			resolve_stages::set_mode( record );

			// resolve staging
			if ( record->m_mode == resolve_data::resolver_modes::resolve_walk ) {
				if ( settings.resolver_type == 2 ) {
					hyperbius::hyperbius_moving( data, record );
				}
				else if ( settings.resolver_type == 0 ) {
					resolve_stages::resolve_walking( data, record );
				}
			}
			else if ( record->m_mode == resolve_data::resolver_modes::resolve_last_move || record->m_mode == resolve_data::resolver_modes::resolve_unknown ) {
				if ( settings.resolver_type == 2 ) {
					hyperbius::hyperbius_standing( record, data, player );
				}
				else if ( settings.resolver_type == 0 ) {
					resolve_stages::resolve_standing( data, record, player );
				}
			}
			else if ( record->m_mode == resolve_data::resolver_modes::resolve_air )
				resolve_stages::resolve_air( data, record );
			else {
				// this shit copasting fucked my clenaerd

				record->m_mode = resolve_data::resolver_modes::resolve_none;
				record->resolver_text[ record->m_player->index( ) ] = "LOGIC";
				fake = false;
			}
			// normalize eye angles
			auto normalize_angles = math::NormalizedAngle;
			normalize_angles( record->m_eye_angles.y );
		}
		void resolve_poses( Player* player, LagRecord* record ) {
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

			// only do this bs when in air.
			if ( record->m_mode == resolve_data::resolver_modes::resolve_air ) {
				player->m_flPoseParameter( )[ 2 ] = g_csgo.RandomInt( 0, 4 ) * 0.25f;

				// body_yaw
				player->m_flPoseParameter( )[ 11 ] = g_csgo.RandomInt( 1, 3 ) * 0.25f;
			}
		}
		void resolve_standing( AimPlayer* data, LagRecord* record, Player* player ) {
			// pointer for easy access & get predicted away angle for the player.
			LagRecord* move = &data->m_walk_record;
			float away = resolve_data::get_away_angle( record );

			// all the resolving bs
			float diff = math::NormalizedAngle( move->m_body - record->m_body );
			float delta = record->m_anim_time - move->m_anim_time;
			auto index = player->index( );
			
			// setup move simtime
			if ( move->m_sim_time > 0.f ) {
				if ( !data->m_moved ) {
					vec3_t delta = move->m_origin - record->m_origin;
					if ( delta.length( ) <= 128.f )
						data->m_moved = true;
				}
			}

			// predict lby flicks
			if ( !record->dormant( ) && !player->dormant( ) ) {
				if ( record->m_anim_velocity.length( ) > 0.1f ) {
					add_lby[ player->index( ) ] = 0.22f;
					next_lby_update[ player->index( ) ] = record->m_anim_time + add_lby[ player->index( ) ];
					data->m_body_update = next_lby_update[ player->index( ) ];
				}
				else if ( record->m_anim_time >= next_lby_update[ player->index( ) ] ) {
					resolve_data::lby_flicking = true;
					add_lby[ player->index( ) ] = 1.1f;
					next_lby_update[ player->index( ) ] = record->m_anim_time + add_lby[ player->index( ) ];
					data->m_body_update = next_lby_update[ player->index( ) ];
				}
				else {
					resolve_data::lby_flicking = false;
				}

				// update via proxy
				if ( data->m_body != data->m_old_body ) {
					resolve_data::lby_flicking = true;
					add_lby[ player->index( ) ] = g_csgo.m_globals->m_interval + 1.1f;
					next_lby_update[ player->index( ) ] = record->m_anim_time + add_lby[ player->index( ) ];
					data->m_body_update = next_lby_update[ player->index( ) ];
				}
				else {
					resolve_data::lby_flicking = false;
				}
			}

			if ( resolve_data::lby_flicking && data->m_upd_index < 2 ) {
				record->m_eye_angles.y = record->m_body;
				resolve_data::i_players[ record->m_player->index( ) ] = false;
				record->m_mode = resolve_data::resolver_modes::resolve_body;
				record->resolver_text[ record->m_player->index( ) ] = "LBY UPDATE";
			}
			else {
				record->m_mode = resolve_data::resolver_modes::resolve_unknown;
				switch ( data->m_missed_shots % 5 ) {
					case 0: {
						if ( fabs( diff ) > 36.f ) {
							record->resolver_text[ record->m_player->index( ) ] = "LAST MOVING LBY [1]";
							record->m_eye_angles.y = move->m_body;
						}
						else {
							record->resolver_text[ record->m_player->index( ) ] = "FREESTAND [1]";
							resolve_anti_freestand::resolve_anti_freestanding( record );
						}
					} break;
					case 1: {
						if ( fabs( diff ) > 36.f ) {
							record->resolver_text[ record->m_player->index( ) ] = "FREESTAND [1:R]";
							resolve_anti_freestand::resolve_anti_freestanding( record );
						}
						else {
							record->resolver_text[ record->m_player->index( ) ] = "FREESTAND [2]";
							resolve_anti_freestand::resolve_anti_freestanding_reversed( record );
						}
					} break;
					case 2: {
						record->resolver_text[ record->m_player->index( ) ] = "BRUTE [B]";
						record->m_eye_angles.y = away + 180.f;
					} break;
					case 3: {
						record->resolver_text[ record->m_player->index( ) ] = "BRUTE [L]";
						record->m_eye_angles.y = away + 90.f;
					} break;
					case 4: {
						record->resolver_text[ record->m_player->index( ) ] = "BRUTE [R]";
						record->m_eye_angles.y = away - 90.f;
					} break;
				}
			}
		}
		void resolve_walking( AimPlayer* data, LagRecord* record ) {
			record->resolver_text[ record->m_player->index( ) ] = "MOVING";

			// apply angles
			record->m_eye_angles.y = record->m_body;

			// reset stand and body index.
			float speed = record->m_velocity.length_2d( );
			if ( speed > 20.f && !record->m_fake_walk )
				data->m_upd_index = 0;

			// set this to false.
			data->m_moved = false;

			// set indexes to 0
			data->m_stand_index = 0;
			data->m_stand_index2 = 0;
			data->m_body_index = 0;
			data->m_last_move = 0;
			data->m_unknown_move = 0;
			data->m_delta_index = 0;

			// copy last record.
			std::memcpy( &data->m_walk_record, record, sizeof( LagRecord ) );
		}
		void resolve_air( AimPlayer* data, LagRecord* record ) {
			record->resolver_text[ record->m_player->index( ) ] = "AIR";

			// walk record
			LagRecord* move = &data->m_walk_record;
			if ( record->m_velocity.length_2d( ) < 50.f ) {
				record->m_eye_angles.y = record->m_body;
				return;
			}

			float velyaw = math::rad_to_deg( std::atan2( record->m_velocity.y, record->m_velocity.x ) );
			float away = resolve_data::get_away_angle( record );

			// brute in air
			switch ( data->m_shots % 3 ) {
				case 0: {
					record->m_eye_angles.y = velyaw + 180.f;
				} break;
				case 1: {
					record->m_eye_angles.y = velyaw - 90.f;
				} break;
				case 2: {
					record->m_eye_angles.y = velyaw + 90.f;
				} break;
			}
		}
	}

	namespace getze_us {
		// thats abit of getze code, i wont leak more than i do there, so dont ask me for complete code
		// full resolver has 700 lines, this is just a semi-leak also unfinished, very
		void layer_resolver( AimPlayer* data, LagRecord* record ) {
			auto player_index = record->m_player->index( );
			auto player_data = record->m_player->m_PlayerAnimState( );
			auto player_eye_angles = record->m_player->m_angEyeAngles( );

			auto onground = ( record->m_player->m_fFlags( ) & FL_ONGROUND );
			auto moving = ( onground && ( record->m_player->m_vecVelocity( ).length_2d( ) > 36.f ) );

			bool updated_lowerbody{};

			if ( moving ) {
				updated_lowerbody = true;
				data->m_body_update = record->m_anim_time + 0.22f;
			}
			else if ( data->m_body != data->m_old_body ) {
				updated_lowerbody = true;
				data->m_body_update = record->m_anim_time + 1.1f;
			}
			else {
				updated_lowerbody = false;
			}
		}
	}

	namespace hyperbius {
		int get_chocked_packet( Player* target ) { // set_angles -> fake_check
			auto ticks = game::TIME_TO_TICKS( target->m_flSimulationTime( ) - target->m_flOldSimulationTime( ) );
			if ( ticks == 0 && last_ticks[ target->index( ) ] > 0 ) {
				return last_ticks[ target->index( ) ] - 1;
			}
			else {
				last_ticks[ target->index( ) ] = ticks;
				return ticks;
			}
		}

		void hyperbius_moving( AimPlayer* data, LagRecord* record ) {
			record->m_mode = resolve_data::resolver_modes::resolve_walk;
			record->resolver_text[ record->m_player->index( ) ] = XOR( "MOVING" );

			// apply lby to eyeangles.
			record->m_eye_angles.y = record->m_body;

			// delay body update.
			data->m_body_update = record->m_anim_time + 0.22f;

			// reset stand and body index.
			data->m_stand_index = 0;
			data->m_stand_index2 = 0;
			data->m_body_index = 0;
			data->m_last_move = 0;
			data->m_unknown_move = 0;

			std::memcpy( &data->m_walk_record, record, sizeof( LagRecord ) );
		}

		void resolve_yaw_bruteforce( LagRecord* record, Player* player, AimPlayer* data ) {
			auto local_player = g_cl.m_local;
			if ( !local_player )
				return;

			record->m_mode = resolve_data::resolver_modes::resolve_stand;
			record->resolver_text[ record->m_player->index() ] = XOR( "BRUTE" );

			const float at_target_yaw = math::CalcAngle( player->m_vecOrigin( ), local_player->m_vecOrigin( ) ).y;

			switch ( data->m_stand_index % 3 )
			{
			case 0:
				record->m_eye_angles.y = resolve_data::get_lby_rotated_yaw( player->m_flLowerBodyYawTarget( ), at_target_yaw + 60.f );
				break;
			case 1:
				record->m_eye_angles.y = at_target_yaw + 140.f;
				break;
			case 2:
				record->m_eye_angles.y = at_target_yaw - 75.f;
				break;
			}
		}

		#define M_RADPI 57.295779513082f
		void calculate_angle( const vec3_t src, const vec3_t dst, ang_t& angles ) {
			auto delta = src - dst;
			vec3_t vec_zero = vec3_t( 0.0f, 0.0f, 0.0f );
			ang_t ang_zero = ang_t( 0.0f, 0.0f, 0.0f );


			if ( delta == vec_zero )
				angles = ang_zero;

			const auto len = delta.length( );

			if ( delta.z == 0.0f && len == 0.0f )
				angles = ang_zero;

			if ( delta.y == 0.0f && delta.x == 0.0f )
				angles = ang_zero;


			angles.z = 0.0f;
			if ( delta.x >= 0.0f ) { angles.y += 180.0f; }

			angles.clamp( );
		}


		void hyperbius_standing( LagRecord* record, AimPlayer* data, Player* player ) {
			// pointer for easy access.
			LagRecord* move = &data->m_walk_record;

			// get predicted away angle for the player.
			float away = resolve_data::get_away_angle( record );

			C_AnimationLayer* curr = &record->m_layers[ 3 ];
			int act = data->m_player->GetSequenceActivity( curr->m_sequence );

			float diff = math::NormalizedAngle( record->m_body - move->m_body );
			float delta = record->m_anim_time - move->m_anim_time;

			ang_t vAngle = ang_t( 0, 0, 0 );
			calculate_angle( player->m_vecOrigin( ), g_cl.m_local->m_vecOrigin( ), vAngle );

			float flToMe = vAngle.y;
			const float at_target_yaw = math::CalcAngle( g_cl.m_local->m_vecOrigin( ), player->m_vecOrigin( ) ).y;
			const auto freestanding_record = player_resolve_records[ player->index( ) ].m_sAntiEdge;

			if ( move->m_sim_time > 0.f ) {
				vec3_t delta = move->m_origin - record->m_origin;

				// check if moving record is close.
				if ( delta.length( ) <= 128.f ) {
					// indicate that we are using the moving lby.
					data->m_moved = true;
				}
			}

			if ( !data->m_moved ) {

				record->m_mode = resolve_data::resolver_modes::resolve_unknown;
				record->resolver_text[ record->m_player->index( ) ] = XOR( "BRUTE" );
				hyperbius::resolve_yaw_bruteforce( record, player, data );

				if ( data->m_body != data->m_old_body ) {
					record->m_eye_angles.y = record->m_body;
					data->m_body_update = record->m_anim_time + 1.1f;

					resolve_data::i_players[ record->m_player->index( ) ] = false;
					record->m_mode = resolve_data::resolver_modes::resolve_body;
					record->resolver_text[ record->m_player->index( ) ] = XOR( "LASTMOVE" );
				}
			}
			else if ( data->m_moved ) {
				float diff = math::NormalizedAngle( record->m_body - move->m_body );
				float delta = record->m_anim_time - move->m_anim_time;

				record->m_mode = resolve_data::resolver_modes::resolve_last_move;
				record->resolver_text[ record->m_player->index( ) ] = XOR( "ANG-DET" );

				const float at_target_yaw = math::CalcAngle( g_cl.m_local->m_vecOrigin( ), player->m_vecOrigin( ) ).y;
				record->m_eye_angles.y = move->m_body;

				if ( data->m_last_move >= 1 )
					hyperbius::resolve_yaw_bruteforce( record, player, data );

				if ( data->m_body != data->m_old_body ) {
					record->m_eye_angles.y = record->m_body;
					data->m_body_update = record->m_anim_time + 1.1f;

					resolve_data::i_players[ record->m_player->index( ) ] = false;
					record->m_mode = resolve_data::resolver_modes::resolve_body;
					record->resolver_text[ record->m_player->index( ) ] = XOR( "LASTMOVE" );
				}
			}
		}
	}
}