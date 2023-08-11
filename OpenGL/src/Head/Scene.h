#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <random>
#include "Material.h"
#include "Shader.h"
#include "map"

void renderPlane();
void renderSphere();
void renderCube();
class Scene {
public:

	std::map<std::string, Material>material;

	Scene() {};
	void add_Material(std::string name);
	void add_Material(std::string name, glm::vec3 albedo, float metallic, float roughness);
	
	void Sphere_1(Shader& shader, bool shadow = false);
	void Plan_1(Shader& shader, bool shadow = false);
	void areatest(Shader& shader, bool shadow = false);
	void shadow_1(Shader& shader, bool shadow = false);
	void bigshadow(Shader& shader, bool shadow = false);
	void Test_Light_Shadow(Shader& shader, bool shadow = false);
	void Test_Shadow(Shader& shader, bool shadow = false);
};