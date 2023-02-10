#include "includes.h"
#include "backend/config/config_new.h"
HVH g_hvh{ };;

// distortion onetap / dopium / immortal
const float DISTORTION_SPEED = (float)settings.distortion_speed;
const float DISTORTION_RANGE = ( float )settings.distortion_angle;

bool g_bSwitch = false;
float g_flTimer = 0.f;

float HVH::Distortion( ) {
	auto local = g_cl.m_local;
	if ( !local || local->m_iHealth( ) <= 0 || !local->alive( ) )
		return FLT_MAX;

	static float flRetValue = FLT_MAX;

	if ( !*g_cl.m_packet ) {
		static float flLastMovingLBY = FLT_MAX;
		static bool bDistortYaw = false;
		static bool bGeneratedRandomYaw = false;
		static float flRandomDistort = FLT_MAX;
		static int nDistortionAttempts = 0;
		static float flTimeElapsed = FLT_MAX;
		static bool bIntensityWasAtLowest = false;
		static bool bIntensityWasAtHighest = false;
		static bool bReGeneratedYaw = false;

		// we dont have manu aa yet
		if ( !g_hvh.m_right && !g_hvh.m_left && !g_hvh.m_back ) {
			flRetValue = 0.f;
		}

		if ( g_hvh.m_right || g_hvh.m_left || g_hvh.m_back ) {
			if ( g_hvh.m_right ) {
				flRetValue = 90.f;
			}
			if ( g_hvh.m_back ) {
				flRetValue = 180.f;
			}
			if ( g_hvh.m_left ) {
				flRetValue = -90.f;
			}
		}
		
		bool bLocalMoving = local->m_vecVelocity( ).length( ) > 0.1f;

		if ( bLocalMoving ) {
			flLastMovingLBY = local->m_flLowerBodyYawTarget( );

			bGeneratedRandomYaw = false;

			bIntensityWasAtLowest = bIntensityWasAtHighest = bReGeneratedYaw = false;
		}

		if ( !bLocalMoving && flLastMovingLBY != FLT_MAX ) {
			// since we saved our last move yaw (that any player could've got and resolved us perfectly)
			// our real is still most likely in very close proximity to the last saved yaw, which means that any good
			// hack will attempt to shoot at our last moving lby with some sort of variation to attempt to 
			// hit our head. let's get as far away from our last moving lby as possible.
			if ( !bDistortYaw ) { //if( fabs( local->m_flLowerBodyYawTarget( ) - flLastMovingLBY ) < 15.f ) {
				bDistortYaw = true;
			}

			// let's distort our yaw
			if ( bDistortYaw ) {
				float flDistortTolerance = 256.f;
				float flDistortIntensity = ( floor( sin( g_csgo.m_globals->m_realtime * 4.20f ) * ( flDistortTolerance / 2 - 1 ) + flDistortTolerance / 2 ) ) / 256.f;

				if ( flDistortIntensity >= 0.9f && !bIntensityWasAtHighest ) {
					bIntensityWasAtHighest = true;
				}

				if ( flDistortIntensity <= 0.0f && !bIntensityWasAtLowest && bIntensityWasAtHighest ) {
					bIntensityWasAtLowest = true;
				}

				// hehe, let's keep goin in a circle!
				if ( bIntensityWasAtLowest && bIntensityWasAtHighest ) {
					bGeneratedRandomYaw = false;
					bReGeneratedYaw = true;
				}

				if ( !bGeneratedRandomYaw ) {
					// let's generate some random yaw that we will base the "distortion" off
					//std::uniform_int_distribution random(-180, 180);
					flRandomDistort = g_csgo.RandomInt( -180, 180 );

					// let's only allow medium distortions, we don't want to be
					// distorting a tiny yaw because that will do no good.
					// was 35
					if ( fabs( flRandomDistort ) > 35.f || fabs( flRandomDistort ) < -35.f ) // if( flRandomDistort < -35.f || flRandomDistort > 35.f )
						bGeneratedRandomYaw = true;
				}

				// let's start at our last move lby and go from there.
				flRetValue = flLastMovingLBY;
				flRetValue += ( flRandomDistort * flDistortIntensity );

				float flTimeDelta = fabs( g_csgo.m_globals->m_realtime - flTimeElapsed );

				// more than enough time has passed, let's check up on the distortion
				if ( flTimeDelta > 3.5f ) {
					// if our yaw is already different enough, then we're done here. was 60
					if ( fabs( local->m_flLowerBodyYawTarget( ) - flLastMovingLBY ) > 60.f ) {
						// make sure we've fucked this shit enough
						if ( bReGeneratedYaw ) {
							// let's stop here.
							bDistortYaw = false;
						}

						// let's finalize our distorted yaw
						flRetValue = flLastMovingLBY + fabs( local->m_flLowerBodyYawTarget( ) - flLastMovingLBY );

						// let's reset everything else too.
						nDistortionAttempts = 0;
						bGeneratedRandomYaw = false;
						flLastMovingLBY = flRandomDistort = FLT_MAX;

						// let's return now.
					jmpForceRet:
						return flRetValue;
					}
					// something didn't go right, let's continue.
					else {
						// we failed to distort twice now, fuck it, let's just do it
						// manually and pray to allah our head doesn't get hit.
						if ( nDistortionAttempts >= 2 ) {
							// let's stop here.
							bDistortYaw = false;

							// mergh, random number, whatever.
							flRetValue = flLastMovingLBY + 75.f;

							// let's reset everything else now.
							nDistortionAttempts = 0;
							bGeneratedRandomYaw = false;
							flLastMovingLBY = flRandomDistort = FLT_MAX;

							// let's return for now.
							goto jmpForceRet;
						}
						else {
							// note the amount of times our distortions have failed.
							nDistortionAttempts++;

							// generate a new random yaw next time.
							bGeneratedRandomYaw = false;
						}
					}
				}
			}
		}
	}

	return flRetValue;
}
void HVH::DoDistortion( ) {
	if ( !settings.distortion )
		return;

	switch ( settings.distortion_type ) {
		case 0: { // dopium -> this might be wrong reversed
			float speed = ( DISTORTION_SPEED * 0.01f ) * 0.0625f;
			float distortion_angle = DISTORTION_RANGE * ( 1.f - std::powf( 1.f - g_flTimer, 2 ) ) - ( DISTORTION_RANGE * 0.5f );

			g_cl.m_cmd->m_view_angles.y += g_bSwitch ? distortion_angle : -distortion_angle;
			g_flTimer += speed;
			if ( g_flTimer >= 0.7f )
			{
				g_flTimer = 0.f;
				g_bSwitch = !g_bSwitch;
			}
		} break;
		case 1: { // onetap
			if ( !g_cl.m_processing )
				return;

			bool bDoDistort = true;

			 // not yet
			if ( g_hvh.m_left || g_hvh.m_right || g_hvh.m_back ) {
				bDoDistort = false;
				return;
			}
			

			if ( !g_cl.m_ground )
				bDoDistort = false;

			static float flLastMoveTime = FLT_MAX;
			static float flLastMoveYaw = FLT_MAX;
			static bool bGenerate = true;
			static float flGenerated = 0.f;
			auto distortdelta = settings.distortion_angle;
			auto distortspeed = settings.distortion_speed;
			if ( g_cl.m_speed > 5.f && g_cl.m_ground && !GetKeyState( settings.fakewalkke ) ) {
				flLastMoveTime = g_csgo.m_globals->m_realtime;
				flLastMoveYaw = g_cl.m_local->m_flLowerBodyYawTarget( );

				if ( true )
					bDoDistort = false;
			}

			if ( flLastMoveTime == FLT_MAX )
				return;

			if ( flLastMoveYaw == FLT_MAX )
				return;

			if ( !bDoDistort ) {
				bGenerate = true;
			}

			if ( !settings.distortion_force_turn ) {
				if ( bDoDistort ) {
					if ( true ) {
						float flDistortion = std::sin( ( g_csgo.m_globals->m_realtime * distortspeed * 2 ) * 0.5f + 0.5f );

						g_cl.m_cmd->m_view_angles.y += distortdelta * flDistortion;
						return;
					}

					if ( bGenerate ) {
						float flNormalised = std::remainderf( 45.f, 360.f );

						flGenerated = g_csgo.RandomFloat( -flNormalised, flNormalised );
						bGenerate = false;
					}

					float flDelta = fabs( flLastMoveYaw - g_cl.m_local->m_flLowerBodyYawTarget( ) );
					g_cl.m_cmd->m_view_angles.y += flDelta + flGenerated;
				}
			}
			else {
				if ( bDoDistort ) {
					if ( true ) {
						distortspeed *= 100;
						g_cl.m_cmd->m_view_angles.y += std::fmod( g_csgo.m_globals->m_curtime * distortspeed, 360.f );
						return;
					}
				}
			}
		} break;
		case 2: { // immortal
			auto local = g_cl.m_local;
			if ( !local || !local->alive( ) )
				return;

			bool bDoDistort = true;
			if ( GetKeyState(settings.fakewalkke) )
				bDoDistort = false;

			if ( settings.disable_distort_in_air && !( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) )
				bDoDistort = false;

			auto m_bGround = g_cl.m_local->m_fFlags( ) & FL_ONGROUND;
			auto Fakewalking = GetKeyState( settings.fakewalkke );

			static float flLastMoveTime = FLT_MAX;
			static float flLastMoveYaw = FLT_MAX;
			static bool bGenerate = true;
			static float flGenerated = 0.f;

			if ( local->m_vecVelocity().length_2d() > 10.0f && m_bGround && !Fakewalking ) {
				flLastMoveTime = g_csgo.m_globals->m_realtime;
				flLastMoveYaw = local->m_flLowerBodyYawTarget( );

				if ( settings.disable_distort_when_run )
					bDoDistort = false;
			}

			// manual aa checking later -- note::

			if ( flLastMoveTime == FLT_MAX )
				return;

			if ( flLastMoveYaw == FLT_MAX )
				return;

			if ( !bDoDistort ) {
				bGenerate = true;
			}

			if ( bDoDistort ) {
				if ( fabs( g_csgo.m_globals->m_realtime - flLastMoveTime ) > settings.distortion_max_time && settings.distortion_max_time > 0.f ) {
					return;
				}

				if ( settings.distort_twist ) {
					float flDistortion = std::sin( ( g_csgo.m_globals->m_realtime * settings.distortion_speed ) * 0.5f + 0.5f );

					g_cl.m_cmd->m_view_angles.y += settings.distortion_angle * flDistortion;
					return;
				}

				if ( bGenerate ) {
					float flNormalised = std::remainderf( settings.distortion_angle, 360.f );

					flGenerated = g_csgo.RandomFloat( -flNormalised, flNormalised );
					bGenerate = false;
				}

				//float flDelta = fabs( flLastMoveYaw - local->m_flLowerBodyYawTarget( ) );
				//g_cl.m_cmd->m_view_angles.y += flDelta + flGenerated;
			}
		}
	}
}

