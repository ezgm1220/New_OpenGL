
#include "IBL.h"

#define STB_IMAGE_IMPLEMENTATION // ������stb_image.h֮ǰ����.
#include "stb_image.h"


void renderCube();
void renderQuad();
glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] =
{
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

IBL::IBL(){
    stbi_set_flip_vertically_on_load(true);
    glGenFramebuffers(1, &mFBO);
    glGenRenderbuffers(1, &mRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRBO);
}
void IBL::Load(string file, string type, bool cube) {

    if (!cube) {
        int width, height, nrComponents;
        float* data = stbi_loadf((file + type).c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            //std::cout << "�ļ���ȡ�ɹ�\n";
        }
        else
        {
            std::cout << "HDR������ͼ:" + file + "����ʧ��" << std::endl;
        }

        glGenTextures(1, &envcubeMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



        Shader cubemapShader("./Shaders/IBL/cubemap.vs", "./Shaders/IBL/get_cubemap.fs");
        cubemapShader.use();
        cubemapShader.setInt("equirectangularMap", 0);
        cubemapShader.setMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glViewport(0, 0, 512, 512); // ��Ҫ���ǽ��ӿ�����Ϊ����ߴ硣
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            cubemapShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envcubeMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);// Ϊ��ǰ�󶨵������Զ�����������Ҫ�Ķ༶��Զ����
    }
    else {

        vector<string>files;
        for (int i = 0; i < 6; i++) {
            files.push_back(file + "/skybox" + to_string(i) + type);
        }

        glGenTextures(1, &envcubeMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < files.size(); i++)
        {
            float* data = stbi_loadf(files[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16, width, height, 0, GL_RGB, GL_FLOAT, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "HDR������ͼ:" + files[i] + "����ʧ��" << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);// Ϊ��ǰ�󶨵������Զ�����������Ҫ�Ķ༶��Զ����
    }
}

void IBL::Get_irradiance() {

    Shader irradianceMapShader("./Shaders/IBL/cubemap.vs", "./Shaders/IBL/get_irradianceMap.fs");

    int ir_width = 32, ir_hight = 32;

    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, ir_width, ir_hight, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);  // ��֡����
    glBindRenderbuffer(GL_RENDERBUFFER, mRBO);// ����Ⱦ����
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ir_width, ir_hight);// ����һ����Ⱥ�ģ�建�����

    // pbr: ͨ����������������֣��Դ���һ�����նȣ������壩ͼ�� ��������
    // -----------------------------------------------------------------------------
    irradianceMapShader.use();
    irradianceMapShader.setInt("environmentMap", 0);
    irradianceMapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);// ������õ���պ�������

    glViewport(0, 0, ir_width, ir_hight); // ��Ҫ���ǽ��ӿ�����Ϊ����ߴ硣
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceMapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IBL::Get_prefilter() {

    Shader prefilterMapShader("./Shaders/IBL/cubemap.vs", "./Shaders/IBL/prefilter.fs");

    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Ϊcubemap����mipmaps���Ա�OpenGL�Զ����������ڴ档
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: �ڻ�������������һ��׼���ؿ���ģ�⣬�Դ���һ��Ԥ���ˣ������壩��ͼ��
    // ----------------------------------------------------------------------------------------------------
    prefilterMapShader.use();
    prefilterMapShader.setInt("environmentMap", 0);
    prefilterMapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envcubeMap);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // ����mip����С������֯֡��������
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, mRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterMapShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterMapShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IBL::Get_LUT() {

    Shader LUTTextureShader("./Shaders/IBL/get_LUT.vs", "./Shaders/IBL/get_LUT.fs");

    glGenTextures(1, &LUTTexture);

    // Ԥ��ΪLUT��������㹻���ڴ档
    glBindTexture(GL_TEXTURE_2D, LUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // ��ȷ������װģʽ����ΪGL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Ȼ���������ò�׽֡���������BRDF��ɫ����Ⱦ��Ļ�ռ���ı��Ρ�
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, LUTTexture, 0);

    glViewport(0, 0, 512, 512);
    LUTTextureShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint loadTexture(std::string path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (nrComponents == 4) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "�������ʧ��:" << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}