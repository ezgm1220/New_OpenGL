#pragma once
//#include "GL.h"
#include <glad/glad.h>
//#include <GL/glew.h>
#include <glfw/glfw3.h>
#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<glm/glm.hpp>



class Shader
{
public:
    unsigned int ID;
    const char* vertexpath, * fragmentpath, * computepath;
    Shader() {}
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    Shader(const char* computePath);
    void use()const;
    void setBool(const std::string& name, bool value)const;
    void setInt(const std::string& name, int value)const;
    void setFloat(const std::string& name, float value)const;
    void setVec2(const std::string& name, const glm::vec2& value)const;
    void setVec3(const std::string& name, const glm::vec3& value)const;
    void setVec4(const std::string& name, const glm::vec4& value)const;
    void setMat2(const std::string& name, const glm::mat2& mat)const;
    void setMat3(const std::string& name, const glm::mat3& mat)const;
    void setMat4(const std::string& name, const glm::mat4& mat)const;
    void checkCompileErrors(unsigned int shader, std::string name);

};
