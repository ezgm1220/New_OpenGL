#pragma once

//#include <string>
#include "Shader.h"

class Save_IBL {
public:
    Shader shader;
    Save_IBL();
    void save_cubemap(GLuint map_id, int map_size_x, int map_size_y, std::string storepath);
    void save_prefilter(GLuint map_id, int map_size, std::string storepath);
};

