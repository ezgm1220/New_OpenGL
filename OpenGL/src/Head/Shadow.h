#pragma once

#include "Shader.h"
#include "Camera.h"
#include "Scene.h"
#include <algorithm>
#include <glm/gtx/string_cast.hpp>


extern Camera camera;
extern enum SHADOW_TYPE;
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
extern Scene scene;
extern float cameraNearPlane;
extern float cameraFarPlane;
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
class Shadow {
public:

	bool _Shadow;
	bool SHADER;
	SHADOW_TYPE type;
	GLuint ShadowMap, depthMapFBO, SATFBO[2], SATMap[2];
	Shader shader;
	int MapSize = 1024;
	glm::vec3 Postion, Center, Direction;
	glm::mat4 LightSpace;
	float theta, varphi, R;
	// CSM
	GLuint LightSpaceUBO;
	std::vector<glm::vec4> LightMatrices;
	std::vector<float> shadowCascadeLevels;
	float lambda = 0.5;
	int cascadeCount;
	int depthMapResolution = 4096;
	//PCSS
	Shader satShader;

	Shadow();
	void Set_DirectionLight(float theta, float varphi, float r =10);
	void Set_CSM(float theta, float varphi, int cascadeCount = 5);
	void Set_VSSM(float theta, float varphi, float r = 25);

	std::vector<glm::mat4> getCSMLightSpace();
	glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane);
	std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
	void Get_ShadowMap(void (Scene::*p)(Shader&, bool));

	void Set_ShadowMap(Shader& shader, int id);

	std::vector<glm::mat4> Test_getCSMLightSpace();

};