void HVH::IdealPitch( ) {
	CCSGOPlayerAnimState *state = g_cl.m_local->m_PlayerAnimState( );
	if( !state )
		return;

	g_cl.m_cmd->m_view_angles.x = state->m_min_pitch;
}

void HVH::AntiAimPitch( ) {
	
	switch( m_pitch ) {
	case 1:
		// down.
		g_cl.m_cmd->m_view_angles.x = 89.f;
		break;

	case 2:
		// up.
		g_cl.m_cmd->m_view_angles.x = -89.f;
		break;

	case 3:
		// random.
		g_cl.m_cmd->m_view_angles.x = g_csgo.RandomFloat( -89.f, 89.f );
		break;

	case 4:
		// ideal.
		IdealPitch( );
		break;

	default:
		break;
	}
}

void HVH::AutoDirection( ) {
	// constants.
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 32.f };

	// best target.
	struct AutoTarget_t { float fov; Player *player; };
	AutoTarget_t target{ 180.f + 1.f, nullptr };

	// iterate players.
	for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player *player = g_csgo.m_entlist->GetClientEntity< Player * >( i );

		// validate player.
		if( !g_aimbot.IsValidTarget( player ) )
			continue;

		// skip dormant players.
		if( player->dormant( ) )
			continue;

		// get best target based on fov.
		float fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, player->WorldSpaceCenter( ) );

		if( fov < target.fov ) {
			target.fov = fov;
			target.player = player;
		}
	}

	if( !target.player ) {
		// we have a timeout.
		if( m_auto_last > 0.f && m_auto_time > 0.f && g_csgo.m_globals->m_curtime < ( m_auto_last + m_auto_time ) )
			return;

		// set angle to backwards.
		m_auto = math::NormalizedAngle( m_view - 180.f );
		m_auto_dist = -1.f;
		return;
	}

	/*
	* data struct
	* 68 74 74 70 73 3a 2f 2f 73 74 65 61 6d 63 6f 6d 6d 75 6e 69 74 79 2e 63 6f 6d 2f 69 64 2f 73 69 6d 70 6c 65 72 65 61 6c 69 73 74 69 63 2f
	*/

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back( m_view - 180.f );
	angles.emplace_back( m_view + 90.f );
	angles.emplace_back( m_view - 90.f );

	// start the trace at the enemy shoot pos.
	vec3_t start = target.player->GetShootPosition( );

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for( auto it = angles.begin( ); it != angles.end( ); ++it ) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ g_cl.m_shoot_pos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * RANGE,
			g_cl.m_shoot_pos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * RANGE,
			g_cl.m_shoot_pos.z };

		// draw a line for debugging purposes.
		//g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize( );

		// should never happen.
		if( len <= 0.f )
			continue;

		// step thru the total distance, 4 units per step.
		for( float i{ 0.f }; i < len; i += STEP ) {
			// get the current step position.
			vec3_t point = start + ( dir * i );

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents( point, MASK_SHOT_HULL );

			// contains nothing that can stop a bullet.
			if( !( contents & MASK_SHOT_HULL ) )
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if( i > ( len * 0.5f ) )
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if( i > ( len * 0.75f ) )
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if( i > ( len * 0.9f ) )
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += ( STEP * mult );

			// mark that we found anything.
			valid = true;
		}
	}

	if( !valid ) {
		// set angle to backwards.
		m_auto = math::NormalizedAngle( m_view - 180.f );
		m_auto_dist = -1.f;
		return;
	}

	// put the most distance at the front of the container.
	std::sort( angles.begin( ), angles.end( ),
		[ ] ( const AdaptiveAngle &a, const AdaptiveAngle &b ) {
		return a.m_dist > b.m_dist;
	} );

	// the best angle should be at the front now.
	AdaptiveAngle *best = &angles.front( );

	// check if we are not doing a useless change.
	if( best->m_dist != m_auto_dist ) {
		// set yaw to the best result.
		m_auto = math::NormalizedAngle( best->m_yaw );
		m_auto_dist = best->m_dist;
		m_auto_last = g_csgo.m_globals->m_curtime;
	}
}

