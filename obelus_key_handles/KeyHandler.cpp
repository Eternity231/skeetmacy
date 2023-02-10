#include "KeyHandler.hpp"

namespace KeyHandler {
	bool IsKeyDown( int key ) {
		return HIWORD( GetKeyState( key ) );
	}
	bool IsKeyUp( int key ) {
		return !HIWORD( GetKeyState( key ) );
	}
	bool CheckKey( int key, int keystyle ) {
		switch ( keystyle ) {
		case 0:
			return true;
		case 1:
			return IsKeyDown( key );
		case 2:
			return LOWORD( GetKeyState( key ) );
		case 3:
			return IsKeyUp( key );
		default:
			return true;
		}
	}
}