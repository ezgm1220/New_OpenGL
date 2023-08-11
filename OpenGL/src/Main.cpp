#include <time.h>
#include <map>
#include "Camera.h"
#include "IBL.h"
#include "Save_IBL.h"
#include "Light.h"
#include "ltc_matrix.hpp"
#include "Scene.h"
#include "Shadow.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void renderSphere();
void renderCube();
void renderQuad();
void renderAreaLight();
void renderPlane();
GLuint loadTexture(std::string path);

// ��Ļ����
extern const unsigned int SCR_WIDTH = 1800;
extern const unsigned int SCR_HEIGHT = 900;
//const unsigned int SCR_WIDTH = 800;
//const unsigned int SCR_HEIGHT = 600;

// �������
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float cameraNearPlane = 0.1f;
float cameraFarPlane = 500;
// ����
Scene scene;
//ָ���ʼ����λ��
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
//��ֹ��һ���ƶ���������������״̬��
bool firstMouse = true;
// ʱ��
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame = 0.0;

bool RealTimeRenew = false;
bool OpenSpot = true;
int AreaLightTexID;
/*
����:
    awds�ƶ�,space����,ctrl�½�
    ͬʱ�� R+T+Y,��ʼʵʱ���¹���Shader,�����ڵ���shader
    ͬʱ�� R+T+N,ֹͣʵʱ���¹���Shader.
    ͬʱ�� Q+O,�򿪾۹��
    ͬʱ�� Q+P,�رվ۹��
*/

GLuint Gbuffer;
GLuint gPosition, gNormal, gMaterial, gAlbedo;

enum LIGHT_TYPE { POINT, DIRECTION, SPOT, AREA };
extern enum SHADOW_TYPE { DIRECTION_LIGHT = 1, CSM };

// ��Ⱦ������ѡ�� 
enum RENDERPATH { DEFERRED, FORWARD, SHOW2D, SHOW3D };
RENDERPATH RenderPath = SHOW2D;


