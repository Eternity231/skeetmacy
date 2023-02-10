#include "StackhackController.hpp"
#include "../backend/config/config_new.h"
#include "../stackhack_v3_autowall/StackerAutowall.hpp"
#include "../stackhack_v3_lagcomp/StackerLag.hpp"

#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#define M_PI_2      (M_PI * 2.f)
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.

#ifdef NDEBUG
#define Assert( _exp ) ((void)0)
#else
#define Assert(x)
#endif

namespace StackHack {
	namespace AimbotControll {
		void sin_cos( float radian, float* sin, float* cos )
		{
			*sin = std::sin( radian );
			*cos = std::cos( radian );
		}

		vec3_t GetHitboxPosition( Player* player, int Hitbox, matrix3x4_t matrix[ ] )
		{
			const model_t* model = player->GetModel( );
			if ( !model )
				return vec3_t{};

			studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel( model );
			mstudiohitboxset_t* set = hdr->GetHitboxSet( 0 );
			mstudiobbox_t* hitbox = set->GetHitbox( Hitbox );

			if ( hitbox )
			{
				vec3_t vMin, vMax, vCenter, sCenter;
				math::VectorTransform( hitbox->m_mins, matrix[ hitbox->m_bone ], vMin );
				math::VectorTransform( hitbox->m_maxs, matrix[ hitbox->m_bone ], vMax );
				vCenter = ( vMin + vMax ) * 0.5;

				return vCenter;
			}

			return vec3_t( 0, 0, 0 );
		}

		vec3_t GetHitboxPosition( Player* player, int Hitbox, matrix3x4_t* Matrix, float* Radius )
		{
			const model_t* model = player->GetModel( );
			if ( !model )
				return vec3_t{};

			studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel( model );
			mstudiohitboxset_t* set = hdr->GetHitboxSet( 0 );
			mstudiobbox_t* hitbox = set->GetHitbox( Hitbox );

			if ( hitbox )
			{
				vec3_t vMin, vMax, vCenter, sCenter;
				math::VectorTransform( hitbox->m_mins, Matrix[ hitbox->m_bone ], vMin );
				math::VectorTransform( hitbox->m_maxs, Matrix[ hitbox->m_bone ], vMax );
				vCenter = ( vMin + vMax ) * 0.5;

				*Radius = hitbox->m_radius;

				return vCenter;
			}

			return vec3_t( 0, 0, 0 );
		}

		void AngleVectors( const vec3_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up )
		{
			float sp, sy, sr, cp, cy, cr;

			sin_cos( math::deg_to_rad( angles.x ), &sp, &cp );
			sin_cos( math::deg_to_rad( angles.y ), &sy, &cy );
			sin_cos( math::deg_to_rad( angles.z ), &sr, &cr );

			if ( forward != nullptr )
			{
				forward->x = cp * cy;
				forward->y = cp * sy;
				forward->z = -sp;
			}

			if ( right != nullptr )
			{
				right->x = -1 * sr * sp * cy + -1 * cr * -sy;
				right->y = -1 * sr * sp * sy + -1 * cr * cy;
				right->z = -1 * sr * cp;
			}

			if ( up != nullptr )
			{
				up->x = cr * sp * cy + -sr * -sy;
				up->y = cr * sp * sy + -sr * cy;
				up->z = cr * cp;
			}
		}

		void VectorAngles( const vec3_t& vecForward, vec3_t& vecAngles )
		{
			vec3_t vecView;
			if ( vecForward[ 1 ] == 0.f && vecForward[ 0 ] == 0.f )
			{
				vecView[ 0 ] = 0.f;
				vecView[ 1 ] = 0.f;
			}
			else
			{
				vecView[ 1 ] = atan2( vecForward[ 1 ], vecForward[ 0 ] ) * 180.f / 3.14159265358979323846f;

				if ( vecView[ 1 ] < 0.f )
					vecView[ 1 ] += 360.f;

				vecView[ 2 ] = sqrt( vecForward[ 0 ] * vecForward[ 0 ] + vecForward[ 1 ] * vecForward[ 1 ] );

				vecView[ 0 ] = atan2( vecForward[ 2 ], vecView[ 2 ] ) * 180.f / 3.14159265358979323846f;
			}

			vecAngles[ 0 ] = -vecView[ 0 ];
			vecAngles[ 1 ] = vecView[ 1 ];
			vecAngles[ 2 ] = 0.f;
		}