void HVH::GetAntiAimDirection( ) {
	// edge aa.
	if( settings.edge && g_cl.m_local->m_vecVelocity( ).length( ) < 320.f ) {

		ang_t ang;
		if( DoEdgeAntiAim( g_cl.m_local, ang ) ) {
			m_direction = ang.y;
			return;
		}
	}

	if ( settings.manualaa ) {
		m_direction = ( m_left ? m_view + 90.f : ( m_right ? m_view - 90.f : ( m_back ? m_view + 180.f : m_direction ) ) );
		if ( m_left || m_right || m_back ) return;
	}


	// lock while standing..
	bool lock = settings.dir_lock;

	// save view, depending if locked or not.
	if( ( lock && g_cl.m_speed > 0.1f ) || !lock )
		m_view = g_cl.m_cmd->m_view_angles.y;

	if( m_base_angle > 0 ) {
		// 'static'.
		if( m_base_angle == 1 )
			m_view = 0.f;

		// away options.
		else {
			float  best_fov{ std::numeric_limits< float >::max( ) };
			float  best_dist{ std::numeric_limits< float >::max( ) };
			float  fov, dist;
			Player *target, *best_target{ nullptr };

			for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
				target = g_csgo.m_entlist->GetClientEntity< Player * >( i );

				if( !g_aimbot.IsValidTarget( target ) )
					continue;

				if( target->dormant( ) )
					continue;

				// 'away crosshair'.
				if( m_base_angle == 2 ) {

					// check if a player was closer to our crosshair.
					fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, target->WorldSpaceCenter( ) );
					if( fov < best_fov ) {
						best_fov = fov;
						best_target = target;
					}
				}

				// 'away distance'.
				else if( m_base_angle == 3 ) {

					// check if a player was closer to us.
					dist = ( target->m_vecOrigin( ) - g_cl.m_local->m_vecOrigin( ) ).length_sqr( );
					if( dist < best_dist ) {
						best_dist = dist;
						best_target = target;
					}
				}
			}

			if( best_target ) {
				// todo - dex; calculate only the yaw needed for this (if we're not going to use the x component that is).
				ang_t angle;
				math::VectorAngles( best_target->m_vecOrigin( ) - g_cl.m_local->m_vecOrigin( ), angle );
				m_view = angle.y;
			}
		}
	}

	// switch direction modes.
	switch( m_dir ) {

		// auto.
	case 0:
		AutoDirection( );
		m_direction = m_auto;
		break;

		// backwards.
	case 1:
		m_direction = m_view + 180.f;
		break;

		// left.
	case 2:
		m_direction = m_view + 90.f;
		break;

		// right.
	case 3:
		m_direction = m_view - 90.f;
		break;

		// custom.
	case 4:
		m_direction = m_view + m_dir_custom;
		break;

	default:
		break;
	}

	// normalize the direction.
	math::NormalizeAngle( m_direction );
}

