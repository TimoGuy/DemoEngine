#include "Mesh.h"

#include <glad/glad.h>
#include "../../MainLoop/MainLoop.h"
#include "../RenderEngine.manager/RenderManager.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, int materialIndex)
{
    Mesh::vertices = vertices;
    Mesh::indices = indices;
    Mesh::materialIndex = materialIndex;

    setupMesh();
}

void Mesh::render(unsigned int shaderId)
{
    if (material != nullptr)
        material->applyTextureUniforms(
            shaderId,
            MainLoop::getInstance().renderManager->getIrradianceMap(),
            MainLoop::getInstance().renderManager->getPrefilterMap(),
            MainLoop::getInstance().renderManager->getBRDFLUTTexture()
        );

    //
    // Draw the mesh
    //
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
}

void Mesh::pickFromMaterialList(std::vector<Material*> materialList)
{
    // TODO: make this spit out an error instead
    material = materialList[materialIndex];
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
