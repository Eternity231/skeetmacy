#include "includes.h"
#include "backend/config/config_new.h"

Aimbot g_aimbot{ };;

bool AimPlayer::BreakingLagCompensation( Player* pEntity ) {
	return false;
}
__forceinline void AimPlayer::fix_lag_data( LagRecord* record, LagRecord* previous ) {
	if ( !previous
		|| previous->dormant( ) )
		return;

	if ( ( record->m_origin - previous->m_origin ).length_2d_sqr( ) > 4096.f )
		record->m_broke_lc = true;

	auto sim_ticks = game::TIME_TO_TICKS( record->m_sim_time - previous->m_sim_time );

	if ( ( sim_ticks - 1 ) > 31 || previous->m_sim_time == 0.f ) {
		sim_ticks = 1;
	}

	auto cur_cycle = record->m_layers[ 11 ].m_cycle;
	auto prev_rate = previous->m_layers[ 11 ].m_playback_rate;

	if ( prev_rate > 0.f &&
		record->m_layers[ 11 ].m_playback_rate > 0.f &&
		record->m_layers[ 11 ].m_sequence == previous->m_layers[ 11 ].m_sequence ) {
		auto prev_cycle = previous->m_layers[ 11 ].m_cycle;
		sim_ticks = 0;

		if ( prev_cycle > cur_cycle )
			cur_cycle += 1.f;

		while ( cur_cycle > prev_cycle ) {
			const auto last_cmds = sim_ticks;

			const auto next_rate = g_csgo.m_globals->m_interval * prev_rate;
			prev_cycle += g_csgo.m_globals->m_interval * prev_rate;

			if ( prev_cycle >= 1.f )
				prev_rate = record->m_layers[ 11 ].m_playback_rate;

			++sim_ticks;

			if ( prev_cycle > cur_cycle && ( prev_cycle - cur_cycle ) > ( next_rate * 0.5f ) )
				sim_ticks = last_cmds;
		}
	}

	record->m_lag = std::clamp( sim_ticks, 0, 17 );

	auto origin_diff = record->m_origin - previous->m_origin;
	const auto lag_time = game::TICKS_TO_TIME( record->m_lag );

	if ( lag_time > 0.f
		&& lag_time < 1.f )
		record->m_anim_velocity = origin_diff * ( 1.f / lag_time );

	if ( record->m_lag >= 2 ) {

		/* ot v4 */
		if ( ( record->m_flags & FL_ONGROUND )
			&& ( previous->m_flags & FL_ONGROUND ) ) {
			if ( record->m_layers[ 6 ].m_playback_rate == 0.f )
				record->m_anim_velocity = {};
			else {
				if ( record->m_layers[ 11 ].m_sequence == previous->m_layers[ 11 ].m_sequence ) {
					if ( record->m_layers[ 11 ].m_weight > 0.f && record->m_layers[ 11 ].m_weight < 1.f ) {
						const auto speed_2d = record->m_anim_velocity.length_2d( );
						const auto max_speed = m_player->GetActiveWeapon( ) ? std::max( 0.1f, m_player->GetActiveWeapon( )->max_speed( m_player->m_bIsScoped( ) ) ) : 260.f;
						if ( speed_2d ) {
							const auto reversed_val = ( 1.f - record->m_layers[ 11 ].m_weight ) * 0.35f;

							if ( reversed_val > 0.f && reversed_val < 1.f ) {
								const auto speed_as_portion_of_run_top_speed = ( ( reversed_val + 0.55f ) * max_speed ) / speed_2d;
								if ( speed_as_portion_of_run_top_speed ) {
									record->m_anim_velocity.x *= speed_as_portion_of_run_top_speed;
									record->m_anim_velocity.y *= speed_as_portion_of_run_top_speed;
								}
							}
						}
					}
				}
			}
		}

		if ( record->m_anim_velocity.length_2d( ) > 0.1f
			&& record->m_layers[ 12u ].m_weight == 0.f
			&& record->m_layers[ 6u ].m_weight == 0.f
			&& m_player->m_fFlags( ) & FL_ONGROUND )
			record->m_anim_velocity = {};

	}
}

