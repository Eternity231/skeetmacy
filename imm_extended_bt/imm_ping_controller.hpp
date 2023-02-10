#pragma once
#include "../includes.h"
#include "../imm_animfix/immortal_interfaces.hpp"
#include "../imm_lagcomp/LagControll.hpp"
namespace PingControll {

	class __declspec( novtable ) IExtendenBacktrack : public LagController::NonCopyable {
	public:
		static IExtendenBacktrack* Get( );
		virtual void SetSuitableInSequence( INetChannel* channel ) = NULL;
		virtual float CalculatePing( INetChannel* channel ) = NULL;
		virtual void FlipState( INetChannel* channel ) = NULL;

	protected:
		IExtendenBacktrack( ) {

		}
		virtual ~IExtendenBacktrack( ) {

		}
	};
}