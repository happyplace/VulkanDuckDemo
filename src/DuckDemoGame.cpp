#include "DuckDemoGame.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "imgui.h"

#include "meshloader/MeshLoader.h"

DuckDemoGame::~DuckDemoGame()
{
    m_renderObjects.clear();

    Free_MeshRenderPass(m_defaultMeshRenderPass);
    Free_MeshRenderPass(m_wireframeMeshRenderPass);
}

bool DuckDemoGame::OnInit()
{
    {
        MeshRenderPassParams params;
        params.m_maxRenderObjectCount = 2;

        params.m_wireframe = false;
        if (!Init_MeshRenderPass(m_defaultMeshRenderPass, params))
        {
            return false;
        }

        params.m_wireframe = true;
        if (!Init_MeshRenderPass(m_wireframeMeshRenderPass, params))
        {
            return false;
        }
    }

    {
        m_renderObjects.emplace_back();
        RenderObject& renderObject = m_renderObjects[m_renderObjects.size() - 1];
        renderObject.objectBufferIndex = static_cast<int32_t>(m_renderObjects.size() - 1);

        const glm::vec3 objectPosition = glm::vec3(0.0f, -2.0f, 0.0f);
        const glm::vec3 objectScale = glm::vec3(1.0f);
        const glm::quat objectRotation = glm::quat(glm::vec3(glm::radians(180.0f), glm::radians(180.0f), glm::radians(0.0f)));
        
        renderObject.objectBuf.uWorld = glm::translate(objectPosition) * glm::toMat4(objectRotation) * glm::scale(objectScale);
        renderObject.objectBuf.uWorld = glm::transpose(renderObject.objectBuf.uWorld);
        renderObject.objectBuf.uDiffuseAlbedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        renderObject.objectBuf.uFresnelR0 = glm::vec3(0.02f, 0.02f, 0.02f);
        renderObject.objectBuf.uRoughness = 0.2f;
        renderObject.objectBuf.uTextureIndex = static_cast<uint>(renderObject.objectBufferIndex);

        UpdateObjectBuffer(renderObject);
        UpdateObjectTexture(renderObject, "../kachujin_g_rosales/Kachujin_diffuse.png");
        UpdateModel(renderObject, "../kachujin_g_rosales/kachujin_g_rosales.fbx");
    }
    {
        m_renderObjects.emplace_back();
        RenderObject& renderObject = m_renderObjects[m_renderObjects.size() - 1];
        renderObject.objectBufferIndex = static_cast<int32_t>(m_renderObjects.size() - 1);

        const glm::vec3 objectPosition = glm::vec3(0.0f, -2.0f, 0.0f);
        const glm::vec3 objectScale = glm::vec3(1.0f);
        const glm::quat objectRotation = glm::quat(glm::vec3(glm::radians(180.0f), glm::radians(180.0f), glm::radians(0.0f)));
        
        renderObject.objectBuf.uWorld = glm::translate(objectPosition) * glm::toMat4(objectRotation) * glm::scale(objectScale);
        renderObject.objectBuf.uWorld = glm::transpose(renderObject.objectBuf.uWorld);
        renderObject.objectBuf.uDiffuseAlbedo = glm::vec4(0.930f, 0.530f, 0.823f, 1.0f);
        renderObject.objectBuf.uFresnelR0 = glm::vec3(0.02f, 0.02f, 0.02f);
        renderObject.objectBuf.uRoughness = 0.2f;
        renderObject.objectBuf.uTextureIndex = static_cast<uint>(renderObject.objectBufferIndex);

        UpdateObjectBuffer(renderObject);
        UpdateObjectTexture(renderObject, "../rogue_texture.png");
        UpdateCubePrimitive(renderObject, 500.0f, 5.0f, 500.0f);
    }

    m_cameraRotationX = 0.0f;
    m_cameraRotationY = 0.0f;
    m_cameraPosition = glm::vec3(0.0f, -80.0f, -150.0f);
    UpdateFrameBuffer();

    return true;
}

void DuckDemoGame::UpdateObjectBuffer(RenderObject& renderObject)
{
    FillVulkanBuffer(
        m_defaultMeshRenderPass.m_vulkanObjectBuffer, 
        &renderObject.objectBuf, 
        sizeof(renderObject.objectBuf), 
        CalculateUniformBufferSize(sizeof(renderObject.objectBuf)) * renderObject.objectBufferIndex);

    FillVulkanBuffer(
        m_wireframeMeshRenderPass.m_vulkanObjectBuffer, 
        &renderObject.objectBuf, 
        sizeof(renderObject.objectBuf), 
        CalculateUniformBufferSize(sizeof(renderObject.objectBuf)) * renderObject.objectBufferIndex);
}

