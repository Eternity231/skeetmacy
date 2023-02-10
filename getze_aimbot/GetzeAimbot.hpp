#pragma once
#include "../includes.h"

class c_player_records;
class C_Hitbox
{
public:
	int hitboxID;
	bool isOBB;
	vec3_t mins;
	vec3_t maxs;
	float radius;
	int bone;
};

namespace Getze {
	namespace GetzeAimbot {
		vec3_t get_hitbox( Player* ent, int ihitbox );
		void get_hitbox_data( C_Hitbox* rtn, Player* ent, int ihitbox, matrix3x4_t* matrix );
		bool safe_point( Player* entity, vec3_t eye_pos, vec3_t aim_point, int hitboxIdx, bool maxdamage );
		bool safe_static_point( Player* entity, vec3_t eye_pos, vec3_t aim_point, int hitboxIdx, bool maxdamage );
		void draw_capsule( Player* ent, int ihitbox );
		vec3_t get_hitbox( Player* ent, int ihitbox, matrix3x4_t mat[ ] );
		bool delay_shot( Player* player, CUserCmd* cmd, bool& send_packet, c_player_records record, Weapon* local_weapon, bool did_shot_backtrack );
		float can_hit( int hitbox, Player* Entity, vec3_t position, matrix3x4_t mx[ ], bool check_center = false, bool predict = false );
		void build_seed_table( );
		bool hit_chance( QAngle angle, Player* ent, float chance, int hitbox, float damage, int& hc );
		bool prefer_baim( Player* player, bool& send_packet, Weapon* local_weapon, bool is_lethal_shot );
		void visualise_hitboxes( Player* entity, matrix3x4_t* mx, Color color, float time );
		void autostop( CUserCmd* cmd, bool& send_packet, Weapon* local_weapon/*, Player * best_player, float dmg, bool hitchanced*/ );
		float check_wall( Weapon* local_weapon, vec3_t startpoint, vec3_t direction, Player* entity );
		bool work( CUserCmd* cmd, bool& send_packet );
		bool knifebot_work( CUserCmd* cmd, bool& send_packet );

		bool knifebot_target( );

		inline float r8cock_time = 0.f;
		inline bool unk_3CCA9A51 = 0;
		inline bool lastAttacked = false;
		inline int historytick = -1;
		inline int m_nBestIndex = -1;
		inline float m_nBestDist = -1;
		inline vec3_t m_nAngle;
		inline Player* pBestEntity;
		inline float last_pitch;
		inline std::vector<std::tuple<float, float, float>> precomputed_seeds = {};
	}
}