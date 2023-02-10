#pragma once

#if 1
template<typename T>
class Encrypted_t {
public:
	T* pointer;

	__forceinline Encrypted_t( T* ptr ) {
		pointer = ptr;
	}

	__forceinline  T* Xor( ) const {
		return  pointer;
	}

	__forceinline  T* operator-> ( ) {
		return Xor( );
	}

	__forceinline bool IsValid( ) const {
		return pointer != nullptr;
	}
};
#else
#pragma  optimize( "", off ) 
template<typename T>
class Encrypted_t {
	__forceinline uintptr_t rotate_dec( uintptr_t c ) const {
		return c;
		//return ( ( c & 0xFFFF ) << 16 | ( c & 0xFFFF0000 ) >> 16 );
#if 0
		return ( c & 0xF ) << 28 | ( c & 0xF0000000 ) >> 28
			| ( c & 0xF0 ) << 20 | ( c & 0x0F000000 ) >> 20
			| ( c & 0xF00 ) << 12 | ( c & 0x00F00000 ) >> 12
			| ( c & 0xF000 ) << 4 | ( c & 0x000F0000 ) >> 4;
#endif
	}
public:
	uintptr_t np;
	uintptr_t xor_val;

	__forceinline Encrypted_t( T* ptr ) {
		auto p = &ptr;
		xor_val = rotate_dec( pulCRCTable[ *( ( uint8_t* )p + 1 ) + ( ( ( uintptr_t( ptr ) >> 16 ) & 7 ) << 8 ) ] );
		np = rotate_dec( rotate_dec( xor_val ) ^ ( uintptr_t( ptr ) + ADD_VAL ) );
	}

	__forceinline  T* Xor( ) const {
		return ( T* )( ( uintptr_t )( rotate_dec( np ) ^ rotate_dec( xor_val ) ) - ADD_VAL );
	}

	__forceinline  T* operator-> ( ) {
		return Xor( );
	}

	__forceinline bool IsValid( ) const {
		return ( ( uintptr_t )( rotate_dec( np ) ^ rotate_dec( xor_val ) ) - ADD_VAL ) != 0;
	}
};
#pragma  optimize( "", on )

#endif

#pragma warning( disable : 4307 ) // '*': integral constant overflow
#pragma warning( disable : 4244 ) // possible loss of data
#pragma warning( disable : 4800 ) // forcing value to bool 'true' or 'false'
#pragma warning( disable : 4838 ) // conversion from '::size_t' to 'int' requires a narrowing conversion
#pragma warning( disable : 4996 ) // conversion from '::size_t' to 'int' requires a narrowing conversion

// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

using ulong_t = unsigned long;

// windows / stl includes.
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <xmmintrin.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <deque>
#include <functional>
#include <map>
#include <shlobj.h>
#include <filesystem>
#include <streambuf>

// our custom wrapper.
#include "unique_vector.h"
#include "tinyformat.h"

// other includes.
#include "hash.h"
#include "xorstr.h"
#include "pe.h"
#include "winapir.h"
#include "address.h"
#include "util.h"
#include "modules.h"
#include "pattern.h"
#include "vmt.h"
#include "stack.h"
#include "nt.h"
#include "x86.h"
#include "syscall.h"

// hack includes.
#include "interfaces.h"
#include "sdk.h"
#include "csgo.h"
#include "penetration.h"
#include "netvars.h"
#include "entoffsets.h"
#include "entity.h"
#include "client.h"
#include "gamerules.h"
#include "hooks.h"
#include "render.h"
#include "pred.h"
#include "lagrecord.h"
#include "visuals.h"
#include "movement.h"
#include "bonesetup.h"
#include "hvh.h"
#include "lagcomp.h"
#include "aimbot.h"
#include "netdata.h"
#include "chams.h"
#include "grenades.h"
#include "skins.h"
#include "events.h"
#include "shots.h"

// gui includes.
#include "json.h"
#include "base64.h"
#include "element.h"
#include "checkbox.h"
#include "dropdown.h"
#include "multidropdown.h"
#include "slider.h"
#include "colorpicker.h"
#include "edit.h"
#include "keybind.h"
#include "button.h"
#include "tab.h"
#include "form.h"
#include "gui.h"
#include "callbacks.h"
#include "menu.h"
#include "config.h"

// new stuff
#include "tickbase/TickbaseController.hpp"
#include "resolver/big_resolve.hpp"
#include "rage_aimbot/rage_aimbot.hpp"
#include "notify_system/NotifyController.hpp"
#include "fix_event_delay/NetworkControll.hpp"
#include "autowall/auto_wall.h"
#include "autowall_immortal/AutowallController.hpp"
#include "imm_setupbones/SetupBonesController.hpp"
#include "getze_visuals/GetzeVisuals.hpp"
#include "obelus_key_handles/KeyHandler.hpp"
#include "getze_framestage/GetzeFrameController.hpp"
#include "getze_aimbot_controller/GetzeAimbotController.hpp"
#include "getze_engine/GetzeEngineController.hpp"
#include "imm_lagcomp/LagControll.hpp"