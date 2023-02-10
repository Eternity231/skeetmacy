#include "TickbaseController.hpp"
#include "../backend/config/config_new.h"

namespace TickbaseControll {
	// we could use : using namespace TickbaseControll, but i like more that code style, you can rework and do it how u want
	// thats my code style so i will let it like that

	// shoutout: machport
	void manipulate_tickbase( ) {
		/* we ve got to setup here how dt gets on
			because we dont use supremacy anymore we will have to do it in another way, if this one doesnt work
		*/
		m_double_tap = GetKeyState( settings.doubletap_key );
		
		auto inc_stop = settings.incriminate_stop;

		if ( !m_double_tap && m_charged ) {
			m_charge_timer = 0;
			m_tick_to_shift = 16;
		}
		if ( !m_double_tap ) return;
		if ( !m_charged ) {
			if ( m_charge_timer > game::TIME_TO_TICKS( .5 ) ) { // .5 seconds after shifting, lets recharge
				m_tick_to_recharge = 16;
			}
			else {
				if ( !g_aimbot.m_target ) {
					m_charge_timer++;
				}
				if ( g_cl.m_cmd->m_buttons & IN_ATTACK && g_cl.m_weapon_fire ) {
					m_charge_timer = 0;

					if ( inc_stop ) {
						switch ( settings.autostop_type ) {
						case 0: {
							g_movement.QuickStop( );
						} break;
						case 1: {
							Getze::FrameController::QuickStopL3D( g_cl.m_cmd );
						} break;
						}
					}
				}
			}
		}
		if ( g_input.GetKeyState( g_menu.main.movement.fakewalk.get( ) ) ) {
			m_charge_timer = 0;
			m_charged = false;
		}
		if ( g_cl.m_cmd->m_buttons & IN_ATTACK && g_cl.m_weapon_fire && m_charged ) {
			// shot.. lets shift tickbase back so we can dt.
			m_charge_timer = 0;
			m_tick_to_shift = 14;
			m_shift_cmd = g_cl.m_cmd->m_command_number;
			m_shift_tickbase = g_cl.m_local->m_nTickBase( );

			// bau
			if ( inc_stop ) {
				switch ( settings.autostop_type ) {
				case 0: {
					g_movement.QuickStop( );
				} break;
				case 1: {
					Getze::FrameController::QuickStopL3D( g_cl.m_cmd );
				} break;
				}
			}

			*g_cl.m_packet = false;
		}
		if ( !m_charged ) {
			m_charged_ticks = 0;
		}
	}

	void ApplyDTSub30( int cmd_num, int commands_to_shift, int original_choke ) { // this is not full
		//auto shift_data = &m_tickbase_array[ cmd_num % 150 ];

		auto serverTickcount = g_csgo.m_globals->m_tick_count;
		const int nCorrectionTicks = game::TIME_TO_TICKS( 0.029999999f );

		int	nIdealFinalTick = serverTickcount + nCorrectionTicks;
		auto simulation_ticks = commands_to_shift + 1 + original_choke;
		int nCorrectedTick = nIdealFinalTick - simulation_ticks + 1;

		//shift_data->command_num = cmd_num;
		//shift_data->extra_shift = 0;
		//shift_data->increace = false;
		//shift_data->doubletap = true;
		//shift_data->tickbase_original = nCorrectedTick;

		g_csgo.m_prediction->m_previous_startframe = -1;
		g_csgo.m_prediction->m_commands_predicted = 0;
	}