void DuckDemoGame::UpdateObjectTexture(RenderObject& renderObject, const std::string& texturePath)
{
    renderObject.m_texture.reset(new VulkanTexture());
    VkResult result = CreateVulkanTexture(texturePath, *renderObject.m_texture.get());
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return;
    }

    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = nullptr;
    descriptorImageInfo.imageView = renderObject.m_texture->m_imageView;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet sampledImageWriteDescriptorSet;
    sampledImageWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sampledImageWriteDescriptorSet.pNext = nullptr;
    sampledImageWriteDescriptorSet.dstSet = m_defaultMeshRenderPass.m_vulkanDescriptorSets[3];
    sampledImageWriteDescriptorSet.dstBinding = 0;
    sampledImageWriteDescriptorSet.dstArrayElement = renderObject.objectBuf.uTextureIndex;
    sampledImageWriteDescriptorSet.descriptorCount = 1;
    sampledImageWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampledImageWriteDescriptorSet.pBufferInfo = nullptr;
    sampledImageWriteDescriptorSet.pImageInfo = &descriptorImageInfo;
    sampledImageWriteDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_vulkanDevice, 1, &sampledImageWriteDescriptorSet, 0, nullptr);

    sampledImageWriteDescriptorSet.dstSet = m_wireframeMeshRenderPass.m_vulkanDescriptorSets[3];

    vkUpdateDescriptorSets(m_vulkanDevice, 1, &sampledImageWriteDescriptorSet, 0, nullptr);
}

void DuckDemoGame::UpdateModel(RenderObject& renderObject, const std::string& modelPath)
{
    std::unique_ptr<DuckDemoFile> modelFile = DuckDemoUtils::LoadFileFromDisk(modelPath);
    if (modelFile == nullptr)
    {
        DUCK_DEMO_ASSERT(false);
        return;
    }

    MeshLoader::Mesh mesh;
    if (!MeshLoader::Loader::LoadModel(modelFile->buffer.get(), modelFile->bufferSize, mesh))
    {
        DUCK_DEMO_ASSERT(false);
        return;
    }

    renderObject.m_vertexBuffer.reset(new VulkanBuffer());
    renderObject.m_indexBuffer.reset(new VulkanBuffer());

    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::Vertex) * mesh.vertexCount), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, *renderObject.m_vertexBuffer.get()));
    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::IndexType) * mesh.indexCount), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, *renderObject.m_indexBuffer.get()));

    FillVulkanBuffer(*renderObject.m_vertexBuffer.get(), mesh.GetVertex(), sizeof(MeshLoader::Vertex) * mesh.vertexCount);
    FillVulkanBuffer(*renderObject.m_indexBuffer.get(), mesh.GetIndex(), sizeof(MeshLoader::IndexType) * mesh.indexCount);

    renderObject.m_indexCount = mesh.indexCount;
}

void DuckDemoGame::UpdateCubePrimitive(RenderObject& renderObject, const float width, const float height, const float depth)
{
    MeshLoader::Mesh mesh;
    if (!MeshLoader::Loader::LoadCubePrimitive(mesh, width, height, depth))
    {
        DUCK_DEMO_ASSERT(false);
        return;
    }

    renderObject.m_vertexBuffer.reset(new VulkanBuffer());
    renderObject.m_indexBuffer.reset(new VulkanBuffer());

    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::Vertex) * mesh.vertexCount), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, *renderObject.m_vertexBuffer.get()));
    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::IndexType) * mesh.indexCount), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, *renderObject.m_indexBuffer.get()));

    FillVulkanBuffer(*renderObject.m_vertexBuffer.get(), mesh.GetVertex(), sizeof(MeshLoader::Vertex) * mesh.vertexCount);
    FillVulkanBuffer(*renderObject.m_indexBuffer.get(), mesh.GetIndex(), sizeof(MeshLoader::IndexType) * mesh.indexCount);

    renderObject.m_indexCount = mesh.indexCount;
}

void DuckDemoGame::OnResize()
{
    Resize_MeshRenderPass(m_defaultMeshRenderPass);
    Resize_MeshRenderPass(m_wireframeMeshRenderPass);
}

