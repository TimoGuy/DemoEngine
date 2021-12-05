#include "Mesh.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../../mainloop/MainLoop.h"
#include "../render_manager/RenderManager.h"


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

void Mesh::render(const glm::mat4& modelMatrix, GLuint shaderIdOverride, bool isTransparentQueue)
{
    bool shaderOverriden = (shaderIdOverride != 0);

    // Apply material
    if (!shaderOverriden)
    {
        if (material != nullptr)
        {
            // Ignore this if statement if it's actually the transparent renderqueue
            if (!isTransparentQueue && material->isTransparent)
            {
                // Bail early so that rendermanager can take care of transparents l8r
                MainLoop::getInstance().renderManager->INTERNALaddTransparentMeshToDeferRender(this, modelMatrix);
                return;
            }
            material->applyTextureUniforms();

            // Apply shaderId
            shaderIdOverride = material->getShaderId();
        }
    }

    // Apply the model matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderIdOverride, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (!shaderOverriden)
        glUniformMatrix3fv(glGetUniformLocation(shaderIdOverride, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix)))));

    //
    // Draw the mesh
    //
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