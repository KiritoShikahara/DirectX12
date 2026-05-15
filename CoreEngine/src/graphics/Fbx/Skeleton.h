#pragma once

#include<Utility/Export/Export.h>
#include"BinaryModel.h"
#include<string>
#include<vector>
#include<unordered_map>

namespace graphics
{
	class ENGINE_API Skeleton
	{
    public:
        void Build(const std::vector<BinBone>& bones);

        int BoneCount()   const { return static_cast<int>(mBones.size()); }
        int FindBone(const std::string& name) const;
        int ParentIndex(int i) const { return mBones[i].ParentIndex; }
        const DirectX::XMFLOAT4X4& BindMatrix(int i) const { return mBones[i].BindMatrix; }
        const std::string& BoneName(int i) const { return mBones[i].Name; }

    private:
        std::vector<BinBone>                 mBones;
        std::unordered_map<std::string, int> mBoneMap;
	};

} // graphics