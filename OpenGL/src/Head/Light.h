#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Camera.h"
#include "ltc_matrix.hpp"
#include "Shader.h"
#include "Shadow.h"

void renderSphere();
void renderAreaLight();
extern Camera camera;
extern enum LIGHT_TYPE;
extern Shadow shadow;
struct Light {
	LIGHT_TYPE type;
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Direction;
    

    float Rotation;
    float Intensity;
    float Radius;
    float CutOff;
    float outerCutOff;
    bool Twoface;
};
class Lights {
public:

    std::vector<Light> lights;
    Shader shader;
    GLuint LTC_1, LTC_2;
    bool Have_AreaLight = false;
    const float constant = 1.0f;
    const float linear = 0.7f;
    const float quadratic = 1.8f;

    Lights();
    //void add(Light light);
    void add_Point(glm::vec3 Pos, glm::vec3 Color);
    void add_Direction(glm::vec3 Dir, glm::vec3 Color);
    void add_Direction(float theta,float varphi, glm::vec3 Color);
    void add_Spot(glm::vec3 Color, float Cut, float OutCut);
    void add_Area(glm::vec3 Pos, glm::vec3 Dir, glm::vec3 Color, float Rot, float Int, bool twoface = true);
    
    int lights_size();
    void Set_Light(Shader &shader,int id = 0);
    void Show_Light();

    void ConfigureLTC();
};

