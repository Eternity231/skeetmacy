#include "LagControll.hpp"
#include "../backend/config/config_new.h"
#include "../imm_animfix/imm_animfix.hpp"
namespace LagController {
	struct LagCompData {
		std::map< int, C_EntityLagData > m_PlayerHistory;

		float m_flLerpTime, m_flOutLatency, m_flServerLatency;
		bool m_GetEvents = false;
	};

	static LagCompData _lagcomp_data;

	class C_LagCompensation : public LagCompensation {
	public:
		virtual void Update( );
		virtual bool IsRecordOutOfBounds( const C_LagRecord& record, float target_time = 0.2f, int tickbase_shift = -1, bool bDeadTimeCheck = true ) const;

		virtual Encrypted_t<C_EntityLagData> GetLagData( int entindex ) {
			C_EntityLagData* data = nullptr;
			if ( lagData->m_PlayerHistory.count( entindex ) > 0 )
				data = &lagData->m_PlayerHistory.at( entindex );
			return Encrypted_t<C_EntityLagData>( data );
		}

		virtual float GetLerp( ) const {
			return lagData.Xor( )->m_flLerpTime;
		}

		virtual void ClearLagData( ) {
			lagData->m_PlayerHistory.clear( );
		}

		C_LagCompensation( ) : lagData( &_lagcomp_data ) { };
		virtual ~C_LagCompensation( ) { };
	private:
		virtual void SetupLerpTime( );
		Encrypted_t<LagCompData> lagData;
	};

	C_LagCompensation g_LagComp;
	LagCompensation* LagCompensation::Get( ) {
		return &g_LagComp;
	}

	C_EntityLagData::C_EntityLagData( ) {
		;
	}

	void C_LagCompensation::Update( ) {
		auto v1 = g_csgo.m_engine;
		if ( !v1->IsInGame( ) || !v1->GetNetChannelInfo( ) ) {
			lagData->m_PlayerHistory.clear( );
			return;
		}

		auto updateReason = 0;
		auto v2 = g_cl.m_local;
		auto v3 = g_cl.m_processing; // hack ready
		if ( !v2 || !v3 )
			return;

		SetupLerpTime( );

		auto v4 = g_csgo.m_globals->m_max_clients;
		for ( int i = 1; i <= v4; ++i ) {
			Player* v5 = g_csgo.m_entlist->GetClientEntity< Player* >( i );
			if ( !v5 || v5 == v2 || !v5->IsPlayer( ) )
				continue;

			player_info_t player_info;
			if ( !v1->GetPlayerInfo( v5->index( ), &player_info ) ) {
				continue;
			}

			auto v6 = Encrypted_t<C_EntityLagData>( &lagData->m_PlayerHistory[ v5->index( ) ] );
			v6->UpdateRecordData( v6, v5, player_info, updateReason );
		} 
	}
	bool C_LagCompensation::IsRecordOutOfBounds( const C_LagRecord& record, float flTargetTime, int nTickbaseShiftTicks, bool bDeadTimeCheck ) const {
		auto v1 = g_csgo.m_engine;
		Encrypted_t<INetChannel> v2 = Encrypted_t<INetChannel>( v1->GetNetChannelInfo( ) );
		if ( !v2.IsValid( ) )
			return true;

		auto v3 = g_cl.m_local;
		if ( !v3 )
			return true;

		float curtime = game::TICKS_TO_TIME( v3->m_nTickBase( ) /* * g_TickbaseController.s_nExtraProcessingTicks */ );
		float correct = lagData.Xor( )->m_flLerpTime + lagData.Xor( )->m_flServerLatency;

		math::clamp( correct, 0.f, 1.0f );
		return std::fabsf( correct - ( curtime - record.m_flSimulationTime ) ) < flTargetTime;
	}

