#include "Light.h"


enum LIGHT_TYPE { POINT, DIRECTION, SPOT, AREA };

std::vector<glm::vec3> areaPosition = {
    glm::vec3(-1.0f,  1.0f, 0.0f),
    glm::vec3(1.0f,  1.0f, 0.0f),
    glm::vec3(1.0f, -1.0f, 0.0f),
    glm::vec3(-1.0f, -1.0f, 0.0f)
};

void Lights::ConfigureLTC() {
    // °ó¶¨LTC_1
    glGenTextures(1, &LTC_1);
    glBindTexture(GL_TEXTURE_2D, LTC_1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, LTC1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    // °ó¶¨LTC_2
    glGenTextures(1, &LTC_2);
    glBindTexture(GL_TEXTURE_2D, LTC_2);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, LTC2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    Have_AreaLight = true;
}

Lights::Lights() {
    shader = Shader("./Shaders/Base/light.vs", "./Shaders/Base/light.fs");
}

void Lights::add_Point(glm::vec3 Pos, glm::vec3 Color) {
    Light light;
    light.type = POINT;
    light.Position = Pos;
    light.Color = Color;
    const float maxBrightness = std::fmaxf(std::fmaxf(light.Color.r, light.Color.g), light.Color.b);
    float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
    light.Radius = radius;
    lights.push_back(light);
}

void Lights::add_Direction(glm::vec3 Dir, glm::vec3 Color) {
    Light light;
    light.type = DIRECTION;
    light.Direction = Dir;
    light.Color = Color;
    lights.push_back(light);
}

void Lights::add_Direction(float theta, float varphi, glm::vec3 Color) {
    glm::vec3 Dir;

    float sin_t, cos_t, sin_p, cos_p;
    sin_t = glm::sin(glm::radians(theta));
    sin_p = glm::sin(glm::radians(varphi));
    cos_t = glm::cos(glm::radians(theta));
    cos_p = glm::cos(glm::radians(varphi));

    float x, y, z;
    x = sin_t * sin_p;
    y = cos_t;
    z = sin_t * cos_p;

    Dir = glm::vec3(-x, -y, -z);

    Light light;
    light.type = DIRECTION;
    light.Direction = Dir;
    light.Color = Color;
    lights.push_back(light);
}

void Lights::add_Spot(glm::vec3 Color, float Cut, float OutCut) {
    Light light;
    light.type = SPOT;
    light.Position = camera.Position;
    light.Direction = camera.Front;
    light.CutOff = glm::cos(glm::radians(Cut));
    light.outerCutOff = glm::cos(glm::radians(OutCut));
    light.Color = Color;
    const float maxBrightness = std::fmaxf(std::fmaxf(light.Color.r, light.Color.g), light.Color.b);
    float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
    light.Radius = radius;
    lights.push_back(light);
}

void Lights::add_Area(glm::vec3 Pos, glm::vec3 Dir, glm::vec3 Color, float Rot, float Int, bool twoface ) {
    Light light;
    light.type = AREA;
    light.Color = Color;
    light.Position = Pos;
    light.Direction = Dir;
    light.Rotation = Rot;
    light.Intensity = Int;
    light.Twoface = twoface;
    lights.push_back(light);
    if (!Have_AreaLight) {
        ConfigureLTC();
    }
}

int Lights::lights_size() {
    return lights.size();
}

void Lights::Set_Light(Shader& shader, int id) {

    for (int i = 0; i < lights.size(); i++) {
        shader.use();
        switch (lights[i].type) {
        case POINT: {
            shader.setInt("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
            shader.setVec3("lights[" + std::to_string(i) + "].Position", lights[i].Position);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].Color);
            shader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
            shader.setFloat("lights[" + std::to_string(i) + "].Radius", lights[i].Radius);
            break;
        }
        case DIRECTION: {
            shader.setInt("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
            shader.setVec3("lights[" + std::to_string(i) + "].Direction", lights[i].Direction);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].Color);
            break;
        }
        case SPOT: {
            shader.setInt("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
            shader.setVec3("lights[" + std::to_string(i) + "].Position", camera.Position);
            shader.setVec3("lights[" + std::to_string(i) + "].Direction", camera.Front);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].Color);
            shader.setFloat("lights[" + std::to_string(i) + "].CutOff", lights[i].CutOff);
            shader.setFloat("lights[" + std::to_string(i) + "].outerCutOff", lights[i].outerCutOff);
            shader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
            shader.setFloat("lights[" + std::to_string(i) + "].Radius", lights[i].Radius);
            break;
        }
        case AREA: {
            glm::mat4 model = glm::mat4(1.0);
            model = glm::translate(model, lights[i].Position);
            model = glm::rotate(model, lights[i].Rotation, lights[i].Direction);

            shader.setInt("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
            shader.setFloat("lights[" + std::to_string(i) + "].Intensity", lights[i].Intensity);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].Color);
            shader.setBool("lights[" + std::to_string(i) + "].twoface", lights[i].Twoface);
            shader.setVec3("lights[" + std::to_string(i) + "].points[0]", glm::vec3(model * glm::vec4(areaPosition[0], 1.0)));
            shader.setVec3("lights[" + std::to_string(i) + "].points[1]", glm::vec3(model * glm::vec4(areaPosition[1], 1.0)));
            shader.setVec3("lights[" + std::to_string(i) + "].points[2]", glm::vec3(model * glm::vec4(areaPosition[2], 1.0)));
            shader.setVec3("lights[" + std::to_string(i) + "].points[3]", glm::vec3(model * glm::vec4(areaPosition[3], 1.0)));
            break;
        }
        }
    }
    if (Have_AreaLight) {
        glActiveTexture(GL_TEXTURE0 + id);
        glBindTexture(GL_TEXTURE_2D, LTC_1);
        glActiveTexture(GL_TEXTURE0 + id + 1);
        glBindTexture(GL_TEXTURE_2D, LTC_2);
    }
    shader.setInt("lights_size", lights_size());
    //std::cout << lights_size();
}

void Lights::Show_Light() {
    glm::mat4 model;
    for (int i = 0; i < lights.size(); i++) {
        if (lights[i].type == DIRECTION || lights[i].type == SPOT)continue;
        else if (lights[i].type == POINT) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, lights[i].Position);
            model = glm::scale(model, glm::vec3(0.25f));
            shader.setMat4("model", model);
            shader.setVec3("color", lights[i].Color);
            renderSphere();
        }
        else {
            model = glm::mat4(1.0);
            model = glm::translate(model, lights[i].Position);
            model = glm::rotate(model, lights[i].Rotation, lights[i].Direction);
            shader.setMat4("model", model);
            shader.setVec3("color", lights[i].Color);
            renderAreaLight();
        }
    }
}