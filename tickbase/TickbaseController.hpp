#pragma once
#include "../includes.h"


// we add this upd there so we wont get errors
typedef void( *CLMove_t )( float accumulated_extra_samples, bool bFinalTick );

namespace cl_move {
	// we do namespace cl_move cuz it looks alot more cleaner
	// we could do fuck it but we want the best quality

	// this should work fine, im not sure tho we will see
	inline CLMove_t o_CLMove;
	void CL_Move( float accumulated_extra_samples, bool bFinalTick );
}

/* note: this is pasted from ambien, and how all know 
	ambien was machport cheat in the first stage so we are going to shoutout: machport
	not zeze who claims he coded this, suck a nigga dick
*/
namespace TickbaseControll {
	// namespace looks alot more cleaner
	/*
			you might ask why we use inline, well
		well if we dont use inline in namespace and we have bools, ints, voids .. etc.. we will get alot of errors during the building
		session
	*/
	inline bool m_double_tap;
	void manipulate_tickbase( );

	//c_tickbase_array m_tickbase_array[ 150 ] = {};
	inline int last_cmd_delta = 0;


	// ez pz lemon squieeze
	inline bool m_shifting;
	inline bool m_charged;
	inline int m_shift_cmd;
	inline int m_shift_tickbase;
	inline int m_charged_ticks;
	inline int m_charge_timer;
	inline int m_tick_to_shift;
	inline int m_tick_to_shift_alternate;
	inline int m_tick_to_recharge;
	inline bool m_shifted;

	inline int ticks_allowed;

	void ApplyDTSub30( int cmd_num, int commands_to_shift, int original_choke );
	void CopyCommandSkeet( CUserCmd* from_cmd );
	void ApplyShift( CUserCmd* cmd, bool* bSendPacket );
	bool IsTickcountValid( int tick );
}

