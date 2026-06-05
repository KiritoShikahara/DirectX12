
struct ModelInstanceData
{
    row_major float4x4 World;
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float Intensity;
    uint BoneOffset; // BoneBuffer 内の先頭インデックス
    uint BoneCount;
    float3 _pad;
};

StructuredBuffer<ModelInstanceData> InstanceBuffer : register(t0);
