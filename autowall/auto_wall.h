#pragma once

struct pen_data_t {
	__forceinline pen_data_t( ) = default;
	__forceinline pen_data_t ( int dmg, int hitbox, int group, int pen ) : m_dmg { dmg }, m_hitbox { hitbox }, m_hit_group { group }, m_remaining_pen { pen } {}

	int m_dmg{}, m_hitbox{}, m_hit_group{}, m_remaining_pen{};
};

class c_auto_wall {
private:
public:
	pen_data_t get( const vec3_t& shoot_pos, const vec3_t& point, const Player* enemy );
	void scale_dmg( Player* player, CGameTrace& trace, WeaponInfo* wpn_info, float& cur_dmg, const int hit_group );

	bool trace_to_exit( const vec3_t& src, const vec3_t& dir,
		const CGameTrace& enter_trace, CGameTrace& exit_trace );

	bool handle_bullet_penetration(
		WeaponInfo* wpn_data, CGameTrace& enter_trace, vec3_t& eye_pos, const vec3_t& direction, int& possible_hits_remain,
		float& cur_dmg, float penetration_power, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration
	);

	bool fire_bullet( Weapon* wpn, vec3_t& direction, bool& visible, float& cur_dmg, int& remaining_pen, int& hit_group,
		int& hitbox, Entity* e = nullptr, float length = 0.f, const vec3_t& pos = { 0.f,0.f,0.f } );
};

inline const auto g_auto_wall = std::make_unique < c_auto_wall >( );