int main() {

    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    }
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PBR_T2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();

        return -1;
    }
    {
        glfwMakeContextCurrent(window);
        //ע��ص�����
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);

        //�������ָ��
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //ʹ��GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }
    }
    
    // ������Դ�ڼ���ʱ��
    clock_t sourceload_star, sourceload_end;
    sourceload_star = clock();

    // ������Ȳ���,С�ڵ���ʱͨ������
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // ����֡����
    if (RenderPath == DEFERRED) {// ����G-Buffer
        glGenFramebuffers(1, &Gbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer);
        // Position
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
        // Normal
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
        // metallic + roughness + ao
        glGenTextures(1, &gMaterial);
        glBindTexture(GL_TEXTURE_2D, gMaterial);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMaterial, 0);
        // albedo color
        glGenTextures(1, &gAlbedo);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gAlbedo, 0);
        // ������ȾĿ��
        unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);
    }

    // ������Ⱦ����
    unsigned int rboDepth;
    {
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    // ������ɫ��
    Shader skyboxShader("./Shaders/Skybox/skybox.vs", "./Shaders/Skybox/skybox.fs");
    Shader GShader, lightShader, ForwardShader, show2DShader;

    switch (RenderPath) {
    case DEFERRED: {
        GShader = Shader("./Shaders/Deferred/G_buffer.vs", "./Shaders/Deferred/G_buffer.fs");
        lightShader = Shader("./Shaders/Deferred/Lighting.vs", "./Shaders/Deferred/Lighting.fs");
        break;
    }
    case FORWARD: {
        ForwardShader = Shader("./Shaders/Forward/Forward.vs", "./Shaders/Forward/Forward.fs");
        break;
    }
    case SHOW2D: {
        show2DShader = Shader("./Shaders/Base/show2D.vs", "./Shaders/Base/show2D.fs");
        break;
    }
    }



    // ��Ӱ
    Shadow shadow;

    // ��ӵƹ�
    Lights lights;
    {
        // PointLight:
        {

        }

        // DirectionLight:
        {
            lights.add_Direction(60, 45, glm::vec3(6));
            shadow.Set_CSM(60, 45);

            //cout << lights.lights[0].Color.x;
        }

        // SpotLight:
        {

        }

        // AreaLight��
        {
            
            lights.Have_AreaLight = true;
            glm::vec3 Postion, Direction, Color;
            float Cut, OutCut, Intensity, Rotation;
            Postion = glm::vec3(2.5, -1, -2.5);
            Direction = glm::vec3(0, 1, 0);
            Color = glm::vec3(0.278431f, 0.547059f, 0.101961f);
            Intensity = 4.0;
            Rotation = 45.0;
            lights.add_Area(Postion, Direction, Color, Rotation, Intensity);
        }
    }

    // ������ɫ������
    GLuint tex2[90];
    {
        skyboxShader.use();
        skyboxShader.setInt("environmentMap", 0);
        switch (RenderPath) {
        case DEFERRED: {

            GShader.use();
            GShader.setInt("albedoMap", 0);
            GShader.setInt("normalMap", 1);
            GShader.setInt("metallicMap", 2);
            GShader.setInt("roughnessMap", 3);
            GShader.setInt("aoMap", 4);

            lightShader.use();
            lightShader.setInt("gPosition", 0);
            lightShader.setInt("gNormal", 1);
            lightShader.setInt("gMaterial", 2);
            lightShader.setInt("gAlbedo", 3);
            lightShader.setInt("irradianceMap", 4);
            lightShader.setInt("prefilterMap", 5);
            lightShader.setInt("brdfLUT", 6);
            lightShader.setInt("ShadowMap", 7);

            if (lights.Have_AreaLight) {
                lightShader.setInt("LTC_1Map", 8);
                lightShader.setInt("LTC_2Map", 9);
                AreaLightTexID = 8;
            }
            break;
        }
        case FORWARD: {

            ForwardShader.use();
            ForwardShader.setInt("albedoMap", 0);
            ForwardShader.setInt("normalMap", 1);
            ForwardShader.setInt("metallicMap", 2);
            ForwardShader.setInt("roughnessMap", 3);
            ForwardShader.setInt("aoMap", 4);
            ForwardShader.setInt("irradianceMap", 5);
            ForwardShader.setInt("prefilterMap", 6);
            ForwardShader.setInt("brdfLUT", 7);
            if (shadow.type == DIRECTION_LIGHT) {
                ForwardShader.setInt("ShadowMap", 8);
            }
            else if (shadow.type == CSM) {
                ForwardShader.setInt("ShadowMap_CSM", 8);
            }

            if (lights.Have_AreaLight) {
                ForwardShader.setInt("LTC_1Map", 9);
                ForwardShader.setInt("LTC_2Map", 10);
                AreaLightTexID = 9;
            }
            break;
        }
        case SHOW2D: {
            show2DShader.use();
            for (int i = 0; i < 12; i++) {
                string tex = "tex";
                tex += to_string(i);
                show2DShader.setInt(tex, i);
            }
            /*for (int i = 0; i < 9; i++) {
                tex2[i] = loadTexture("test/picture/" + to_string(i) + ".png");
            }*/

            break;
        }
        }
    }

    // ���IBL
    IBL ibl;
    string sourcepath = "C:/Users/czzzz/Desktop/HDR/";
    //ibl.Load(sourcepath + "envmap",".tga", true);
    //ibl.Load(sourcepath + "dikhololo_night_4k", ".hdr");
    //ibl.PrepareIBL();

    // �������������

    //scene.add_Material("gold");
    //scene.add_Material("suliao", glm::vec3(0.5, 0.5, 0.5), 0.1, 0.1);
    //Material wall("rusted_iron"), gold("gold");

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // �����Դ����ʱ��
    sourceload_end = clock();
    printf("��Դ���ص�ʱ��Ϊ:%lf\n", double(sourceload_end - sourceload_star) / CLOCKS_PER_SEC);


    // ���ó���
    void(Scene:: * Render_Scene)(Shader&, bool) = &Scene::Test_Shadow;

    // ��Ⱦѭ��

    while (!glfwWindowShouldClose(window)) {

        // ����ʱ��
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // cout << "\r" << int(1.0 / deltaTime);
        // ���������¼�:

        processInput(window);


        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();;
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, cameraNearPlane, cameraFarPlane);;
        glm::mat4 model;

        if (shadow._Shadow) {
            shadow.Get_ShadowMap(Render_Scene);
        }

        switch (RenderPath) {

        case DEFERRED: {
            // ����G-Buffer��
                // �������޳�
            glEnable(GL_CULL_FACE);
            glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            GShader.use();
            GShader.setMat4("projection", projection);
            GShader.setMat4("view", view);

            // ѡȡ����
            scene.Plan_1(GShader);
            //Sphere_1(GShader, wall, gold);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // �ر����޳�
            glDisable(GL_CULL_FACE);
            // ��Ⱦ����:
                // ʵʱ���Թ���Shader
            if (RealTimeRenew) {
                lightShader = Shader("./Shaders/Deferred/Lighting.vs", "./Shaders/Deferred/Lighting.fs");
                lightShader.use();
                lightShader.setInt("gPosition", 0);
                lightShader.setInt("gNormal", 1);
                lightShader.setInt("gMaterial", 2);
                lightShader.setInt("gAlbedo", 3);
                lightShader.setInt("irradianceMap", 4);
                lightShader.setInt("prefilterMap", 5);
                lightShader.setInt("brdfLUT", 6);
                if (lights.Have_AreaLight) {
                    lightShader.setInt("LCT_1Map", 7);
                    lightShader.setInt("LCT_2Map", 8);
                }
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            lightShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gMaterial);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, gAlbedo);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilterMap);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, ibl.LUTTexture);

            // �����Դ��Ϣ
            lights.Set_Light(lightShader, AreaLightTexID);

            lightShader.setVec3("camPos", camera.Position);
            lightShader.setBool("OpenSpot", OpenSpot);
            lightShader.setBool("Have_AreaLight", lights.Have_AreaLight);
            renderQuad();

            // ������Ȼ��嵽Ĭ��buffer��
            glBindFramebuffer(GL_READ_FRAMEBUFFER, Gbuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            break;
        }

        case FORWARD: {
            // �������޳�
            glEnable(GL_CULL_FACE);
            // �ָ���Ļ��С
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

            if (1) {// ʵʱ���Թ���Shader
                ForwardShader = Shader("./Shaders/Forward/Forward.vs", "./Shaders/Forward/Forward.fs");
                ForwardShader.use();

                ForwardShader.setInt("albedoMap", 0);
                ForwardShader.setInt("normalMap", 1);
                ForwardShader.setInt("metallicMap", 2);
                ForwardShader.setInt("roughnessMap", 3);
                ForwardShader.setInt("aoMap", 4);
                ForwardShader.setInt("irradianceMap", 5);
                ForwardShader.setInt("prefilterMap", 6);
                ForwardShader.setInt("brdfLUT", 7);
                if (shadow.type == DIRECTION_LIGHT) {
                    ForwardShader.setInt("ShadowMap", 8);
                }
                else if (shadow.type == CSM) {
                    ForwardShader.setInt("ShadowMap_CSM", 8);
                }

                if (lights.Have_AreaLight) {
                    ForwardShader.setInt("LTC_1Map", 9);
                    ForwardShader.setInt("LTC_2Map", 10);
                    AreaLightTexID = 9;
                }
            }

            ForwardShader.use();
            ForwardShader.setMat4("projection", projection);
            ForwardShader.setMat4("view", view);
            ForwardShader.setVec3("camPos", camera.Position);
            ForwardShader.setBool("OpenSpot", OpenSpot);
            ForwardShader.setBool("Have_AreaLight", lights.Have_AreaLight);

            if (lights.Have_AreaLight) {
                ForwardShader.setBool("HaveAreaLight", true);
            }
            else {
                ForwardShader.setBool("HaveAreaLight", false);
            }

            if (shadow._Shadow) {
                shadow.Set_ShadowMap(ForwardShader, 8);
            }

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilterMap);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, ibl.LUTTexture);
            lights.Set_Light(ForwardShader, AreaLightTexID);

            (scene.*Render_Scene)(ForwardShader, false);

            // �ر����޳�
            glDisable(GL_CULL_FACE);
            break;
        }

        case SHOW2D: {
            show2DShader = Shader("./Shaders/Base/show2D.vs", "./Shaders/Base/show2D.fs");
            show2DShader.use();
            show2DShader.setInt("layer", 1);
            show2DShader.setInt("tex0", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            renderQuad();
            break;
        }
        }

        if (RenderPath == FORWARD || RenderPath == DEFERRED) {
            // �ƹ���Ⱦ
            lights.shader.use();
            lights.shader.setMat4("view", view);
            lights.shader.setMat4("projection", projection);
            lights.Show_Light();

            // ��պ���Ⱦ:
            skyboxShader.use();
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envcubeMap);
            renderCube();
        }

        // �洢��ɫ���壬��Ϊ�����ʾ����Ļ�ϣ�
        glfwSwapBuffers(window);
        // �����û�д���ʲô�¼�
        glfwPollEvents();

    }


    glfwTerminate();
    return 0;
}