void AimPlayer::UpdateAnimations( LagRecord* record ) {
	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState( );
	if ( !state )
		return;

	// player respawned.
	if ( m_player->m_flSpawnTime( ) != m_spawn ) {
		// reset animation state.
		game::ResetAnimationState( state );

		// note new spawn time.
		m_spawn = m_player->m_flSpawnTime( );
	}

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = m_player->m_vecOrigin( );
	backup.m_abs_origin = m_player->GetAbsOrigin( );
	backup.m_velocity = m_player->m_vecVelocity( );
	backup.m_abs_velocity = m_player->m_vecAbsVelocity( );
	backup.m_flags = m_player->m_fFlags( );
	backup.m_eflags = m_player->m_iEFlags( );
	backup.m_duck = m_player->m_flDuckAmount( );
	backup.m_body = m_player->m_flLowerBodyYawTarget( );
	m_player->GetAnimLayers( backup.m_layers );

	AimPlayer* data = &g_aimbot.m_players[ m_player->index( ) - 1 ];
	LagRecord* previous = m_records.size( ) > 1 ? m_records[ 1 ].get( ) : nullptr;

	fix_lag_data( record, previous );

	/*
		// is shifting
		if ( record->m_sim_time != record->m_old_sim_time ) {
			record->is_shifting = ( record->m_sim_time / g_csgo.m_globals->m_interval ) - g_csgo.m_globals->m_tick_count;
		}
	*/

	// is player a bot?
	bool bot = game::IsFakePlayer( m_player->index( ) );

	if ( previous
		&& !previous->dormant( ) ) {
		m_player->SetAnimLayers( previous->m_layers );

		m_player->m_PlayerAnimState( )->feet_cycle( ) = previous->m_layers[ 6u ].m_cycle;
		m_player->m_PlayerAnimState( )->feet_yaw_rate( ) = previous->m_layers[ 6u ].m_weight;

		const bool on_ground = ( m_player->m_fFlags( ) & FL_ONGROUND );
		record->m_unfixed_on_ground = on_ground;

		m_player->m_fFlags( ) &= ~FL_ONGROUND;

		if ( m_player->m_fFlags( ) & FL_ONGROUND
			&& previous->m_unfixed_on_ground )
			m_player->m_fFlags( ) |= FL_ONGROUND;
		else {
			if ( record->m_layers[ 4 ].m_weight != 1.f
				&& previous->m_layers[ 4 ].m_weight == 1.f
				&& record->m_layers[ 5 ].m_weight != 0.f ) {
				m_player->m_fFlags( ) |= FL_ONGROUND;
			}
			if ( on_ground ) {
				bool is_fixed_on_ground = ( m_player->m_fFlags( ) & FL_ONGROUND );
				if ( !previous->m_unfixed_on_ground )
					is_fixed_on_ground = false;

				if ( is_fixed_on_ground ) {
					m_player->m_fFlags( ) |= FL_ONGROUND;
				}
				else {
					m_player->m_fFlags( ) &= ~FL_ONGROUND;
				}
			}
		}
	}
	else {
		m_player->SetAnimLayers( record->m_layers );
		if ( record->m_flags & FL_ONGROUND ) {
			m_player->m_PlayerAnimState( )->m_ground = true;
			m_player->m_PlayerAnimState( )->m_land = false;
		}
	}

	if ( record->m_lag >= 2
		&& previous ) {
		const auto duck_delta = ( record->m_duck - previous->m_duck ) / record->m_lag;
		m_player->m_flDuckAmount( ) = previous->m_duck + duck_delta;
	}
	else {
		m_player->m_flDuckAmount( ) = record->m_duck;
	}
	m_player->m_vecVelocity( ) = m_player->m_vecAbsVelocity( ) = record->m_anim_velocity;

	record->m_mode = resolve_record::resolve_data::resolver_modes::resolve_none;

	bool fake = !bot;


	if ( fake ) {
		// there we will do more stuff later, more resolvers etc

		switch ( settings.resolver_type ) {
		case 0: {
			resolve_record::resolve_stages::resolve_angles( m_player, record );
		} break;
		case 1: { // onetap resolver
			resolve_record::onetap_reverse::handle_resolver( data, record, previous );
		} break;
		case 2: { // oh god
			resolve_record::resolve_stages::resolve_angles( m_player, record );
		} break;
		}

	}

	m_player->m_vecOrigin( ) = record->m_origin;
	m_player->m_flLowerBodyYawTarget( ) = record->m_body;

	m_player->m_iEFlags( ) &= ~0x1000;

	m_player->m_iEFlags( ) &= ~( EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY );

	// write potentially resolved angles.
	m_player->m_angEyeAngles( ) = record->m_eye_angles;

	const auto frame_count = g_csgo.m_globals->m_frame;
	const auto real_time = g_csgo.m_globals->m_realtime;
	const auto cur_time = g_csgo.m_globals->m_curtime;
	const auto frame_time = g_csgo.m_globals->m_frametime;
	const auto abs_frame_time = g_csgo.m_globals->m_abs_frametime;
	const auto interp_amt = g_csgo.m_globals->m_interp_amt;
	const auto tick_count = g_csgo.m_globals->m_tick_count;

	const auto server_tick = record->m_anim_time / g_csgo.m_globals->m_interval;
	const int server_frame = server_tick + 0.5f;
	g_csgo.m_globals->m_realtime = record->m_anim_time;
	g_csgo.m_globals->m_curtime = record->m_anim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frame = server_frame;
	g_csgo.m_globals->m_tick_count = server_frame;
	g_csgo.m_globals->m_interp_amt = 0.f;

	if ( state->m_frame >= g_csgo.m_globals->m_frame )
		state->m_frame = g_csgo.m_globals->m_frame - 1;

	m_player->m_bClientSideAnimation( ) = g_cl.m_update_anims = true;
	m_player->UpdateClientSideAnimation( );
	m_player->m_bClientSideAnimation( ) = g_cl.m_update_anims = false;

	/* doesn't look like ot resolving poses anywhere */
	if ( fake )
		resolve_record::resolve_stages::resolve_poses( m_player, record );

	m_player->GetPoseParameters( record->m_poses );
	record->m_abs_ang = m_player->GetAbsAngles( );

	g_csgo.m_globals->m_realtime = real_time;
	g_csgo.m_globals->m_curtime = cur_time;
	g_csgo.m_globals->m_frametime = frame_time;
	g_csgo.m_globals->m_abs_frametime = abs_frame_time;
	g_csgo.m_globals->m_interp_amt = interp_amt;
	g_csgo.m_globals->m_frame = frame_count;
	g_csgo.m_globals->m_tick_count = tick_count;

	m_player->m_vecOrigin( ) = backup.m_origin;
	m_player->m_vecVelocity( ) = backup.m_velocity;
	m_player->m_vecAbsVelocity( ) = backup.m_abs_velocity;
	m_player->m_fFlags( ) = backup.m_flags;
	m_player->m_iEFlags( ) = backup.m_eflags;
	m_player->m_flDuckAmount( ) = backup.m_duck;
	m_player->m_flLowerBodyYawTarget( ) = backup.m_body;
	m_player->SetAbsOrigin( backup.m_abs_origin );
	m_player->SetAnimLayers( backup.m_layers );

	return m_player->InvalidatePhysicsRecursive( InvalidatePhysicsBits_t::ANIMATION_CHANGED );
}


void AimPlayer::OnNetUpdate( Player* player ) {
	bool reset = ( !settings.enable || player->m_lifeState( ) == LIFE_DEAD || !player->enemy( g_cl.m_local ) );
	bool disable = ( !reset && !g_cl.m_processing );

	// if this happens, delete all the lagrecords.
	if ( reset ) {
		player->m_bClientSideAnimation( ) = true;
		m_records.clear( );
		return;
	}

	// just disable anim if this is the case.
	if ( disable ) {
		player->m_bClientSideAnimation( ) = true;
		return;
	}

	// update player ptr if required.
	// reset player if changed.
	if ( m_player != player )
		m_records.clear( );

	// update player ptr.
	m_player = player;
	if ( player->dormant( ) ) {
		bool insert = true;

		// we have any records already?
		if ( !m_records.empty( ) ) {

			LagRecord* front = m_records.front( ).get( );

			// we already have a dormancy separator.
			if ( front->dormant( ) )
				insert = false;
		}

		if ( insert ) {
			m_moved = false;

			// add new record.
			m_records.emplace_front( std::make_shared< LagRecord >( player ) );

			// get reference to newly added record.
			LagRecord* current = m_records.front( ).get( );

			// mark as dormant.
			current->m_dormant = true;
		}
	}

	bool update = ( m_records.empty( ) || player->m_flSimulationTime( ) > m_records.front( ).get( )->m_sim_time );

	if ( !update && !player->dormant( ) && player->m_vecOrigin( ) != player->m_vecOldOrigin( ) ) {
		update = true;

		// fix data.
		player->m_flSimulationTime( ) = game::TICKS_TO_TIME( g_csgo.m_cl->m_server_tick );
	}

	// this is the first data update we are receving
	// OR we received data with a newer simulation context.
	if ( update ) {
		// add new record.
		m_records.emplace_front( std::make_shared< LagRecord >( player ) );

		// get reference to newly added record.
		LagRecord* current = m_records.front( ).get( );

		// mark as non dormant.
		current->m_dormant = false;

		// update animations on current record.
		// call resolver.
		UpdateAnimations( current );

		// create bone matrix for this record.
		g_bones.setup( m_player, nullptr, current );

		// generate visual matrix
		//controll_bones::SetupBonesRebuild( player, nullptr, 128, 0x7FF00, player->m_flSimulationTime( ), BoneSetupFlags::None );
		//controll_bones::BuildBones( player, BONE_USED_BY_ANYTHING, BoneSetupFlags::None );

	}

	// no need to store insane amt of data.
	while ( m_records.size( ) > 256 )
		m_records.pop_back( );
}

