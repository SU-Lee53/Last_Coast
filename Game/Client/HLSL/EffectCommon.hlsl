#ifndef _EFFECT_COMMON_
#define _EFFECT_COMMON_


cbuffer cbCameraData : register(b0)
{
    matrix gmtxView;
    matrix gmtxProjection;
    float3 gvCameraPosition;
};

#define MAX_EFFECT_PER_DRAW 100

struct ParticleData
{
    float3 vPosition;
    float fElapsedTime;
    float3 vForce;
    float fAdditionalData;
};

cbuffer cbParticleData : register(b1)
{
    ParticleData gParticleData[MAX_EFFECT_PER_DRAW];
};

cbuffer cbParticleIndexData : register(b2)
{
    uint gnDataIndex;
};

struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 initialVelocity : VELOCITY;
    float2 initialSize : SIZE;
    float randomValue : RANDOM;
    float startTime : STARTTIME;
    float lifeTime : LIFETIME;
    float mass : MASS;
};

struct VS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float4 color : COLOR;
    float2 size : SIZE;
    uint nInstanceID : INSTANCEID;
};

struct GS_PARTICLE_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

const static float3 gvGravity = float3(0.f, -9.8f, 0.f);

#endif
