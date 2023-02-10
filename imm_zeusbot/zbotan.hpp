#pragma once
#include "../includes.h"
#include "../imm_lagcomp/LagControll.hpp"
#include "../imm_animfix/immortal_interfaces.hpp"

namespace Zeusan {
	class __declspec( novtable ) ZeusBot : public LagController::NonCopyable {
	public:
		static ZeusBot* Get( );
		virtual void Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) = NULL;
	protected:
		ZeusBot( ) {

		}
		virtual ~ZeusBot( ) {

		}
	};
}