void AimPlayer::OnRoundStart( Player *player ) {
	m_player = player;
	m_walk_record = LagRecord{ };
	m_shots = 0;
	m_missed_shots = 0;

	if ( settings.resolver_type == 1 ) { // we dont need to reset those if resolver is not set for onetap's
		m_misses_but_who_tf_cares = 0;
		m_anti_fs_angle = 0.f;
		m_move_lby = 0.f;
		m_stand_ticks = 0;
		m_stand_lby = 0.f;
		m_stand_resolving = false;
	}

	// reset stand and body index.
	m_stand_index = 0;
	m_stand_index2 = 0;
	m_body_index = 0;
	m_upd_index = 0;

	/* i took that from pandora.gg v5 */
	m_moved = false; /*
		that resets move check for resolver when round starts so it dont fuck out
	*/

	g_aimbot.m_bFailedHitchance = false; // we want to set this to false

	m_records.clear( );
	m_hitboxes.clear( );

	// IMPORTANT: DO NOT CLEAR LAST HIT SHIT.
}

void AimPlayer::SetupHitboxes( LagRecord *record, bool history ) {
	/*
		i've also took this from l3d, also llama does that for optimizations/performances improvements
	*/
	if ( !record )
		return;

	// reset hitboxes.
	m_hitboxes.clear( );

	if ( g_cl.m_weapon_id == ZEUS ) {
		// hitboxes for the zeus.
		m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );
		return;
	}

	if ( settings.body_aimm ) {
		m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::LETHAL } );

		// prefer, fake.
		if ( record->m_mode != resolve_record::resolve_data::resolver_modes::resolve_none && record->m_mode != resolve_record::resolve_data::resolver_modes::resolve_walk )
			m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );

		// prefer, in air.
		if ( !( record->m_pred_flags & FL_ONGROUND ) )
			m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );

		if ( settings.baim_condition_dt ) {
			if ( TickbaseControll::m_double_tap )
				m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );
		}
	}

	if ( settings.smart_limbs ) {
		const model_t* model = m_player->GetModel( );
		if ( !model )
			return;

		auto hdr = g_csgo.m_model_info->GetStudioModel( model );
		if ( !hdr )
			return;

		auto hitboxSet = hdr->GetHitboxSet( m_player->m_nHitboxSet( ) );
		if ( !hitboxSet )
			return;

		// delay limb
		for ( int i = 0; i < HITBOX_MAX; i++ ) {
			bool bDelayLimb = false;
			auto hitbox = hitboxSet->GetHitbox( i );
			auto bIsLimb = hitbox->m_group == HITGROUP_LEFTARM || hitbox->m_group == HITGROUP_RIGHTARM || hitbox->m_group == HITGROUP_RIGHTLEG || hitbox->m_group == HITGROUP_LEFTLEG;

			if ( bIsLimb ) {
				if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) < 3.25 ) {
					bDelayLimb = true;
				}
				else {
					continue;
				}

				if ( settings.ignor_limbs_when_moving && record->m_velocity.length_2d_sqr( ) > 1.0f ) {
					continue;
				}
			}

		}
	}

	// this is completly coded by me
	auto hitbox_priority = settings.hitbox_prioritize;

	auto speed_type = settings.get_record_speed;
	auto lenght_type = settings.get_2d_speed;
	auto get_player_speed_length = speed_type ? record->m_velocity.length() : m_player->m_vecVelocity( ).length( );
	auto get_player_speed_length_2d = speed_type ? record->m_velocity.length_2d() : m_player->m_vecVelocity( ).length_2d( );
	auto mainly_speed = lenght_type ? get_player_speed_length_2d : get_player_speed_length;

	auto on_ground = record->m_flags & FL_ONGROUND;

	auto standing = on_ground && ( mainly_speed <= 10.0f || record->m_fake_walk );
	auto moving = on_ground && ( mainly_speed > 10.0f && !record->m_fake_walk );
	auto hitbox_is_automatic = hitbox_priority == 3;

	// alpha helped me with this, all credits goes to him
	if ( !g_aimbot.m_force_baim ) {
		if ( hitbox_priority == 1 ) {
			m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::PREFER } );
		}
		else if ( hitbox_is_automatic ) { // we want to go for head if enemy is moving
			if ( standing ) {
				m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::NORMAL } );
				record->hitbox_prioritize_type[ m_player->index( ) ] = "BAIM";
			}
			else if ( moving ) {
				// that should work
				m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::PREFER } );
				record->hitbox_prioritize_type[ m_player->index( ) ] = "HEAD";
			}
			else { // thats in air so fuck it , lets go for baim
				m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::NORMAL } );
				record->hitbox_prioritize_type[ m_player->index( ) ] = "BAIM:R";
			}
		}
		else if ( hitbox_priority != 1 || !hitbox_is_automatic ) {
			m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::NORMAL } );
		}

		// run this down, we dont put prefeer on this
		m_hitboxes.push_back( { HITBOX_UPPER_CHEST, HitscanMode::NORMAL } );
	}

	if ( hitbox_priority == 2 ) {
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );
		m_hitboxes.push_back( { HITBOX_THORAX, HitscanMode::PREFER } );
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
		m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );
	}
	else if ( hitbox_is_automatic ) { // we want to go for head if enemy is moving
		if ( standing ) {
			// we aleardy store the prooritize index fuck it
			m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_THORAX, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );
		}
		else if ( moving ) {
			m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::NORMAL } );
			m_hitboxes.push_back( { HITBOX_THORAX, HitscanMode::NORMAL } );
			m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::NORMAL } );
			m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::NORMAL } );
		}
		else { // thats air index
			m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_THORAX, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
			m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::PREFER } );
		}
	}
	else if ( hitbox_priority != 2 || !hitbox_is_automatic ) {
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_THORAX, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_BODY, HitscanMode::NORMAL } );
	}


	m_hitboxes.push_back( { HITBOX_L_UPPER_ARM, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_R_UPPER_ARM, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_L_THIGH, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_R_THIGH, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_L_CALF, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_R_CALF, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_L_FOOT, HitscanMode::NORMAL } );
	m_hitboxes.push_back( { HITBOX_R_FOOT, HitscanMode::NORMAL } );
}

