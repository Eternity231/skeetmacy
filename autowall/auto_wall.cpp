
#include "../includes.h"

pen_data_t c_auto_wall::get( const vec3_t& shoot_pos, const vec3_t& point, const Player* enemy ) {
	const auto who_tf = point - shoot_pos;
	ang_t angle = {};
	math::VectorAngles( who_tf, angle );

	vec3_t dir = {};
	math::AngleVectors( angle, &dir );

	dir.normalize( );

	auto dmg{ 0.f };
	auto hitbox{ -1 };
	auto remain_pen{ -1 };
	auto hit_group{ -1 };
	bool dont_care{};
	if ( fire_bullet( g_cl.m_local->GetActiveWeapon ( ), dir, dont_care, dmg, remain_pen, hit_group, hitbox, ( Entity* )enemy, 0.0f, shoot_pos ) )
		return pen_data_t( static_cast < int > ( dmg ), hitbox, hit_group, remain_pen );
	else
		return pen_data_t( -1, -1, -1, -1 );
}

__forceinline bool is_armored( Player* player, int hit_group ) {
	const bool has_helmet = player->m_bHasHelmet( );
	const bool has_heavy_armor = player->m_bHasHeavyArmor( );
	const float armor_val = player->m_ArmorValue( );

	if ( armor_val > 0.f ) {
		switch ( hit_group ) {
		case 2:
		case 3:
		case 4:
		case 5:
			return true;
			break;
		case 1:
			return has_helmet || has_heavy_armor;
			break;
		default:
			return has_heavy_armor;
			break;
		}
	}

	return false;
}


void c_auto_wall::scale_dmg( Player* player, CGameTrace& trace, WeaponInfo* wpn_info, float& dmg, const int hit_group ) {
	if ( !player->IsPlayer( ) )
		return;

	const auto team = player->m_iTeamNum( );

	static auto mp_dmg_scale_ct_head = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_ct_head" ) );
	static auto mp_dmg_scale_t_head = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_t_head" ) );

	static auto mp_dmg_scale_ct_body = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_ct_body" ) );
	static auto mp_dmg_scale_t_body = g_csgo.m_cvar->FindVar( HASH( "mp_damage_scale_t_body" ) );

	float head_dmg_scale = team == 3 ? mp_dmg_scale_ct_head->GetFloat( ) : mp_dmg_scale_t_head->GetFloat( );
	const float body_dmg_scale = 3 ? mp_dmg_scale_ct_body->GetFloat( ) : mp_dmg_scale_t_body->GetFloat( );

	const auto armored = is_armored( player, hit_group );
	const bool has_heavy_armor = player->m_bHasHeavyArmor( );
	const bool is_zeus = g_cl.m_local->GetActiveWeapon ( ) ? g_cl.m_local->GetActiveWeapon( )->m_iItemDefinitionIndex( ) == ZEUS : false;

	const auto armor_val = static_cast < float > ( player->m_ArmorValue( ) );

	if ( has_heavy_armor )
		head_dmg_scale *= 0.5f;

	if ( !is_zeus ) {
		switch ( hit_group ) {
		case 1:
			dmg = ( dmg * 4.f ) * head_dmg_scale;
			break;
		case 3:
			dmg = ( dmg * 1.25f ) * body_dmg_scale;
			break;
		case 6:
		case 7:
			dmg = ( dmg * 0.75f ) * body_dmg_scale;
			break;
		default:
			break;
		}
	}

	if ( !g_cl.m_local
		|| !g_cl.m_local->GetActiveWeapon( )
		|| !g_cl.m_local->GetActiveWeapon( )->GetWpnData( ) )
		return;

	const auto armor_ratio = g_cl.m_local->GetActiveWeapon( )->GetWpnData( )->m_armor_ratio;

	if ( armored ) {
		float armor_scale = 1.f;
		float armor_bonus_ratio = 0.5f;
		float armor_ratio_calced = armor_ratio * 0.5f;
		float dmg_to_health = 0.f;

		if ( has_heavy_armor ) {
			armor_ratio_calced = armor_ratio * 0.25f;
			armor_bonus_ratio = 0.33f;
			armor_scale = 0.33f;

			dmg_to_health = ( dmg * armor_ratio_calced ) * 0.85f;
		}
		else
			dmg_to_health = dmg * armor_ratio_calced;

		float dmg_to_armor = ( dmg - dmg_to_health ) * ( armor_scale * armor_bonus_ratio );

		if ( dmg_to_armor > armor_val )
			dmg_to_health = dmg - ( armor_val / armor_bonus_ratio );

		dmg = dmg_to_health;
	}

	dmg = std::floor( dmg );
}

