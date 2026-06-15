struct FbxSceneData
{
    float4x4 ViewProjection; // CPU 側転置済み
    float3 CameraPosition;
    uint LightCount;
};
StructuredBuffer<FbxSceneData> SceneBuffer : register(t8, space0);

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float3 WorldDir : TEXCOORD0; // PS 側で normalize() してサンプリングに使う
};

VS_OUTPUT main(uint vertexId : SV_VertexID)
{
    // --- フルスクリーントライアングル ---
    // vertexId: 0=(-1,-1)  1=(-1, 3)  2=(3,-1)
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 ndc = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);

    // --- VP^-1 を Cramer's rule で算出してワールド方向を逆投影 ---
    //
    // FbxSceneData は VP 合成済み行列のみを持つため VP^-1 で逆変換する。
    // VS は 3 頂点しか走らないため逆行列計算のコストは許容範囲。

    float4x4 VP = SceneBuffer[0].ViewProjection;

    float4x4 invVP;
    {
        float3 a = float3(VP[0][0], VP[1][0], VP[2][0]);
        float3 b = float3(VP[0][1], VP[1][1], VP[2][1]);
        float3 c = float3(VP[0][2], VP[1][2], VP[2][2]);
        float3 d = float3(VP[0][3], VP[1][3], VP[2][3]);

        float x = VP[3][0];
        float y = VP[3][1];
        float z = VP[3][2];
        float w = VP[3][3];

        float3 s = cross(a, b);
        float3 t = cross(c, d);
        float3 u = a * y - b * x;
        float3 v = c * w - d * z;

        float invDet = 1.0f / (dot(s, v) + dot(t, u));
        s *= invDet;
        t *= invDet;
        u *= invDet;
        v *= invDet;

        float3 r0 = cross(b, v) + t * y;
        float3 r1 = cross(v, a) - t * x;
        float3 r2 = cross(d, u) + s * w;
        float3 r3 = cross(u, c) - s * z;

        invVP = float4x4(
            float4(r0, -dot(b, t)),
            float4(r1, dot(a, t)),
            float4(r2, -dot(d, s)),
            float4(r3, dot(c, s))
        );
    }

    // NDC (near 面 z=0) → ワールド空間
    float4 worldH = mul(float4(ndc, 0.0f, 1.0f), invVP);
    float3 worldPos = worldH.xyz / worldH.w;

    // カメラ位置を引いてサンプリング方向を得る (平行移動成分を除去)
    float3 worldDir = worldPos - SceneBuffer[0].CameraPosition;

    VS_OUTPUT output;
    output.Position = float4(ndc, 1.0f, 1.0f); // z=w=1 → NDC 深度 = 1.0 (最遠面)
    output.WorldDir = worldDir; // PS 側で normalize()

    return output;
}