bool HVH::DoEdgeAntiAim( Player *player, ang_t &out ) {
	CGameTrace trace;
	static CTraceFilterSimple_game filter{ };

	if( player->m_MoveType( ) == MOVETYPE_LADDER )
		return false;

	// skip this player in our traces.
	filter.SetPassEntity( player );

	// get player bounds.
	vec3_t mins = player->m_vecMins( );
	vec3_t maxs = player->m_vecMaxs( );

	// make player bounds bigger.
	mins.x -= 20.f;
	mins.y -= 20.f;
	maxs.x += 20.f;
	maxs.y += 20.f;

	// get player origin.
	vec3_t start = player->GetAbsOrigin( );

	// offset the view.
	start.z += 56.f;

	g_csgo.m_engine_trace->TraceRay( Ray( start, start, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	if( !trace.m_startsolid )
		return false;

	float  smallest = 1.f;
	vec3_t plane;

	// trace around us in a circle, in 20 steps (anti-degree conversion).
	// find the closest object.
	for( float step{ }; step <= math::pi_2; step += ( math::pi / 10.f ) ) {
		// extend endpoint x units.
		vec3_t end = start;

		// set end point based on range and step.
		end.x += std::cos( step ) * 32.f;
		end.y += std::sin( step ) * 32.f;

		g_csgo.m_engine_trace->TraceRay( Ray( start, end, { -1.f, -1.f, -8.f }, { 1.f, 1.f, 8.f } ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );

		// we found an object closer, then the previouly found object.
		if( trace.m_fraction < smallest ) {
			// save the normal of the object.
			plane = trace.m_plane.m_normal;
			smallest = trace.m_fraction;
		}
	}

	// no valid object was found.
	if( smallest == 1.f || plane.z >= 0.1f )
		return false;

	// invert the normal of this object
	// this will give us the direction/angle to this object.
	vec3_t inv = -plane;
	vec3_t dir = inv;
	dir.normalize( );

	// extend point into object by 24 units.
	vec3_t point = start;
	point.x += ( dir.x * 24.f );
	point.y += ( dir.y * 24.f );

	// check if we can stick our head into the wall.
	if( g_csgo.m_engine_trace->GetPointContents( point, CONTENTS_SOLID ) & CONTENTS_SOLID ) {
		// trace from 72 units till 56 units to see if we are standing behind something.
		g_csgo.m_engine_trace->TraceRay( Ray( point + vec3_t{ 0.f, 0.f, 16.f }, point ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );

		// we didnt start in a solid, so we started in air.
		// and we are not in the ground.
		if( trace.m_fraction < 1.f && !trace.m_startsolid && trace.m_plane.m_normal.z > 0.7f ) {
			// mean we are standing behind a solid object.
			// set our angle to the inversed normal of this object.
			out.y = math::rad_to_deg( std::atan2( inv.y, inv.x ) );
			return true;
		}
	}

	// if we arrived here that mean we could not stick our head into the wall.
	// we can still see if we can stick our head behind/asides the wall.

	// adjust bounds for traces.
	mins = { ( dir.x * -3.f ) - 1.f, ( dir.y * -3.f ) - 1.f, -1.f };
	maxs = { ( dir.x * 3.f ) + 1.f, ( dir.y * 3.f ) + 1.f, 1.f };

	// move this point 48 units to the left 
	// relative to our wall/base point.
	vec3_t left = start;
	left.x = point.x - ( inv.y * 48.f );
	left.y = point.y - ( inv.x * -48.f );

	g_csgo.m_engine_trace->TraceRay( Ray( left, point, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	float l = trace.m_startsolid ? 0.f : trace.m_fraction;

	// move this point 48 units to the right 
	// relative to our wall/base point.
	vec3_t right = start;
	right.x = point.x + ( inv.y * 48.f );
	right.y = point.y + ( inv.x * -48.f );

	g_csgo.m_engine_trace->TraceRay( Ray( right, point, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	float r = trace.m_startsolid ? 0.f : trace.m_fraction;

	// both are solid, no edge.
	if( l == 0.f && r == 0.f )
		return false;

	// set out to inversed normal.
	out.y = math::rad_to_deg( std::atan2( inv.y, inv.x ) );

	// left started solid.
	// set angle to the left.
	if( l == 0.f ) {
		out.y += 90.f;
		return true;
	}

	// right started solid.
	// set angle to the right.
	if( r == 0.f ) {
		out.y -= 90.f;
		return true;
	}

	return false;
}

float AngleDiff( float src, float dst ) {
	float i;

	for ( ; src > 180.0; src = src - 360.0 )
		;
	for ( ; src < -180.0; src = src + 360.0 )
		;
	for ( ; dst > 180.0; dst = dst - 360.0 )
		;
	for ( ; dst < -180.0; dst = dst + 360.0 )
		;
	for ( i = dst - src; i > 180.0; i = i - 360.0 )
		;
	for ( ; i < -180.0; i = i + 360.0 )
		;

	return i;
}

float lby_timer;
bool lby_update( bool update ) { // onetap reversed code
	if ( !update )
		return false;

	auto state = g_cl.m_local->m_PlayerAnimState( );
	if ( !state )
		return false;

	float predicted_cur_time = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );
	float lby_timer = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );

	if ( state->m_speed < 10.f || fabs( state->m_fall_velocity ) > 100.f ) {
		lby_timer = predicted_cur_time + 0.22f;
	}
	else if ( predicted_cur_time > lby_timer && fabs( AngleDiff( state->m_cur_feet_yaw, state->m_eye_yaw ) ) > 35.f ) {
		lby_timer = predicted_cur_time + 1.1f;
	}

	return true; // this might be wrong reversed
}

bool boxhackbreaker_flip;
void HVH::DoRealAntiAim( ) {
	// if we have a yaw antaim.
	if( m_yaw > 0 ) {
		switch ( settings.exploit_aa ) {
			case 0: { // breaker
				if ( GetKeyState( settings.breaker_key ) ) {
					if ( settings.breaker ) {
						if ( TickbaseControll::m_charged ) {
							m_direction = g_cl.m_cmd->m_view_angles.y + 180;

							if ( g_cl.m_cmd->m_side_move == 0 ) {
								static bool boxhackbreaker_side_flip = false;
								g_cl.m_cmd->m_side_move = boxhackbreaker_side_flip ? 3.25 : -3.25;
								boxhackbreaker_side_flip = !boxhackbreaker_side_flip;
							}
							if ( g_cl.m_cmd->m_forward_move == 0 ) {
								static bool boxhackbreaker_for_flip = false;
								g_cl.m_cmd->m_forward_move = boxhackbreaker_for_flip ? 3.25 : -3.25;
								boxhackbreaker_for_flip = !boxhackbreaker_for_flip;
							}

							boxhackbreaker_flip = !boxhackbreaker_flip;
							m_direction -= boxhackbreaker_flip ? ( settings.breaker_side * ( GetKeyState( settings.invert_aa_side ) ? -1 : 1 ) ) : 0;
							TickbaseControll::m_tick_to_shift_alternate = boxhackbreaker_flip ? 16 : 0;
							g_cl.m_cmd->m_view_angles.y = m_direction;
						}
					}
				}
			} break;
			case 1: {
				if ( !g_cl.m_lag && g_csgo.m_globals->m_curtime >= g_cl.m_body_pred && !GetKeyState( settings.fakewalkke ) ) {
					if ( !TickbaseControll::m_double_tap && GetKeyState( settings.LBYExploit ) ) {
						g_cl.m_cmd->m_view_angles.y -= 90.f;
						g_cl.m_cmd->m_tick = INT_MAX;
						*g_cl.m_packet = false;
					}
				}
			} break;
			case 2: { // this reverse is a bit broken dont fucking use it
				if ( !TickbaseControll::m_shifting ) {
					*g_cl.m_packet = false;
					TickbaseControll::m_shifting = true;
				}
				else {
					g_cl.m_cmd->m_view_angles.x = -89.0f;
					g_cl.m_cmd->m_view_angles.y -= 180.0f;
					g_cl.m_cmd->m_view_angles.normalize( );

					//if ( g_cl.m_lag != 0 ) {
					//	g_cl.m_cmd->m_tick = 0x54F12F43; // suppress command // overflow
					//}

					if ( g_cl.m_lag < 6 ) {
						*g_cl.m_packet = false;
						return;
					}
					else {
						*g_cl.m_packet = true;
						TickbaseControll::m_shifting = false;
					}
				}
			} break;
		}
		

		// if we have a yaw active, which is true if we arrived here.
		// set the yaw to the direction before applying any other operations.
		g_cl.m_cmd->m_view_angles.y = m_direction;

		bool stand = settings.body_fake_stand > 0 && m_mode == AntiAimMode::STAND;

		// one tick before the update.
		if( stand && !g_cl.m_lag && g_csgo.m_globals->m_curtime >= ( g_cl.m_body_pred - g_cl.m_anim_frame ) && g_csgo.m_globals->m_curtime < g_cl.m_body_pred ) {
			// z mode.
			if( settings.body_fake_stand == 4 )
				g_cl.m_cmd->m_view_angles.y -= 90.f;
		}

		// check if we will have a lby fake this tick.
		if ( !g_cl.m_lag && g_csgo.m_globals->m_curtime >= g_cl.m_body_pred && ( stand ) ) {
			// there will be an lbyt update on this tick.
			if( stand ) {
				switch( settings.body_fake_stand ) {

					// left.
				case 1:
					g_cl.m_cmd->m_view_angles.y += 110.f;
					break;

					// right.
				case 2:
					g_cl.m_cmd->m_view_angles.y -= 110.f;
					break;

					// opposite.
				case 3:
					g_cl.m_cmd->m_view_angles.y += 180.f;
					break;

					// z.
				case 4:
					g_cl.m_cmd->m_view_angles.y += 90.f;
					break;
				}
			}
		}

		// run normal aa code.
		else {
			switch( m_yaw ) {

				// direction.
			case 1:
				// do nothing, yaw already is direction.
				break;

				// jitter.
			case 2: {

				// get the range from the menu.
				float range = m_jitter_range / 2.f;

				// set angle.
				g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -range, range );
				break;
			}

				  // rotate.
			case 3: {
				// set base angle.
				g_cl.m_cmd->m_view_angles.y = ( m_direction - m_rot_range / 2.f );

				// apply spin.
				g_cl.m_cmd->m_view_angles.y += std::fmod( g_csgo.m_globals->m_curtime * ( m_rot_speed * 20.f ), m_rot_range );

				break;
			}

				  // random.
			case 4:
				// check update time.
				if( g_csgo.m_globals->m_curtime >= m_next_random_update ) {

					// set new random angle.
					m_random_angle = g_csgo.RandomFloat( -180.f, 180.f );

					// set next update time
					m_next_random_update = g_csgo.m_globals->m_curtime + m_rand_update;
				}

				// apply angle.
				g_cl.m_cmd->m_view_angles.y = m_random_angle;
				break;

			default:
				break;
			}
		}
	}

	// normalize angle.
	math::NormalizeAngle( g_cl.m_cmd->m_view_angles.y );
}

void HVH::DoFakeAntiAim( ) {
	// do fake yaw operations.

	// enforce this otherwise low fps dies.
	// cuz the engine chokes or w/e
	// the fake became the real, think this fixed it.
	*g_cl.m_packet = true;

	switch( settings.fake_yaw ) {

		// default.
	case 1:
		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply 45 degree jitter.
		g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -90.f, 90.f );
		break;

		// relative.
	case 2:
		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply offset correction.
		g_cl.m_cmd->m_view_angles.y += settings.fake_relative;
		break;

		// relative jitter.
	case 3: {
		// get fake jitter range from menu.
		float range = settings.fake_jitter_range / 2.f;

		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply jitter.
		g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -range, range );
		break;
	}

		  // rotate.
	case 4:
		g_cl.m_cmd->m_view_angles.y = m_direction + 90.f + std::fmod( g_csgo.m_globals->m_curtime * 360.f, 180.f );
		break;

		// random.
	case 5:
		g_cl.m_cmd->m_view_angles.y = g_csgo.RandomFloat( -180.f, 180.f );
		break;

		// local view.
	case 6:
		g_cl.m_cmd->m_view_angles.y = g_cl.m_view_angles.y;
		break;

	default:
		break;
	}

	// normalize fake angle.
	math::NormalizeAngle( g_cl.m_cmd->m_view_angles.y );
}

bool HVH::fake_flick( ) {
	static bool wtfwtfwtf = false;
	static bool is_flicking = false;
	static bool OffShit = false;

	if ( GetKeyState( settings.fakeflickanol ) ) {
		if ( ( g_cl.m_cmd->m_forward_move == 0 && g_cl.m_cmd->m_side_move == 0 ) ) {
			if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 35 )
				return false;
			*g_cl.m_packet = !( g_cl.m_cmd->m_tick % 2 == 0 );
			CUserCmd* cmd = g_cl.m_cmd;
			static int tick_counter = 0;
			static bool flick_flip = false;
			static bool after_flick = false;
			if ( cmd->m_tick % 2 == 0 )
			{
				DoRealAntiAim( );
				tick_counter++;
				if ( tick_counter == 7 || tick_counter == 1 )
				{
					flick_flip = !flick_flip;
					g_cl.m_cmd->m_view_angles.y += ( flick_flip ? -120 : 110 );
					g_cl.m_cmd->m_forward_move += ( flick_flip ? 21 : -21 );
					tick_counter = tick_counter == 7 ? 0 : tick_counter;
					after_flick = true;
					is_flicking = true;
					return false;
				}
				if ( after_flick )
				{
					after_flick = false;
					is_flicking = true;
					g_cl.m_cmd->m_view_angles.y -= 120;
				}
				return false;
			}
			else
			{
				DoFakeAntiAim( );
			}
		}
		if ( m_mode == AntiAimMode::STAND )
			return false;
	}

	return true;
}

void HVH::AntiAim( ) {
	bool attack, attack2;

	if( !settings.enable_aa )
		return;

	attack = g_cl.m_cmd->m_buttons & IN_ATTACK;
	attack2 = g_cl.m_cmd->m_buttons & IN_ATTACK2;

	if( g_cl.m_weapon && g_cl.m_weapon_fire ) {
		bool knife = g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS;
		bool revolver = g_cl.m_weapon_id == REVOLVER;

		// if we are in attack and can fire, do not anti-aim.
		if( attack || ( attack2 && ( knife || revolver ) ) )
			return;
	}

	// disable conditions.
	if( g_csgo.m_gamerules->m_bFreezePeriod( ) || ( g_cl.m_flags & FL_FROZEN ) || g_cl.m_round_end || ( g_cl.m_cmd->m_buttons & IN_USE ) )
		return;

	// grenade throwing
	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if( g_cl.m_weapon_type == WEAPONTYPE_GRENADE
		&& ( !g_cl.m_weapon->m_bPinPulled( ) || attack || attack2 )
		&& g_cl.m_weapon->m_fThrowTime( ) > 0.f && g_cl.m_weapon->m_fThrowTime( ) < g_csgo.m_globals->m_curtime )
		return;


	// we dont want to do custom aa states
	m_pitch = settings.pitch_stand;
	m_yaw = settings.yaw_stand;
	m_jitter_range = settings.jitter_range_stand;
	m_rot_range = settings.rot_range_stand;
	m_rot_speed = settings.rot_speed_stand;
	m_rand_update = settings.rand_update_stand;
	m_dir = settings.dir_stand;
	m_dir_custom = settings.dir_custom_stand;
	m_base_angle = settings.base_angle_stand;
	m_auto_time = settings.dir_time_stand;

	// set pitch.
	AntiAimPitch( );

	// if we have any yaw.
	if( m_yaw > 0 ) {
		// set direction.
		GetAntiAimDirection( );
	}

	// we have no real, but we do have a fake.
	else if( settings.fake_yaw > 0 )
		m_direction = g_cl.m_cmd->m_view_angles.y;

	auto speed = g_cl.m_local->m_vecVelocity( ).length_2d( ) > 10.f;
	if ( !speed ) {
		if ( !fake_flick( ) )
			return;
	}

	if( settings.fake_yaw ) {
		// do not allow 2 consecutive sendpacket true if faking angles.
		if( *g_cl.m_packet && g_cl.m_old_packet )
			*g_cl.m_packet = false;

		// run the real on sendpacket false.
		if ( !*g_cl.m_packet || !*g_cl.m_final_packet ) {
			DoRealAntiAim( );
			DoDistortion( );
		}			
		else DoFakeAntiAim( );
	}

	// no fake, just run real.
	else DoRealAntiAim( );
}

void HVH::SendPacket( ) {
	// if not the last packet this shit wont get sent anyway.
	// fix rest of hack by forcing to false.
	if( !*g_cl.m_final_packet )
		*g_cl.m_packet = false;

	// fake-lag enabled.
	if( settings.lag_enable && !g_csgo.m_gamerules->m_bFreezePeriod( ) && !( g_cl.m_flags & FL_FROZEN ) ) {
		// limit of lag.
		int limit = std::min( ( int )settings.lag_limit, g_cl.m_max_lag );

		// fakelag 0 while dt'ing
		if ( TickbaseControll::m_double_tap ) {
			limit = 0;
		}

		// indicates wether to lag or not.
		bool active{ };
		bool should_choked{}; // aimware reverse

		// get current origin.
		vec3_t cur = g_cl.m_local->m_vecOrigin( );

		// get prevoius origin.
		vec3_t prev = g_cl.m_net_pos.empty( ) ? g_cl.m_local->m_vecOrigin( ) : g_cl.m_net_pos.front( ).m_pos;

		// delta between the current origin and the last sent origin.
		float delta = ( cur - prev ).length_sqr( );

		// move.
		if ( delta > 0.1f && g_cl.m_speed > 0.1f ) {
			active = true; // return;??
		}

		// air.
		else if ( ( ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_flags & FL_ONGROUND ) ) ) {
			active = true;// return;??
		}

		// crouch.
		else if ( g_cl.m_local->m_bDucking( ) ) {
			active = true;// return;??
		}
		if( active ) {
			int mode = settings.lag_mode;

			// max.
			if( mode == 0 )
				*g_cl.m_packet = false;

			// break.
			else if( mode == 1 && delta <= 4096.f )
				*g_cl.m_packet = false;

			// random.
			else if( mode == 2 ) {
				// compute new factor.
				if( g_cl.m_lag >= m_random_lag )
					m_random_lag = g_csgo.RandomInt( 2, limit );

				// factor not met, keep choking.
				else *g_cl.m_packet = false;
			}

			// break step.
			else if( mode == 3 ) {
				// normal break.
				if( m_step_switch ) {
					if( delta <= 4096.f )
						*g_cl.m_packet = false;
				}

				// max.
				else *g_cl.m_packet = false;
			}

			else if ( mode == 4 ) {
				limit = static_cast< int >( rand() % 13 + 0.5 - 1 ); // this might be wrong reversed im not sure bro, im just porting it
				*g_cl.m_packet = g_csgo.m_cl->m_choked_commands < limit;
			}

			else if ( mode == 5 ) {
				if ( g_cl.m_lag % 30 >= 15 )
					*g_cl.m_packet = true;
			}

			if( g_cl.m_lag >= limit )
				*g_cl.m_packet = true;
		}

		if ( settings.adaptive_fl_onetap ) {
			bool should_choke = false;
			const float units_per_tick = std::ceilf( 68.0f / game::TICKS_TO_TIME( g_cl.m_local->m_vecVelocity( ).length( ) ) );

			if ( units_per_tick > 0 && units_per_tick < 14 ) {
				auto velocity = g_cl.m_local->m_vecVelocity( );
				if ( velocity.length_2d( ) > 0.15f ) {
					if ( velocity.length_2d_sqr( ) > 4096.f ) {
						limit = 14; // set limits
					}
					else {
						limit = units_per_tick ? 14 - 1 : 15;
					}
				}
				else {
					limit = settings.lag_limit;
				}

				if ( limit > 0 && limit <= 14 )
					should_choke = false;
				else
					should_choke = true;
			}

			if ( should_choke ) {
				*g_cl.m_packet = true;
			}
			else if ( !should_choke ) {
				*g_cl.m_packet = false;
			}

			//*g_cl.m_packet = should_choke; // u sure this work
		}
	}

	if( !settings.lag_land ) {
		vec3_t                start = g_cl.m_local->m_vecOrigin( ), end = start, vel = g_cl.m_local->m_vecVelocity( );
		CTraceFilterWorldOnly filter;
		CGameTrace            trace;

		// gravity.
		vel.z -= ( g_csgo.sv_gravity->GetFloat( ) * g_csgo.m_globals->m_interval );

		// extrapolate.
		end += ( vel * g_csgo.m_globals->m_interval );

		// move down.
		end.z -= 2.f;

		g_csgo.m_engine_trace->TraceRay( Ray( start, end ), MASK_SOLID, &filter, &trace );

		// check if landed.
		if( trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f && !( g_cl.m_flags & FL_ONGROUND ) )
			*g_cl.m_packet = true;
	}


	// do not lag while shooting.
	if( g_cl.m_old_shot )
		*g_cl.m_packet = true;

	// we somehow reached the maximum amount of lag.
	// we cannot lag anymore and we also cannot shoot anymore since we cant silent aim.
	if( g_cl.m_lag >= g_cl.m_max_lag ) {
		// set bSendPacket to true.
		*g_cl.m_packet = true;

		// disable firing, since we cannot choke the last packet.
		g_cl.m_weapon_fire = false;
	}
}