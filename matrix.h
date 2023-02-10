#pragma once
#include <DirectXMath.h>

template<typename T>
T ToRadians( T degrees ) {
	return ( degrees * ( static_cast< T >( RadPi ) / static_cast< T >( DegPi ) ) );
}

class matrix3x4_t {
public:
	__forceinline matrix3x4_t( ) {}

	__forceinline matrix3x4_t( float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23 ) {
		m_flMatVal[ 0 ][ 0 ] = m00;
		m_flMatVal[ 0 ][ 1 ] = m01;
		m_flMatVal[ 0 ][ 2 ] = m02;
		m_flMatVal[ 0 ][ 3 ] = m03;
		m_flMatVal[ 1 ][ 0 ] = m10;
		m_flMatVal[ 1 ][ 1 ] = m11;
		m_flMatVal[ 1 ][ 2 ] = m12;
		m_flMatVal[ 1 ][ 3 ] = m13;
		m_flMatVal[ 2 ][ 0 ] = m20;
		m_flMatVal[ 2 ][ 1 ] = m21;
		m_flMatVal[ 2 ][ 2 ] = m22;
		m_flMatVal[ 2 ][ 3 ] = m23;
	}

	__forceinline void Init( const vec3_t &x, const vec3_t &y, const vec3_t &z, const vec3_t &origin ) {
		m_flMatVal[ 0 ][ 0 ] = x.x;
		m_flMatVal[ 0 ][ 1 ] = y.x;
		m_flMatVal[ 0 ][ 2 ] = z.x;
		m_flMatVal[ 0 ][ 3 ] = origin.x;
		m_flMatVal[ 1 ][ 0 ] = x.y;
		m_flMatVal[ 1 ][ 1 ] = y.y;
		m_flMatVal[ 1 ][ 2 ] = z.y;
		m_flMatVal[ 1 ][ 3 ] = origin.y;
		m_flMatVal[ 2 ][ 0 ] = x.z;
		m_flMatVal[ 2 ][ 1 ] = y.z;
		m_flMatVal[ 2 ][ 2 ] = z.z;
		m_flMatVal[ 2 ][ 3 ] = origin.z;
	}

	__forceinline matrix3x4_t( const vec3_t &x, const vec3_t &y, const vec3_t &z, const vec3_t &origin ) {
		m_flMatVal[ 0 ][ 0 ] = x.x;
		m_flMatVal[ 0 ][ 1 ] = y.x;
		m_flMatVal[ 0 ][ 2 ] = z.x;
		m_flMatVal[ 0 ][ 3 ] = origin.x;
		m_flMatVal[ 1 ][ 0 ] = x.y;
		m_flMatVal[ 1 ][ 1 ] = y.y;
		m_flMatVal[ 1 ][ 2 ] = z.y;
		m_flMatVal[ 1 ][ 3 ] = origin.y;
		m_flMatVal[ 2 ][ 0 ] = x.z;
		m_flMatVal[ 2 ][ 1 ] = y.z;
		m_flMatVal[ 2 ][ 2 ] = z.z;
		m_flMatVal[ 2 ][ 3 ] = origin.z;
	}

	__forceinline void SetOrigin( const vec3_t& p ) {
		m_flMatVal[ 0 ][ 3 ] = p.x;
		m_flMatVal[ 1 ][ 3 ] = p.y;
		m_flMatVal[ 2 ][ 3 ] = p.z;
	}