		void NormalizeAngles( vec3_t& angles )
		{
			for ( auto i = 0; i < 3; i++ ) {
				while ( angles[ i ] < -180.0f ) angles[ i ] += 360.0f;
				while ( angles[ i ] > 180.0f ) angles[ i ] -= 360.0f;
			}
		}

		float GRD_TO_BOG( float GRD ) {
			return ( M_PI / 180 ) * GRD;
		}

		void AngleVectors( const vec3_t& angles, vec3_t* forward )
		{
			Assert( s_bMathlibInitialized );
			Assert( forward );

			float	sp, sy, cp, cy;

			sy = sin( math::deg_to_rad( angles[ 1 ] ) );
			cy = cos( math::deg_to_rad( angles[ 1 ] ) );

			sp = sin( math::deg_to_rad( angles[ 0 ] ) );
			cp = cos( math::deg_to_rad( angles[ 0 ] ) );

			forward->x = cp * cy;
			forward->y = cp * sy;
			forward->z = -sp;
		}

		bool hit_chance( Player* pEnt, vec3_t Angle, vec3_t Point, int chance ) {
			if ( !chance )
				return true;

			int iHit = 0;
			int iHitsNeed = ( int )( ( float )256 * ( ( float )chance / 100.f ) );
			bool bHitchance = false;

			vec3_t forward, right, up;
			AngleVectors( Angle, &forward, &right, &up );

			auto v1 = g_cl.m_weapon;
			v1->UpdateAccuracyPenalty( );

			for ( auto i = 0; i < 256; ++i ) {
				float RandomA = g_csgo.RandomFloat( 0.0f, 1.0f );
				float RandomB = 1.0f - RandomA * RandomA;

				RandomA = g_csgo.RandomFloat( 0.0f, M_PI_F * 2.0f );
				RandomB *= v1->GetInaccuracy( );

				float SpreadX1 = ( cos( RandomA ) * RandomB );
				float SpreadY1 = ( sin( RandomA ) * RandomB );

				float RandomC = g_csgo.RandomFloat( 0.0f, 1.0f );
				float RandomF = RandomF = 1.0f - RandomC * RandomC;

				RandomC = g_csgo.RandomFloat( 0.0f, M_PI_F * 2.0f );
				RandomF *= v1->GetSpread( );

				float SpreadX2 = ( cos( RandomC ) * RandomF );
				float SpreadY2 = ( sin( RandomC ) * RandomF );

				float fSpreadX = SpreadX1 + SpreadX2;
				float fSpreadY = SpreadY1 + SpreadY2;

				vec3_t vSpreadForward;
				vSpreadForward[ 0 ] = forward[ 0 ] + ( fSpreadX * right[ 0 ] ) + ( fSpreadY * up[ 0 ] );
				vSpreadForward[ 1 ] = forward[ 1 ] + ( fSpreadX * right[ 1 ] ) + ( fSpreadY * up[ 1 ] );
				vSpreadForward[ 2 ] = forward[ 2 ] + ( fSpreadX * right[ 2 ] ) + ( fSpreadY * up[ 2 ] );
				vSpreadForward.NormalizeInPlace( );

				vec3_t qaNewAngle;
				VectorAngles( vSpreadForward, qaNewAngle );
				NormalizeAngles( qaNewAngle );

				vec3_t vEnd;
				AngleVectors( qaNewAngle, &vEnd );

				vEnd = g_cl.m_local->get_eye_pos( ) + ( vEnd * 8192.f );

				CGameTrace tr;
				Ray_t ray;

				ray.Init( g_cl.m_local->get_eye_pos( ), vEnd );
				g_csgo.m_engine_trace->ClipRayToEntity( ray, MASK_SHOT | CONTENTS_GRATE, pEnt, &tr );

				if ( tr.m_entity == pEnt )
					iHit++;

				if ( ( int )( ( ( float )iHit / 256 ) * 100.f ) >= chance ) {
					bHitchance = true;
					break;
				}

				if ( ( 256 - 1 - i + iHit ) < iHitsNeed )
					break;
			}

			return bHitchance;
		}

