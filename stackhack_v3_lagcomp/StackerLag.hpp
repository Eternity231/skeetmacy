#pragma once
#include "../includes.h"

struct PlayerRecords
{
	matrix3x4_t Matrix[ 128 ];
	float Velocity;
	float SimTime;
	bool Shot;
};

class LagComp
{
public:
	int ShotTick[ 65 ];
	std::deque<PlayerRecords> PlayerRecord[ 65 ] = {  };
	void StoreRecord( Player* pEnt, CUserCmd* cmd );
	void ClearRecords( int i );
	float LerpTime( );
	bool ValidTick( int tick, CUserCmd* cmd );

	template<class T, class U>
	T clamp( T in, U low, U high );
private:
};
extern LagComp* g_LagComp;