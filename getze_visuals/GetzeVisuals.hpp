#pragma once
#include "../includes.h"

namespace Getze {
	namespace VisualsControll {
		void fakelag_indicator( );
		void bomb_indicator( Player* entity );

		void indicator( );
		void indicator_ui( );
		void indicator_spect( );

		void clear_dmg( );
		void add_dmg( IGameEvent* event );
		void draw_dmg( );
		void draw_beam( vec3_t Start, vec3_t End, Color color, float Width );
		void add_trail( );

		void manual_arrows( );
		void sunset_control( );
	}

	namespace Drawing {
		inline void DrawRect( int x, int y, int w, int h, Color color) {
			render::rect_filled( x, y, w, h, color );
		}

		inline void DrawOutlinedRect( int x, int y, int w, int h, Color color ) {
			render::rect( x, y, w, h, color );
		}

		inline void DrawString( render::Font font, int x, int y, Color color, render::StringFlags_t flags, std::string text ) {
			font.string( x, y, color, text, flags );
		}

		inline void DrawLine( int x, int y, int w, int h, Color color ) {
			render::line( x, y, w, h, color );
		}

		inline render::FontSize_t GetTextSize( render::Font font, std::string text ) {
			return font.size( text );
		}
	}

	namespace SurfaceEngine {
		void draw_progresstext( vec2_t& position, const char* name, const float progress );
		void draw_progressbar( vec2_t& position, const char* name, const Color color, const float progress );
		void draw_progressbar( vec2_t& position, const char* name, const Color color, const float progress, render::Font font );
	}
}