CameraInput DuckDemoGame::GetCameraInput()
{
    constexpr int axisDeadZone = 8000;
    constexpr float maxAxisValue = 32768.0f - axisDeadZone;

    CameraInput cameraInput;

    const int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i)
    {
        SDL_GameController* gameController = SDL_GameControllerOpen(i);
        if (gameController == nullptr)
        {
            DUCK_DEMO_SHOW_ERROR("SDL_GameControllerOpen Error", SDL_GetError());
        }

        {
            const Sint16 xAxisRawValue = SDL_GameControllerGetAxis(gameController, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX);
            float xAxis = SDL_max(0, SDL_abs(xAxisRawValue) - axisDeadZone) / maxAxisValue;
            xAxis = SDL_min(1.0f, xAxis);
            xAxis = xAxisRawValue < 0 ? xAxis : -xAxis;

            const Sint16 yAxisRawValue = SDL_GameControllerGetAxis(gameController, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY);
            float yAxis = SDL_max(0, SDL_abs(yAxisRawValue) - axisDeadZone) / maxAxisValue;
            yAxis = SDL_min(1.0f, yAxis);
            yAxis = yAxisRawValue < 0 ? yAxis : -yAxis;

            glm::vec2 controllerAxis = glm::vec2(xAxis, yAxis);
            if (glm::epsilonNotEqual(0.0f, glm::length(controllerAxis), glm::epsilon<float>()))
            {
                controllerAxis = glm::normalize(controllerAxis);
            }

            cameraInput.xCameraAxis = controllerAxis.x;
            cameraInput.yCameraAxis = controllerAxis.y;
        }

        {
            const Sint16 yAxisRawValue = SDL_GameControllerGetAxis(gameController, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY);
            float yAxis = SDL_max(0, SDL_abs(yAxisRawValue) - axisDeadZone) / maxAxisValue;
            yAxis = SDL_min(1.0f, yAxis);
            yAxis = yAxisRawValue < 0 ? yAxis : -yAxis;

            const Sint16 xAxisRawValue = SDL_GameControllerGetAxis(gameController, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX);
            float xAxis = SDL_max(0, SDL_abs(xAxisRawValue) - axisDeadZone) / maxAxisValue;
            xAxis = SDL_min(1.0f, xAxis);
            xAxis = xAxisRawValue < 0 ? xAxis : -xAxis;

            glm::vec2 controllerAxis = glm::vec2(xAxis, yAxis);
            if (glm::epsilonNotEqual(0.0f, glm::length(controllerAxis), glm::epsilon<float>()))
            {
                controllerAxis = glm::normalize(controllerAxis);
            }

            cameraInput.yMoveAxis = controllerAxis.y;
            cameraInput.xMoveAxis = controllerAxis.x;
        }

        {
            cameraInput.cameraUp = SDL_GameControllerGetButton(gameController, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) == 1;
            cameraInput.cameraDown = SDL_GameControllerGetButton(gameController, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) == 1;
        }
    }

    {
        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
        if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP])
        {
            cameraInput.yMoveAxis += 1.0f;
        }
        if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN])
        {
            cameraInput.yMoveAxis -= 1.0f;
        }
        if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT])
        {
            cameraInput.xMoveAxis += 1.0f;
        }
        if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT])
        {
            cameraInput.xMoveAxis -= 1.0f;
        }

        cameraInput.cameraUp = keyboardState[SDL_SCANCODE_Q];
        cameraInput.cameraDown = keyboardState[SDL_SCANCODE_E];

        int mouseX;
        int mouseY;
        Uint32 mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

        if (mouseState & SDL_BUTTON_RMASK)
        {
            if (!SDL_GetWindowMouseGrab(Game::Get()->GetWindow()))
            {
                SDL_SetWindowMouseGrab(Game::Get()->GetWindow(), SDL_TRUE);
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }

            constexpr float mouseDivider = 5.0f;
            cameraInput.xCameraAxis = -mouseX / mouseDivider;
            cameraInput.yCameraAxis = -mouseY / mouseDivider;
        }
        else
        {
            if (SDL_GetWindowMouseGrab(Game::Get()->GetWindow()))
            {
                SDL_SetWindowMouseGrab(Game::Get()->GetWindow(), SDL_FALSE);
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
        }
    }

    return cameraInput;
}

