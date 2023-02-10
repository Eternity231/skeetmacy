#pragma once
#include "../includes.h"
#include "../imm_animfix/immortal_interfaces.hpp"

namespace KnifeBot {
	class __declspec( novtable ) KnifeBot : public LagController::NonCopyable {
	public:
		static KnifeBot* Get( );
		virtual void Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) = NULL;
	protected:
		KnifeBot( ) {

		}
		virtual ~KnifeBot( ) {

		}
	};
}