	void C_LagCompensation::SetupLerpTime( ) {
		auto v1 = g_csgo.m_cvar;

		auto v2 = v1->FindVar( HASH( "sv_minupdaterate" ) );
		auto v3 = v1->FindVar( HASH( "sv_maxupdaterate" ) );

		auto v4 = v1->FindVar( HASH( "sv_client_min_interp_ratio" ) );
		auto v5 = v1->FindVar( HASH( "sv_client_max_interp_ratio" ) );

		float updaterate = g_csgo.cl_updaterate->GetFloat( );
		float minupdaterate = v2->GetFloat( );
		float maxupdaterate = v3->GetFloat( );

		float min_interp = v4->GetFloat( );
		float max_interp = v5->GetFloat( );

		float flLerpAmount = g_csgo.cl_interp->GetFloat( );
		float flLerpRatio = g_csgo.cl_interp_ratio->GetFloat( );
		flLerpRatio = math::Clamp( flLerpRatio, min_interp, max_interp );
		if ( flLerpRatio == 0.0f )
			flLerpRatio = 1.0f;

		float updateRate = math::Clamp( updaterate, minupdaterate, maxupdaterate );
		lagData->m_flLerpTime = std::fmaxf( flLerpAmount, flLerpRatio / updateRate );
		
		auto v6 = g_csgo.m_engine;
		auto netchannel = Encrypted_t<INetChannel>( v6->GetNetChannelInfo( ) );
		lagData->m_flOutLatency = netchannel->GetLatency( 0 );
		lagData->m_flServerLatency = netchannel->GetLatency( 1 );
	}

	#include "../backend/config/config_new.h"