	__forceinline void AngleMatrix( const ang_t& angles ) {
		float m[ 3 ][ 4 ] = { };
		float sr, sp, sy, cr, cp, cy;
		DirectX::XMScalarSinCos( &sy, &cy, ToRadians( angles[ 1 ] ) );
		DirectX::XMScalarSinCos( &sp, &cp, ToRadians( angles[ 0 ] ) );
		DirectX::XMScalarSinCos( &sr, &cr, ToRadians( angles[ 2 ] ) );

		// matrix = (YAW * PITCH) * ROLL
		m[ 0 ][ 0 ] = cp * cy;
		m[ 1 ][ 0 ] = cp * sy;
		m[ 2 ][ 0 ] = -sp;

		float crcy = cr * cy;
		float crsy = cr * sy;
		float srcy = sr * cy;
		float srsy = sr * sy;
		m[ 0 ][ 1 ] = sp * srcy - crsy;
		m[ 1 ][ 1 ] = sp * srsy + crcy;
		m[ 2 ][ 1 ] = sr * cp;

		m[ 0 ][ 2 ] = ( sp * crcy + srsy );
		m[ 1 ][ 2 ] = ( sp * crsy - srcy );
		m[ 2 ][ 2 ] = cr * cp;

		m[ 0 ][ 3 ] = 0.0f;
		m[ 1 ][ 3 ] = 0.0f;
		m[ 2 ][ 3 ] = 0.0f;
	}


	__forceinline void MatrixSetColumn( const vec3_t& in, int column ) {
		float m[ 3 ][ 4 ] = { };
		m[ 0 ][ column ] = in.x;
		m[ 1 ][ column ] = in.y;
		m[ 2 ][ column ] = in.z;
	}

	__forceinline void AngleMatrix( const ang_t& angles, const vec3_t& position ) {
		AngleMatrix( angles );
		MatrixSetColumn( position, 3 );
	}

	__forceinline vec3_t GetOrigin( ) {
		return { m_flMatVal[ 0 ][ 3 ], m_flMatVal[ 1 ][ 3 ], m_flMatVal[ 2 ][ 3 ] };
	}

	__forceinline float* operator[]( int i ) {
		return m_flMatVal[ i ];
	}

	__forceinline const float* operator[]( int i ) const {
		return m_flMatVal[ i ];
	}

	__forceinline float* Base( ) {
		return &m_flMatVal[ 0 ][ 0 ];
	}

	__forceinline const float* Base( ) const {
		return &m_flMatVal[ 0 ][ 0 ];
	}

public:
	float m_flMatVal[ 3 ][ 4 ];
};

class Quaternion {
public:
	float x, y, z, w;
};

class QAngle
{
public:
	QAngle( );
	QAngle( float x, float y, float z );
	QAngle( const QAngle& v );
	QAngle( const float* v );

public:
	void Set( float x = 0.0f, float y = 0.0f, float z = 0.0f );

	void Normalize( );
	void Clamp( );

	bool IsZero( float tolerance = 0.01f );
	bool AngleAreEqual( QAngle angle, float tolerance = 0.01f );

	QAngle Normalized( );
	QAngle Clamped( );

