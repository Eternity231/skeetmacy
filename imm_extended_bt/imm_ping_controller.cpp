#include "imm_ping_controller.hpp"

namespace PingControll {
	class ExtendedBacktrack : public IExtendenBacktrack {
	public:
		void SetSuitableInSequence( INetChannel* channel ) override;
		float CalculatePing( INetChannel* channel ) override;
		void FlipState( INetChannel* channel ) override;
	private:
		bool m_bIsFlipedState{ };
	};

	IExtendenBacktrack* IExtendenBacktrack::Get( ) {
		static ExtendedBacktrack instance;
		return &instance;
	}

	void ExtendedBacktrack::SetSuitableInSequence( INetChannel* channel ) {
		if ( m_bIsFlipedState ) {
			m_bIsFlipedState = false;
			return;
		}

		const auto spike = game::TIME_TO_TICKS( CalculatePing( channel ) );
		if ( channel->m_in_seq > spike )
			channel->m_in_seq -= spike;
	}

	float ExtendedBacktrack::CalculatePing( INetChannel* channel ) {
		auto wanted_ping = 0.f;

		bool checkbox = false; // add this as a checkbox

		if ( checkbox )
			wanted_ping = 200.f / 1000.f;
		else
			return 0.f;

		return std::max( 0.f, wanted_ping - channel->GetLatency( 0 ) );
	}

	void ExtendedBacktrack::FlipState( INetChannel* channel ) {
		static auto last_reliable_state = -1;

		if ( channel->m_in_rel_state != last_reliable_state )
			m_bIsFlipedState = true;

		last_reliable_state = channel->m_in_rel_state;
	}

}