#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, set = 0, binding = 0) uniform FrameBuf
{
    mat4 uViewProj;
} Frame;

layout(std140, set = 0, binding = 1) uniform ObjectBuf
{
    mat4 uWorld;
} Object;

layout(location = 0) in vec3 aPosition;

void main()
{
    vec4 posW = vec4(aPosition, 1.0) * Object.uWorld;
    gl_Position = posW * Frame.uViewProj;
}