	QAngle Direction( ) {
		return QAngle( cos( y * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( y * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( -x * 3.14159265358979323846 / 180.0f ) ).Normalized( );
	}

	QAngle Forward( ) {
		return Direction( );
	}


	QAngle Up( ) {
		return QAngle( cos( y * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( y * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( -( x - 90.0f ) * 3.14159265358979323846 / 180.0f ) ).Normalized( );
	}

	QAngle Right( ) {
		return QAngle( cos( ( y + 90.f ) * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( ( y + 90.f ) * 3.14159265358979323846 / 180.0f ) * cos( x * 3.14159265358979323846 / 180.0f ), sin( -x * 3.14159265358979323846 / 180.0f ) ).Normalized( );
	}

	vec3_t ToVectors( vec3_t* side = nullptr, vec3_t* up = nullptr );
	vec3_t ToVectorsTranspose( vec3_t* side = nullptr, vec3_t* up = nullptr );

public:
	float operator [] ( const std::uint32_t index );
	const float operator [] ( const std::uint32_t index ) const;

	QAngle& operator = ( const QAngle& v );
	QAngle& operator = ( const float* v );

	QAngle& operator += ( const QAngle& v );
	QAngle& operator -= ( const QAngle& v );
	QAngle& operator *= ( const QAngle& v );
	QAngle& operator /= ( const QAngle& v );

	QAngle& operator += ( float fl );
	QAngle& operator -= ( float fl );
	QAngle& operator *= ( float fl );
	QAngle& operator /= ( float fl );

	QAngle operator + ( const QAngle& v ) const;
	QAngle operator - ( const QAngle& v ) const;
	QAngle operator * ( const QAngle& v ) const;
	QAngle operator / ( const QAngle& v ) const;

	QAngle operator + ( float fl ) const;
	QAngle operator - ( float fl ) const;
	QAngle operator * ( float fl ) const;
	QAngle operator / ( float fl ) const;

public:
	static QAngle Zero;

public:
	union {
		struct {
			float pitch;
			float yaw;
			float roll;
		};

		struct {
			float x;
			float y;
			float z;
		};
	};
};


class matrix3x4_t_imm {
public:
	float m[ 3 ][ 4 ] = { };

	float* operator [] ( const std::uint32_t index );
	const float* operator [] ( const std::uint32_t index ) const;

	vec3_t at( const std::uint32_t i ) const;

	void TransformAABB( const vec3_t& vecMinsIn, const vec3_t& vecMaxsIn, vec3_t& vecMinsOut, vec3_t& vecMaxsOut ) const;
	void AngleMatrix( const QAngle& angles );
	void AngleMatrix( const QAngle& angles, const vec3_t& position );
	void MatrixAngles( QAngle& angles );
	void MatrixAngles( QAngle& angles, vec3_t& position );
	void MatrixSetColumn( const vec3_t& in, int column );
	void QuaternionMatrix( const Quaternion& q );
	void QuaternionMatrix( const Quaternion& q, const vec3_t& pos );
	matrix3x4_t_imm ConcatTransforms( matrix3x4_t_imm in ) const;

	matrix3x4_t_imm( ) { }
	matrix3x4_t_imm(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23 ) {
		m[ 0 ][ 0 ] = m00; m[ 0 ][ 1 ] = m01; m[ 0 ][ 2 ] = m02; m[ 0 ][ 3 ] = m03;
		m[ 1 ][ 0 ] = m10; m[ 1 ][ 1 ] = m11; m[ 1 ][ 2 ] = m12; m[ 1 ][ 3 ] = m13;
		m[ 2 ][ 0 ] = m20; m[ 2 ][ 1 ] = m21; m[ 2 ][ 2 ] = m22; m[ 2 ][ 3 ] = m23;
	}

	vec3_t operator*( const vec3_t& vVec ) const;
	matrix3x4_t_imm operator+( const matrix3x4_t_imm& other ) const;
	matrix3x4_t_imm operator-( const matrix3x4_t_imm& other ) const;
	matrix3x4_t_imm operator*( const float& other ) const;
	matrix3x4_t_imm operator*( const matrix3x4_t_imm& vm ) const {
		return ConcatTransforms( vm );
	}
};

class BoneArray : public matrix3x4_t {
public:
	bool get_bone( vec3_t & out, int bone = 0 ) {
		if( bone < 0 || bone >= 128 )
			return false;

		matrix3x4_t* bone_matrix = &this[ bone ];

		if( !bone_matrix )
			return false;

		out = { bone_matrix->m_flMatVal[ 0 ][ 3 ], bone_matrix->m_flMatVal[ 1 ][ 3 ], bone_matrix->m_flMatVal[ 2 ][ 3 ] };

		return true;
	}
};

class __declspec( align( 16 ) ) matrix3x4a_t : public matrix3x4_t {
public:
	__forceinline matrix3x4a_t& operator=( const matrix3x4_t& src ) {
		std::memcpy( Base( ), src.Base( ), sizeof( float ) * 3 * 4 );
		return *this;
	};
};

class VMatrix {
public:
	__forceinline float* operator[]( int i ) {
		return m[ i ];
	}
	__forceinline const float* operator[]( int i ) const {
		return m[ i ];
	}

	__forceinline float* Base( ) {
		return &m[ 0 ][ 0 ];
	}
	__forceinline const float* Base( ) const {
		return &m[ 0 ][ 0 ];
	}

public:
	float m[ 4 ][ 4 ];
};