		bool should_baim( Player* pEnt ) {
			static float oldSimtime[ 65 ];
			static float storedSimtime[ 65 ];

			static float ShotTime[ 65 ];
			static float NextShotTime[ 65 ];
			static bool BaimShot[ 65 ];

			if ( storedSimtime[ pEnt->index( ) ] != pEnt->m_flSimulationTime( ) )
			{
				oldSimtime[ pEnt->index( ) ] = storedSimtime[ pEnt->index( ) ];
				storedSimtime[ pEnt->index( ) ] = pEnt->m_flSimulationTime( );
			}

			float simDelta = storedSimtime[ pEnt->index( ) ] - oldSimtime[ pEnt->index( ) ];
			bool Shot = false;

			if ( pEnt->GetActiveWeapon( ) && !( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_KNIFE ) )
			{
				if ( ShotTime[ pEnt->index( ) ] != pEnt->GetActiveWeapon( )->m_fLastShotTime( ) )
				{
					Shot = true;
					BaimShot[ pEnt->index( ) ] = false;
					ShotTime[ pEnt->index( ) ] = pEnt->GetActiveWeapon( )->m_fLastShotTime( );
				}
				else
					Shot = false;
			}
			else
			{
				Shot = false;
				ShotTime[ pEnt->index( ) ] = 0.f;
			}

			if ( Shot )
			{
				NextShotTime[ pEnt->index( ) ] = pEnt->m_flSimulationTime( ) + g_cl.m_weapon->FireRate( );

				if ( simDelta >= g_cl.m_weapon->FireRate( ) )
					BaimShot[ pEnt->index( ) ] = true;
			}

			if ( BaimShot[ pEnt->index( ) ] )
			{
				if ( pEnt->m_flSimulationTime( ) >= NextShotTime[ pEnt->index( ) ] )
					BaimShot[ pEnt->index( ) ] = false;
			}

			return false;
		}

		float Distance( vec2_t point1, vec2_t point2 )
		{
			float diffY = point1.y - point2.y;
			float diffX = point1.x - point2.x;
			return sqrt( ( diffY * diffY ) + ( diffX * diffX ) );
		}

