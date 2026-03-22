#include "NewCommon.hlsl"
#include "Light.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardShader

VS_STANDARD_OUTPUT VSForwardStandard(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;

	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	
	matrix mtxWorld = gWorldTransforms[gnWorldTransformIndex].mtxWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[gnWorldTransformIndex].mtxInvWorld);
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
		position += fWeights[i] * mul(float4(input.position, 1.f), gBoneTransforms[input.blendInices[i]]).xyz;
		normal += fWeights[i] * mul(input.normal, (float3x3) gBoneTransforms[input.blendInices[i]]);
		tangent += fWeights[i] * mul(input.tangent, (float3x3) gBoneTransforms[input.blendInices[i]]);
	}
	
	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	matrix mtxWorld = gWorldTransforms[gnWorldTransformIndex].mtxWorld;
	
	float4 positionW = mul(float4(position, 1.f), mtxWorld);
	output.positionW = positionW.xyz;
	output.position = mul(positionW, mtxViewProjection);
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[gnWorldTransformIndex].mtxInvWorld);
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
	MaterialData m = gMaterialData[gnMaterialIndex];
	float fSpecularPower = lerp(8.f, 256.f, pow(m.fGlossiness, 2));
	float fSpecularIntensity = saturate(m.cSpecular.a);
	
	float3 viewDir = normalize(gCamera.v3CameraPosition - input.positionW);
	
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
				finalColor += PointLight(input.positionW, input.normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i);
				break;
		
			case SPOT_LIGHT:
				finalColor += SpotLight(input.positionW, input.normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i);
				break;
			
			case DIRECTIONAL_LIGHT:
				finalColor += DirectionalLight(input.positionW, input.normalW, viewDir, albedoColor, fSpecularPower, fSpecularIntensity, i);
				break;
		}
	}
	
	return float4(finalColor, alpha);
}
