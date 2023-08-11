#include "Shadow.h"

enum SHADOW_TYPE { DIRECTION_LIGHT = 1, CSM, VSSM};

Shadow::Shadow() {

	_Shadow = false;
	//std::cout << "阴影贴图着色器编译成功\n";
	SHADER = false;
}

void Shadow::Set_DirectionLight(float theta, float varphi, float r) {

	if (!SHADER) {
		shader = Shader("./Shaders/Shadow/Shadow.vs", "./Shaders/Shadow/Shadow.fs");

		// 创建纹理
		glGenTextures(1, &ShadowMap);
		glBindTexture(GL_TEXTURE_2D, ShadowMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// 配置缓冲区
		glGenFramebuffers(1, &depthMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ShadowMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		SHADER = false;
	}

	type = DIRECTION_LIGHT;
	_Shadow = true;

	float sin_t, cos_t, sin_p, cos_p;
	sin_t = glm::sin(glm::radians(theta));
	sin_p = glm::sin(glm::radians(varphi));
	cos_t = glm::cos(glm::radians(theta));
	cos_p = glm::cos(glm::radians(varphi));

	float x, y, z;
	x = r * sin_t * sin_p;
	y = r * cos_t;
	z = r * sin_t * cos_p;

	Postion = glm::vec3(x, y, z);
	Direction = glm::vec3(-x, -y, -z);
	Center = glm::vec3(0, 0, 0);
	R = r;
}

void Shadow::Set_CSM(float theta, float varphi, int cascadeCount) {
	if (!SHADER) {

		shader = Shader("./Shaders/Shadow/CSM.vs", "./Shaders/Shadow/CSM.fs", "./Shaders/Shadow/CSM.gs");

		//shadowCascadeLevels = std::vector<float>{ cameraFarPlane / 50.0f, cameraFarPlane / 25.0f, cameraFarPlane / 10.0f, cameraFarPlane / 2.0f };
		//计算子视锥的切割方法
		float nd = cameraNearPlane;
		float fd = cameraFarPlane;
		float ratio = fd / nd;
		for (int i = 1; i < cascadeCount; i++)
		{
			float si = i / (float)cascadeCount;
			float t_near = lambda * (nd * powf(ratio, si)) + (1 - lambda) * (nd + (fd - nd) * si);
			shadowCascadeLevels.push_back(t_near);
		}



		glGenFramebuffers(1, &depthMapFBO);

		glGenTextures(1, &ShadowMap);
		glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMap);
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, int(shadowCascadeLevels.size()) + 1,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "CSM帧缓冲发生错误\n";
			throw 0;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// configure UBO
		// --------------------



		

		glGenBuffers(1, &LightSpaceUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, LightSpaceUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, LightSpaceUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		SHADER = false;
	}

	type = CSM;
	_Shadow = true;

	float sin_t, cos_t, sin_p, cos_p;
	sin_t = glm::sin(glm::radians(theta));
	sin_p = glm::sin(glm::radians(varphi));
	cos_t = glm::cos(glm::radians(theta));
	cos_p = glm::cos(glm::radians(varphi));

	float x, y, z;
	x = sin_t * sin_p;
	y = cos_t;
	z = sin_t * cos_p;

	int R = 1;
	Postion = glm::vec3(x * R, y * R, z * R);
	Direction = glm::vec3(-x, -y, -z);
	Center = glm::vec3(0, 0, 0);
	this->cascadeCount = cascadeCount;

}

void Shadow::Set_VSSM(float theta, float varphi, float r) {
	if (!SHADER) {

		shader = Shader("./Shaders/Shadow/VSSM/shadow.vs", "./Shaders/Shadow/VSSM/shadow.fs");
		satShader = Shader("./Shaders/Shadow/VSSM/SAT.comp");
		// 创建纹理
		glGenTextures(1, &ShadowMap);
		glBindTexture(GL_TEXTURE_2D, ShadowMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MapSize, MapSize, 0, GL_RG, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_TEXTURE_MIN_FILTER
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //GL_TEXTURE_MAG_FILTER
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_TEXTURE_WRAP_S
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //GL_TEXTURE_WRAP_T
		float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);
		// 配置缓冲区
		glGenFramebuffers(1, &depthMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ShadowMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 创建SAT纹理
		glGenFramebuffers(2, SATFBO);
		glGenTextures(2, SATMap);

		for (int i = 0; i < 2; ++i) {
			glBindFramebuffer(GL_FRAMEBUFFER, SATFBO[i]);
			glBindTexture(GL_TEXTURE_2D, SATMap[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, MapSize, MapSize, 0, GL_RG, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SATMap[i], 0);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bordercolor);

		SHADER = false;
	}

	type = VSSM;
	_Shadow = true;

	float sin_t, cos_t, sin_p, cos_p;
	sin_t = glm::sin(glm::radians(theta));
	sin_p = glm::sin(glm::radians(varphi));
	cos_t = glm::cos(glm::radians(theta));
	cos_p = glm::cos(glm::radians(varphi));

	float x, y, z;
	x = r * sin_t * sin_p;
	y = r * cos_t;
	z = r * sin_t * cos_p;

	Postion = glm::vec3(x, y, z);
	Direction = glm::vec3(-x, -y, -z);
	Center = glm::vec3(0, 0, 0);
	R = r;
}

void Shadow::Get_ShadowMap(void(Scene::* p)(Shader&, bool)) {

	if (type == DIRECTION_LIGHT) {

		glm::mat4 view, projection;
		float near_plane = 0.1f, far_plane = 100;

		view = glm::lookAt(Postion, Center, glm::vec3(0, 1, 0));
		projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

		LightSpace = projection * view;

		shader.use();
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		shader.setMat4("lightSpace", LightSpace);
		glm::mat4 model = glm::mat4(1.0);
		shader.setMat4("model", model);

		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		(scene.*p)(shader, true);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
	if (type == CSM) {

		// 0. UBO setup
		const auto lightMatrices = getCSMLightSpace();
		//const auto lightMatrices = Test_getCSMLightSpace();
		glBindBuffer(GL_UNIFORM_BUFFER, LightSpaceUBO);
		for (size_t i = 0; i < lightMatrices.size(); ++i)
		{
			glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		shader.use();

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glViewport(0, 0, depthMapResolution, depthMapResolution);
		glClear(GL_DEPTH_BUFFER_BIT);
		//glCullFace(GL_FRONT);  // peter panning
		(scene.*p)(shader, false);
		//glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if (type == VSSM) {
		// 获取ShadowMap
		{
			glm::mat4 view, projection;
			float near_plane = cameraNearPlane, far_plane = cameraFarPlane;

			view = glm::lookAt(Postion, Center, glm::vec3(0, 1, 0));
			projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

			LightSpace = projection * view;

			shader.use();
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			shader.setMat4("lightSpace", LightSpace);
			glm::mat4 model = glm::mat4(1.0);
			shader.setMat4("model", model);

			glViewport(0, 0, MapSize, MapSize);

			glClear(GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_FRONT);
			(scene.*p)(shader, true);
			glCullFace(GL_BACK);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		{
			satShader.use();
			glBindImageTexture(0, ShadowMap, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
			glBindImageTexture(1, SATMap[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
			glDispatchCompute(MapSize, 1, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glBindImageTexture(0, SATMap[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
			glBindImageTexture(1, SATMap[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
			glDispatchCompute(MapSize, 1, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}
}


void Shadow::Set_ShadowMap(Shader& shader, int id) {
	if (type == DIRECTION_LIGHT) {
		shader.setInt("Shadow_type", (int)type);
		shader.setMat4("LightSpace", LightSpace);
		shader.setVec3("ShadowDirection", Direction);
		glActiveTexture(GL_TEXTURE0 + id);
		glBindTexture(GL_TEXTURE_2D, ShadowMap);
	}
	if (type == CSM) {

		shader.setInt("Shadow_type", (int)type);
		shader.setVec3("ShadowDirection", Direction);
		shader.setInt("cascadeCount", cascadeCount - 1);
		for (size_t i = 0; i < shadowCascadeLevels.size(); ++i)
		{
			shader.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
		}
		shader.setFloat("farPlane", cameraFarPlane);
		glActiveTexture(GL_TEXTURE0 + id);
		glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMap);
	}
}

std::vector<glm::mat4> Shadow::getCSMLightSpace() {

	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(getLightSpaceMatrix(cameraNearPlane, shadowCascadeLevels[i]));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
		}
		else
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], cameraFarPlane));
		}
	}
	return ret;
}

glm::mat4 Shadow::getLightSpaceMatrix(const float nearPlane, const float farPlane) {
	// 这里的 near 和 far 都是子锥体的信息
	const auto proj = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearPlane, farPlane);
	// 在相机下是个标准的正方形,我们通过相机的透视矩阵和视角矩阵将其逆变换到世界空间中.
	const auto corners = getFrustumCornersWorldSpace(proj, camera.GetViewMatrix());

	// 测试数据
	glm::vec3 frustum[10];
	glm::vec3 Test_center = glm::vec3(0, 0, 0);
	{
		glm::vec3 viewPos = camera.Position;
		glm::vec3 viewDir = camera.Front;
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::cross(viewDir, up);
		float ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
		// 计算子视锥的世界坐标
		{
			float near = nearPlane, far = farPlane;
			glm::vec3 fc = viewPos + viewDir * far;
			glm::vec3 nc = viewPos + viewDir * near;
			right = glm::normalize(right);
			up = glm::normalize(glm::cross(right, viewDir));
			// 计算当前分片的近平面和远平面宽高的一半
			float near_height = tan(camera.Zoom / 2.0f) * near;
			float near_width = near_height * ratio;
			float far_height = tan(camera.Zoom / 2.0f) * far;
			float far_width = far_height * ratio;
			//记录视锥8个顶点
			frustum[0] = nc - up * near_height - right * near_width;
			frustum[1] = nc + up * near_height - right * near_width;
			frustum[2] = nc + up * near_height + right * near_width;
			frustum[3] = nc - up * near_height + right * near_width;
			frustum[4] = fc - up * far_height - right * far_width;
			frustum[5] = fc + up * far_height - right * far_width;
			frustum[6] = fc + up * far_height + right * far_width;
			frustum[7] = fc - up * far_height + right * far_width;
		}

		
		for (int i = 0; i < 8; i++) {
			Test_center += glm::vec3(frustum[i]);
		}
	}


	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();// 获得到立方体的中心
	Test_center /= 8.0;

	glm::vec3 Pos = center + Postion;
	const auto lightView = glm::lookAt(Pos, center, glm::vec3(0.0f, 1.0f, 0.0f));

	//printf("Log:%s\n", glm::to_string(lightView).c_str());

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	// 求AABB包围盒
	for (const auto& v : corners)
	{
		const auto trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// 根据场景调整该参数
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	return lightProjection * lightView;
}

std::vector<glm::vec4> Shadow::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {

	const auto inv = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

std::vector<glm::mat4> Shadow::Test_getCSMLightSpace() {

	std::vector<glm::mat4> ret;

	glm::vec3 viewPos = camera.Position;
	glm::vec3 viewDir = camera.Front;
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::cross(viewDir, up);
	glm::vec3 frustum[10];
	float ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
	//printf("%.lf\n", camera.Zoom);
	for (int i = 0; i < cascadeCount; i++)
	{
		// 计算子视锥的世界坐标
		{
			float near, far;
			if (i == 0)
			{
				near = cameraNearPlane;
				far = shadowCascadeLevels[i];
			}
			else if (i < shadowCascadeLevels.size())
			{
				near = shadowCascadeLevels[i - 1];
				far = shadowCascadeLevels[i];
			}
			else
			{
				near = shadowCascadeLevels[i - 1];
				far = cameraFarPlane;
			}
			glm::vec3 fc = viewPos + viewDir * far;
			glm::vec3 nc = viewPos + viewDir * near;
			right = glm::normalize(right);
			up = glm::normalize(glm::cross(right, viewDir));
			// 计算当前分片的近平面和远平面宽高的一半
			float near_height = tan(camera.Zoom / 2.0f) * near;
			float near_width = near_height * ratio;
			float far_height = tan(camera.Zoom / 2.0f) * far;
			float far_width = far_height * ratio;
			//记录视锥8个顶点
			frustum[0] = nc - up * near_height - right * near_width;
			frustum[1] = nc + up * near_height - right * near_width;
			frustum[2] = nc + up * near_height + right * near_width;
			frustum[3] = nc - up * near_height + right * near_width;
			frustum[4] = fc - up * far_height - right * far_width;
			frustum[5] = fc + up * far_height - right * far_width;
			frustum[6] = fc + up * far_height + right * far_width;
			frustum[7] = fc - up * far_height + right * far_width;
		}

		glm::vec3 center = glm::vec3(0.0);
		for (int j = 0; j < 8; j++) {
			center += glm::vec3(frustum[j]);
		}
		center /= 8.0;
		glm::mat4 lightProjMat;
		glm::mat4 lightViewMat = glm::lookAt(center + Postion, center, glm::vec3(0, 1, 0));
		//1. 找出光空间中八个顶点的最大最小z值
		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		// 求AABB包围盒
		glm::vec3 max(maxX, maxX, maxX);
		glm::vec3 min(minX, minX, minX);
		glm::vec4 transf;
		for (int j = 0; j < 8; j++)
		{
			transf = lightViewMat * glm::vec4(frustum[j], 1.0f);
			if (transf.z > max.z) { max.z = transf.z; }
			if (transf.z < min.z) { min.z = transf.z; }
			/*if (transf.x > max.x) { max.x = transf.x; }
			if (transf.x < min.x) { min.x = transf.x; }
			if (transf.y > max.y) { max.y = transf.y; }
			if (transf.y < min.y) { min.y = transf.y; }*/
		}
		//1.1 扩展光视锥体的大小，使其包括所有的遮挡物
		//min.z = 0;
		//2. 设光空间的正交投影矩阵为ortho，他的x,y∈[-1,1]，z∈[-tmax.z,-tmin.z]
		//使用x,y∈[-1,1]，是因为每个分块的投影矩阵都可以使用单位x,y缩放平移后获得
		//使用z∈[-tmax.z,-tmin.z]，是因为摄像机空间指向负Z方向，而glm::ortho传入的是近平面和远平面指向正Z方向
		//glm::mat4 ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, min.z, max.z);
		glm::mat4 ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -max.z, -min.z);
		/*glm::mat4 ortho = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
		ret.push_back(lightProjMat * lightViewMat);
		continue;*/

		//2.1 在光空间中，找出视锥体切片各顶点的x、y的标准设备坐标范围。因为我们设的投影矩阵的x、y都是标准设备坐标，我们需要求出x、y的变化范围，以便对ortho进行缩放平移
		glm::mat4 lightVP = ortho * lightViewMat;
		for (int j = 0; j < 8; j++)
		{
			transf = lightVP * glm::vec4(frustum[j], 1.0f);
			transf.x /= transf.w;
			transf.y /= transf.w;
			if (transf.x > max.x) { max.x = transf.x; }
			if (transf.x < min.x) { min.x = transf.x; }
			if (transf.y > max.y) { max.y = transf.y; }
			if (transf.y < min.y) { min.y = transf.y; }
		}
		//2.2 根据正交投影矩阵的公式，设置缩放平移量(计算过程在后面)
		glm::vec2 scale(2.0f / (max.x - min.x), 2.0f / (max.y - min.y));
		glm::vec2 offset(-0.5f * (max.x + min.x) * scale.x, -0.5f * (max.y + min.y) * scale.y);
		glm::mat4 crop = glm::mat4(1.0);
		//2.3 设置缩放平移的变换矩阵
		crop[0][0] = scale.x;
		crop[1][1] = scale.y;
		crop[0][3] = offset.x;
		crop[1][3] = offset.y;
		crop = glm::transpose(crop);//注意glm按列储存，实际矩阵要转置
		//2.4 计算出光空间中的正交投影矩阵
		lightProjMat = crop * ortho;
		//保存光空间的投影矩阵
		//projection_matrices[i] = lightProjMat;
		//保存世界坐标到光空间变换的矩阵
		ret.push_back(lightProjMat * lightViewMat);

	}
	return ret;
}