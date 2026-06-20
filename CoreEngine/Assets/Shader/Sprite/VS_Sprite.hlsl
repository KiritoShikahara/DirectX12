#include"SpriteHeader.hlsli"

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    // SV_InstanceID は DrawInstanced の StartInstanceLocation を加算済みの値を返すはずの
    // 仕様だが、GPU/ドライバ依存で StartInstanceLocation が反映されないケースがあるため、
    // 常に 0 始まりの相対値として扱い、CPU から渡された InstanceOffset を明示的に加算する。
    uint index = InstanceOffset + instanceID;
    SpriteShaderData data = gInstanceData[index];

    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), data.WVP);

    // スプライトシート切り出し: 標準UV(0,0)~(1,1)にスケール+オフセットを適用
    // FillAmount/FillType によるクリップ判定（PS側）はローカルUV空間(0,0)~(1,1)が前提のため、
    // 変換前の input.TexCoord をそのまま FillAmount/FillType の判定には使う想定（PS側は無改造）
    output.TexCoord = input.TexCoord * data.UVScale + data.UVOffset;

    output.Color = data.Color;
    output.Intensity = data.Intensity;
    output.FillAmount = data.FillAmount;
    output.FillType = data.FillType;

    return output;
}