void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.jump();
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.down();
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) RealTimeRenew = true;
            if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) RealTimeRenew = false;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)OpenSpot = true;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)OpenSpot = false;
    }
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}
// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
             // bottom face
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
              1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             // top face
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
              1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
              1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
              1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

GLuint areaLightVBO, areaLightVAO = 0;
void renderAreaLight()
{
    if (areaLightVAO == 0) {
        float areaLightVertices[48] = {
    -1.0f,  1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  0.0f, 0.0f, // 0 1 5 4
     1.0f,  1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  0.0f, 1.0f,
     1.0f, -1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  1.0f, 1.0f,
    -1.0f,  1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  0.0f, 0.0f,
     1.0f, -1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  1.0f, 1.0f,
    -1.0f, -1.0f ,  0.0f,   0.0f, 0.0f, 1.0f ,  1.0f, 0.0f
        };
        glGenVertexArrays(1, &areaLightVAO);
        glBindVertexArray(areaLightVAO);

        glGenBuffers(1, &areaLightVBO);
        glBindBuffer(GL_ARRAY_BUFFER, areaLightVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(areaLightVertices), areaLightVertices, GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        glBindVertexArray(0);
    }
    glBindVertexArray(areaLightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

GLuint planeVBO, planeVAO = 0;
void renderPlane() {
    if (planeVAO == 0) {
        const GLfloat psize = 25.0f;
        const GLfloat P_Y = -2.0f;
        float planeVertices[48] = {
            -psize, P_Y, -psize ,  0.0f, 1.0f, 0.0f ,  0.0f, 0.0f   ,
            -psize, P_Y,  psize ,  0.0f, 1.0f, 0.0f ,  0.0f, 1.0f   ,
             psize, P_Y,  psize ,  0.0f, 1.0f, 0.0f ,  1.0f, 1.0f   ,
            -psize, P_Y, -psize ,  0.0f, 1.0f, 0.0f ,  0.0f, 0.0f   ,
             psize, P_Y,  psize ,  0.0f, 1.0f, 0.0f ,  1.0f, 1.0f   ,
             psize, P_Y, -psize ,  0.0f, 1.0f, 0.0f ,  1.0f, 0.0f
        };

        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);

        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
            (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}