#pragma once
#include "../includes.h"

namespace LagController {
	class C_BaseLagRecord {
	public:
		Player* player = nullptr;
		vec3_t m_vecMins, m_vecMaxs;
		vec3_t m_vecOrigin;
		ang_t m_angAngles;
		float m_flAbsRotation;

		float m_flSimulationTime;

		alignas( 16 ) matrix3x4_t m_BoneMatrix[ 128 ];

		void Setup( Player* player );
		void Apply( Player* player );
	};

	class C_LagRecord : public C_BaseLagRecord {
	public:
		float m_flServerLatency;
		float m_flRealTime;
		float m_flLastShotTime;
		float m_flEyeYaw;
		float m_flEyePitch;
		float m_flAnimationVelocity;

		bool m_bIsShoting;
		bool m_bIsValid;
		bool m_bBonesCalculated;
		bool m_bExtrapolated;
		bool m_bResolved;
		bool m_bIsWalking;
		bool m_bIsRunning;
		bool m_bIsSideways;

		float m_flAbsRotation;

		int m_iFlags;
		int m_iLaggedTicks = 0;
		int m_iResolverMode;

		float m_flInterpolateTime = 0.f;

		// for sorting
		vec3_t m_vecVelocity;
		float m_flDuckAmount;

		bool m_bSkipDueToResolver = false; // skip record in hitscan
		bool m_bTeleportDistance = false; // teleport distance was broken

		float GetAbsYaw( );
		matrix3x4_t* GetBoneMatrix( );
		void Setup( Player* player );
		void Apply( Player* player );
	};

	class C_EntityLagData {
	public:
		C_EntityLagData( );

		static void UpdateRecordData( Encrypted_t< C_EntityLagData > pThis, Player* player, const player_info_t& info, int updateType );

		void Clear( );

		std::deque<C_LagRecord> m_History = {};
		int m_iUserID = -1;

		float m_flLastUpdateTime = 0.0f;
		float m_flRate = 0.0f;

		int m_iMissedShots = 0;
		int m_iMissedShotsLBY = 0;

		bool m_bHitLastMove = false;
		std::deque< float > m_flLastMoveYaw;

		bool m_bGotAbsYaw = false;
		bool m_bGotAbsYawShot = false;
		bool m_bNotResolveIfShooting = false;
		bool m_bRateCheck = false;
		float m_flAbsYawHandled = 0.f;

		// ragebot scan data
		float m_flLastSpread, m_flLastInaccuracy;
		float m_flLastScanTime;
		vec3_t m_vecLastScanPos;

		// autowall resolver stuff
		float m_flEdges[ 4 ];
		float m_flDirection;

		// player prediction, need many improvments
		static bool DetectAutoDirerction( Encrypted_t< C_EntityLagData > pThis, Player* player );
	};

	class NonCopyable {
	protected:
		NonCopyable( ) { }
		~NonCopyable( ) { }

	private:
		NonCopyable( const NonCopyable& ) = delete;
		NonCopyable& operator=( const NonCopyable& ) = delete;

		NonCopyable( NonCopyable&& ) = delete;
		NonCopyable& operator=( NonCopyable&& ) = delete;
	};

	class __declspec( novtable ) LagCompensation : public NonCopyable {
	public:
		static LagCompensation* Get( );

		virtual void Update( ) = 0;
		virtual bool IsRecordOutOfBounds( const C_LagRecord& record, float target_time = 0.2f, int tickbase_shift = -1, bool tick_count_check = true ) const = 0;;
		virtual float GetLerp( ) const = 0;

		virtual Encrypted_t<C_EntityLagData> GetLagData( int entindex ) = 0;

		virtual void ClearLagData( ) = 0;

		float m_flOutLatency;
		float m_flServerLatency;
	protected:
		LagCompensation( ) { };
		virtual ~LagCompensation( ) { };
	};
}