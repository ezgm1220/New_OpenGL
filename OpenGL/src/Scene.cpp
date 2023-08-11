
#include "Scene.h"

void Scene::add_Material(std::string name) {
    material[name] = Material(name);
}
void Scene::add_Material(std::string name, glm::vec3 albedo, float metallic, float roughness) {
    material[name] = Material(albedo, metallic, roughness);
}

void Scene::Sphere_1(Shader& shader, bool shadow) {
    shader.setBool("haveTexture", true);
    int sta = 0;
    glm::mat4 model;
    for (float x = -6; x < 6.5;) {
        for (float y = -6; y < 6.5;) {
            model = model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, y, -2));
            y += 3;

            /*switch (sta) {
            case 0:grass.Set_Texture(0); break;
            case 1:gold.Set_Texture(0); break;
            case 2:wall.Set_Texture(0); break;
            case 3:plastic.Set_Texture(0); break;
            case 4:iron.Set_Texture(0); break;
            }*/

            if (x > 0) {
                material["wall"].Set_Texture(0);
            }
            else {
                material["gold"].Set_Texture(0);
            }
            shader.setMat4("model", model);
            shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            renderSphere();
        }
        x += 3;
        sta++;
    }
}

void Scene::Plan_1(Shader& shader, bool shadow) {
    shader.setBool("haveTexture", true);
    glm::mat4 model = glm::mat4(1.0);
    material["gold"].Set_Texture(0);
    shader.setMat4("model", model);
    shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    renderPlane();
}

void Scene::areatest(Shader& shader, bool shadow) {
    //std::cout << "½øÈëÁËareatest\n";
    glm::mat4 model = glm::mat4(1.0);
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", true);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["gold"].Set_Texture(0);
    }
    renderPlane();
    
    model = glm::mat4(1.0);
    model = glm::translate(model, glm::vec3(3, 3, 3));
    model = glm::scale(model, glm::vec3(0.5));
    shader.setMat4("model", model);
    Material metallicSphere(glm::vec3(0.5, 0.5, 0.5), 0.95, 0.1);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        metallicSphere.Set_Material(shader);
    }
    renderSphere();

    metallicSphere.roughness = 0.02;
    model = glm::mat4(1.0);
    model = glm::translate(model, glm::vec3(-4, 3, -4));
    model = model = glm::scale(model, glm::vec3(3));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        metallicSphere.Set_Material(shader);
    }
    renderSphere();


}

void Scene::shadow_1(Shader& shader, bool shadow) {

    glm::mat4 model = glm::mat4(1.0);
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderPlane();

    // cube1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderCube();
    // cube2
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.01f, 1.0f, 0.01f));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderCube();
    // cube3
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.25));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderCube();
}

void Scene::bigshadow(Shader& shader, bool shadow) {
    glm::mat4 model = glm::mat4(1.0);
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderPlane();
    static std::vector<glm::mat4> modelMatrices;
    if (modelMatrices.size() == 0)
    {
        std::random_device device;
        std::mt19937 generator = std::mt19937(device());
        for (int i = 0; i < 10; ++i)
        {
            static std::uniform_real_distribution<float> offsetDistribution = std::uniform_real_distribution<float>(-10, 10);
            static std::uniform_real_distribution<float> scaleDistribution = std::uniform_real_distribution<float>(1.0, 2.0);
            static std::uniform_real_distribution<float> rotationDistribution = std::uniform_real_distribution<float>(0, 180);

            auto model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(offsetDistribution(generator), offsetDistribution(generator) + 10.0f, offsetDistribution(generator)));
            model = glm::rotate(model, glm::radians(rotationDistribution(generator)), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            model = glm::scale(model, glm::vec3(scaleDistribution(generator)));
            modelMatrices.push_back(model);
        }
    }
    for (const auto& model : modelMatrices)
    {
        shader.setMat4("model", model);
        if (!shadow) {
            shader.setBool("haveTexture", true);
            shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            material["gold"].Set_Texture(0);
        }
        renderCube();
    }
}

void Scene::Test_Light_Shadow(Shader& shader, bool shadow) {
    glm::mat4 model = glm::mat4(1.0);
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderPlane();

    model = glm::translate(model, glm::vec3(8, 2, 8));
    model = glm::scale(model, glm::vec3(3));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", true);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["gold"].Set_Texture(0);
    }
    renderSphere();

    model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(-2, -2, -2));
    model = glm::scale(model, glm::vec3(2));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", true);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["gold"].Set_Texture(0);
    }
    renderSphere();

}

void Scene::Test_Shadow(Shader& shader, bool shadow) {

    glm::mat4 model = glm::mat4(1.0);
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", false);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["suliao"].Set_Material(shader);
    }
    renderPlane();

    model = glm::translate(model, glm::vec3(1, -2, 1));
    model = glm::scale(model, glm::vec3(0.2,5,0.2));
    shader.setMat4("model", model);
    if (!shadow) {
        shader.setBool("haveTexture", true);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        material["gold"].Set_Texture(0);
    }
    renderCube();

}