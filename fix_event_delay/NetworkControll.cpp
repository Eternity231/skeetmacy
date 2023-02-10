#include "NetworkControll.hpp"
#include "../backend/config/config_new.h"

namespace NetworkControll {
	void KeepComunication( ) {
		if ( !g_cl.m_processing )
			return;

		const auto net_channel = g_csgo.m_cl->m_net_channel;
		if ( !net_channel )
			return;

		const auto current_choke = net_channel->m_choked_packets;

		net_channel->m_choked_packets = 0;
		net_channel->SendDatagram( );

		if ( settings.networking_debug )
			notify_controller::g_notify.push_event( "fixed event delay\n", "[networking]" );

		--net_channel->m_out_seq;
		net_channel->m_choked_packets = current_choke;
	}
}