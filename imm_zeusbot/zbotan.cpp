#include "zbotan.hpp"
#define C_CSPlayer Player
#define Vector vec3_t
#include "../imm_shot_info/shot_info.hpp"

#include "../backend/config/config_new.h"
namespace Zeusan {
	struct ZeusBotData {
		C_CSPlayer* m_pCurrentTarget = nullptr;
		C_CSPlayer* m_pLocalPlayer = nullptr;
		Weapon* m_pLocalWeapon = nullptr;
		Encrypted_t<WeaponInfo> m_pWeaponInfo = nullptr;
		Encrypted_t<CUserCmd> m_pCmd = nullptr;
		Vector m_vecEyePos;
	};

	ZeusBotData _zeus_bot_data;

	class CZeusBot : public ZeusBot {
	public:
		CZeusBot( ) : zeusBotData( &_zeus_bot_data ) { }

		void Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) override;
	private:
		bool TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket, LagController::C_LagRecord* record );

		Encrypted_t<ZeusBotData> zeusBotData;
	};

	ZeusBot* ZeusBot::Get( ) {
		static CZeusBot instance;
		return &instance;
	}

	void CZeusBot::Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) {
		if ( !Interfac::m_pEngine->IsInGame( ) || !settings.izeusan )
			return;

		zeusBotData->m_pLocalPlayer = g_cl.m_local;// C_CSPlayer::GetLocalPlayer( );
		if ( !zeusBotData->m_pLocalPlayer || !zeusBotData->m_pLocalPlayer->alive( ) )
			return;

		zeusBotData->m_pLocalWeapon = ( Weapon* )zeusBotData->m_pLocalPlayer->m_hActiveWeapon( ).Get( );
		if ( !zeusBotData->m_pLocalWeapon || !zeusBotData->m_pLocalWeapon->m_hWeapon( ) )
			return;

		zeusBotData->m_pWeaponInfo = zeusBotData->m_pLocalWeapon->GetWpnData( );
		if ( !zeusBotData->m_pWeaponInfo.IsValid( ) )
			return;

		zeusBotData->m_pCmd = pCmd;
		if ( zeusBotData->m_pLocalPlayer->m_flNextAttack( ) > Interfac::m_pGlobalVars->m_curtime || zeusBotData->m_pLocalWeapon->m_flNextPrimaryAttack( ) > Interfac::m_pGlobalVars->m_curtime )
			return;

		if ( zeusBotData->m_pLocalWeapon->m_iItemDefinitionIndex( ) != WEAPONTYPE_TASER )
			return;

		zeusBotData->m_vecEyePos = zeusBotData->m_pLocalPlayer->get_eye_pos( );
		for ( int i = 1; i <= Interfac::m_pGlobalVars->m_max_clients; i++ ) {
			C_CSPlayer* Target = ( C_CSPlayer* )Interfac::m_pEntList->GetClientEntity( i );
			if ( !Target
				|| !Target->alive( )
				|| Target->dormant( )
				|| !Target->IsPlayer( )
				|| Target->m_iTeamNum( ) == zeusBotData->m_pLocalPlayer->m_iTeamNum( )
				 )
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
					if ( TargetEntity( Target, sendPacket, &record ) ) {
						zeusBotData->m_pCmd->m_tick = game::TIME_TO_TICKS( record.m_flSimulationTime + LagController::LagCompensation::Get( )->GetLerp( ) );
						break;
					}
				}
			}

			backup.Apply( Target );
		}
	}

	bool CZeusBot::TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket, LagController::C_LagRecord* record ) {
		if ( !record )
			return false;

		zeusBotData->m_pCurrentTarget = pPlayer;

		//g_Vars.misc.zeus_bot_hitchance
		auto hdr = *( studiohdr_t** )( pPlayer->m_studioHdr( ) );
		if ( hdr ) {
			auto hitboxSet = hdr->GetHitboxSet( pPlayer->m_nHitboxSet( ) );
			if ( hitboxSet ) {
				auto pStomach = hitboxSet->GetHitbox( HITGROUP_STOMACH );
				auto vecHitboxPos = ( pStomach->m_maxs + pStomach->m_mins ) * 0.5f;
				//vecHitboxPos = vecHitboxPos.Transform( pPlayer->m_CachedBoneData( ).Base( )[ pStomach->bone ] );

				// run awall
				imm_autowall::C_FireBulletData fireData;

				//Vector vecDirection = vecHitboxPos - zeusBotData->m_vecEyePos;
				//vecDirection.Normalize( );

				fireData.m_bPenetration = false;
				fireData.m_vecStart = zeusBotData->m_vecEyePos;
				//fireData.m_vecDirection = vecDirection;
				fireData.m_iHitgroup = HITGROUP_STOMACH;
				fireData.m_Player = zeusBotData->m_pLocalPlayer;
				fireData.m_TargetPlayer = pPlayer;
				fireData.m_WeaponData = zeusBotData->m_pWeaponInfo.Xor( );
				fireData.m_Weapon = zeusBotData->m_pLocalWeapon;
				fireData.m_iPenetrationCount = 0;

				// note - alpha; 
				// ghetto, shit, but good enough for zeusbot;
				// have fun doing ragebot hitchance with this implementation
				const bool bIsAccurate = !( zeusBotData->m_pLocalWeapon->GetInaccuracy( ) >= ( ( 100.0f - settings.izeushc ) * 0.65f * 0.01125f ) );
				const float flDamage = imm_autowall::FireBullets( &fireData );
				if ( flDamage >= 105.f && bIsAccurate ) {
					ShotInformations::C_ShotInformation::Get( )->CreateSnapshot( pPlayer, zeusBotData->m_vecEyePos, vecHitboxPos, record, false, HITGROUP_STOMACH, HITGROUP_STOMACH, int( flDamage ) );

					//zeusBotData->m_pCmd->viewangles = vecDirection.ToEulerAngles( );
					zeusBotData->m_pCmd->m_buttons |= IN_ATTACK;

					//if( !g_TickbaseController.bInRapidFire )
					//	*sendPacket = true;

					return true;
				}
			}
		}

		return false;
	}
}