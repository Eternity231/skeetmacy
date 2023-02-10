#include "kbotan.hpp"
#define C_CSPlayer Player
#define Vector vec3_t

#include "../backend/config/config_new.h"

namespace KnifeBot {
	struct KnifeBotData {
		C_CSPlayer* m_pCurrentTarget = nullptr;
		C_CSPlayer* m_pLocalPlayer = nullptr;
		Weapon* m_pLocalWeapon = nullptr;
		Encrypted_t<WeaponInfo> m_pWeaponInfo = nullptr;
		Encrypted_t<CUserCmd> m_pCmd = nullptr;
		Vector m_vecEyePos;
	};

	KnifeBotData _knife_bot_data;

	class CKnifeBot : public KnifeBot {
	public:
		CKnifeBot( ) : knifeBotData( &_knife_bot_data ) { }

		void Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) override;
	private:
		int GetMinimalHp( );
		bool TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket );
		int DeterminateHitType( bool stabType, Vector eyeDelta );

		Encrypted_t<KnifeBotData> knifeBotData;
	};

	KnifeBot* KnifeBot::Get( ) {
		static CKnifeBot instance;
		return &instance;
	}

	void CKnifeBot::Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) {
		if ( !Interfac::m_pEngine->IsInGame( ) || !settings.jnifebot )
			return;

		knifeBotData->m_pLocalPlayer = g_cl.m_local;

		if ( !knifeBotData->m_pLocalPlayer || !knifeBotData->m_pLocalPlayer->alive( ) )
			return;

		knifeBotData->m_pLocalWeapon = ( Weapon* )knifeBotData->m_pLocalPlayer->m_hActiveWeapon( ).Get( );

		if ( !knifeBotData->m_pLocalWeapon || !knifeBotData->m_pLocalWeapon->m_hWeapon( ) )
			return;

		knifeBotData->m_pWeaponInfo = knifeBotData->m_pLocalWeapon->GetWpnData( );

		if ( !knifeBotData->m_pWeaponInfo.IsValid( ) || !( knifeBotData->m_pWeaponInfo->m_weapon_type == WEAPONTYPE_KNIFE ) )
			return;

		knifeBotData->m_pCmd = pCmd;

		if ( knifeBotData->m_pLocalPlayer->m_flNextAttack( ) > Interfac::m_pGlobalVars->m_curtime || knifeBotData->m_pLocalWeapon->m_flNextPrimaryAttack( ) > Interfac::m_pGlobalVars->m_curtime )
			return;

		if ( knifeBotData->m_pLocalWeapon->m_iItemDefinitionIndex( ) == WEAPONTYPE_TASER )
			return;

		knifeBotData->m_vecEyePos = knifeBotData->m_pLocalPlayer->get_eye_pos( );

		for ( int i = 1; i <= Interfac::m_pGlobalVars->m_max_clients; i++ ) {
			C_CSPlayer* Target = ( C_CSPlayer* )Interfac::m_pEntList->GetClientEntity( i );
			if ( !Target
				|| !Target->alive( )
				|| Target->dormant( )
				|| !Target->IsPlayer( )
				|| Target->m_iTeamNum( ) == knifeBotData->m_pLocalPlayer->m_iTeamNum( ) )
				continue;

			auto lag_data = LagController::LagCompensation::Get( )->GetLagData( Target->index() );
			if ( !lag_data.IsValid( ) || lag_data->m_History.empty( ) )
				continue;

			LagController::C_LagRecord* previousRecord = nullptr;
			LagController::C_LagRecord backup;
			backup.Setup( Target );
			for ( auto& record : lag_data->m_History ) {
				if ( LagController::LagCompensation::Get( )->IsRecordOutOfBounds( record, 0.2f )
					|| record.m_bSkipDueToResolver
					|| !record.m_bIsValid ) {
					continue;
				}

				if ( !previousRecord
					|| previousRecord->m_vecOrigin != record.m_vecOrigin
					|| previousRecord->m_flEyeYaw != record.m_flEyeYaw
					|| previousRecord->m_angAngles.y != record.m_angAngles.y
					|| previousRecord->m_vecMaxs != record.m_vecMaxs
					|| previousRecord->m_vecMins != record.m_vecMins ) {
					previousRecord = &record;

					record.Apply( Target );
					if ( TargetEntity( Target, sendPacket ) ) {
						knifeBotData->m_pCmd->m_tick = game::TIME_TO_TICKS( record.m_flSimulationTime + LagController::LagCompensation::Get( )->GetLerp( ) );
						break;
					}
				}
			}

			backup.Apply( Target );
		}
	}

	int CKnifeBot::GetMinimalHp( ) {
		if ( Interfac::m_pGlobalVars->m_curtime > ( knifeBotData->m_pLocalWeapon->m_flNextPrimaryAttack( ) + 0.4f ) )
			return 34;

		return 21;
	}

	bool CKnifeBot::TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket ) {
		knifeBotData->m_pCurrentTarget = pPlayer;

		Vector vecOrigin = pPlayer->m_vecOrigin( );

		//Vector vecOBBMins = pPlayer->m_Collision( )->m_vecMins;
		//Vector vecOBBMaxs = pPlayer->m_Collision( )->m_vecMaxs;

		//Vector vecMins = vecOBBMins + vecOrigin;
		//Vector vecMaxs = vecOBBMaxs + vecOrigin;

		Vector vecEyePos = pPlayer->get_eye_pos( );
		const vec3_t vecPos = pPlayer->m_vecOrigin( ) + ( ( pPlayer->m_vecMins( ) + pPlayer->m_vecMaxs( ) ) * 0.5f );
		Vector vecDelta = vecPos - knifeBotData->m_vecEyePos;
		vecDelta.normalize( );

		int attackType = DeterminateHitType( 0, vecDelta );
		if ( attackType ) {
			if ( settings.modeknife == 1 ) {
				if ( attackType == 2 && knifeBotData->m_pCurrentTarget->m_iHealth( ) <= 76 ) {
				first_attack:
					knifeBotData->m_pCmd->m_view_angles = vecDelta.ToEulerAngles( );
					knifeBotData->m_pCmd->m_buttons |= IN_ATTACK;

					*sendPacket = true;

					return true;
				}
			}
			else if ( settings.modeknife == 2 ) {
				if ( knifeBotData->m_pCurrentTarget->m_iHealth( ) > 46 ) {
					goto first_attack;
				}
			}
			else {
				int hp = attackType == 2 ? 76 : GetMinimalHp( );
				if ( hp >= knifeBotData->m_pCurrentTarget->m_iHealth( ) )
					goto first_attack;
			}
		}

		if ( !DeterminateHitType( 1, vecDelta ) )
			return false;

		knifeBotData->m_pCmd->m_view_angles = vecDelta.ToEulerAngles( );
		knifeBotData->m_pCmd->m_buttons |= IN_ATTACK2;

		*sendPacket = true;

		return true;
	}

	int CKnifeBot::DeterminateHitType( bool stabType, Vector eyeDelta ) {
		float minDistance = stabType ? 32.0f : 48.0f;

		Vector vecEyePos = knifeBotData->m_vecEyePos;
		Vector vecEnd = vecEyePos + ( eyeDelta * minDistance );
		Vector vecOrigin = knifeBotData->m_pCurrentTarget->m_vecOrigin( );

		CTraceFilterSimple filter;
		filter.pSkip = knifeBotData->m_pLocalPlayer;

		Ray_t ray;
		ray.Init( vecEyePos, vecEnd, Vector( -16.0f, -16.0f, -18.0f ), Vector( 16.0f, 16.0f, 18.0f ) );
		CGameTrace tr;
		g_csgo.m_engine_trace->TraceRay( ray, 0x200400B, &filter, &tr );

		if ( !tr.m_entity )
			return 0;

		if ( knifeBotData->m_pCurrentTarget ) {
			if ( tr.m_entity != knifeBotData->m_pCurrentTarget )
				return 0;
		}
		/*
		else { // guess this only for trigger bot
			C_CSPlayer* ent = game::ToCSPlayer( tr.m_entity->GetBaseEntity( ) );

			if ( !ent || !ent->alive( ) || ent->dormant( ) )
				return 0;

			if ( ent->m_iTeamNum( ) == knifeBotData->m_pLocalPlayer->m_iTeamNum( ) )
				return 0;
		}
		*/

		ang_t angles = tr.m_entity->GetAbsAngles( );
		float cos_pitch = cos( math::deg_to_rad( angles.x ) );
		float sin_yaw, cos_yaw;
		DirectX::XMScalarSinCos( &sin_yaw, &cos_yaw, math::deg_to_rad( angles.y ) );

		Vector delta = vecOrigin - vecEyePos;
		return ( ( ( cos_yaw * cos_pitch * delta.x ) + ( sin_yaw * cos_pitch * delta.y ) ) >= 0.475f ) + 1;
	}
}