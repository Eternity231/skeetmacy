#pragma once
#include "../includes.h"
#include "../imm_animfix/immortal_interfaces.hpp"

namespace FakelagSys {
	class __declspec( novtable ) FakeLag : public LagController::NonCopyable {
	public:
		static Encrypted_t<FakeLag> Get( );
		virtual void Main( bool* bSendPacket, Encrypted_t<CUserCmd> cmd ) = 0;
		virtual int GetFakelagChoke( ) const = 0;
		virtual int GetMaxFakelagChoke( ) const = 0;
		virtual bool IsPeeking( Encrypted_t<CUserCmd> cmd ) = 0;
	protected:
		FakeLag( ) { };
		virtual ~FakeLag( ) {
		}
	};
}