		vec3_t hitscan( Player* pEnt ) {
			float DamageArray[ 28 ];
			float tempDmg = 0.f;
			vec3_t tempHitbox = { 0,0,0 };
			static int HitboxForMuti[ ] = { 2,2,4,4,6,6 };

			float angToLocal = math::CalcAngle( g_cl.m_local->m_vecOrigin( ), pEnt->m_vecOrigin( ) ).y;

			vec2_t MutipointXY = { ( sin( GRD_TO_BOG( angToLocal ) ) ),( cos( GRD_TO_BOG( angToLocal ) ) ) };
			vec2_t MutipointXY180 = { ( sin( GRD_TO_BOG( angToLocal + 180 ) ) ) ,( cos( GRD_TO_BOG( angToLocal + 180 ) ) ) };
			vec2_t Mutipoint[ ] = { vec2_t( MutipointXY.x, MutipointXY.y ), vec2_t( MutipointXY180.x, MutipointXY180.y ) };
			float Velocity = abs( pEnt->m_vecVelocity( ).length_2d( ) );

			if ( !settings.delay_shortan && Velocity > 29.f )
				Velocity = 30.f;

			std::vector<int> Scan;

			int HeadHeight = 0;

			bool Baim = should_baim( pEnt );
			if ( !Baim )
				Scan.push_back( HITBOX_HEAD );

			if ( Velocity <= 215.f || Baim ) {
				Scan.push_back( HITBOX_PELVIS );
				Scan.push_back( 19 ); // pelvis mutipoint 1
				Scan.push_back( 20 ); // pelvis mutipoint 2
				Scan.push_back( HITBOX_THORAX );
				Scan.push_back( 21 ); // thorax mutipoint 1
				Scan.push_back( 22 ); // thorax mutipoint 2
				Scan.push_back( 5 );
				Scan.push_back( HITBOX_UPPER_CHEST );
				Scan.push_back( 23 ); // upper chest mutipoint 1
				Scan.push_back( 24 ); // upper chest mutipoint 2
				Scan.push_back( 17 );
				Scan.push_back( 15 );
				Scan.push_back( 7 );
				Scan.push_back( 8 );
				Scan.push_back( 9 );
				Scan.push_back( 10 );

				HeadHeight = settings.dickhead_height;
			}

			vec3_t Hitbox;
			int bestHitboxint = 0;

			for ( int hitbox : Scan ) {
				if ( hitbox < 19 )
					Hitbox = GetHitboxPosition( pEnt, hitbox, matrix[ pEnt->index( ) ] );
				else if ( hitbox > 18 && hitbox < 25 ) {
					float Radius = 0;
					Hitbox = GetHitboxPosition( pEnt, HitboxForMuti[ hitbox - 19 ], matrix[ pEnt->index( ) ], &Radius );
					Radius *= ( settings.dickbody_scale / 100.f );
					Hitbox = vec3_t( Hitbox.x + ( Radius * Mutipoint[ ( ( hitbox - 19 ) % 2 ) ].x ), Hitbox.y - ( Radius * Mutipoint[ ( ( hitbox - 19 ) % 2 ) ].y ), Hitbox.z );

				}
				else if ( hitbox > 24 && hitbox < 28 ) {
					float Radius = 0;
					Hitbox = GetHitboxPosition( pEnt, 0, matrix[ pEnt->index( ) ], &Radius );
					Radius *= ( HeadHeight / 100.f );
					if ( hitbox != 27 )
						Hitbox = vec3_t( Hitbox.x + ( Radius * Mutipoint[ ( ( hitbox - 25 ) % 2 ) ].x ), Hitbox.y - ( Radius * Mutipoint[ ( ( hitbox - 25 ) % 2 ) ].y ), Hitbox.z );
					else
						Hitbox += vec3_t( 0, 0, Radius );

				}

				float Damage = g_Autowall->CanHit( g_cl.m_local->get_eye_pos( ), Hitbox );
				if ( Damage > 0.f )
					DamageArray[ hitbox ] = Damage;
				else
					DamageArray[ hitbox ] = 0;

				if ( DamageArray[ hitbox ] > tempDmg )
				{
					tempHitbox = Hitbox;
					bestHitboxint = hitbox;
					tempDmg = DamageArray[ hitbox ];
				}
			}

			PlayerRecords pPlayerEntityRecord = g_LagComp->PlayerRecord[ pEnt->index( ) ].at( 0 );

			backtrack[ pEnt->index( ) ] = false;
			shot_backtrack[ pEnt->index( ) ] = false;

			static float ciocan = 0;

			if ( settings.dick_backtrack && g_LagComp->ShotTick[ pEnt->index( ) ] != -1 && g_Autowall->CanHitFloatingPoint( GetHitboxPosition( pEnt, HITBOX_HEAD, g_LagComp->PlayerRecord[ pEnt->index( ) ].at( g_LagComp->ShotTick[ pEnt->index( ) ] ).Matrix ) + vec3_t( 0, 0, 1 ), g_cl.m_local->get_eye_pos(), &ciocan ) && !Baim )
			{
				best_ent_dmg = ( 1000000.f - fabs( Distance( vec2_t( g_cl.m_local->m_vecOrigin( ).x, g_cl.m_local->m_vecOrigin( ).y ), vec2_t( pEnt->m_vecOrigin( ).x, pEnt->m_vecOrigin( ).y ) ) ) ); // just doing this to get the closest player im backtracking
				shot_backtrack[ pEnt->index( ) ] = true;
				return GetHitboxPosition( pEnt, HITBOX_HEAD, g_LagComp->PlayerRecord[ pEnt->index( ) ].at( g_LagComp->ShotTick[ pEnt->index( ) ] ).Matrix ) + vec3_t( 0, 0, 1 );
			}
			else if ( tempDmg >= settings.dickdamage )
			{
				best_ent_dmg = tempDmg;

				if ( ( bestHitboxint == 25 || bestHitboxint == 26 || bestHitboxint == 27 ) && abs( DamageArray[ HITBOX_HEAD ] - DamageArray[ bestHitboxint ] ) <= 10.f )
					return GetHitboxPosition( pEnt, HITBOX_HEAD, matrix[ pEnt->index( ) ] );
				else if ( ( bestHitboxint == 19 || bestHitboxint == 20 ) && DamageArray[ HITBOX_PELVIS ] > 30 )
					return GetHitboxPosition( pEnt, HITBOX_PELVIS, matrix[ pEnt->index( ) ] );
				else if ( ( bestHitboxint == 21 || bestHitboxint == 22 ) && DamageArray[ HITBOX_THORAX ] > 30 )
					return GetHitboxPosition( pEnt, HITBOX_THORAX, matrix[ pEnt->index( ) ] );
				else if ( ( bestHitboxint == 23 || bestHitboxint == 24 ) && DamageArray[ HITBOX_UPPER_CHEST ] > 30 )
					return GetHitboxPosition( pEnt, HITBOX_UPPER_CHEST, matrix[ pEnt->index( ) ] );

				return tempHitbox;
			}
			else if ( settings.dickbacktrackpos && pPlayerEntityRecord.Velocity >= 29.f && g_Autowall->CanHitFloatingPoint( GetHitboxPosition( pEnt, HITBOX_HEAD, pPlayerEntityRecord.Matrix ), g_cl.m_local->get_eye_pos( ), &ciocan ) )
			{
				best_ent_dmg = ( 100000.f - fabs( Distance( vec2_t( g_cl.m_local->m_vecOrigin( ).x, g_cl.m_local->m_vecOrigin( ).y ), vec2_t( pEnt->m_vecOrigin( ).x, pEnt->m_vecOrigin( ).y ) ) ) );
				backtrack[ pEnt->index( ) ] = true;
				return GetHitboxPosition( pEnt, HITBOX_HEAD, pPlayerEntityRecord.Matrix );
			}

			return vec3_t( 0, 0, 0 );
		}

