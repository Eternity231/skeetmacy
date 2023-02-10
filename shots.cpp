#include "includes.h"

Shots g_shots{ };

void Shots::OnShotFire( Player *target, float damage, int bullets, LagRecord *record ) {

	// iterate all bullets in this shot.
	for ( int i{ }; i < bullets; ++i ) {
		// setup new shot data.
		ShotRecord shot;
		shot.m_target = target;
		shot.m_record = record;
		shot.m_time = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );
		shot.m_lat = g_cl.m_latency;
		shot.m_damage = damage;
		shot.m_pos = g_cl.m_local->GetShootPosition( );

		// we are not shooting manually.
		// and this is the first bullet, only do this once.
		if ( target && i == 0 ) {
			// increment total shots on this player.
			AimPlayer *data = &g_aimbot.m_players[ target->index( ) - 1 ];
			if ( data )
				++data->m_shots;
		}

		// add to tracks.
		m_shots.push_front( shot );
	}

	// no need to keep an insane amount of shots.
	while ( m_shots.size( ) > 128 )
		m_shots.pop_back( );
}
#include "backend/config/config_new.h"
void Shots::OnImpact( IGameEvent *evt ) {
	int        attacker;
	vec3_t     pos, dir, start, end;
	float      time;
	CGameTrace trace;

	// screw this.
	if ( !evt || !g_cl.m_local )
		return;

	// get attacker, if its not us, screw it.
	attacker = g_csgo.m_engine->GetPlayerForUserID( evt->m_keys->FindKey( HASH( "userid" ) )->GetInt( ) );
	if ( attacker != g_csgo.m_engine->GetLocalPlayer( ) )
		return;

	// decode impact coordinates and convert to vec3.
	pos = {
		evt->m_keys->FindKey( HASH( "x" ) )->GetFloat( ),
		evt->m_keys->FindKey( HASH( "y" ) )->GetFloat( ),
		evt->m_keys->FindKey( HASH( "z" ) )->GetFloat( )
	};

	// get prediction time at this point.
	time = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );

	// add to visual impacts if we have features that rely on it enabled.
	// todo - dex; need to match shots for this to have proper GetShootPosition, don't really care to do it anymore.
	if ( settings.bullet_tracers )
		m_vis_impacts.push_back( { pos, g_cl.m_local->GetShootPosition( ), g_cl.m_local->m_nTickBase( ) } );

	// we did not take a shot yet.
	if ( m_shots.empty( ) )
		return;

	struct ShotMatch_t { float delta; ShotRecord *shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max( );
	match.shot = nullptr;

	// iterate all shots.
	for ( auto &s : m_shots ) {

		// this shot was already matched
		// with a 'bullet_impact' event.
		if ( s.m_matched )
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs( time - predicted );

		// fuck this.
		if ( delta > 1.f )
			continue;

		// store this shot as being the best for now.
		if ( delta < match.delta ) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord *shot = match.shot;
	if ( !shot )
		return;

	// this shot was matched.
	shot->m_matched = true;

	// create new impact instance that we can match with a player hurt.
	ImpactRecord impact;
	impact.m_shot = shot;
	impact.m_tick = g_cl.m_local->m_nTickBase( );
	impact.m_pos = pos;

	//g_cl.print( "imp %x time: %f lat: %f dmg: %f\n", shot->m_record, shot->m_time, shot->m_lat, shot->m_damage );

	// add to track.
	m_impacts.push_front( impact );

	// no need to keep an insane amount of impacts.
	while ( m_impacts.size( ) > 128 )
		m_impacts.pop_back( );

	// nospread mode.
	if ( g_menu.main.config.mode.get( ) == 1 )
		return;

	// not in nospread mode, see if the shot missed due to spread.
	Player *target = shot->m_target;
	if ( !target )
		return;

	// not gonna bother anymore.
	if ( !target->alive( ) )
		return;

	AimPlayer *data = &g_aimbot.m_players[ target->index( ) - 1 ];
	if ( !data )
		return;

	// this record was deleted already.
	if ( !shot->m_record->m_bones )
		return;

	// we are going to alter this player.
	// store all his og data.
	BackupRecord backup;
	backup.store( target );

	// write historical matrix of the time that we shot
	// into the games bone cache, so we can trace against it.
	shot->m_record->cache( );

	// start position of trace is where we took the shot.
	start = shot->m_pos;

	// the impact pos contains the spread from the server
	// which is generated with the server seed, so this is where the bullet
	// actually went, compute the direction of this from where the shot landed
	// and from where we actually took the shot.
	dir = ( pos - start ).normalized( );

	// get end pos by extending direction forward.
	// todo; to do this properly should save the weapon range at the moment of the shot, cba..
	end = start + ( dir * 8192.f );

	// intersect our historical matrix with the path the shot took.
	g_csgo.m_engine_trace->ClipRayToEntity( Ray( start, end ), MASK_SHOT, target, &trace );

	// we did not hit jackshit, or someone else.
	if ( !trace.m_entity || !trace.m_entity->IsPlayer( ) || trace.m_entity != target )
		notify_controller::g_notify.push_event( XOR( "shot missed due to spread\n" ) );

	// we should have 100% hit this player..
	// this is a miss due to wrong angles.
	else if ( trace.m_entity == target ) {
		size_t mode = shot->m_record->m_mode;

		// if we miss a shot on body update.
		// we can chose to stop shooting at them.
		if ( mode == resolve_record::resolve_data::resolver_modes::resolve_body ) {
			//++data->m_body_index;
			data->m_upd_index++;
		}

		//else if ( mode == Resolver::Modes::RESOLVE_DELTA )
		//{
		//	data->m_delta_index++;
		//}

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_last_move ) {
			data->m_last_move++;
		}

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_unknown ) {
			data->m_unknown_move++;
		}

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_stand ) {
			data->m_stand_index++;
		}

		//else if ( mode == Resolver::Modes::RESOLVE_STAND2 ) {
		//	data->m_stand_index2++;
		//}

		++data->m_missed_shots;
	}

	// restore player to his original state.
	backup.restore( target );
}

