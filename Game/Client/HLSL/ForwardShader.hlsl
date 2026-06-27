#include "NewCommon.hlsl"
#include "Light.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardShader

VS_STANDARD_OUTPUT VSForwardStandard(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;
	int nWorldTransformBase = gnWorldTransformOffset;

	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	
	matrix mtxWorld = gWorldTransforms[nWorldTransformBase].mtxWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	output.normalW = normalize(mul(input.normal, mtxNormal));
	output.tangentW = normalize(mul(input.tangent, mtxNormal));
	output.uv = input.uv;
    
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AnimatedShader

VS_STANDARD_OUTPUT VSForwardAnimated(VS_SKINNED_INPUT input)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;
	int nBoneTransformBase = gBoneTransformOffsets[0];
	int nWorldTransformBase = gnWorldTransformOffset;

	float fWeights[4] = { 0.f, 0.f, 0.f, 0.f };
	fWeights[0] = input.blendWeights.x;
	fWeights[1] = input.blendWeights.y;
	fWeights[2] = input.blendWeights.z;
	fWeights[3] = input.blendWeights.w;
	
	float3 position = float3(0, 0, 0);
	float3 normal = float3(0, 0, 0);
	float3 tangent = float3(0, 0, 0);
	
	[unroll(4)]
	for (int i = 0; i < 4; ++i)
	{
		position += fWeights[i] * mul(float4(input.position, 1.f), gBoneTransforms[nBoneTransformBase + input.blendInices[i]]).xyz;
		normal += fWeights[i] * mul(input.normal, (float3x3) gBoneTransforms[nBoneTransformBase + input.blendInices[i]]);
		tangent += fWeights[i] * mul(input.tangent, (float3x3) gBoneTransforms[nBoneTransformBase + input.blendInices[i]]);
	}
	
	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	matrix mtxWorld = gWorldTransforms[nBoneTransformBase].mtxWorld;
	
	float4 positionW = mul(float4(position, 1.f), mtxWorld);
	output.positionW = positionW.xyz;
	output.position = mul(positionW, mtxViewProjection);
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nBoneTransformBase].mtxInvWorld);
	output.normalW = normalize(mul(normal, mtxNormal));
	output.tangentW = normalize(mul(tangent, mtxNormal));
	output.uv = input.uv;
    
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Lighting for transprent

float4 PSForwardLighting(VS_STANDARD_OUTPUT input) : SV_Target0
{
	// Albedo
	float4 cAlbedo = gtxtTextures[gnTextureIndex.x].Sample(gSamplerState, input.uv);
	
	float3 albedoColor = cAlbedo.rgb;
	float alpha = cAlbedo.a;
	
	// Materials
	MaterialData m = gMaterialDatas[gnMaterialIndex];
	float fRoughness = max(1.0f - saturate(m.fSmoothness), 0.82f);
	float fGloss = 1.0f - fRoughness;
	float fSpecularPower = lerp(4.f, 192.f, fGloss * fGloss);
	float fSpecularIntensity = min(saturate(m.cSpecular.a), 0.10f) * lerp(0.05f, 1.0f, fGloss * fGloss);
	
	float3 viewDir = normalize(gCamera.v3CameraPosition - input.positionW);
	float3 normalW = (gnTextureIndex.y != -1) ? ComputeNormal(input.normalW, input.tangentW, input.uv) : input.normalW;
	
	float fShadow = ComputeCascadeShadow(input.positionW);
	
	float3 finalColor = 0;
	[unroll(MAX_LIGHTS)]
	for (int i = 0; i < gSceneGlobal.nNumLights; ++i)
	{
		LightData lightData = gLightData[i];
		if (lightData.bEnable == 0)
		{
			continue;
		}
		
		switch (lightData.nType)
		{
			case POINT_LIGHT:
				finalColor += PointLight(input.positionW, normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i);
				break;
		
			case SPOT_LIGHT:
				finalColor += SpotLight(input.positionW, normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i);
				break;
			
			case DIRECTIONAL_LIGHT:
				finalColor += DirectionalLight(input.positionW, normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i) * fShadow;
				break;
		}
	}
	
	finalColor = ApplyFog(finalColor, input.positionW);
	return float4(finalColor, alpha);
}
