#pragma once
#include "../includes.h"

enum BoneSetupFlags {
	None = 0,
	UseInterpolatedOrigin = ( 1 << 0 ),
	UseCustomOutput = ( 1 << 1 ),
	ForceInvalidateBoneCache = ( 1 << 2 ),
	AttachmentHelper = ( 1 << 3 ),
};

namespace controll_bones {
	bool SetupBonesRebuild( Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags );
	bool BuildBones( Player* entity, int boneMask, int flags );

}