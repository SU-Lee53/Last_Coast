#include "NewCommon.hlsl"

struct VS_SKYBOX_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_SKYBOX_OUTPUT
{
	float4 position : SV_POSITION;
	float3 dir : TEXCOORD0;
};

VS_SKYBOX_OUTPUT VSSkybox(VS_SKYBOX_INPUT input)
{
	VS_SKYBOX_OUTPUT output = (VS_SKYBOX_OUTPUT) 0;
	
	float4x4 mtxView = gCamera.mtxView;
	mtxView._41_42_43 = float3(0, 0, 0);
	
	float4x4 VP = mul(mtxView, gCamera.mtxProjection);
	float4 posW = mul(float4(input.position, 1.0f), VP);
	
	output.position = posW.xyww;
	output.dir = input.position;
	
	return output;
}

float GetSkyGradientColor(float y)
{
	// [-1 ~ +1]
	float t = saturate(y * 0.5f + 0.5f);
	t = pow(t, 0.35f); // Make the sky blue spread more naturally on top
	return t;
}

float3 EvaluateProceduralSky(float3 dir, float fDayBlend)
{
	float t = GetSkyGradientColor(dir.y);
	
	float3 daySky = lerp(gSkybox.v3DayHorizonColor, gSkybox.v3DayZenithColor, t);
	float3 nightSky = lerp(gSkybox.v3NightHorizonColor, gSkybox.v3NightZenithColor, t);

	return lerp(nightSky, daySky, fDayBlend);
}

float3 RotateY(float3 v, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	return float3(v.x * c - v.z * s, v.y, v.x * s + v.z * c);
}

// Generate random float
float Hash12(float2 p)
{
	float h = dot(p, float2(127.1f, 311.7f));
	return frac(sin(h) * 43758.5453123f);
}

// Generate random float2
float2 Hash22(float2 p)
{
	float x = dot(p, float2(127.1f, 311.7f));
	float y = dot(p, float2(269.5f, 183.3f));
	return frac(sin(float2(x, y)) * 43758.5453123f);
}

// Map dir to Spherical UV
float2 DirToSphericalUV(float3 dir)
{
	dir = normalize(dir);
	float u = atan2(dir.z, dir.x) / (2.0f * PI) + 0.5f;
	float v = acos(clamp(dir.y, -1.f, 1.f)) / PI;
	
	return float2(u, v);
}

float EvaluateStarfield(float3 dir, float fTime)
{
	const float fRotationSpeed = 0.001f;
	float3 starDir = normalize(RotateY(dir, fTime * fRotationSpeed));
	
	float2 uv = DirToSphericalUV(starDir);
	float2 gridUV = uv * float2(420.f, 220.f);	// This determines density of stars
	
	float2 cell = floor(gridUV);
	float2 local = frac(gridUV);
	
	float starChance = Hash12(cell);
	float fStarThreshold = lerp(0.995f, 0.90f, saturate(gSkybox.fStarDensity));
	if (starChance < fStarThreshold)
	{
		return 0.f;
	}
	
	float2 starOffset = Hash22(cell);
	float2 d = local - starOffset;
	float dist = length(d);
	
	float fBaseStarSize = lerp(0.03f, 0.12f, saturate(gSkybox.fStarScale));
	float fSizeVar = lerp(fBaseStarSize * 0.6f, fBaseStarSize * 1.4f, Hash12(cell + 53.1f));
	float fStarCore = smoothstep(fSizeVar, 0.0f, dist);
	float fStarBrightness = lerp(0.4f, 1.0f, Hash12(cell + 17.13f));
	//float twinkle = 0.85f + 0.15f * sin(fTime * 1.5f + starChance * 100.f);
	
	return fStarCore * fStarBrightness;
}

float GetNightFactor(float fDayBlend)
{
	float fNight = 1.0f - saturate(fDayBlend);
	fNight = smoothstep(0.2f, 1.0f, fNight);
	return fNight;
}

float GetStarHorizonMask(float3 dir)
{
	return saturate(pow(saturate(dir.y * 0.5f + 0.5f), 2.5f));
}

