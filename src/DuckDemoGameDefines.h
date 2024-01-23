#pragma once 

#include "glm/glm.hpp"

#include "DuckDemoUtils.h"

struct DUCK_DEMO_ALIGN(16) DirectionalLightBuf
{
    glm::vec3 uStrength;
    float padding0;
    glm::vec3 uDirection;
};

struct DUCK_DEMO_ALIGN(16) SpotLightBuf
{
    glm::vec3 uStrength;
    float uFalloffStart;
    glm::vec3 uDirection;
    float uFalloffEnd;
    glm::vec3 uPosition;
    float uSpotPower;
};

struct DUCK_DEMO_ALIGN(16) PointLightBuf
{
    glm::vec3 uStrength;
    float uFalloffStart;
    glm::vec3 uPosition;
    float uFalloffEnd;
};

struct DUCK_DEMO_ALIGN(16) FrameBuf
{
    glm::mat4 uViewProj;
    glm::vec3 uEyePosW;
    float padding0;
    glm::vec4 uAmbientLight;
    DirectionalLightBuf uDirLight;
    SpotLightBuf uSpotLight;
    PointLightBuf uPointLights[2];
};

struct DUCK_DEMO_ALIGN(16) ObjectBuf
{
    glm::mat4 uWorld;
    glm::vec4 uDiffuseAlbedo;
    glm::vec3 uFresnelR0;
    float uRoughness;
    uint uTextureIndex;
};
