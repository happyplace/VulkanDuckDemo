#pragma once

#include <vector>

#include "Game.h"
#include "DuckDemoUtils.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "meshloader/Mesh.h"
#include "MeshRenderPass.h"
#include "DuckDemoGameDefines.h"
#include "RenderObject.h"

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
    virtual bool OnInit() override;
    virtual void OnResize() override;
    virtual void OnUpdate(const GameTimer& gameTimer) override;
    virtual void OnRender() override;
    
    void OnImGui();

    void UpdateFrameBuffer();
    void UpdateObjectBuffer(RenderObject& renderObject);
    void UpdateObjectTexture(RenderObject& renderObject, const std::string& texturePath);
    void UpdateModel(RenderObject& renderObject, const std::string& modelPath);
    void UpdateCubePrimitive(RenderObject& renderObject, const float width, const float height, const float depth);

    CameraInput GetCameraInput();

    std::vector<RenderObject> m_renderObjects;
    
    MeshRenderPass m_defaultMeshRenderPass;
    MeshRenderPass m_wireframeMeshRenderPass;

    glm::quat m_cameraRotation;
    glm::vec3 m_cameraPosition;
    float m_cameraRotationX;
    float m_cameraRotationY;

    bool m_wireframe = false;
};
