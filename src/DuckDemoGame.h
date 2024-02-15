#pragma once

#include <array>
#include <vector>

#include "Game.h"
#include "DuckDemoUtils.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "meshloader/Mesh.h"
#include "MeshRenderPass.h"
#include "DuckDemoGameDefines.h"
#include "RenderObject.h"
#include "WaterRenderPass.h"
#include "WaterComputePass.h"

struct CameraInput
{
    float xCameraAxis = 0.0f;
    float yCameraAxis = 0.0f;
    float xMoveAxis = 0.0f;
    float yMoveAxis = 0.0f;
    bool cameraUp = false;
    bool cameraDown = false;
};

class DuckDemoGame : public Game
{
public:
    virtual ~DuckDemoGame() override;

private:
    enum RenderPassType
    {
        RenderPassType_Default = 0,
        RenderPassType_Wireframe,
        RenderPassType_COUNT,
    };

    virtual bool OnInit() override;
    virtual void OnResize() override;
    virtual void OnUpdate(const GameTimer& gameTimer) override;
    virtual void OnRender() override;
    
    void OnImGui();

    void UpdateFrameBuffer();
    void UpdateObjectBuffer(RenderObject& renderObject, const bool waterPass = false);
    void UpdateObjectTexture(RenderObject& renderObject, const std::string& texturePath, const bool waterPass = false);
    void UpdateModel(RenderObject& renderObject, const std::string& modelPath);
    void UpdateWaterPrimitive(RenderObject& renderObject, const float width, const float depth, const uint32_t gridX, const uint32_t gridY);

    CameraInput GetCameraInput();

    RenderObject m_duckRenderObject;
    RenderObject m_waterRenderObject;
    
    std::array<MeshRenderPass, RenderPassType_COUNT> m_meshRenderPasses;
    std::array<WaterRenderPass, RenderPassType_COUNT> m_waterRenderPasses;
    WaterComputePass m_waterComputePass;

    glm::quat m_cameraRotation;
    glm::vec3 m_cameraPosition;
    float m_cameraRotationX;
    float m_cameraRotationY;

    bool m_wireframe = false;
    float m_cameraMoveSpeed = 100.0f;
};
