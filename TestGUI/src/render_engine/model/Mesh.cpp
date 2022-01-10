#include "Mesh.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../../mainloop/MainLoop.h"
#include "../render_manager/RenderManager.h"
#include "../material/Texture.h"


Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, const RenderAABB& bounds, const std::string& materialName)
{
    Mesh::vertices = vertices;
    Mesh::indices = indices;
    Mesh::bounds = bounds;
    Mesh::materialName = materialName;

    setupMesh();
}

Mesh::~Mesh()
{
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

void Mesh::render(const glm::mat4& modelMatrix, GLuint shaderIdOverride, const std::vector<glm::mat4>* boneTransforms, RenderStage renderStage)
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
            material->applyTextureUniforms();
            shaderIdOverride = material->getShaderId();
        }
    }

    // Apply ditherAlpha and main texture to pass if overriding
    if (needMainTexture && material != nullptr)
    {
        if (renderStage == RenderStage::Z_PASS)
            glUniform1f(glGetUniformLocation(shaderIdOverride, "ditherAlpha"), material->ditherAlpha);
        else
            glUniform1f(glGetUniformLocation(shaderIdOverride, "ditherAlpha"), 1.0f);

        Texture* mainTexture = material->getMainTexture();
        if (mainTexture != nullptr)
        {
            glBindTextureUnit(0, mainTexture->getHandle());
            glUniform1i(glGetUniformLocation(shaderIdOverride, "ubauTexture"), 0);      // @Hardcode: when abstracted shaders come, pls make sure this is gone!
        }
    }

    // Apply the model matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderIdOverride, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (renderStage != RenderStage::OVERRIDE)           // @TODO: when abstracting the shaders, do this kinda logic in the shader instead!
        glUniformMatrix3fv(glGetUniformLocation(shaderIdOverride, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix)))));

    // Apply bone transformations
    if (boneTransforms != nullptr)
        MainLoop::getInstance().renderManager->INTERNALupdateSkeletalBonesUBO(boneTransforms);

    // Draw the mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
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