// 2D value noise
float ValueNoise(float2 uv)
{
	float2 i = floor(uv);
	float2 f = frac(uv);
	
	float a = Hash12(i);
	float b = Hash12(i + float2(1.0f, 0.0f));
	float c = Hash12(i + float2(0.0f, 1.0f));
	float d = Hash12(i + float2(1.0f, 1.0f));
	
	float2 u = f * f * (3.0f - 2.0f * f);
	
	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float FBM_Cloud(float2 uv)
{
	float fValue = 0.f;
	float fAmplitude = 0.5f;
	float fFrequency = 1.0f;
	
	fValue += ValueNoise(uv * fFrequency) * fAmplitude;
	fFrequency *= 2.0f;
	fAmplitude *= 0.5f;
	
	fValue += ValueNoise(uv * fFrequency) * fAmplitude;
	fFrequency *= 2.0f;
	fAmplitude *= 0.5f;
	
	fValue += ValueNoise(uv * fFrequency) * fAmplitude;
	fFrequency *= 2.0f;
	fAmplitude *= 0.5f;
	
	fValue += ValueNoise(uv * fFrequency) * fAmplitude;
	
	return fValue;
}

float GetCloudHeightMask(float3 dir)
{
	float h = saturate(dir.y * 1.35f + 0.05f);
	h = smoothstep(0.0f, 1.0f, h);
	h = pow(h, 1.2f);
	return h;
}

float2 WarpCloudUV(float2 uv)
{
	float wx = ValueNoise(uv * 0.28f + float2(11.3f, 7.1f));
	float wy = ValueNoise(uv * 0.31f + float2(3.7f, 19.4f));
	
	float2 warp = float2(wx, wy) * 2.0f - 1.0f;
	return uv + warp * 0.10f;
}

float2 GetCloudUV(float3 dir, float fTime, float fSpeedMul, float fScaleMul)
{
	float3 cloudDir = normalize(RotateY(dir, fTime * gSkybox.fCloudSpeed * fSpeedMul));
	float fHemisphere = saturate(cloudDir.y);
	float fDenominator = 0.6f + fHemisphere * 0.8f;
	float2 uv = cloudDir.xz / fDenominator;
	uv *= gSkybox.fCloudScale * fScaleMul;
	
	return uv;
}

float EvaluateCloudMask_Main(float3 dir, float fTime)
{
	float2 uv = GetCloudUV(dir, fTime, 1.0f, 1.0f);
	uv = WarpCloudUV(uv);
	
	float fShapeNoise = FBM_Cloud(uv * 1.0f); // Noise for big
	float fDetailNoise = FBM_Cloud(uv * 3.0f + float2(13.7f, 5.2f)); // Noise for small detail
	
	float fCloudNoise = lerp(fShapeNoise, fDetailNoise, 0.38f);
	fCloudNoise = saturate((fCloudNoise - 0.06f) / 0.94f);
	
	float fThreshold = lerp(0.68f, 0.35f, saturate(gSkybox.fCloudCoverage));
	float fSoftness = lerp(0.18f, 0.06f, saturate(gSkybox.fCloudDensity));
	
	float fCloudMask = smoothstep(fThreshold, fThreshold + fSoftness, fCloudNoise);
	fCloudMask *= GetCloudHeightMask(dir);
	
	return saturate(fCloudMask);
}

float EvaluateCloudMask_Secondary(float3 dir, float fTime)
{
	float2 uv = GetCloudUV(dir, fTime, 1.35f, 1.8f);
	uv = WarpCloudUV(uv);
	
	float fShapeNoise = FBM_Cloud(uv * 1.4f + float2(7.2f, 3.1f));
	float fDetailNoise = FBM_Cloud(uv * 4.5f + float2(21.3f, 11.7f));

	float fCloudNoise = lerp(fShapeNoise, fDetailNoise, 0.45f);
	fCloudNoise = saturate((fCloudNoise - 0.18f) / 0.82f);
	
	float fThreshold = lerp(0.80f, 0.55f, saturate(gSkybox.fCloudCoverage));
	float fSoftness = lerp(0.12f, 0.04f, saturate(gSkybox.fCloudDensity));

	float fCloudMask = smoothstep(fThreshold, fThreshold + fSoftness, fCloudNoise);
	fCloudMask *= GetCloudHeightMask(dir);

	return saturate(fCloudMask * 0.55f);
}

float EvaluateCloudMask(float3 dir, float fTime)
{
	float fMainMask = EvaluateCloudMask_Main(dir, fTime);
	float fSecondaryMask = EvaluateCloudMask_Secondary(dir, fTime);
	
	float fCombined = saturate(fMainMask + fSecondaryMask * (1.0f - fMainMask * 0.5f));
	return fCombined;
}

float3 EvaluateCloudColor(float3 skyColor, float fDayBlend)
{
	float3 dayCloudColor = float3(0.96f, 0.97f, 0.98f);
	float3 nightCloudColor = float3(0.16f, 0.20f, 0.28f);

	float3 cloudBase = lerp(nightCloudColor, dayCloudColor, fDayBlend);

	return lerp(cloudBase, skyColor * 1.05f, 0.10f);
}

float GetCloudEdgeFactor(float fCloudMask)
{
	float a = smoothstep(0.05f, 0.35f, fCloudMask);
	float b = 1.0f - smoothstep(0.55f, 0.95f, fCloudMask);
	return saturate(a * b);
}

float GetHorizonCloudFade(float3 dir)
{
	float h = 1.0f - abs(dir.y);
	h = saturate(h);
	h = pow(h, 1.5f);
	return h;
}

float GetCloudPhaseLighting(float3 dir, float3 sunDir)
{
	float mu = dot(dir, sunDir);
	
	// Hard forward scattering near sun direction
	float fForward = saturate(mu * 0.5f + 0.5f);
	fForward = pow(fForward, max(gSkybox.fCloudLightIntensity, 0.001f));
	
	// soft scattering from side direction
	float fSideways = saturate(1.0f - abs(mu));
	fSideways = pow(fSideways, 3.0f);
	
	return lerp(fForward, fSideways, 0.2);
}

float3 ApplyCloudScattering(float3 cloudColor, float3 skyColor, float3 dir, float3 sunDir, float fCloudMask, float fCloudEdge)
{
	float phase = GetCloudPhaseLighting(dir, sunDir);
	
	// Thick clouds pass through less light 
	float fTransmittance = lerp(1.0f, 0.72f, fCloudMask);
	
	// Silver lining near sun
	float fSunFacing = saturate(dot(dir, sunDir));
	float fSilverLining = pow(fSunFacing, 8.0f) * fCloudEdge;
	
	float fScatter = lerp(0.85f, 1.10f, phase);
	
	float3 litColor = cloudColor;
	litColor *= fTransmittance;
	litColor *= fScatter;
	
	float3 liningColor = lerp(cloudColor, skyColor * 1.15f, 0.35f);
	litColor += liningColor * fSilverLining * 0.25f;
	
	return litColor;
}

float ComputeSkyFogFactor(float3 dir)
{
	float fHorizon = 1.0f - abs(dir.y);
	fHorizon = saturate(fHorizon);
	fHorizon = pow(fHorizon, 1.5f);
	
	float fLower = saturate((-dir.y + 1.0f) * 0.5f);
	fLower = pow(fLower, 1.25f);
	
	float fFogStrength = saturate(gfFogDistanceDensity * 100.f + gfFogHeightDensity * 10.f);
	
	float fFogFactor = fHorizon * 0.75f + fLower * 0.25f;
	fFogFactor *= lerp(0.35f, 1.0f, fFogStrength);
	
	return saturate(fFogFactor);
}

float3 ApplySkyFog(float3 color, float3 dir, float fStrengthMul)
{
	float fFogFactor = ComputeSkyFogFactor(dir);
	fFogFactor *= gfFogMaxOpacity;
	fFogFactor *= fStrengthMul;
	
	return lerp(color, gfogColor.rgb, saturate(fFogFactor));
}

float4 PSSkybox(VS_SKYBOX_OUTPUT input) : SV_Target
{
	float3 inDir = normalize(input.dir);
	float3 sunDir = normalize(gSkybox.v3SunDirection);
	
	float fDayBlend = saturate(gSkybox.fDayNightBlend);
	fDayBlend = smoothstep(0.f, 1.f, fDayBlend);
	
	// 1. Base procedural sky gradient
	float3 skyColor = EvaluateProceduralSky(inDir, fDayBlend);
	
	// 2. Twilight timing from sun height
	float fSunHeight = sunDir.y;
	float fTwilightTime = 1.0f - smoothstep(0.f, gSkybox.fTwilightWidth, abs(fSunHeight));
	
	// 3. Directional twilight toward sun
	float fSunForward = saturate(dot(inDir, sunDir));
	float fSunSideFactor = pow(fSunForward, gSkybox.fTwilightSunFocus);
	
	// 4. Stronger near horizon
	float fHorizonFactor = 1.0f - abs(inDir.y);
	fHorizonFactor = pow(saturate(fHorizonFactor), 1.35f);
	
	float3 warmTwilightColor = gSkybox.v3TwilightColor * float3(1.15f, 1.0f, 0.85f);
	
	float fGlobalTwilight = fTwilightTime * fHorizonFactor * 0.25f;
	float fDirectionalTwilight = fTwilightTime * fHorizonFactor * fSunSideFactor;
	
	// 5. Apply tint
	float fTwilightTintFactor = saturate(fGlobalTwilight * 0.4f + fDirectionalTwilight * 0.8f);
	float3 twilightTintColor = lerp(float3(1.f, 1.f, 1.f), warmTwilightColor, fTwilightTintFactor);
	
	skyColor *= twilightTintColor;
	
	// 6. Additive twilight
	float3 finalColor = skyColor;
	finalColor += gSkybox.v3TwilightColor * fGlobalTwilight * (gSkybox.fTwilightIntensity * 0.1f);
	finalColor += gSkybox.v3TwilightColor * fDirectionalTwilight * (gSkybox.fTwilightIntensity * 0.6f);
	
	// Clouds
	float fCloudMask = EvaluateCloudMask(inDir, gSceneGlobal.fTotalTime);
	float3 cloudColor = EvaluateCloudColor(finalColor, fDayBlend);
	
	float fCloudEdge = GetCloudEdgeFactor(fCloudMask);
	
	float3 litCloudColor = ApplyCloudScattering(cloudColor, finalColor, inDir, sunDir, fCloudMask, fCloudEdge);
	
	float fHorizonCloudFade = GetHorizonCloudFade(inDir);
	litCloudColor = lerp(litCloudColor, finalColor, fHorizonCloudFade * 0.25f);
	
	float fCloudStrength = lerp(0.35f, 0.85f, fDayBlend);
	finalColor = lerp(finalColor, litCloudColor, fCloudMask * fCloudStrength);
	
	// Compute stars
	float fNightFactor = GetNightFactor(fDayBlend);
	float fStarHorizonMask = GetStarHorizonMask(inDir);
	float fStarMask = EvaluateStarfield(inDir, gSceneGlobal.fTotalTime);
	fStarMask *= (1.0f - fCloudMask);	// Clouds cover stars
	
	//float3 starColor = float3(1.f, 1.f, 1.f);
	float3 starColor = lerp(
	   float3(0.8f, 0.85f, 1.0f),
	float3(1.0f, 0.95f, 0.85f),
	Hash12(floor(DirToSphericalUV(inDir) * 100.f)));
	
	finalColor += starColor * fStarMask * fNightFactor * fStarHorizonMask * 1.2f;
	
	// Lower hemishere darkening
	float fLowerFade = saturate(inDir.y * 0.5f + 0.5f);
	finalColor *= lerp(0.35f, 1.0f, fLowerFade);
	
	finalColor *= gSkybox.fSkyIntensity;
	finalColor = ApplySkyFog(finalColor, inDir, 1.0f);
	
	return float4(finalColor, 1.0f);
}

float4 PSCelestialDisk(VS_SKYBOX_OUTPUT input) : SV_Target
{
	float3 dir = normalize(input.dir);
	
	float3 sunDir = normalize(gSkybox.v3SunDirection);
	float3 moonDir = -sunDir;
	
	float sunDot = dot(dir, sunDir);
	float moonDot = dot(dir, moonDir);
	
	float sunCore = smoothstep(gSkybox.fSunDiskSize, 1.0f, sunDot);
	float moonCore = smoothstep(gSkybox.fMoonDiskSize, 1.0f, moonDot);
	
	float sunGlow = smoothstep(gSkybox.fSunGlowSize, 1.0f, sunDot);
	float moonGlow = smoothstep(gSkybox.fMoonGlowSize, 1.0f, moonDot);
	
	float sunVisible = saturate(gSkybox.fDayNightBlend);
	float moonVisible = 1 - sunVisible;
	
	float fSunHeight = sunDir.y;
	float fTwilightTime = 1.0f - smoothstep(0.f, gSkybox.fTwilightWidth, abs(fSunHeight));
	
	float3 sunColor = lerp(gSkybox.v3SunColor, gSkybox.v3TwilightColor, fTwilightTime) * gSkybox.fSunIntensity;
	float3 moonColor = gSkybox.v3MoonColor * gSkybox.fMoonIntensity;
	
	float3 color = 0.f;
	color += sunColor * sunGlow * sunVisible * 0.15f;
	color += sunColor * sunCore * sunVisible;

	color += moonColor * moonGlow * moonVisible * 0.05f;
	color += moonColor * moonCore * moonVisible;

	color = ApplySkyFog(color, dir, 0.35f);
	return float4(color, 1.f);
}
