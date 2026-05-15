#include "Skeleton.h"

namespace graphics
{
    void Skeleton::Build(const std::vector<BinBone>& bones)
    {
        mBones = bones;
        mBoneMap.clear();
        for (int i = 0; i < static_cast<int>(mBones.size()); ++i)
            mBoneMap[mBones[i].Name] = i;
    }

    int Skeleton::FindBone(const std::string& name) const
    {
        auto it = mBoneMap.find(name);
        return (it != mBoneMap.end()) ? it->second : -1;
    }
}