void Aimbot::init( ) {
	// clear old targets.
	m_targets.clear( );

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max( );
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max( );
	m_best_height = std::numeric_limits< float >::max( );
}

void Aimbot::think( ) {
	// do all startup routines.
	init( );

	// sanity.
	if ( !g_cl.m_weapon )
		return;

	// no grenades or bomb.
	if ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4 )
		return;

	// we have no aimbot enabled.
	if ( !settings.enable )
		return;

	static float last_shoot_time = 0.f;
	if ( ( g_csgo.m_globals->m_realtime - last_shoot_time ) > 0.25f ) { } // you might use this on smth
	
	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;

	// one tick before being able to shoot.
	if ( revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query ) {
		*g_cl.m_packet = false;
		return;
	}

	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	if ( g_cl.m_weapon_fire && !g_cl.m_lag && !revolver ) {
		*g_cl.m_packet = false;
		return;
	}

	// no point in aimbotting if we cannot fire this tick.
	if ( !g_cl.m_weapon_fire )
		return;

	//bool delayshot{};

	// setup bones for all valid targets.
	for ( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player *player = g_csgo.m_entlist->GetClientEntity< Player * >( i );

		if ( !IsValidTarget( player ) )
			continue;

		AimPlayer *data = &m_players[ i - 1 ];
		if ( !data )
			continue;

		//delayshot = ( settings.delay_shot > 0.f && ( g_csgo.m_globals->m_realtime - last_shoot_time ) < ( settings.delay_shot / 1000.f ) ) || Getze::AimbotController::delay_shot( player, g_cl.m_cmd, g_cl.m_weapon );// || cheat::features::aaa.player_resolver_records[entity_index].is_shifting && tick_record <= 0;

		// store player as potential target this tick.
		m_targets.emplace_back( data );
	}

	//if ( settings.delay_shot ) {
	//	if ( delayshot )
		//	return;
	//}

	// run knifebot.
	if ( g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS ) {

		if ( settings.knifebot )
			knife( );

		return;
	}

	// corection.
	if ( settings.aim_corection ) {
		auto local = g_cl.m_local;
		if ( !local )
			return;

		auto punch_angle = local->m_aimPunchAngle( );
		auto recoil_scale = g_csgo.weapon_recoil_scale->GetFloat( );
		auto correction = punch_angle * recoil_scale;

		g_cl.m_cmd->m_view_angles -= correction;
		g_cl.m_cmd->m_view_angles.normalize( );
	}

	if ( !g_cl.m_local || !g_cl.m_local->alive( ) ) {
		g_aimbot.m_bFailedHitchance = false;
		return; // fix memory leak
	}

	// scan available targets... if we even have any.
	find( );

	// finally set data when shooting.
	apply( );

	last_shoot_time = g_csgo.m_globals->m_realtime;
}