void DuckDemoGame::OnUpdate(const GameTimer& gameTimer)
{
    CameraInput cameraInput = GetCameraInput();

    constexpr float cameraMoveSpeed = 100.0f;
    constexpr float cameraTurnSpeed = 120.0f;
    constexpr float cameraRaiseSpeed = 150.0f;

    {
        m_cameraRotationX += cameraInput.xCameraAxis * cameraTurnSpeed * static_cast<float>(gameTimer.DeltaTime());
        m_cameraRotationY += cameraInput.yCameraAxis * cameraTurnSpeed * static_cast<float>(gameTimer.DeltaTime());

        m_cameraRotationX = DuckDemoUtils::WrapAngle<float>(m_cameraRotationX);
        m_cameraRotationY = DuckDemoUtils::WrapAngle<float>(m_cameraRotationY);

        m_cameraRotation = glm::quat(glm::vec3(glm::radians(m_cameraRotationY), glm::radians(m_cameraRotationX), 0.0f));
    }

    {
        const glm::vec3 cameraForward = m_cameraRotation * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        const glm::vec3 cameraRight = m_cameraRotation * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

        m_cameraPosition += cameraForward * cameraMoveSpeed * static_cast<float>(gameTimer.DeltaTime()) * cameraInput.yMoveAxis;
        m_cameraPosition += cameraRight * cameraMoveSpeed * static_cast<float>(gameTimer.DeltaTime()) * cameraInput.xMoveAxis;
    }

    {
        int yAxis = 0.0f;
        if (cameraInput.cameraUp)
        {
            yAxis -= 1;
        }
        if (cameraInput.cameraDown)
        {
            yAxis += 1;
        }

        const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        m_cameraPosition += worldUp * cameraRaiseSpeed * static_cast<float>(gameTimer.DeltaTime()) * static_cast<float>(yAxis);
    }

    UpdateFrameBuffer();
}

void DuckDemoGame::UpdateFrameBuffer()
{
    const glm::vec3 cameraForward = m_cameraRotation * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    const glm::vec3 lookAtUp = m_cameraRotation * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec3 lookAtTarget = m_cameraPosition + cameraForward;
    const glm::mat4x4 view = glm::lookAt(m_cameraPosition, lookAtTarget, lookAtUp);

    const glm::mat4x4 proj = glm::perspectiveFov(
        glm::radians(90.0f), 
        static_cast<float>(m_vulkanSwapchainWidth), 
        static_cast<float>(m_vulkanSwapchainHeight), 
        1.0f, 
        1000.0f);

    FrameBuf frameBuf;
    frameBuf.uViewProj = glm::transpose(proj * view);
    frameBuf.uEyePosW = m_cameraPosition;
    
    frameBuf.uAmbientLight = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);

    frameBuf.uDirLight.uDirection = glm::vec3(0.57735f, -0.57735f, 0.57735f);
    frameBuf.uDirLight.uStrength = glm::vec3(0.6f, 0.6f, 0.6f);

    frameBuf.uSpotLight.uDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    frameBuf.uSpotLight.uStrength = glm::vec3(0.6f, 0.6f, 0.6f);
    frameBuf.uSpotLight.uPosition = glm::vec3(0.0f, 10.0f, 0.0f);
    frameBuf.uSpotLight.uFalloffStart = 200.0f;
    frameBuf.uSpotLight.uFalloffEnd = 300.0f;
    frameBuf.uSpotLight.uSpotPower = 80.0f;

    frameBuf.uPointLights[0].uStrength = glm::vec3(1.0f, 0.0f, 0.0f);
    frameBuf.uPointLights[0].uPosition = glm::vec3(25.0f, -180.0f, -25.0f);
    frameBuf.uPointLights[0].uFalloffStart = 30.0f;
    frameBuf.uPointLights[0].uFalloffEnd = 50.0f;

    frameBuf.uPointLights[1].uStrength = glm::vec3(0.0f, 1.0f, 0.0f);
    frameBuf.uPointLights[1].uPosition = glm::vec3(-25.0f, -100.0f, -25.0f);
    frameBuf.uPointLights[1].uFalloffStart = 30.0f;
    frameBuf.uPointLights[1].uFalloffEnd = 50.0f;

    FillVulkanBuffer(m_defaultMeshRenderPass.m_vulkanFrameBuffer, &frameBuf, sizeof(frameBuf));
    FillVulkanBuffer(m_wireframeMeshRenderPass.m_vulkanFrameBuffer, &frameBuf, sizeof(frameBuf));
}

void DuckDemoGame::OnRender()
{
    Render_MeshRenderPass(m_wireframe ? m_wireframeMeshRenderPass : m_defaultMeshRenderPass, m_vulkanPrimaryCommandBuffer, m_renderObjects);

    BeginRender_ImGuiRenderPass(m_imGuiRenderPass);
    OnImGui();
    EndRender_ImGuiRenderPass(m_imGuiRenderPass, m_vulkanPrimaryCommandBuffer);
}

void DuckDemoGame::OnImGui()
{
    ImGui::Begin("Options");
    if (ImGui::RadioButton("Wireframe", m_wireframe))
    {
        m_wireframe = !m_wireframe;
    }
    ImGui::End();
}
