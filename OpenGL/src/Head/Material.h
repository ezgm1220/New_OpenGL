#pragma once

#include <string>


#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"


class Material {

public:
    std::string type;
    GLuint albedo, normal, metallic, roughness, ao;
    glm::vec3 _albedo;
    float _roughness, _metallic;
    Material() {}
    Material(std::string name);
    Material(glm::vec3 albedo, float metallic, float roughness);
    void Set_Texture(int i);
    void Set_Material(Shader& shader);
};

