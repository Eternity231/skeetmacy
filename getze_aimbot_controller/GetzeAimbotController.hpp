#pragma once
#include "../includes.h"

namespace Getze {
	namespace AimbotController {
		bool TimeDeltaLarger( Player* player );
		bool delay_shot( Player* player, CUserCmd* cmd, Weapon* local_weapon );

	}
}