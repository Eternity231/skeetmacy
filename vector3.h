#pragma once
#include "angle.h"
#define FastSqrt(x)	(sqrt)(x)

class quaternion_t {
public:
	float x, y, z, w;
};

constexpr auto RadPi = 3.14159265358979323846;
constexpr auto DegPi = 180.0;
template<typename T>
T ToDegrees( T radians ) {
	return ( radians * ( static_cast< T >( DegPi ) / static_cast< T >( RadPi ) ) );
}

class vec3_t {
public:
	// data member variables
	float x, y, z;

public:
	// ctors.
	__forceinline vec3_t( ) : x{}, y{}, z{} {}
	__forceinline vec3_t( float x, float y, float z ) : x{ x }, y{ y }, z{ z } {}

	// at-accesors.
	__forceinline float& at( const size_t index ) {
		return ( ( float* )this )[ index ];
	}
	__forceinline float& at( const size_t index ) const {
		return ( ( float* )this )[ index ];
	}	
	__forceinline void Set( float x = 0.0f, float y = 0.0f, float z = 0.0f ) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	// index operators.
	__forceinline float& operator( )( const size_t index ) {
		return at( index );
	}
	__forceinline const float& operator( )( const size_t index ) const {
		return at( index );
	}
	__forceinline float& operator[ ]( const size_t index ) {
		return at( index );
	}
	__forceinline const float& operator[ ]( const size_t index ) const {
		return at( index );
	}

	// equality operators.
	__forceinline bool operator==( const vec3_t& v ) const {
		return v.x == x && v.y == y && v.z == z;
	}
	__forceinline bool operator!=( const vec3_t& v ) const {
		return v.x != x || v.y != y || v.z != z;
	}

	// copy assignment.
	__forceinline vec3_t& operator=( const vec3_t& v ) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	// negation-operator.
	__forceinline vec3_t operator-( ) const {
		return vec3_t{ -x, -y, -z };
	}

	// arithmetic operators.
	__forceinline vec3_t operator+( const vec3_t& v ) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	__forceinline vec3_t operator-( const vec3_t& v ) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	__forceinline vec3_t operator*( const vec3_t& v ) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	__forceinline vec3_t operator/( const vec3_t& v ) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	// compound assignment operators.
	__forceinline vec3_t& operator+=( const vec3_t& v ) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline vec3_t& operator-=( const vec3_t& v ) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline vec3_t& operator*=( const vec3_t& v ) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline vec3_t& operator/=( const vec3_t& v ) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	// arithmetic operators w/ float.
	__forceinline vec3_t operator+( float f ) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	__forceinline vec3_t operator-( float f ) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	__forceinline vec3_t operator*( float f ) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	__forceinline vec3_t operator/( float f ) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	// compound assignment operators w/ float.
	__forceinline vec3_t& operator+=( float f ) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	__forceinline vec3_t& operator-=( float f ) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	__forceinline vec3_t& operator*=( float f ) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	__forceinline vec3_t& operator/=( float f ) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	// methods.
	__forceinline void clear( ) {
		x = y = z = 0.f;
	}

	__forceinline float length_sqr( ) const {
		return ( ( x * x ) + ( y * y ) + ( z * z ) );
	}

	__forceinline float length_2d_sqr( ) const {
		return ( ( x * x ) + ( y * y ) );
	}

	__forceinline float Dot( const vec3_t& v ) const {
		return ( this->x * v.x +
			this->y * v.y +
			this->z * v.z );
	}
	__forceinline float DistanceSquared( const vec3_t& v ) const {
		return ( ( *this - v ).length_sqr( ) );
	}

	__forceinline vec2_t ToVector2D( ) {
		return { this->x, this->y };
	}

	__forceinline float NormalizeInPlace( )
	{
		float radius = FastSqrt( x * x + y * y + z * z );

		// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
		float iradius = 1.f / ( radius + FLT_EPSILON );

		x *= iradius;
		y *= iradius;
		z *= iradius;

		return radius;
	}