void Aimbot::find( ) {
	struct BestTarget_t { Player *player; vec3_t pos; float damage; LagRecord *record; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;

	if ( m_targets.empty( ) )
		return;

	if ( g_cl.m_weapon_id == ZEUS && !settings.zeusbot )
		return;

	// iterate all targets.
	for ( const auto &t : m_targets ) {
		if ( t->m_records.empty( ) )
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if ( settings.lagfix && g_lagcomp.StartPrediction( t ) ) {
			LagRecord *front = t->m_records.front( ).get( );

			t->SetupHitboxes( front, false );
			if ( t->m_hitboxes.empty( ) )
				continue;

			// rip something went wrong..
			if ( t->GetBestAimPosition( tmp_pos, tmp_damage, front ) && SelectTarget( front, tmp_pos, tmp_damage ) ) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = front;
			}
		}

		// player did not break lagcomp.
		// history aim is possible at this point.
		else {
			LagRecord *ideal = resolve_record::resolve_data::find_ideal_record( t );
			if ( !ideal )
				continue;

			t->SetupHitboxes( ideal, false );
			if ( t->m_hitboxes.empty( ) )
				continue;

			// try to select best record as target. -> credits l3d -> i know that doesnt make any sense
			if ( settings.sort_target_for_record ) {
				if ( t->GetBestAimPosition( tmp_pos, tmp_damage, ideal ) /* && SelectTarget( ideal, tmp_pos, tmp_damage ) */ ) {
					if ( g_aimbot.SelectTarget( ideal, tmp_pos, tmp_damage ) ) {
						// if we made it so far, set shit.
						best.player = t->m_player;
						best.pos = tmp_pos;
						best.damage = tmp_damage;
						best.record = ideal;
					}
				}
			}
			else {
				if ( t->GetBestAimPosition( tmp_pos, tmp_damage, ideal ) && SelectTarget( ideal, tmp_pos, tmp_damage ) ) {
					// if we made it so far, set shit.
					best.player = t->m_player;
					best.pos = tmp_pos;
					best.damage = tmp_damage;
					best.record = ideal;
				}
			}

			LagRecord *last = resolve_record::resolve_data::find_last_record( t );
			if ( !last || last == ideal )
				continue;

			t->SetupHitboxes( last, true );
			if ( t->m_hitboxes.empty( ) )
				continue;

			// rip something went wrong..
			if ( t->GetBestAimPosition( tmp_pos, tmp_damage, last ) && SelectTarget( last, tmp_pos, tmp_damage ) ) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last;
			}

			// we found a target we can shoot at and deal damage? fuck yeah. (THIS IS TEMPORARY TILL WE REPLACE THE TARGET SELECTION)
			if ( settings.smart_damage_dealing ) {
				if ( best.damage > 0.f && best.player && best.record )
					break;
			}
		}
	}

	// verify our target and set needed data.
	if ( best.player && best.record ) {
		// calculate aim angle.
		math::VectorAngles( best.pos - g_cl.m_shoot_pos, m_angle );

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;

		// write data, needed for traces / etc.
		m_record->cache( );


		// sanity.
		if ( !g_cl.m_weapon_fire )
			return;

		if ( settings.autostopp ) {
			// set autostop shit.
			m_stop = !( g_cl.m_buttons & IN_JUMP );

			if ( settings.dont_stop_in_air ) {
				switch ( settings.autostop_type ) {
					case 0: {
						g_movement.QuickStop( );
					} break;
					case 1: {
						Getze::FrameController::QuickStopL3D( g_cl.m_cmd );
					} break;
				}

			}
		}

		// alpha credits.
		const bool bOnLand = !( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) && g_cl.m_local->m_fFlags( ) & FL_ONGROUND;
		bool htcFailed = g_aimbot.m_bFailedHitchance;
		if ( htcFailed ) {
			if ( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) {
				if ( settings.autostopp && settings.stop_if_failed_hc && !TickbaseControll::m_double_tap ) {
					switch ( settings.autostop_type ) {
					case 0: {
						g_movement.QuickStop( );
					} break;
					case 1: {
						Getze::FrameController::QuickStopL3D( g_cl.m_cmd );
					} break;
					}
					auto RemoveButtons = [ & ]( int key ) { g_cl.m_cmd->m_buttons &= ~key; };
					RemoveButtons( IN_MOVERIGHT );
					RemoveButtons( IN_MOVELEFT );
					RemoveButtons( IN_FORWARD );
					RemoveButtons( IN_BACK );
				}
			}

			htcFailed = false;
		}

		bool on = settings.hitchance && g_menu.main.config.mode.get( ) == 0;
		bool hit = on && CheckHitchance( m_target, m_angle );

		if ( !g_aimbot.m_hc_override ) {
			auto hc = settings.hitchance_amount;
			if ( hc > 0.0f ) {
				if ( !g_cl.CanFireWeapon( ) )
					return; // fuck it if we cant fire

				if ( !CheckHitchance( m_target, m_angle ) ) {
					g_aimbot.m_bFailedHitchance = true;

					if ( settings.hc_debug )
						notify_controller::g_notify.push_event( "hitchance failed\n", "[debug]" );

					return; // fix memory leak
				} 
			}
		}

		// if we can scope.
		bool can_scope = !g_cl.m_local->m_bIsScoped( ) && ( g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE );

		if ( can_scope ) {
			// always.
			g_cl.m_cmd->m_buttons |= IN_ATTACK2;
		}

		if ( hit || !on ) {
			// right click attack.
			if ( g_menu.main.config.mode.get( ) == 1 && g_cl.m_weapon_id == REVOLVER )
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;

			// left click attack.
			else
				g_cl.m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}

static constexpr auto total_seeds = 256;
void Aimbot::build_seed_table( )
{
	if ( precomputed_seeds.size( ) >= total_seeds )
		return;

	for ( auto i = 0; i < total_seeds; i++ ) {
		g_csgo.RandomSeed( i + 1 );

		const auto pi_seed = g_csgo.RandomFloat( 0.f, 6.2831855f );

		precomputed_seeds.emplace_back( g_csgo.RandomFloat( 0.f, 1.f ),
			sin( pi_seed ), cos( pi_seed ) );
	}
}

bool Aimbot::CheckHitchance( Player *player, const ang_t &angle ) {
	if ( settings.precompilseed ) {
		build_seed_table( );
	}

	constexpr float HITCHANCE_MAX = 100.f;
	int seeds;

	int hitchance_amm;
	if ( g_aimbot.m_hc_override ) {
		hitchance_amm = settings.override_hc;
	}
	else {
		hitchance_amm = settings.hitchance_amount;
	}

	auto trace_low = settings.lower_tracing;
	if ( trace_low ) {
		seeds = 128;
	}
	else {
		seeds = 255;
	}

	auto hitchance_mode = settings.hitchance_mode;
	if ( hitchance_mode == 0 || hitchance_mode == 1 ) {
		vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
		float      inaccuracy, spread;
		CGameTrace tr;
		size_t     total_hits{ }, needed_hits{ ( size_t )std::ceil( ( hitchance_amm * seeds ) / HITCHANCE_MAX ) };

		// get needed directional vectors.
		math::AngleVectors( angle, &fwd, &right, &up );

		// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
		inaccuracy = g_cl.m_weapon->GetInaccuracy( );
		spread = g_cl.m_weapon->GetSpread( );

		if ( settings.automatic_hc_increase ) {
			auto weapon = g_cl.m_weapon;
			if ( !weapon )
				return false;

			if ( weapon->m_iItemDefinitionIndex( ) == SCAR20 || weapon->m_iItemDefinitionIndex( ) == G3SG1 ) {
				if ( ( inaccuracy <= 0.0380f ) && weapon->m_zoomLevel( ) == 0 ) {
					needed_hits = 50.f;
					return true;
				}
			}
		}

		// autowall setup
		penetration::PenetrationInput_t in;

		in.m_damage = 1.f;
		in.m_damage_pen = 1.f;
		in.m_can_pen = true;
		in.m_target = player;
		in.m_from = g_cl.m_local;

		penetration::PenetrationOutput_t out;

		int autowalls_done = 0;
		int autowalls_hit = 0;

		if ( settings.update_accuray ) {
			// update accuracy penalty, alpha does that - llama does that
			g_cl.m_weapon->UpdateAccuracyPenalty( );
		}

		// iterate all possible seeds.
		for ( int i{ }; i <= seeds; ++i ) {
			// get spread.
			wep_spread = g_cl.m_weapon->CalculateSpread( i /* we might sintetize seeds so we have much accuracy, but not now */, inaccuracy, spread );

			// get spread direction.
			dir = ( fwd + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized( );

			// get end of trace.
			end = start + ( dir * g_cl.m_weapon_info->m_range );

			// setup ray and trace.
			if ( hitchance_mode == 0 ) {
				g_csgo.m_engine_trace->ClipRayToEntity( Ray( start, end ), MASK_SHOT, player, &tr );

				// check if we hit a valid player / hitgroup on the player and increment total hits.
				if ( tr.m_entity == player && game::IsValidHitgroup( tr.m_hitgroup ) )
					++total_hits;

				// we made it.
				if ( total_hits >= needed_hits )
					return true;

				// we cant make it anymore.
				if ( ( seeds - i + total_hits ) < needed_hits )
					return false;
			}
			else if ( hitchance_mode == 1 ) {
				// setup ray and trace.
				g_csgo.m_engine_trace->ClipRayToEntity( Ray( start, end ), MASK_SHOT_HULL | CONTENTS_HITBOX, player, &tr );

				// check if we hit a valid player / hitgroup on the player and increment total hits.
				if ( tr.m_entity == player && game::IsValidHitgroup( tr.m_hitgroup ) ) {
					in.m_pos = end;
					++total_hits;

					if ( penetration::run( &in, &out ) && ( i % 2 ) == 1 ) {
						++autowalls_done;
						if ( out.m_damage >= 1.f )
							++autowalls_hit;
					}
				}

				// we cant make it anymore.
				if ( ( seeds - i + total_hits ) < needed_hits )
					break;
			}
		}

		if ( hitchance_mode == 1 ) {
			if ( total_hits >= needed_hits ) {
				if ( autowalls_hit >= ( ( g_menu.main.aimbot.hitchance_amount.get( ) / HITCHANCE_MAX ) * autowalls_done ) )
					return true;
			}
		}

		return false;
	}
	else {
		int traces_hit = 0;
		int awalls_hit = 0;

		CGameTrace tr;
		vec3_t forward, right, up;
		auto eye_position = g_cl.m_local->get_eye_pos( );
		math::AngleVectors( angle/* + cheat::main::local()->m_aimPunchAngle() * 2.f*/, &forward, &right, &up ); // maybe add an option to not account for punch.

		auto weapon = g_cl.m_weapon;
		if ( !weapon || !weapon->GetWpnData( ) )
			return false;


		weapon->UpdateAccuracyPenalty( );
		float weap_spread = weapon->GetSpread( );
		float weap_inaccuracy = weapon->GetInaccuracy( );

		const auto round_acc = [ ]( const float accuracy ) { return roundf( accuracy * 1000.f ) / 1000.f; };
		const auto sniper = weapon->m_iItemDefinitionIndex( ) == AWP || weapon->m_iItemDefinitionIndex( ) == G3SG1 || weapon->m_iItemDefinitionIndex( ) == SCAR20 || weapon->m_iItemDefinitionIndex( ) == SSG08;
		const auto crouched = g_cl.m_local->m_fFlags( ) & FL_DUCKING;

		auto hc = settings.hitchance_amount;
		auto chance = settings.hitchance_amount;

		if ( crouched )
		{
			if ( round_acc( weap_inaccuracy ) == round_acc( sniper ? weapon->GetWpnData( )->m_inaccuracy_crouch_alt : weapon->GetWpnData( )->m_inaccuracy_crouch ) ) {
				hc = 100;
				return true;
			}
		}
		else
		{
			if ( round_acc( weap_inaccuracy ) == round_acc( sniper ? weapon->GetWpnData( )->m_inaccuracy_stand_alt : weapon->GetWpnData( )->m_inaccuracy_stand ) ) {
				hc = 100;
				return true;
			}
		}

		if ( weap_inaccuracy == 0.f )
			return true;

		if ( weapon->m_iItemDefinitionIndex( ) == REVOLVER && g_cl.m_local->m_fFlags( ) & FL_ONGROUND )
			return weap_inaccuracy < ( g_cl.m_local->m_fFlags( )& FL_DUCKING ? .0020f : .0055f );

		if ( precomputed_seeds.empty( ) )
			return false;

		vec3_t total_spread, spread_angle, end;
		float inaccuracy, spread_x, spread_y;
		std::tuple<float, float, float>* seed;

		for ( int i = 0; i < total_seeds; i++ ) {
			seed = &precomputed_seeds[ i ];

			// calculate spread.
			inaccuracy = std::get<0>( *seed ) * weap_inaccuracy;
			spread_x = std::get<2>( *seed ) * inaccuracy;
			spread_y = std::get<1>( *seed ) * inaccuracy;
			total_spread = ( forward + right * spread_x + up * spread_y );

			// calculate angle with spread applied.
			Getze::L3D_Math::VectorAngles( total_spread, spread_angle );
			
			// calculate end point of trace.
			Getze::L3D_Math::AngleVectors( spread_angle, &end );
			end = eye_position + end * 8092.f;

			bool intersect = false;
			if ( tr.m_entity == player && game::IsValidHitgroup( tr.m_hitgroup ) ) {
				++traces_hit;

				if ( settings.accuracy_boost > 1.f )
					++awalls_hit;
			}

			if ( ( ( static_cast< float >( traces_hit ) / static_cast< float >( total_seeds ) ) >= ( chance / 100.f ) ) )
			{
				if ( ( ( static_cast< float >( awalls_hit ) / static_cast< float >( total_seeds ) ) >= ( settings.accuracy_boost / 100.f ) ) || settings.accuracy_boost <= 1.f )
					return true;
			}

			// abort if we can no longer reach hitchance.
			if ( ( ( static_cast< float >( traces_hit + total_seeds - i ) / static_cast< float >( total_seeds ) ) < ( chance / 100.f ) ) )
				return false;
		}

		hc = int( traces_hit * 0.392156863f );
		return ( ( static_cast< float >( traces_hit ) / static_cast< float >( total_seeds ) ) >= ( chance / 100.f ) );
	}
}

bool AimPlayer::SetupHitboxPoints( LagRecord *record, BoneArray *bones, int index, std::vector< vec3_t > &points ) {
	// reset points.
	points.clear( );

	const model_t *model = m_player->GetModel( );
	if ( !model )
		return false;

	studiohdr_t *hdr = g_csgo.m_model_info->GetStudioModel( model );
	if ( !hdr )
		return false;

	mstudiohitboxset_t *set = hdr->GetHitboxSet( m_player->m_nHitboxSet( ) );
	if ( !set )
		return false;

	mstudiobbox_t *bbox = set->GetHitbox( index );
	if ( !bbox )
		return false;

	// get hitbox scales.
	float scale;// = settings.scale / 100.f;
	float bscale;// = settings.body_scale / 100.f;
	
	if ( settings.static_point_scale ) {
		scale = settings.scale / 100.f;
		bscale = settings.body_scale / 100.f;
	}

	// big inair fix.
	if ( settings.in_airFix ) {
		if ( !( record->m_pred_flags ) & FL_ONGROUND )
			scale = 0.7f;

		// this shit is pasted
	}

	// these indexes represent boxes.
	if ( bbox->m_radius <= 0.f ) {
		// references: 
		//      https://developer.valvesoftware.com/wiki/Rotation_Tutorial
		//      CBaseAnimating::GetHitboxBonePosition
		//      CBaseAnimating::DrawServerHitboxes

		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		g_csgo.AngleMatrix( bbox->m_angle, rot_matrix );

		// apply the rotation to the entity input space (local).
		matrix3x4_t matrix;
		math::ConcatTransforms( bones[ bbox->m_bone ], rot_matrix, matrix );

		// extract origin from matrix.
		vec3_t origin = matrix.GetOrigin( );

		// compute raw center point.
		vec3_t center = ( bbox->m_mins + bbox->m_maxs ) / 2.f;

		// the feet hiboxes have a side, heel and the toe.
		if ( index == HITBOX_R_FOOT || index == HITBOX_L_FOOT ) {
			float d1 = ( bbox->m_mins.z - center.z ) * 0.875f;

			// invert.
			if ( index == HITBOX_L_FOOT )
				d1 *= -1.f;

			// side is more optimal then center.
			points.push_back( { center.x, center.y, center.z + d1 } );

			// always on
			float d2 = ( bbox->m_mins.x - center.x ) * scale;
			float d3 = ( bbox->m_maxs.x - center.x ) * scale;

			// heel.
			points.push_back( { center.x + d2, center.y, center.z } );

			// toe.
			points.push_back( { center.x + d3, center.y, center.z } );
		}

		// nothing to do here we are done.
		if ( points.empty( ) )
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for ( auto &p : points ) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot( matrix[ 0 ] ), p.dot( matrix[ 1 ] ), p.dot( matrix[ 2 ] ) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else {
		// factor in the pointscale.
		float r = bbox->m_radius * scale;
		float br = bbox->m_radius * bscale;

		// compute raw center point.
		vec3_t center = ( bbox->m_mins + bbox->m_maxs ) / 2.f;

		if ( !settings.static_point_scale ) {
			scale = 0.91f;

			// transofrm center
			auto transformed_center = center;
			math::VectorTransform( transformed_center, bones[ bbox->m_bone ], transformed_center );

			// main vars
			auto spreadCone = g_cl.m_weapon->GetSpread( ) + g_cl.m_weapon->GetInaccuracy( );
			auto dist = transformed_center.dist_to( g_cl.m_local->GetShootPosition( ) );

			dist /= sinf( math::deg_to_rad( 90.0f - math::rad_to_deg( spreadCone ) ) );
			spreadCone = sinf( spreadCone );

			float radiusScaled = ( bbox->m_radius - ( dist * spreadCone ) );
			if ( radiusScaled < 0.0f ) {
				radiusScaled *= -1.f;
			}

			// point-scale
			float ps = scale;
			scale = ( radiusScaled / bbox->m_radius );
			scale = math::Clamp( scale, 0.0f, ps );

			bscale = scale;
		}


		// head has 5 points.
		if ( index == HITBOX_HEAD ) {
			// add center.
			points.push_back( center );
			constexpr float rotation = 0.70710678f;

			// top/back 45 deg.
			// this is the best spot to shoot at.
			points.push_back( { bbox->m_maxs.x + ( rotation * r ), bbox->m_maxs.y + ( -rotation * r ), bbox->m_maxs.z } );

			// right.
			points.push_back( { bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r } );

			// left.
			points.push_back( { bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r } );

			// back.
			points.push_back( { bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z } );

			// get animstate ptr.
			CCSGOPlayerAnimState* state = record->m_player->m_PlayerAnimState( );

			// add this point only under really specific circumstances.
			// if we are standing still and have the lowest possible pitch pose.
			if ( state && record->m_anim_velocity.length( ) <= 0.1f && record->m_eye_angles.x <= state->m_min_pitch ) {

				// bottom point.
				points.push_back( { bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z } );
			}
		}

		// body has 5 points.
		else if ( index == HITBOX_BODY ) {
			// center.
			points.push_back( center );

			// back.
			points.push_back( { center.x, bbox->m_maxs.y - br, center.z } );
		}

		else if ( index == HITBOX_PELVIS || index == HITBOX_UPPER_CHEST ) {
			// back.
			points.push_back( { center.x, bbox->m_maxs.y - r, center.z } );
		}

		// other stomach/chest hitboxes have 2 points.
		else if ( index == HITBOX_THORAX || index == HITBOX_CHEST ) {
			// add center.
			points.push_back( center );

			// add extra point on back.
			points.push_back( { center.x, bbox->m_maxs.y - r, center.z } );
		}

		else if ( index == HITBOX_R_CALF || index == HITBOX_L_CALF ) {
			// add center.
			points.push_back( center );

			// half bottom.
			points.push_back( { bbox->m_maxs.x - ( bbox->m_radius / 2.f ), bbox->m_maxs.y, bbox->m_maxs.z } );
		}

		else if ( index == HITBOX_R_THIGH || index == HITBOX_L_THIGH ) {
			// add center.
			points.push_back( center );
		}

		// arms get only one point.
		else if ( index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM ) {
			// elbow.
			points.push_back( { bbox->m_maxs.x + bbox->m_radius, center.y, center.z } );
		}

		// nothing left to do here.
		if ( points.empty( ) )
			return false;

		// transform capsule points.
		for ( auto &p : points )
			math::VectorTransform( p, bones[ bbox->m_bone ], p );
	}

	return true;
}

bool AimPlayer::GetBestAimPosition( vec3_t &aim, float &damage, LagRecord *record ) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	// thx alpha
	float percentage = 0.34;
	float v4 = percentage * ( g_cl.m_local->m_bIsScoped( ) ? g_cl.m_weapon_info->m_max_player_speed_alt : g_cl.m_weapon_info->m_max_player_speed );

	if ( settings.wait_for_accurate_shot ) {
		if ( g_cl.m_local->m_vecVelocity( ).length( ) > v4 ) {
			return false;
		}
	}

	if ( settings.delay_shot_on_unduck && g_cl.m_local->m_flDuckAmount( ) >= 0.125f ) {
		if ( g_cl.m_flPreviousDuckAmount > g_cl.m_local->m_flDuckAmount( ) ) {
			return false;
		}
	}

	// get player hp.
	int hp = std::min( 100, m_player->m_iHealth( ) );

	if ( g_cl.m_weapon_id == ZEUS ) {
		dmg = pendmg = hp;
		pen = true; // please use brain and fix that its ez bro, just think why
	}

	else {
		/*
			i took this from l3d's 2k17 src, even llama does that, its way more accurate		
		*/
		dmg = std::fmin( hp, settings.minimal_damage );
		pendmg = std::fmin( hp, settings.penetrate_minimal_damage ); 

		if ( g_aimbot.m_damage_override )
			dmg = pendmg = settings.overriden_dmg;

		pen = true;
	}

	// write all data of this record l0l.
	record->cache( );

	// iterate hitboxes.
	for ( const auto &it : m_hitboxes ) {
		done = false;

		// setup points on hitbox.
		if ( !SetupHitboxPoints( record, record->m_bones, it.m_index, points ) )
			continue;

		// iterate points on hitbox.
		for ( const auto &point : points ) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = pen;
			in.m_target = m_player;
			in.m_from = g_cl.m_local;
			in.m_pos = point;

			// ignore mindmg.
			if ( it.m_mode == HitscanMode::LETHAL || it.m_mode == HitscanMode::LETHAL2 )
				in.m_damage = in.m_damage_pen = 1.f;

			penetration::PenetrationOutput_t out;

			// we can hit p!
			if ( penetration::run( &in, &out ) ) {

				// nope we did not hit head..
				if ( it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD )
					continue;

				// prefered hitbox, just stop now.
				if ( it.m_mode == HitscanMode::PREFER )
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if ( it.m_mode == HitscanMode::LETHAL && out.m_damage >= m_player->m_iHealth( ) )
					done = true;

				// 2 shots will be sufficient to kill.
				else if ( it.m_mode == HitscanMode::LETHAL2 && ( out.m_damage * 2.f ) >= m_player->m_iHealth( ) )
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if ( it.m_mode == HitscanMode::NORMAL ) {
					// we did more damage.
					if ( out.m_damage > scan.m_damage ) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point;

						// if the first point is lethal
						// screw the other ones.
						if ( point == points.front( ) && out.m_damage >= m_player->m_iHealth( ) )
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if ( done ) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if ( done )
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if ( scan.m_damage > 0.f ) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		return true;
	}

	return false;
}

bool Aimbot::SelectTarget( LagRecord *record, const vec3_t &aim, float damage ) {
	float dist, fov, height;
	int   hp;

	// fov check.
	if ( settings.fov ) {
		// if out of fov, retn false.
		if ( math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, aim ) > settings.fov_amount )
			return false;
	}

	switch ( settings.selection ) {

		// distance.
	case 0:
		dist = ( record->m_pred_origin - g_cl.m_shoot_pos ).length( );

		if ( dist < m_best_dist ) {
			m_best_dist = dist;
			return true;
		}

		break;

		// crosshair.
	case 1:
		fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, aim );

		if ( fov < m_best_fov ) {
			m_best_fov = fov;
			return true;
		}

		break;

		// damage.
	case 2:
		if ( damage > m_best_damage ) {
			m_best_damage = damage;
			return true;
		}

		break;

		// lowest hp.
	case 3:
		// fix for retarded servers?
		hp = std::min( 100, record->m_player->m_iHealth( ) );

		if ( hp < m_best_hp ) {
			m_best_hp = hp;
			return true;
		}

		break;

		// least lag.
	case 4:
		if ( record->m_lag < m_best_lag ) {
			m_best_lag = record->m_lag;
			return true;
		}

		break;

		// height.
	case 5:
		height = record->m_pred_origin.z - g_cl.m_local->m_vecOrigin( ).z;

		if ( height < m_best_height ) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

void Aimbot::apply( ) {
	bool attack, attack2;

	// attack states.
	attack = ( g_cl.m_cmd->m_buttons & IN_ATTACK );
	attack2 = ( g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2 );

	// ensure we're attacking.
	if ( attack || attack2 ) {
		// choke every shot.
		*g_cl.m_packet = false;

		if ( m_target ) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if ( m_record && !m_record->m_broke_lc )
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS( m_record->m_sim_time + g_cl.m_lerp );

			// set angles to target.
			g_cl.m_cmd->m_view_angles = m_angle;

			// if not silent aim, apply the viewangles.
			if ( !settings.silent )
				g_csgo.m_engine->SetViewAngles( m_angle );

			if ( settings.clientt_impact ) {
				CGameTrace trace, trace2;
				CTraceFilterSimple filter, filter2;
				filter.SetPassEntity( g_cl.m_local );

				g_csgo.m_engine_trace->TraceRay( Ray{ g_cl.m_shoot_pos, m_aim }, 0x46004003, &filter, &trace );
				g_csgo.m_debug_overlay->AddBoxOverlay( trace.m_endpos, vec3_t( -2.f, -2.f, -2.f ), vec3_t( 2.f, 2.f, 2.f ), ang_t( 0.f, 0.f, 0.f ), 255, 0, 0, 127, 3.f );
				if ( ( trace.m_endpos - m_aim ).length_2d( ) > 10.f )
					g_csgo.m_debug_overlay->AddBoxOverlay( m_aim, vec3_t( -2.f, -2.f, -2.f ), vec3_t( 2.f, 2.f, 2.f ), ang_t( 0.f, 0.f, 0.f ), 255, 0, 0, 127, 3.f );
			}

			g_visuals.DrawHitboxMatrix( m_record, colors::white, 3.f ); // fix lag
		}

		// norecoil.
		if ( settings.norecoil )
			g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( );

		// store fired shot.
		g_shots.OnShotFire( m_target ? m_target : nullptr, m_target ? m_damage : -1.f, g_cl.m_weapon_info->m_bullets, m_target ? m_record : nullptr );

		// set that we fired.
		g_cl.m_shot = true;
	}
}

void Aimbot::NoSpread( ) {
	bool    attack2;
	vec3_t  spread, forward, right, up, dir;

	// revolver state.
	attack2 = ( g_cl.m_weapon_id == REVOLVER && ( g_cl.m_cmd->m_buttons & IN_ATTACK2 ) );

	// get spread.
	spread = g_cl.m_weapon->CalculateSpread( g_cl.m_cmd->m_random_seed, attack2 );

	// compensate.
	g_cl.m_cmd->m_view_angles -= { -math::rad_to_deg( std::atan( spread.length_2d( ) ) ), 0.f, math::rad_to_deg( std::atan2( spread.x, spread.y ) ) };
}