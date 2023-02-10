#pragma once
#include "../includes.h"

namespace Interfac {
	inline auto m_pEngine = g_csgo.m_engine;
	inline auto m_pGlobalVars = g_csgo.m_globals;
	inline auto m_pClientState = g_csgo.m_cl;
	inline auto m_pEntList = g_csgo.m_entlist;
	inline auto m_pMoveHelper = g_csgo.m_move_helper;
	inline auto m_pGameMovement = g_csgo.m_game_movement;
	inline auto m_pPrediction = g_csgo.m_prediction;
}