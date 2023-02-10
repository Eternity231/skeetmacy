#pragma once
#include "../includes.h"

namespace KeyHandler {
	bool IsKeyDown( int key );
	bool IsKeyUp( int key );
	bool CheckKey( int key, int keystyle );
}