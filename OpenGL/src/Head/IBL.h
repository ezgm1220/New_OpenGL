#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <vector>
#include "Shader.h"


using namespace std;




class IBL {
public:

    GLuint hdrTexture,envcubeMap, irradianceMap, prefilterMap, LUTTexture;
    GLuint mFBO, mRBO;

    IBL();
    void Load(string file, string type, bool cube = false);

    void Get_irradiance();
    void Get_prefilter();
    void Get_LUT();

    void PrepareIBL() {
        Get_irradiance();
        Get_prefilter();
        Get_LUT();
    }

};