	void C_EntityLagData::UpdateRecordData( Encrypted_t< C_EntityLagData > pThis, Player* player, const player_info_t& info, int updateType ) {
		auto v1 = g_cl.m_local;
		auto v2 = settings.enable;
		auto team_check = v2 && player->m_iTeamNum( );
		if ( !player->alive( ) || team_check ) {
			pThis->m_History.clear( );
			pThis->m_flLastUpdateTime = 0.0f;
			pThis->m_flLastScanTime = 0.0f;
			return;
		}

		bool isDormant = player->dormant( );
		auto v3 = g_csgo.m_globals;
		while ( pThis->m_History.size( ) > int( 1.0f / v3->m_interval ) ) {
			pThis->m_History.pop_back( );
		}

		if ( isDormant ) {
			pThis->m_flLastUpdateTime = 0.0f;
			if ( pThis->m_History.size( ) > 0 && pThis->m_History.front( ).m_bTeleportDistance ) {
				pThis->m_History.clear( );
			}

			return;
		}

		if ( info.m_user_id != pThis->m_iUserID ) {
			pThis->Clear( );
			pThis->m_iUserID = info.m_user_id;
		}

		// did player update?
		float simTime = player->m_flSimulationTime( );
		if ( pThis->m_flLastUpdateTime >= simTime ) {
			return;
		}

		if ( player->m_flOldSimulationTime( ) > simTime ) {
			return;
		}

		// weee need imm animfix
		
		auto anim_data = AnimationFixx::AnimationSystem::Get( )->GetAnimationData( player->index() );
		if ( !anim_data )
			return;

		if ( anim_data->m_AnimationRecord.empty( ) )
			return;

			auto anim_record = &anim_data->m_AnimationRecord.front( );
		if( anim_record->m_bShiftingTickbase )
			return;
		

		pThis->m_flLastUpdateTime = simTime;
		bool isTeam = v1->m_iTeamNum( );

		auto record = Encrypted_t<C_LagRecord>( &pThis->m_History.emplace_front( ) );

		auto v4 = g_csgo.m_engine;

		// we will uncomment this when we add that stuff there

		record->Setup( player );
		record->m_flRealTime = g_csgo.m_globals->m_realtime;
		record->m_flServerLatency = LagCompensation::Get( )->m_flServerLatency;
		record->m_flDuckAmount = anim_record->m_flDuckAmount;
		record->m_flEyeYaw = anim_record->m_angEyeAngles.y;
		record->m_flEyePitch = anim_record->m_angEyeAngles.x;
		record->m_bIsShoting = anim_record->m_bIsShoting;
		record->m_bIsValid = !anim_record->m_bIsInvalid;
		record->m_bBonesCalculated = anim_data->m_bBonesCalculated;
		record->m_flAnimationVelocity = player->m_PlayerAnimState2( )->m_velocity;
		record->m_bTeleportDistance = anim_record->m_bTeleportDistance;
		record->m_flAbsRotation = player->m_PlayerAnimState2( )->m_flAbsRotation;
		record->m_iLaggedTicks = game::TIME_TO_TICKS( player->m_flSimulationTime( ) - player->m_flOldSimulationTime( ) );
		record->m_bResolved = anim_record->m_bResolved;
		record->m_iResolverMode = anim_record->m_iResolverMode;

		std::memcpy( record->m_BoneMatrix, anim_data->m_Bones, player->m_BoneCache( ).m_CachedBoneCount * sizeof( matrix3x4_t ) );
	}
	bool C_EntityLagData::DetectAutoDirerction( Encrypted_t< C_EntityLagData > pThis, Player* player ) {
		if ( !player || !player->alive( ) )
			return false;

		auto v1 = g_cl.m_local;

		auto local = v1;
		if ( !local || !local->alive( ) )
			return false;

		auto v2 = g_cl.m_weapon;
		auto weapon = v2;
		if ( !weapon )
			return false;

		auto v3 = g_cl.m_weapon_info;
		auto weaponData = v3;
		if ( !weaponData )
			return false;

		auto eye_position = v1->GetShootPosition( );
		auto enemy_eye_position = player->get_eye_pos( );

		auto delta = enemy_eye_position - eye_position;
		delta.normalize( );

		int iterator = 0;
		bool result = false;

		CTraceFilterWorldOnly filter;
		float edges[ 4 ] = { 0.0f };

		float directionRag = atan2( delta.y, delta.x );
		float atTargetYaw = math::rad_to_deg( directionRag );

		for ( int i = 0; i < 4; ++i ) {
			float radAngle = math::rad_to_deg( ( i * 90.0f ) + atTargetYaw );
			float angleSin, angleCos;
			DirectX::XMScalarSinCos( &angleSin, &angleCos, radAngle );

			vec3_t pos;
			pos.x = enemy_eye_position.x + ( angleCos * 16.0f );
			pos.y = enemy_eye_position.y + ( angleSin * 16.0f );
			pos.z = enemy_eye_position.z;

			vec3_t extrapolated = eye_position;
			imm_autowall::C_FireBulletData data;

			data.m_bPenetration = true;
			data.m_Player = local;
			data.m_Weapon = weapon;
			data.m_WeaponData = weaponData;
			data.m_vecStart = extrapolated;
			data.m_bShouldIgnoreDistance = true;

			data.m_vecDirection = pos - extrapolated;
			data.m_flPenetrationDistance = data.m_vecDirection.normalize( );
			//data.filter = &filter;

			float dmg = imm_autowall::FireBullets( &data );

			if ( dmg >= 1.0f )
				result = true;
			else if ( dmg < 0.0f )
				dmg = 0.0f;

			edges[ i ] = dmg;
		}

		if ( result
			&& ( int( edges[ 0 ] ) != int( edges[ 1 ] )
				|| int( edges[ 0 ] ) != int( edges[ 2 ] )
				|| int( edges[ 0 ] ) != int( edges[ 3 ] ) ) ) {
			pThis->m_flEdges[ 0 ] = edges[ 0 ];
			pThis->m_flEdges[ 1 ] = edges[ 1 ];
			pThis->m_flEdges[ 2 ] = edges[ 2 ];
			pThis->m_flEdges[ 3 ] = edges[ 3 ];

			pThis->m_flDirection = atTargetYaw;
		}
		else {
			result = false;
		}

		return result;
	}