	void CopyCommandSkeet( CUserCmd* from_cmd ) {
		auto viewangles = g_cl.m_cmd->m_view_angles;
		auto net_channel = static_cast< INetChannel* >( g_csgo.m_cl->m_net_channel );

		g_csgo.m_engine->GetViewAngles( viewangles );

		const auto o_chocked = g_csgo.m_cl->m_choked_commands;

		auto commands_to_shift = ticks_allowed - o_chocked - 1;

		if ( commands_to_shift > 13 )
			commands_to_shift = 13;

		if ( commands_to_shift > 0 && net_channel ) {
			const auto time = g_csgo.m_globals->m_curtime;

			vec3_t old_movement = {};
			from_cmd->m_side_move = old_movement.y;
			from_cmd->m_forward_move = old_movement.x;

			if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) <= 2 ) {
				from_cmd->m_side_move = 0.f;
				from_cmd->m_forward_move = 0.f;
			}
			else {
				if ( abs( from_cmd->m_side_move ) > 10.f )
					from_cmd->m_side_move = copysignf( 450.f, from_cmd->m_side_move );
				else
					from_cmd->m_side_move = 0.f;

				if ( abs( from_cmd->m_forward_move ) > 10.f )
					from_cmd->m_forward_move = copysignf( 450.f, from_cmd->m_forward_move );
				else
					from_cmd->m_forward_move = 0.f;
			}

			auto m_nTickBase = &g_cl.m_local->m_nTickBase( );

			*m_nTickBase -= commands_to_shift;

			g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME( *m_nTickBase );
			++* m_nTickBase;

			std::vector<CUserCmd> cmds = {};
			for ( auto i = 0; i < commands_to_shift; i++ ) {
				auto command = &cmds.emplace_back( );

				memcpy( command, from_cmd, 0x64 );

				command->m_view_angles = viewangles;
				command->m_command_number += i;
					
				// run prediction;
				g_inputpred.update( );
				g_movement.FixMove( from_cmd, g_cl.m_real_angle );

				command->m_view_angles.clamp( );
			}

			g_csgo.m_globals->m_curtime = time;

			for ( auto i = 0; i < commands_to_shift; i++ )
			{
				auto command = g_csgo.m_input->GetUserCmd( cmds[ i ].m_command_number );
				auto v8 = g_csgo.m_input->GetVerifiedUserCmd( cmds[ i ].m_command_number ); // please add this if u need, i dont like to fuck my brain out

				memcpy( command, &cmds[ i ], sizeof( CUserCmd ) );

				bool v6 = command->m_tick == 0x7F7FFFFF;
				command->m_predicted = v6;

				v8->m_cmd = *command;
				//v8->m_crc = command->GetChecksum( );

				++net_channel->m_choked_packets;
				++net_channel->m_out_seq;
				++g_csgo.m_cl->m_choked_commands;
			}

			if ( commands_to_shift <= 15 )
				ApplyDTSub30( from_cmd->m_command_number, commands_to_shift, o_chocked );

			cmds.clear( );
		}
	}

	bool IsTickcountValid( int tick ) {
		return ( g_cl.m_cmd->m_tick + 64 + 8 ) > tick;
	}

	void ApplyShift( CUserCmd* cmd, bool* bSendPacket ) {
		if ( *bSendPacket ) {
			INetChannel* net_channel = ( INetChannel* )( g_csgo.m_cl->m_net_channel );
			auto v7 = net_channel->m_choked_packets;
			if ( v7 >= 0 ) {
				auto v8 = ticks_allowed;
				auto v9 = cmd->m_command_number - v7;
				do
				{
					auto v10 = &g_csgo.m_input->m_commands[ cmd->m_command_number - 150 * ( v9 / 150 ) - v7 ];
					if ( !v10
						|| IsTickcountValid( v10->m_tick ) )
					{
						if ( --v8 <= 0 )
							v8 = 0;
						ticks_allowed = v8;
					}
					++v9;
					--v7;
				} while ( v7 >= 0 );
			}
		}

		auto v14 = ticks_allowed;
		if ( v14 > 16 ) {
			auto v15 = v14 - 1;
			//ticks_allowed = math::clamp( v15, 0, 16 ); // please fix this
		}
	}
}

