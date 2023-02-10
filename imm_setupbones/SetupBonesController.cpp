#include "SetupBonesController.hpp"

#define MAX_NUM_LODS 8

namespace controll_bones {
	bool SetupBonesRebuild( Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags ) {
		if ( *( int* )( uintptr_t( entity ) + g_entoffsets.m_nSequence ) == -1 ) {
			return false;
		}

		if ( boneMask == -1 ) {
			boneMask = entity->m_iPrevBoneMask( );
		}

		boneMask = boneMask | 0x80000;

		// If we're setting up LOD N, we have set up all lower LODs also
		// because lower LODs always use subsets of the bones of higher LODs.
		int nLOD = 0;
		int nMask = BONE_USED_BY_VERTEX_LOD0;
		for ( ; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1 ) {
			if ( boneMask & nMask )
				break;
		}
		for ( ; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1 ) {
			boneMask |= nMask;
		}

		auto model_bone_counter = **( unsigned long** )( g_csgo.InvalidateBoneCache + 0x000A );
		CBoneAccessor backup_bone_accessor = entity->m_BoneAccessor( );
		CBoneAccessor* bone_accessor = &entity->m_BoneAccessor( );
		if ( !bone_accessor )
			return false;

		if ( entity->m_iMostRecentModelBoneCounter( ) != model_bone_counter || ( flags & BoneSetupFlags::ForceInvalidateBoneCache ) ) {
			if ( FLT_MAX >= entity->m_flLastBoneSetupTime( ) || time < entity->m_flLastBoneSetupTime( ) ) {
				bone_accessor->m_ReadableBones = 0;
				bone_accessor->m_WritableBones = 0;
				entity->m_flLastBoneSetupTime( ) = ( time );
			}

			entity->m_iPrevBoneMask( ) = entity->m_iAccumulatedBoneMask( );
			entity->m_iAccumulatedBoneMask( ) = 0;

			auto hdr = entity->m_studioHdr( );
			if ( hdr ) { // profiler stuff
				( ( CStudioHdrEx* )hdr )->m_nPerfAnimatedBones = 0;
				( ( CStudioHdrEx* )hdr )->m_nPerfUsedBones = 0;
				( ( CStudioHdrEx* )hdr )->m_nPerfAnimationLayers = 0;
			}
		}

		// Keep track of everything asked for over the entire frame
		// But not those things asked for during bone setup
		entity->m_iAccumulatedBoneMask( ) |= boneMask;

		// fix enemy poses getting raped when going out of pvs
		entity->m_iOcclusionFramecount( ) = 0;
		entity->m_iOcclusionFlags( ) = 0;

		// Make sure that we know that we've already calculated some bone stuff this time around.
		entity->m_iMostRecentModelBoneCounter( ) = model_bone_counter;

		bool bReturnCustomMatrix = ( flags & BoneSetupFlags::UseCustomOutput ) && pBoneMatrix;
		CStudioHdr* hdr = entity->m_studioHdr( );
		if ( !hdr ) {
			return false;
		}

		// Setup our transform based on render angles and origin.
		vec3_t origin = ( flags & BoneSetupFlags::UseInterpolatedOrigin ) ? entity->GetAbsOrigin( ) : entity->m_vecOrigin( );
		ang_t angles = entity->GetAbsAngles( );

		alignas( 16 ) matrix3x4_t parentTransform;
		parentTransform.AngleMatrix( angles, origin );

		boneMask |= entity->m_iPrevBoneMask( );

		if ( bReturnCustomMatrix ) {
			bone_accessor->m_pBones = pBoneMatrix;
		}

		// Allow access to the bones we're setting up so we don't get asserts in here.
		int oldReadableBones = bone_accessor->GetReadableBones( );
		int oldWritableBones = bone_accessor->GetWritableBones( );
		int newWritableBones = oldReadableBones | boneMask;
		bone_accessor->SetWritableBones( newWritableBones );
		bone_accessor->SetReadableBones( newWritableBones );

		if ( !( hdr->m_pStudioHdr->m_flags & 0x00000010 ) ) {
			entity->m_fEffects( ) |= EF_NOINTERP;

			entity->m_iEFlags( ) |= EFL_SETTING_UP_BONES;

			//entity->m_pIk( ) = nullptr;
			entity->m_EntClientFlags |= 2; // ENTCLIENTFLAGS_DONTUSEIK

			alignas( 16 ) vec3_t pos[ 128 ];
			alignas( 16 ) quaternion_t q[ 128 ];
			uint8_t computed[ 0x100 ];

			entity->StandardBlendingRules( hdr, pos, q, time, boneMask );

			std::memset( computed, 0, 0x100 );
			entity->BuildTransformations( hdr, pos, q, parentTransform, boneMask, computed );

			entity->m_iEFlags( ) &= ~EFL_SETTING_UP_BONES;

			// entity->ControlMouth( hdr );

			if ( !bReturnCustomMatrix /*&& !bSkipAnimFrame*/ ) {
				memcpy( entity->m_vecBonePos( ), &pos[ 0 ], sizeof( vec3_t ) * hdr->m_pStudioHdr->m_num_bones );
				memcpy( entity->m_quatBoneRot( ), &q[ 0 ], sizeof( quaternion_t ) * hdr->m_pStudioHdr->m_num_bones );
			}
		}
		else {
			parentTransform = bone_accessor->m_pBones[ 0 ];
		}

		/*
				if ( /*boneMask & BONE_USED_BY_ATTACHMENT flags& BoneSetupFlags::AttachmentHelper ) {
					static auto att_helper = pattern::find( g_csgo.m_client_dll, "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4" );

					using AttachmentHelperFn = void( __thiscall* )( Player*, CStudioHdr* );
					( ( AttachmentHelperFn )att_helper )( entity, hdr );
				}
		*/

		// don't override bone cache if we're just generating a standalone matrix
		if ( bReturnCustomMatrix ) {
			*bone_accessor = backup_bone_accessor;

			return true;
		}

		return true;
	}

	bool BuildBones( Player* entity, int boneMask, int flags ) {
		// no need to restore this
		entity->m_bIsJiggleBonesEnabled( ) = false;

		// setup bones :-)
		return SetupBonesRebuild( entity, nullptr, -1, boneMask, g_csgo.m_globals->m_curtime, flags );
	}
}