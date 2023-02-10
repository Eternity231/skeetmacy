#pragma once
#include "../includes.h"
#include <utility>
#include <cmath>
namespace rage_immortal {
	struct BoundingBox {
		vec3_t minn, maxx;
		int idx;

		BoundingBox( void ) { };
		BoundingBox( const vec3_t& minn, const vec3_t& maxx, int idx ) :
			minn( minn ), maxx( maxx ), idx( idx ) {
		};
	};

	struct CapsuleHitbox {
		CapsuleHitbox( ) = default;
		CapsuleHitbox( const vec3_t& mins, const vec3_t& maxs, const float radius, int idx )
			: m_mins( mins ), m_maxs( maxs ), m_radius( radius ), m_idx( idx ) {
		}

		vec3_t m_mins;
		vec3_t m_maxs;
		float m_radius;
		int m_idx;
	};


	struct C_AimTarget;
	struct C_AimPoint;

	struct C_AimPoint {
		vec3_t position;

		float damage = 0.0f;
		float hitchance = 0.0f;
		float pointscale = 0.0f;

		int healthRatio = 0;
		int hitboxIndex = 0;
		int hitgroup = 0;

		bool center = false;
		bool penetrated = false;
		bool isLethal = false;
		bool isHead = false;
		bool isBody = false;
		bool is_should_baim = false;
		bool is_should_headaim = false;

		C_AimTarget* target = nullptr;
		LagRecord* record = nullptr;
		//AnimationRecord* animrecord = nullptr;
	};

	struct C_AimTarget {
		std::vector<C_AimPoint> points;
		std::vector<BoundingBox> obb;
		std::vector<CapsuleHitbox> capsules;
		//Engine::C_BaseLagRecord backup;
		LagRecord* record = nullptr;
	//	Engine::C_AnimationRecord* animrecord = nullptr;
		Player* player = nullptr;
		bool overrideHitscan = false;
		bool preferHead = false;
		bool preferBody = false;
		bool hasLethal = false;
		bool onlyHead = false;
		bool hasCenter = false;
	};

	struct C_PointsArray {
		C_AimPoint* points = nullptr;
		int pointsCount = 0;
	};

	struct C_HitchanceData {
		vec3_t direction;
		vec3_t end;
		bool hit = false;
		bool damageIsAccurate = false;
	};

	struct C_HitchanceArray {
		C_AimPoint* point;
		C_HitchanceData* data;
		int dataCount;
	};

	enum SelectTarget_e : int {
		SELECT_HIGHEST_DAMAGE = 0,
		SELECT_FOV,
		SELECT_LOWEST_HP,
		SELECT_LOWEST_DISTANCE,
		SELECT_LOWEST_PING,
	};

	bool Run( CUserCmd* cmd );
	bool GetBoxOption( mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& ps, bool override_hitscan );
	void AddPoint( Player* player, LagRecord* record, int side, std::vector<std::pair<vec3_t, bool>>& points, const vec3_t& point, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, bool isMultipoint );
	bool RunInternal( );
	bool Hitchance( C_AimPoint* pPoint, const vec3_t& vecStart, float flChance );
	std::pair< bool, C_AimPoint> RunHitscan( );
	bool SetupTargets( );
	void SelectBestTarget( );
	bool AimAtPoint( C_AimPoint* bestPoint );
	void ScanPoint( C_AimPoint* pPoint );
	int GeneratePoints( Player* player, std::vector<C_AimTarget>& aim_targets, std::vector<C_AimPoint>& aim_points );
	bool IsPointAccurate( C_AimPoint* point, const vec3_t& start );
	void Multipoint( Player* player, int side, std::vector<vec3_t>& points, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& pointScale, int hitboxIndex );

	namespace rage_data {
		inline float m_flSpread;
		inline float m_flInaccuracy;

		inline CUserCmd* m_pCmd = nullptr;
		inline Player* m_pLocal = nullptr;
		inline Weapon* m_pWeapon = nullptr;
		inline WeaponInfo* m_pWeaponInfo = nullptr;
		inline bool * m_pSendPacket = nullptr;

		inline Player* m_pLastTarget = nullptr;

		inline std::vector<C_AimTarget> m_targets;
		inline std::vector<C_AimPoint> m_aim_points;
		inline std::vector<C_AimTarget> m_aim_data;

		inline C_AimTarget* m_pBestTarget;

		inline vec3_t m_vecEyePos;

		// last entity iterated in list
		inline int m_nIterativeTarget = 0;

		// failed hitchance this tick
		inline bool m_bFailedHitchance = false;

		// no need to autoscope
		inline bool m_bNoNeededScope = false;

		inline bool m_bResetCmd = false;
		inline bool m_bRePredict = false;
		inline int m_RestoreZoomLevel = INT_MAX;

		inline bool m_bPredictedScope = false;

		inline int m_iChokedCommands = -1;
		inline int m_iDelay = 0;

		inline bool m_bDelayedHeadAim = false;

		inline vec3_t m_PeekingPosition;
		inline float m_flLastPeekTime;

		inline bool m_bDebugGetDamage = false;

		inline bool m_bEarlyStop = false;

		// hitchance
		static std::vector<std::tuple<float, float, float>> precomputed_seeds; // this might be inline
	}
}