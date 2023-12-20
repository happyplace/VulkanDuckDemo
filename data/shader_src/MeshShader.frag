#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

struct DirectionalLightBuf
{
    vec3 uStrength;
    float padding0;
    vec3 uDirection;
};

layout(std140, set = 0, binding = 0) uniform FrameBuf
{
    mat4 uViewProj;
    vec3 uEyePosW;
    float padding0;
    vec4 uAmbientLight;
    DirectionalLightBuf uDirLight;
} Frame;

layout(std140, set = 1, binding = 0) uniform ObjectBuf
{
    mat4 uWorld;
    vec4 uDiffuseAlbedo;
    vec3 uFresnelR0;
    float uRoughness;
} Object;

layout(location = 0) in vec3 vNormalW;
layout(location = 1) in vec3 vPositionW;

layout(location = 0) out vec4 fFragColor;

float saturate(float value)
{
    return clamp(value, 0.0, 1.0); 
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
vec3 SchlickFresnel(vec3 R0, vec3 normal, vec3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    vec3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

vec3 BlinnPhong(vec3 lightStrength, vec3 lightVec, vec3 normal, vec3 toEye)
{
    float shininess = 1.0f - Object.uRoughness;
    float m = shininess * 256.0f;
    vec3 halfVec = normalize(toEye + lightVec);

    float roughtnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    vec3 fresnelFactor = SchlickFresnel(Object.uFresnelR0, halfVec, lightVec);

    vec3 specAlbedo = fresnelFactor * roughtnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (Object.uDiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

vec3 ComputeDirectionalLight(DirectionalLightBuf light, vec3 normal, vec3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    vec3 lightVec = -light.uDirection;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    vec3 lightStrength = light.uStrength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye);
}

void main()
{
#ifdef IS_WIREFRAME
    fFragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
#else
    vec3 normalizedNormalW = normalize(vNormalW);
    vec3 toEyeW = normalize(Frame.uEyePosW - vPositionW);

    vec4 ambient = Frame.uAmbientLight * Object.uDiffuseAlbedo;

    const float shadowFactor = 1.0f;

    vec3 lightResult = vec3(0.0f, 0.0f, 0.0f);
    lightResult += shadowFactor * ComputeDirectionalLight(Frame.uDirLight, normalizedNormalW, toEyeW);

    fFragColor = ambient + vec4(lightResult.x, lightResult.y, lightResult.z, 0.0f);
    
    // Common convention to take alpha from diffuse material.
    fFragColor.a = Object.uDiffuseAlbedo.a;
#endif // IS_WIREFRAME
}