// we've got another namespace for clmove
namespace cl_move {
	void CL_Move( float accumulated_extra_samples, bool bFinalTick ) {
		// now there we use the namespace, because we dont have the tickbasecontroller on class anymore
		// so we need to use namespace now

		if ( TickbaseControll::m_tick_to_recharge > 0 ) {
			TickbaseControll::m_tick_to_recharge--;
			TickbaseControll::m_charged_ticks++;

			if ( TickbaseControll::m_tick_to_recharge == 0 ) {
				TickbaseControll::m_charged = true;
			}

			// now return
			return;
		}

		// we call constructor ( thats how i name it )
		o_CLMove( accumulated_extra_samples, bFinalTick );
		
		// not shifted
		TickbaseControll::m_shifted = false;
		if ( TickbaseControll::m_tick_to_shift > 0 ) {
			TickbaseControll::m_shifting = true;

			for ( ; TickbaseControll::m_tick_to_shift > 0; TickbaseControll::m_tick_to_shift-- ) {
				// we call constructor 
				o_CLMove( accumulated_extra_samples, bFinalTick );
				TickbaseControll::m_charged_ticks--;
			}

			// return false
			TickbaseControll::m_charged = false;
			TickbaseControll::m_shifting = false;
			TickbaseControll::m_shifted = false;
		}
	}
}

// we dont have a namspace for that because we call it directly from create move, we dont it
// in that way to clean mess from hooks.cpp so it could be more cleaner
void write_user_cmd( bf_write* buf, CUserCmd* in, CUserCmd* out ) {
	static auto write_user_cmd_syntax = pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) );

	__asm
	{
		mov ecx, buf
		mov edx, in
		push out
		call write_user_cmd_syntax
		add esp, 4
	}
}
bool Hooks::WriteUsercmdDeltaToBuffer( int slot, bf_write* buf, int from, int to, bool isnewcommand ) {
	if ( g_cl.m_processing && g_csgo.m_engine->IsConnected( ) && g_csgo.m_engine->IsInGame( ) ) {
		uintptr_t stackbase;
		__asm mov stackbase, ebp;
		CCLCMsg_Move_t* msg = reinterpret_cast< CCLCMsg_Move_t* >( stackbase + 0xFCC );

		if ( TickbaseControll::m_tick_to_shift_alternate > 0 ) {
			if ( from != -1 )
				return true;

			int32_t new_commands = msg->new_commands;
			int Next_Command = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands + 1;
			int CommandsToAdd = std::min( TickbaseControll::m_tick_to_shift_alternate, 16 );

			TickbaseControll::m_tick_to_shift_alternate = 0;
			msg->new_commands = CommandsToAdd;
			msg->backup_commands = 0;
			from = -1;

			for ( to = Next_Command - new_commands + 1; to <= Next_Command; to++ ) {
				if ( !g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >( 23 )( this, slot, buf, from, to, isnewcommand ) )
					return false;
				from = to;
			}

			// update prexdiction flag
			if ( settings.update_prediction_dt ) { // credits: reis, thanks so much nemesis p cheat
				g_csgo.m_prediction->Update( g_csgo.m_cl->m_delta_tick, g_csgo.m_cl->m_delta_tick > 0, g_csgo.m_cl->m_last_command_ack, g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands );
			}

			CUserCmd* last_command = g_csgo.m_input->GetUserCmd( slot, from );
			CUserCmd nullcmd;
			CUserCmd ShiftCommand;

			if ( last_command )
				nullcmd = *last_command;

			ShiftCommand = nullcmd;

			// interpolate commandz
			if ( settings.interpolate_command ) {
				ShiftCommand.m_command_number = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands + 1;
			}
			else {
				ShiftCommand.m_command_number++;
			}

			ShiftCommand.m_tick += settings.increase_tickcount ? 200 : 100;

			for ( int i = new_commands; i <= CommandsToAdd; i++ ) {
				write_user_cmd( buf, &ShiftCommand, &nullcmd );
				nullcmd = ShiftCommand;
				ShiftCommand.m_command_number++; // im not sure if i need to modify this too
				ShiftCommand.m_tick++;
			}
		}
	}

	return g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >( 23 )( this, slot, buf, from, to, isnewcommand );
}