bool c_auto_wall::handle_bullet_penetration(
	WeaponInfo* wpn_data, CGameTrace& enter_trace, vec3_t& eye_pos, const vec3_t& direction, int& possible_hits_remain,
	float& cur_dmg, float penetration_power, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration
) {
	if ( wpn_data->m_penetration <= 0.0f )
		return false;

	if ( possible_hits_remain <= 0 )
		return false;

	auto contents_grate = enter_trace.m_contents & CONTENTS_GRATE;
	auto surf_nodraw = enter_trace.m_surface.m_flags & SURF_NODRAW;

	auto enter_surf_data = g_csgo.m_phys_props->GetSurfaceData( enter_trace.m_surface.m_surface_props );
	auto enter_material = enter_surf_data->m_game.m_material;

	auto is_solid_surf = enter_trace.m_contents >> 3 & CONTENTS_SOLID;
	auto is_light_surf = enter_trace.m_surface.m_flags >> 7 & 0x0001;

	CGameTrace exit_trace;

	if ( !trace_to_exit( enter_trace.m_endpos, direction, enter_trace, exit_trace )
		&& !( g_csgo.m_engine_trace->GetPointContents( enter_trace.m_endpos, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) )
		return false;

	auto enter_penetration_modifier = enter_surf_data->m_game.m_penetration_modifier;
	auto exit_surface_data = g_csgo.m_phys_props->GetSurfaceData( exit_trace.m_surface.m_surface_props );

	if ( !exit_surface_data )
		return false;

	auto exit_material = exit_surface_data->m_game.m_material;
	auto exit_penetration_modifier = exit_surface_data->m_game.m_penetration_modifier;

	auto combined_damage_modifier = 0.16f;
	auto combined_penetration_modifier = ( enter_penetration_modifier + exit_penetration_modifier ) * 0.5f;

	if ( enter_material == 'Y' || enter_material == 'G' )
	{
		combined_penetration_modifier = 3.0f;
		combined_damage_modifier = 0.05f;
	}
	else if ( contents_grate || surf_nodraw )
		combined_penetration_modifier = 1.0f;
	else if ( enter_material == 'F' && ( ( Player* ) enter_trace.m_entity )->m_iTeamNum( ) == g_cl.m_local->m_iTeamNum( ) && ff_damage_reduction_bullets )
	{
		if ( !ff_damage_bullet_penetration )
			return false;

		combined_penetration_modifier = ff_damage_bullet_penetration;
		combined_damage_modifier = 0.16f;
	}

	if ( enter_material == exit_material )
	{
		if ( exit_material == 'W' || exit_material == 'U' )
			combined_penetration_modifier = 3.0f;
		else if ( exit_material == 'L' )
			combined_penetration_modifier = 2.0f;
	}

	auto penetration_modifier = std::fmaxf( 0.0f, 1.0f / combined_penetration_modifier );
	auto penetration_distance = ( exit_trace.m_endpos - enter_trace.m_endpos ).length( );

	penetration_distance = penetration_distance * penetration_distance * penetration_modifier * 0.041666668f;

	auto damage_modifier = std::max( 0.0f, 3.0f / wpn_data->m_penetration * 1.25f ) * penetration_modifier * 3.0f + cur_dmg * combined_damage_modifier + penetration_distance;
	auto damage_lost = std::max( 0.0f, damage_modifier );

	if ( damage_lost > cur_dmg )
		return false;

	cur_dmg -= damage_lost;

	if ( cur_dmg < 1.0f )
		return false;

	eye_pos = exit_trace.m_endpos;
	--possible_hits_remain;

	return true;
}


bool c_auto_wall::fire_bullet( Weapon* wpn, vec3_t& direction, bool& visible, float& cur_dmg, int& remaining_pen, int& hit_group,
	int& hitbox, Entity* entity, float length, const vec3_t& pos )
{
	static CTraceFilterSkipTwoEntities_game filter{};
	if ( !wpn )
		return false;

	auto wpn_data = wpn->GetWpnData( );

	if ( !wpn_data )
		return false;

	CGameTrace enter_trace;

	cur_dmg = ( float ) wpn_data->m_damage;

	auto eye_pos = pos;
	auto cur_dist = 0.0f;
	auto max_range = wpn_data->m_range;
	auto pen_dist = 3000.0f;
	auto pen_power = wpn_data->m_penetration;
	auto possible_hit_remain = 4;
	remaining_pen = 4;
	filter.SetPassEntity( g_cl.m_local );
	filter.SetPassEntity2( nullptr );
	while ( cur_dmg > 0.f )
	{
		max_range -= cur_dist;
		auto end = eye_pos + direction * max_range;

		g_csgo.m_engine_trace->TraceRay( Ray( eye_pos, end ), MASK_SHOT, ( ITraceFilter* ) &filter, &enter_trace );
	
		if ( entity )
			penetration::ClipTraceToPlayer( eye_pos, end + ( direction * 40.f ), MASK_SHOT, &enter_trace, ( Player* )entity, -60.f );

		auto enter_surf_data = g_csgo.m_phys_props->GetSurfaceData( enter_trace.m_surface.m_surface_props );
		auto enter_surf_pen_mod = enter_surf_data->m_game.m_penetration_modifier;
		auto enter_mat = enter_surf_data->m_game.m_material;

		if ( enter_trace.m_fraction == 1.0f )
			break;

		cur_dist += enter_trace.m_fraction * max_range;
		cur_dmg *= pow( wpn_data->m_range_modifier, cur_dist / 500.0f );

		auto hit_player = static_cast < Player* > ( enter_trace.m_entity );

		if ( cur_dist > pen_dist && wpn_data->m_penetration || enter_surf_pen_mod < 0.1f )
			break;

		auto can_do_dmg = enter_trace.m_hitgroup != HITGROUP_GEAR && enter_trace.m_hitgroup != HITGROUP_GENERIC;
		auto is_player = ( ( Player* ) enter_trace.m_entity )->IsPlayer( );
		auto is_enemy = ( ( Player* ) enter_trace.m_entity )->m_iTeamNum( ) != g_cl.m_local->m_iTeamNum ( );

		if ( can_do_dmg
			&& is_player
			&& is_enemy
			&& hit_player
			&& hit_player->IsPlayer( ) )
		{
			scale_dmg( hit_player, enter_trace, wpn_data, cur_dmg, static_cast < std::ptrdiff_t > ( enter_trace.m_hitgroup ) );
			hitbox = static_cast < int > ( enter_trace.m_hitbox );
			hit_group = static_cast< int > ( enter_trace.m_hitgroup );
			return true;
		}

		if ( !possible_hit_remain )
			break;

		static auto dmg_reduction_bullets = g_csgo.m_cvar->FindVar( HASH( "ff_damage_reduction_bullets" ) );
		static auto dmg_bullet_pen = g_csgo.m_cvar->FindVar( HASH( "ff_damage_bullet_penetration" ) );

		if ( !handle_bullet_penetration( wpn_data, enter_trace, eye_pos, direction,
			possible_hit_remain, cur_dmg, pen_power, dmg_reduction_bullets->GetFloat( ), dmg_bullet_pen->GetFloat( ) ) ) {
			remaining_pen = possible_hit_remain;
			break;
		}

		remaining_pen = possible_hit_remain;

		visible = false;
	}

	return false;
}

bool c_auto_wall::trace_to_exit(
	const vec3_t& src, const vec3_t& dir,
	const CGameTrace& enter_trace, CGameTrace& exit_trace )
{
	static CTraceFilterSimple_game filter{};
	float dist{};
	int first_contents{};

	constexpr auto k_step_size = 4.f;
	constexpr auto k_max_dist = 90.f;

	while ( dist <= k_max_dist ) {
		dist += k_step_size;

		const auto out = src + ( dir * dist );

		const auto cur_contents = g_csgo.m_engine_trace->GetPointContents( out, MASK_SHOT | CONTENTS_HITBOX );

		if ( !first_contents )
			first_contents = cur_contents;

		if ( cur_contents & MASK_SHOT_HULL
			&& ( !( cur_contents & CONTENTS_HITBOX ) || cur_contents == first_contents ) )
			continue;

		//g_csgo.m_engine_trace->TraceRay( { out, out - dir * k_step_size }, MASK_SHOT, nullptr, &exit_trace );

		if ( exit_trace.m_startsolid
			&& exit_trace.m_surface.m_flags & SURF_HITBOX ) {
			filter.SetPassEntity( exit_trace.m_entity );

			g_csgo.m_engine_trace->TraceRay( Ray( out, src ), MASK_SHOT_HULL, ( ITraceFilter* ) &filter, &exit_trace );

			if ( exit_trace.hit( ) && !exit_trace.m_startsolid ) {
				return true;
			}

			continue;
		}

		if ( !exit_trace.hit( )
			|| exit_trace.m_startsolid ) {
			if ( enter_trace.m_entity
				&& enter_trace.m_entity->index ( )
				&& game::IsBreakable( enter_trace.m_entity ) ) {
				exit_trace = enter_trace;
				exit_trace.m_endpos = src + dir;

				return true;
			}

			continue;
		}

		if ( exit_trace.m_surface.m_flags & SURF_NODRAW ) {
			if ( game::IsBreakable( exit_trace.m_entity )
				&& game::IsBreakable( enter_trace.m_entity ) )
				return true;

			if ( !( enter_trace.m_surface.m_flags & SURF_NODRAW ) )
				continue;
		}

		if ( exit_trace.m_plane.m_normal.dot( dir ) <= 1.f )
			return true;
	}

	return false;
}