	void C_EntityLagData::Clear( ) {
		this->m_History.clear( );
		m_iUserID = -1;
		m_flLastScanTime = 0.0f;
		m_flLastUpdateTime = 0.0f;
	}

	float C_LagRecord::GetAbsYaw( ) {
		return this->m_angAngles.y;
	}

	matrix3x4_t* C_LagRecord::GetBoneMatrix( ) {
		if ( !this->m_bBonesCalculated )
			return this->m_BoneMatrix;

		return this->m_BoneMatrix;
	}

	void C_LagRecord::Setup( Player* player ) {
		//auto collidable = player->m_Collision( );
		//this->m_vecMins = collidable->m_vecMins;
		//this->m_vecMaxs = collidable->m_vecMaxs;

		this->m_vecOrigin = player->m_vecOrigin( );
		this->m_angAngles = player->GetAbsAngles( );

		this->m_vecVelocity = player->m_vecVelocity( );
		this->m_flSimulationTime = player->m_flSimulationTime( );

		this->m_iFlags = player->m_fFlags( );

	//	std::memcpy( this->m_BoneMatrix, player->m_CachedBoneData( ).Base( ),
		//	player->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

		this->player = player;
	}

	void C_LagRecord::Apply( Player* player ) {
		//auto collidable = player->m_Collision( );
		//collidable->SetCollisionBounds( this->m_vecMins, this->m_vecMaxs );

		player->m_flSimulationTime( ) = this->m_flSimulationTime;

		ang_t absAngles = this->m_angAngles;
		absAngles.y = this->GetAbsYaw( );

		player->m_fFlags( ) = this->m_iFlags;

		player->SetAbsAngles( absAngles );
		player->SetAbsOrigin( this->m_vecOrigin );

		matrix3x4_t* matrix = GetBoneMatrix( );

		if ( matrix ) {
			//std::memcpy( player->m_CachedBoneData( ).Base( ), matrix,
				//player->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

			// force bone cache
			player->m_iMostRecentModelBoneCounter( ) = *( int* )g_csgo.MostRecentModelBoneCounter;
			player->m_BoneAccessor( ).m_ReadableBones = player->m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
			player->m_flLastBoneSetupTime( ) = FLT_MAX;
		}
	}

	void C_BaseLagRecord::Setup( Player* player ) {
		//auto collidable = player->m_Collision( );
		//this->m_vecMins = collidable->m_vecMins;
		//this->m_vecMaxs = collidable->m_vecMaxs;

		this->m_flSimulationTime = player->m_flSimulationTime( );

		this->m_angAngles = player->GetAbsAngles( );
		this->m_vecOrigin = player->m_vecOrigin( );

		if ( player->m_PlayerAnimState2( ) != nullptr )
			this->m_flAbsRotation = player->m_PlayerAnimState2( )->m_flAbsRotation;

		///std::memcpy( this->m_BoneMatrix, player->m_CachedBoneData( ).Base( ),
			//player->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

		this->player = player;
	}

	void C_BaseLagRecord::Apply( Player* player ) {
		//auto collidable = player->m_Collision( );
		//collidable->SetCollisionBounds( this->m_vecMins, this->m_vecMaxs );

		player->m_flSimulationTime( ) = this->m_flSimulationTime;

		player->SetAbsAngles( this->m_angAngles );
		player->SetAbsOrigin( this->m_vecOrigin );

		if ( player->m_PlayerAnimState2( ) != nullptr )
			player->m_PlayerAnimState2( )->m_flAbsRotation = this->m_flAbsRotation;

		//std::memcpy( player->m_CachedBoneData( ).Base( ), this->m_BoneMatrix,
			//player->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

		// force bone cache
		player->m_iMostRecentModelBoneCounter( ) = *( int* )g_csgo.MostRecentModelBoneCounter;
		player->m_BoneAccessor( ).m_ReadableBones = player->m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
		player->m_flLastBoneSetupTime( ) = FLT_MAX;
	}
}