	__forceinline ang_t ToEulerAngles( vec3_t* pseudoup = nullptr ) {
		auto pitch = 0.0f;
		auto yaw = 0.0f;
		auto roll = 0.0f;

		auto length = this->ToVector2D( ).length( );

		if ( pseudoup ) {
			auto left = pseudoup->cross( *this );

			left.normalize( );

			pitch = ToDegrees( std::atan2( -this->z, length ) );

			if ( pitch < 0.0f )
				pitch += 360.0f;

			if ( length > 0.001f ) {
				yaw = ToDegrees( std::atan2( this->y, this->x ) );

				if ( yaw < 0.0f )
					yaw += 360.0f;

				auto up_z = ( this->x * left.y ) - ( this->y * left.x );

				roll = ToDegrees( std::atan2( left.z, up_z ) );

				if ( roll < 0.0f )
					roll += 360.0f;
			}
			else {
				yaw = ToDegrees( std::atan2( -left.x, left.y ) );

				if ( yaw < 0.0f )
					yaw += 360.0f;
			}
		}
		else {
			if ( this->x == 0.0f && this->y == 0.0f ) {
				if ( this->z > 0.0f )
					pitch = 270.0f;
				else
					pitch = 90.0f;
			}
			else {
				pitch = ToDegrees( std::atan2( -this->z, length ) );

				if ( pitch < 0.0f )
					pitch += 360.0f;

				yaw = ToDegrees( std::atan2( this->y, this->x ) );

				if ( yaw < 0.0f )
					yaw += 360.0f;
			}
		}

		return { pitch, yaw, roll };
	}

	__forceinline void GetVectors( vec3_t& right, vec3_t& up ) { // VectorVectors
		vec3_t tmp;
		if ( x == 0.f && y == 0.f ) {
			// pitch 90 degrees up/down from identity
			right[ 0 ] = 0.f;
			right[ 1 ] = -1.f;
			right[ 2 ] = 0.f;
			up[ 0 ] = -z;
			up[ 1 ] = 0.f;
			up[ 2 ] = 0.f;
		}
		else {
			tmp[ 0 ] = 0.f;
			tmp[ 1 ] = 0.f;
			tmp[ 2 ] = 1.0f;
			right = cross( tmp );
			up = right.cross( *this );

			right.normalize( );
			up.normalize( );
		}
	}

	__forceinline float length( ) const {
		return std::sqrt( length_sqr( ) );
	}

	__forceinline float length_2d( ) const {
		return std::sqrt( length_2d_sqr( ) );
	}

	__forceinline float dot( const vec3_t& v ) const {
		return ( x * v.x + y * v.y + z * v.z );
	}

	__forceinline float dot( float* v ) const {
		return ( x * v[ 0 ] + y * v[ 1 ] + z * v[ 2 ] );
	}

	__forceinline vec3_t cross( const vec3_t &v ) const {
		return {
			( y * v.z ) - ( z * v.y ),
			( z * v.x ) - ( x * v.z ),
			( x * v.y ) - ( y * v.x )
		};
	}

	__forceinline float dist_to( const vec3_t &vOther ) const {
		vec3_t delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.length_2d( );
	}

	__forceinline float normalize( ) {
		float len = length( );

		( *this ) /= ( length( ) + std::numeric_limits< float >::epsilon( ) );

		return len;
	}

	__forceinline vec3_t normalized( ) const {
		auto vec = *this;

		vec.normalize( );

		return vec;
	}
};

__forceinline vec3_t operator*( float f, const vec3_t& v ) {
	return v * f;
}

class __declspec( align( 16 ) ) vec_aligned_t : public vec3_t {
public:
	__forceinline vec_aligned_t( ) {}

	__forceinline vec_aligned_t( const vec3_t& vec ) {
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = 0.f;
	}

	void Init( float ix = 0.0f, float iy = 0.0f, float iz = 0.0f ) {
		x = ix; y = iy; z = iz;
	}

	float w;
};