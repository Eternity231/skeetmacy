#pragma once
#include "../includes.h"

namespace resolve_record {
	namespace resolve_data {
		// resolver modes -> extremly cleaned
		// we have to add more resolve stages but those are enough for now
		enum resolver_modes : size_t {
			resolve_none = 0,
			resolve_stand,
			resolve_walk,
			resolve_air,
			resolve_body,
			resolve_last_move,
			resolve_unknown,
		};

		// aimbot records
		LagRecord* find_ideal_record( AimPlayer* data );
		LagRecord* find_last_record( AimPlayer* data );

		// resolver stuff
		float get_lby_rotated_yaw( float lby, float yaw );
		bool is_yaw_sideways( Player* entity, float yaw );
		float get_away_angle( LagRecord* record );
		void match_shot( AimPlayer* data, LagRecord* record );

		// lowerbody yaw
		void on_body_update( Player* player, float value );
		
		// missed
		void reset_indexes( AimPlayer* data );

		// vars.
		inline bool lby_flicking;
		inline int i_players[ 64 ];
	}

	namespace getze_us {
		// thats abit of getze code, i wont leak more than i do there, so dont ask me for complete code
		void layer_resolver( AimPlayer* data, LagRecord* record );
	}

	namespace resolve_anti_freestand {
		void resolve_anti_freestanding( LagRecord* record );
		void resolve_anti_freestanding_reversed( LagRecord* record );
	}

	namespace hyperbius {
		int get_chocked_packet( Player* target );
		inline int last_ticks[ 64 ];

		void hyperbius_moving( AimPlayer* data, LagRecord* record );
		void hyperbius_standing( LagRecord* record, AimPlayer* data, Player* player );
		void resolve_yaw_bruteforce( LagRecord* record, Player* player, AimPlayer* data );
	}

	namespace onetap_reverse {
		// crdits to l1ney
		void handle_resolver( AimPlayer* data, LagRecord* record, LagRecord* previous );
		void set_final_angle( AimPlayer* data, LagRecord* record, float& out_yaw );
		void anti_freestand( AimPlayer* data, LagRecord* record );
		float get_pitch_angle( AimPlayer* data, LagRecord* record );
	}

	namespace resolve_stages {
		void set_mode( LagRecord* record );
		void resolve_angles( Player* player, LagRecord* record );
		void resolve_poses( Player* player, LagRecord* record );

		// resolvers
		void resolve_walking( AimPlayer* data, LagRecord* record );
		void resolve_standing( AimPlayer* data, LagRecord* record, Player* player );
		void resolve_air( AimPlayer* data, LagRecord* record );
	}

	class PlayerResolveRecord
	{
	public:
		struct AntiFreestandingRecord
		{
			int right_damage = 0, left_damage = 0;
			float right_fraction = 0.f, left_fraction = 0.f;
		};

	public:
		AntiFreestandingRecord m_sAntiEdge;
	};

	inline PlayerResolveRecord player_resolve_records[ 33 ];
}