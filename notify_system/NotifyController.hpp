#pragma once

class notify_text {
public:
	std::string m_text;
	Color m_color;
	float m_time;
public:
	__forceinline notify_text( const std::string& text, Color color, float time ) : m_text{ text }, m_color{ color }, m_time{ time } { }
};


namespace notify_controller {
	class notify {
	private:
		std::vector< std::shared_ptr< notify_text > > m_notify_text;
	public:
		__forceinline notify( ) : m_notify_text{ } { }
		__forceinline void push_event( const std::string& text, std::string prefix = "", Color color = colors::white, float time = 8.f ) {
			m_notify_text.push_back( std::make_shared< notify_text >( text, color, time ) );

			// we always want to print to console
			if ( !prefix.empty( ) ) {
				g_cl.print( prefix + text );
			}
			else if ( prefix.empty( ) ) {
				g_cl.print( text );
			}
		}

		void think( ) {
			int x{ 8 }, y{ 5 }, size{ render::menu_shade.m_size.m_height + 1 };
			Color	color;
			float	left, delta;

			for ( size_t i{ }; i < m_notify_text.size( ); ++i ) {
				auto notify = m_notify_text[ i ];

				notify->m_time -= g_csgo.m_globals->m_frametime;

				if ( notify->m_time <= 0.f ) {
					m_notify_text.erase( m_notify_text.begin( ) + i );
					continue;
				}
			}

			// we have nothing to draw.
			if ( m_notify_text.empty( ) )
				return;

			for ( size_t i{ }; i < m_notify_text.size( ); ++i ) {
				auto notify = m_notify_text[ i ];

				// thanks alpha
				if ( notify->m_text.find( XorStr( "%%%%" ) ) != std::string::npos )
					notify->m_text.erase( notify->m_text.find( XorStr( "%" ) ), 3 );

				left = notify->m_time;
				color = notify->m_color;
				delta = fabs( g_csgo.m_globals->m_frametime - left );

				auto text_size = render::menu_shade.size( notify->m_text );
				auto prefix_size = render::menu_shade.size( "[sup] " );

				if ( left < .5f ) {
					float f = left;
					f = math::Clamp( f, 0.f, .5f );

					f /= .5f;

					color.a( ) = ( int )( f * 255.f );

					if ( i == 0 && f < 0.2f ) {
						y -= size * ( 1.f - f / 0.2f );
					}
				}
				else {
					color.a( ) = 255;
					if ( delta > 7.65f )
						x = 8 + ( ( ( left - ( 7.65f ) ) / 0.25f ) * static_cast< float >( -( text_size.m_width + prefix_size.m_width ) ) );
				}

				render::menu_shade.string( x, y, Color( 255, 122, 122 ), "[sup] " );
				render::menu_shade.string( x + prefix_size.m_width, y, color, notify->m_text );
				y += size;
			}

		}
	};
	extern notify_controller::notify g_notify;
}