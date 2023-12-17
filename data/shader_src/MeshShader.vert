#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, set = 0, binding = 0) uniform FrameBuf
{
    mat4 uViewProj;
    vec3 uEyePosW;
} Frame;

layout(std140, set = 0, binding = 1) uniform ObjectBuf
{
    mat4 uWorld;
    vec4 uDiffuseAlbedo;
    vec3 uFresnelR0;
    float uRoughness;
} Object;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

layout(location = 0) out vec3 vNormalW;
layout(location = 1) out vec3 vPositionW;

void main()
{
    vec4 posW = vec4(aPosition, 1.0f) * Object.uWorld;
    gl_Position = posW * Frame.uViewProj;

    vNormalW = aNormal * mat3(Object.uWorld);
}
