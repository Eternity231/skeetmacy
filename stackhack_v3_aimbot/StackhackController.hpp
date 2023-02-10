#pragma once
#include "../includes.h"

namespace StackHack {
	namespace AimbotControll {
		inline matrix3x4_t matrix[ 65 ][ 128 ];
		inline matrix3x4_t matrix2[ 128 ];
		inline int sequence;
		inline int best_ent_dmg;
		inline bool backtrack[ 65 ];
		inline bool shot_backtrack[ 65 ];

		void on_create_move( CUserCmd* pCmd, bool* bSendPacket );
		void stop_movement( CUserCmd* cmd );
		vec3_t hitscan( Player* pEnt );
		bool hit_chance( Player* pEnt, vec3_t Angle, vec3_t Point, int chance );
		void autostop( );
	}
}