template <typename T>
std::string to_string_with_precision( const T a_value, const int n = 3 )
{
	std::ostringstream out;
	out.precision( n );
	out << std::fixed << a_value;
	return out.str( );
}


void Shots::OnHurt( IGameEvent *evt ) {
	int         attacker, victim, group, hp;
	float       damage;
	std::string name;

	if ( !evt || !g_cl.m_local )
		return;

	attacker = g_csgo.m_engine->GetPlayerForUserID( evt->m_keys->FindKey( HASH( "attacker" ) )->GetInt( ) );
	victim = g_csgo.m_engine->GetPlayerForUserID( evt->m_keys->FindKey( HASH( "userid" ) )->GetInt( ) );

	// skip invalid player indexes.
	// should never happen? world entity could be attacker, or a nade that hits you.
	if ( attacker < 1 || attacker > 64 || victim < 1 || victim > 64 )
		return;

	// we were not the attacker or we hurt ourselves.
	else if ( attacker != g_csgo.m_engine->GetLocalPlayer( ) || victim == g_csgo.m_engine->GetLocalPlayer( ) )
		return;

	// get hitgroup.
	// players that get naded ( DMG_BLAST ) or stabbed seem to be put as HITGROUP_GENERIC.
	group = evt->m_keys->FindKey( HASH( "hitgroup" ) )->GetInt( );

	// invalid hitgroups ( note - dex; HITGROUP_GEAR isn't really invalid, seems to be set for hands and stuff? ).
	if ( group == HITGROUP_GEAR )
		return;

	// get the player that was hurt.
	Player *target = g_csgo.m_entlist->GetClientEntity< Player * >( victim );
	if ( !target )
		return;

	// get player info.
	player_info_t info;
	if ( !g_csgo.m_engine->GetPlayerInfo( victim, &info ) )
		return;

	// get player name;
	name = std::string( info.m_name ).substr( 0, 24 );

	// get damage reported by the server.
	damage = ( float )evt->m_keys->FindKey( HASH( "dmg_health" ) )->GetInt( );

	// get remaining hp.
	hp = evt->m_keys->FindKey( HASH( "health" ) )->GetInt( );

	// hitmarker.
	if ( settings.hitmarker ) {
		g_visuals.m_hit_duration = 1.f;
		g_visuals.m_hit_start = g_csgo.m_globals->m_curtime;
		g_visuals.m_hit_end = g_visuals.m_hit_start + g_visuals.m_hit_duration;

		g_csgo.m_sound->EmitAmbientSound( XOR( "buttons/arena_switch_press_02.wav" ), 1.f );
	}

	// print this shit.
	if ( settings.damagelog ) {
		std::string l3dzeus;
		if ( settings.getzezuz_hitflag )
			l3dzeus = tfm::format( XOR( "Hurt %s in the %s for %i %s has %i health remaining\n" ), name, m_groups[ group ], ( int )damage, name, hp );
		else
			l3dzeus = tfm::format( XOR( "hit %s in the %s for %i (%i remaining)\n" ), name, m_groups[ group ], ( int )damage, hp );

		std::string out = l3dzeus;
		notify_controller::g_notify.push_event( out, "[hit] " );
	}

	if ( group == HITGROUP_GENERIC )
		return;

	// if we hit a player, mark vis impacts.
	if ( !m_vis_impacts.empty( ) ) {
		for ( auto &i : m_vis_impacts ) {
			if ( i.m_tickbase == g_cl.m_local->m_nTickBase( ) )
				i.m_hit_player = true;
		}
	}

	// no impacts to match.
	if ( m_impacts.empty( ) )
		return;

	ImpactRecord *impact{ nullptr };

	// iterate stored impacts.
	for ( auto &i : m_impacts ) {

		// this impact doesnt match with our current hit.
		if ( i.m_tick != g_cl.m_local->m_nTickBase( ) )
			continue;

		// wrong player.
		if ( i.m_shot->m_target != target )
			continue;

		// shit fond.
		impact = &i;
		break;
	}

	// no impact matched.
	if ( !impact )
		return;

	// setup new data for hit track and push to hit track.
	HitRecord hit;
	hit.m_impact = impact;
	hit.m_group = group;
	hit.m_damage = damage;


	m_hits.push_front( hit );

	while ( m_hits.size( ) > 128 )
		m_hits.pop_back( );

	AimPlayer *data = &g_aimbot.m_players[ target->index( ) - 1 ];
	if ( !data )
		return;

	if ( settings.debug_logs ) {
		std::string bar = " | "; // this has aleardy space dont put it again
		std::string debug = "[fire-debug] ";
		std::string hitbox = "hitbox: ";
		std::string predicted_hitbox = "pred hitbox: ";
		std::string dmg = "dmg: ";
		std::string predicted_dmg = "pred dmg: ";
		std::string lby = "lby: ";
		std::string velocity = "vel: ";
		std::string resolver = "r: ";
		std::string hpr = "hitbox priority: ";
		std::string miss1 = "lby_i: ";
		std::string miss2 = "last_m: ";
		std::string miss3 = "unknown_m: ";
		std::string miss4 = "stand_i: ";

		std::string out_debug;
		out_debug += debug;
		out_debug += "ent: " + name + bar;
		out_debug += hitbox + m_groups[ group ] + bar;
		//out_debug += predicted_hitbox + std::to_string( impact->m_shot->m_hitbox ) + bar; -> we will need this after we modify shot_informations
		out_debug += dmg + std::to_string( ( int )damage ) + bar;
		out_debug += predicted_dmg + std::to_string( ( int )impact->m_shot->m_damage ) + bar;
		out_debug += lby + std::to_string( ( int )impact->m_shot->m_record->m_body ) + bar;
		out_debug += velocity + to_string_with_precision( impact->m_shot->m_record->m_velocity.length( ), 1 ) + bar; // we might not need + bar there

		LagRecord* current = data->m_records.front( ).get( );
		out_debug += miss1 + std::to_string( data->m_upd_index ) + bar;
		out_debug += miss2 + std::to_string( data->m_last_move ) + bar;
		out_debug += miss3 + std::to_string( data->m_unknown_move ) + bar;
		out_debug += miss4 + std::to_string( data->m_stand_index ) + bar;  


		if ( data && data->m_records.size( ) ) {
			if ( current ) {
				out_debug += resolver + current->resolver_text[current->m_player->index( ) ] + bar;

				if ( settings.hitbox_prioritize == 3 )
					out_debug += hpr + current->hitbox_prioritize_type[ current->m_player->index( ) ];
				else
					out_debug += hpr + "normal";
			}
		}
		
		if ( !settings.dont_print )
			notify_controller::g_notify.push_event( out_debug + "\n" ); // now it gets on a new line
		else
			g_cl.print( out_debug + "\n" );
	}

	if ( settings.event_mode_type == 0 ) {
		// we hit, reset missed shots counter.
		data->m_missed_shots = 0;

		size_t mode = impact->m_shot->m_record->m_mode;

		if ( mode == resolve_record::resolve_data::resolver_modes::resolve_body && data->m_upd_index > 0 )
			--data->m_upd_index;

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_last_move && data->m_last_move > 0 )
			--data->m_last_move;

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_unknown && data->m_unknown_move > 0 )
			--data->m_unknown_move;

		else if ( mode == resolve_record::resolve_data::resolver_modes::resolve_stand && data->m_stand_index > 0 )
			--data->m_stand_index;
	}
}