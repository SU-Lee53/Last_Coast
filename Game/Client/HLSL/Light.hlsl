#ifndef _LIGHT_
#define _LIGHT_

#include "Common.hlsl"

// ==============================================================
// 10.31
// 현재 조명처리에서 Material 반영을 제거함
// 사유 : Sphere 모델의 Material에 정체모를 푸른색이 들어가있음;;
// ==============================================================


float4 DirectionalLight(int nIndex, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = -gLights[nIndex].vDirection;
    float fDiffuseFactor = dot(vToLight, vNormal);
    float fSpecularFactor = 0.0f;
    if (fDiffuseFactor > 0.0f)
    {
        if (gMaterial.cSpecular.a != 0.0f)
        {
#ifdef _WITH_REFLECT
			float3 vReflect = reflect(-vToLight, vNormal);
			fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
            float3 vHalf = normalize(vToCamera + vToLight);
#else
			float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
            fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.cSpecular.a);
#endif
        }
    }

    return ((gLights[nIndex].cAmbient) + (gLights[nIndex].cDiffuse * fDiffuseFactor) + (gLights[nIndex].cSpecular * fSpecularFactor));
}

float4 PointLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = gLights[nIndex].vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].fRange)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (gMaterial.cSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
                float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
                fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.cSpecular.a);
#endif
            }
        }
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

        return (((gLights[nIndex].cAmbient) + (gLights[nIndex].cDiffuse * fDiffuseFactor) + (gLights[nIndex].cSpecular * fSpecularFactor)) * fAttenuationFactor);
    }
    else
    {
        return (float4(0.0f, 0.0f, 0.0f, 0.0f));
    }
}

float4 SpotLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = gLights[nIndex].vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].fRange)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (gMaterial.cSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
                float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
                fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.cSpecular.a);
#endif
            }
        }
#ifdef _WITH_THETA_PHI_CONES
        float fAlpha = max(dot(-vToLight, gLights[nIndex].vDirection), 0.0f);
        float fSpotFactor = pow(max(((fAlpha - gLights[nIndex].fPhi) / (gLights[nIndex].fTheta - gLights[nIndex].fPhi)), 0.0f), gLights[nIndex].fFalloff);
#else
		float fSpotFactor = pow(max(dot(-vToLight, gLights[i].vDirection), 0.0f), gLights[i].fFalloff);
#endif
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

        return (((gLights[nIndex].cAmbient) + (gLights[nIndex].cDiffuse * fDiffuseFactor) + (gLights[nIndex].cSpecular * fSpecularFactor)) * fAttenuationFactor * fSpotFactor);
    }
    else
    {
        return (float4(0.0f, 0.0f, 0.0f, 0.0f));
    }
}

float4 Lighting(float3 vPosition, float3 vNormal)
{
    float3 vCameraPosition = gCameraData.gvCameraPosition.xyz;
    float3 vToCamera = normalize(vCameraPosition - vPosition);

    float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	[unroll(MAX_LIGHTS)]
    for (int i = 0; i < gnLights; i++)
    {
        if (gLights[i].bEnable)
        {
            if (gLights[i].nType == DIRECTIONAL_LIGHT)
            {
                cColor += DirectionalLight(i, vNormal, vToCamera);
            }
            else if (gLights[i].nType == POINT_LIGHT)
            {
                cColor += PointLight(i, vPosition, vNormal, vToCamera);
            }
            else if (gLights[i].nType == SPOT_LIGHT)
            {
                cColor += SpotLight(i, vPosition, vNormal, vToCamera);
            }
        }
    }
    cColor += (gcGlobalAmbientLight * 1.0);
    cColor.a = gMaterial.cDiffuse.a;

    return (cColor);
}

#endif