		void on_create_move( CUserCmd* pCmd, bool* bSendPacket ) {
			if ( !g_csgo.m_engine->IsInGame( ) )
				return;

			vec3_t Aimpoint = { 0,0,0 };
			Player* Target = nullptr;

			int targetID = 0;
			int tempDmg = 0;
			static bool shot = false;
			static float ShotTime = 0;

			for ( int i = 1; i <= g_csgo.m_globals->m_max_clients; ++i ) {
				Player* pPlayerEntity = g_csgo.m_entlist->GetClientEntity< Player* >( i );

				if ( !pPlayerEntity || !pPlayerEntity->alive( ) || pPlayerEntity->dormant( ) )
				{
					g_LagComp->ClearRecords( i );
					continue;
				}

				g_LagComp->StoreRecord( pPlayerEntity, pCmd );

				if ( pPlayerEntity == g_cl.m_local || pPlayerEntity->m_iTeamNum( ) == g_cl.m_local->m_iTeamNum( ) )
					continue;

				// i dont use this anywhere
				//g_Globals->EnemyEyeAngs[ i ] = pPlayerEntity->m_angEyeAngles( );

				if ( g_LagComp->PlayerRecord[ i ].size( ) == 0 || !g_cl.m_local->alive( ) || !settings.enable_stacker )
					continue;

				if ( !g_cl.m_weapon || ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_KNIFE ) )
					continue;

				best_ent_dmg = 0;

				vec3_t Hitbox = hitscan( pPlayerEntity );
				if ( Hitbox != vec3_t( 0, 0, 0 ) && tempDmg <= best_ent_dmg )
				{
					Aimpoint = Hitbox;
					Target = pPlayerEntity;
					targetID = Target->index( );
					tempDmg = best_ent_dmg;
				}
			}

			if ( !g_cl.m_local->alive( ) )
			{
				shot = false;
				return;
			}

			if ( !g_cl.m_weapon || ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_KNIFE ) )
			{
				shot = false;
				return;
			}

			float flServerTime = g_cl.m_local->m_nTickBase( ) * g_csgo.m_globals->m_interval;
			bool canShoot = ( g_cl.m_weapon->m_flNextPrimaryAttack() <= flServerTime );

			if ( Target ) {
				static int TargetIndex = -1;
				TargetIndex = targetID;

				float SimulationTime = 0.f;

				if ( backtrack[ targetID ] )
					SimulationTime = g_LagComp->PlayerRecord[ targetID ].at( 0 ).SimTime;
				else
					SimulationTime = g_LagComp->PlayerRecord[ targetID ].at( g_LagComp->PlayerRecord[ targetID ].size( ) - 1 ).SimTime;

				if ( shot_backtrack[ targetID ] )
					SimulationTime = g_LagComp->PlayerRecord[ targetID ].at( g_LagComp->ShotTick[ targetID ] ).SimTime;


				vec3_t Angle = math::CalcAngle( g_cl.m_local->get_eye_pos(), Aimpoint );

				if ( !( pCmd->m_buttons & IN_ATTACK ) && canShoot && hit_chance( Target, Angle, Aimpoint, settings.dickchance ) )
				{
					//if ( !backtrack[ targetID ] && !shot_backtrack[ targetID ] )
						//g_Globals->ShotBos[ targetID ] = true;

					*bSendPacket = true;
					shot = true;

					auto punch_angle = g_cl.m_local->m_aimPunchAngle( );
					auto recoil_scale = g_csgo.weapon_recoil_scale->GetFloat( );
					auto correction = punch_angle * recoil_scale;

					g_cl.m_cmd->m_view_angles -= correction;
					g_cl.m_cmd->m_view_angles.normalize( );

					pCmd->m_buttons |= IN_ATTACK;
					pCmd->m_tick = game::TIME_TO_TICKS( SimulationTime + g_LagComp->LerpTime( ) );
				}
			}
		}
	}
}