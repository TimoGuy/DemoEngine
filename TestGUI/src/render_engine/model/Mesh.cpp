#include "Mesh.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../../mainloop/MainLoop.h"
#include "../render_manager/RenderManager.h"
#include "../material/Texture.h"
#include "../material/Shader.h"


Mesh::Mesh(glm::vec3 centerOfGravity, std::vector<Vertex> vertices, std::vector<unsigned int> indices, const RenderAABB& bounds, const std::string& materialName)
{
    Mesh::centerOfGravity = centerOfGravity;
    Mesh::vertices = vertices;
    Mesh::indices = indices;
    Mesh::bounds = bounds;
    Mesh::materialName = materialName;
    Mesh::depthPriority = 0.0f;

    setupMesh();
}

Mesh::~Mesh()
{
    // @TODO: create a destructor ya dungus!
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    // bone ids
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, MAX_BONE_INFLUENCE, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
    // bone weights
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

    glBindVertexArray(0);
}

void Mesh::render(const glm::mat4& modelMatrix, Shader* shaderOverride, const std::vector<glm::mat4>* boneTransforms, RenderStage renderStage)
{
    bool needMainTexture = false;

    // Apply stage's needs
    if (renderStage == RenderStage::OVERRIDE)
    {
        needMainTexture = true;
    }
    else if (renderStage == RenderStage::Z_PASS)
    {
        needMainTexture = true;

        if (material->isTransparent)
        {
            MainLoop::getInstance().renderManager->INTERNALaddMeshToTransparentRenderQueue(this, modelMatrix, boneTransforms);
            return;  // Bail early; transparents aren't included in Z-pass
        }
        else
        {
            MainLoop::getInstance().renderManager->INTERNALaddMeshToOpaqueRenderQueue(this, modelMatrix, boneTransforms);
        }
    }
    else if (renderStage == RenderStage::OPAQUE_RENDER_QUEUE || renderStage == RenderStage::TRANSPARENT_RENDER_QUEUE)
    {
        if (material != nullptr)
        {
            material->applyTextureUniforms(materialInjections);
            shaderOverride = material->getShader();
        }
    }

    // Apply ditherAlpha and main texture to pass if overriding
    if (needMainTexture && material != nullptr)
    {
        if (renderStage == RenderStage::Z_PASS)
        {
            shaderOverride->setFloat("ditherAlpha", material->ditherAlpha);

            glm::vec3 x, y, z;
            z = -glm::normalize(MainLoop::getInstance().camera.orientation);
            x = glm::normalize(glm::vec3(z.z, 0.0f, -z.x));
            y = glm::cross(z, x);
            glm::mat3 normalToViewSpace(x, y, z);
            shaderOverride->setMat3("normalToViewSpace", normalToViewSpace);
        }

        Texture* mainTexture = material->getMainTexture();
        if (mainTexture != nullptr)
        {
            shaderOverride->setSampler("ubauTexture", mainTexture->getHandle());
            shaderOverride->setFloat("fadeAlpha", material->fadeAlpha);
        }
    }

    // Apply the model matrix
    shaderOverride->setMat4("modelMatrix", modelMatrix);
    if (renderStage != RenderStage::OVERRIDE)           // @TODO: when abstracting the shaders, do this kinda logic in the shader instead!
        shaderOverride->setMat3("normalsModelMatrix", glm::mat3(glm::transpose(glm::inverse(modelMatrix))));

    // Apply bone transformations
    if (boneTransforms != nullptr)
        MainLoop::getInstance().renderManager->INTERNALupdateSkeletalBonesUBO(boneTransforms);

    // Draw the mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);

    //
    // Do backside render?
    //
    if (material != nullptr && material->doFrontRenderThenBackRender)
    {
        material->applyTextureUniformsBackRender(materialInjections);

        // Draw the mesh again
        glCullFace(GL_FRONT);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
        glCullFace(GL_BACK);
    }
}

void Mesh::pickFromMaterialList(std::map<std::string, Material*> materialMap)
{
    if (materialName.empty())
        return;

    if (materialMap.find(materialName) != materialMap.end())
    {
        material = materialMap[materialName];
    }
    else
        std::cout << "ERROR: material assignment: Material \"" << materialName << "\" not found!" << std::endl;
}
