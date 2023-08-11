#include "Material.h"


//#include "stb_image.h"


GLuint loadTexture(std::string path);
Material::Material(std::string name) {
    type = name;
    std::string file = "pbr/" + name + "/";
    albedo = loadTexture(file + "albedo.png");
    normal = loadTexture(file + "normal.png");
    metallic = loadTexture(file + "metallic.png");
    roughness = loadTexture(file + "roughness.png");
    ao = loadTexture(file + "ao.png");
}

Material::Material(glm::vec3 albedo, float metallic, float roughness) {
    _metallic = metallic;
    _roughness = roughness;
    _albedo = albedo;
}

void Material::Set_Texture(int i) {

    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, albedo);
    glActiveTexture(GL_TEXTURE0 + i + 1);
    glBindTexture(GL_TEXTURE_2D, normal);
    glActiveTexture(GL_TEXTURE0 + i + 2);
    glBindTexture(GL_TEXTURE_2D, metallic);
    glActiveTexture(GL_TEXTURE0 + i + 3);
    glBindTexture(GL_TEXTURE_2D, roughness);
    glActiveTexture(GL_TEXTURE0 + i + 4);
    glBindTexture(GL_TEXTURE_2D, ao);

}

void Material::Set_Material(Shader& shader) {

    shader.setVec3("_Albedo", _albedo);
    shader.setFloat("_Metallic", _metallic);
    shader.setFloat("_Roughness", _roughness);
    shader.setFloat("_Ao", 1.0);
}