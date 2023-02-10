#pragma once
#include "../includes.h"

class CFlashLightEffect // pasted from getze
{
public:
	bool m_bIsOn; //0x0000 
	char pad_0x0001[ 0x3 ]; //0x0001
	int m_nEntIndex; //0x0004 
	WORD m_FlashLightHandle; //0x0008 
	char pad_0x000A[ 0x2 ]; //0x000A
	float m_flMuzzleFlashBrightness; //0x000C 
	float m_flFov; //0x0010 
	float m_flFarZ; //0x0014 
	float m_flLinearAtten; //0x0018 
	bool m_bCastsShadows; //0x001C 
	char pad_0x001D[ 0x3 ]; //0x001D
	float m_flCurrentPullBackDist; //0x0020 
	DWORD m_MuzzleFlashTexture; //0x0024 
	DWORD m_FlashLightTexture; //0x0028 
	char m_szTextureName[ 64 ]; //0x1559888 
}; //Size=0x006C

namespace Getze {
	namespace FrameController {
		void UpdateFlashLight( CFlashLightEffect* pFlashLight, const vec3_t& vecPos, const vec3_t& vecForward, const vec3_t& vecRight, const vec3_t& vecUp );
		CFlashLightEffect* CreateFlashLight( int nEntIndex, const char* pszTextureName, float flFov, float flFarZ, float flLinearAtten );
		void DestroyFlashLight( CFlashLightEffect* pFlashLight );

		void RunFrame( ); // run flash frame

		void ExtendChokePackets( );
		void PredictVelocity( vec3_t* velocity );
		void QuickStopL3D( CUserCmd* cmd );
	}

	namespace L3D_Math {
		inline void VectorAngles( const vec3_t& vecForward, vec3_t& vecAngles )
		{
			vec3_t vecView;
			if ( vecForward.y == 0.f && vecForward.x == 0.f )
			{
				vecView.x = 0.f;
				vecView.y = 0.f;
			}
			else
			{
				vecView.y = atan2( vecForward.y, vecForward.x ) * 180.f / math::pi;

				if ( vecView.y < 0.f )
					vecView.y += 360;

				vecView.z = sqrt( vecForward.x * vecForward.x + vecForward.y * vecForward.y );

				vecView.x = atan2( vecForward.z, vecView.z ) * 180.f / math::pi;
			}

			vecAngles.x = -vecView.x;
			vecAngles.y = vecView.y;
			vecAngles.z = 0.f;
		}

		inline void SinCos( float radians, float* sine, float* cosine )
		{
			*sine = sin( radians );
			*cosine = cos( radians );
		}

		inline void AngleVectors( const vec3_t& angles, vec3_t* forward )
		{
			float sp, sy, cp, cy;

			SinCos( math::deg_to_rad( angles.y ), &sy, &cy );
			SinCos( math::deg_to_rad( angles.x ), &sp, &cp );

			forward->x = cp * cy;
			forward->y = cp * sy;
			forward->z = -sp;
		}
	}
}

