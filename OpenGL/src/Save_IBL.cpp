#include "Save_IBL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


extern glm::mat4 captureProjection;
extern glm::mat4 captureViews[];
using namespace std;
void renderCube();

Save_IBL::Save_IBL() { 
    shader = Shader("./Shaders/IBL/background.vs", "./Shaders/IBL/save_cubemap.fs");
    shader.use();
    shader.setInt("equirectangularMap", 0);
    shader.setMat4("projection", captureProjection); 
}
void Save_IBL::save_cubemap(GLuint map_id, int map_size_x, int map_size_y, std::string storepath) {

    shader.use();

    std::cout << "正在生成：" + storepath + "的cubemap\n";
    int width = map_size_x, height = map_size_y;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, map_id);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader.setMat4("view", captureViews[i]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();

        long long int tmpPixelSize = width * height * 3;
        char* tmpPixelsBuffer = (char*)malloc(tmpPixelSize);

        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, tmpPixelsBuffer);
        std::string file2 = "C:/Users/czzzz/Desktop/skybox/" + storepath + "/skybox" + std::to_string(i) + ".tga";
        stbi_write_tga(file2.c_str(), width, height, 3, tmpPixelsBuffer);
        std::cout << "  第" + to_string(i + 1) + "张图片成功生成。\n";
    }
    std::cout << storepath << "图片生成成功，每张图的宽为：" + std::to_string(width) + "长为：" + to_string(height) << "\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Save_IBL::save_prefilter(GLuint map_id, int map_size, std::string storepath) {

    shader.use();

    cout << "正在生成：" + storepath + "的各级prefilter\n";
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, map_id);

    for (int leve = 0; leve < 5; leve++) {
        unsigned int mapsize = static_cast<unsigned int>(map_size * std::pow(0.5, leve));
        glViewport(0, 0, mapsize, mapsize);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        cout << "正在渲染第" + to_string(leve) + "级的mipmap.  ";
        cout << "该mipmap的大小为:" << mapsize << "*" << mapsize << "\n";

        shader.setBool("prefilter", true);
        shader.setInt("leve", leve);
        for (unsigned int i = 0; i < 6; ++i)
        {
            shader.setMat4("view", captureViews[i]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();

            long long int tmpPixelSize = mapsize * mapsize * 3;
            char* tmpPixelsBuffer = (char*)malloc(tmpPixelSize);

            glReadPixels(0, 0, mapsize, mapsize, GL_RGB, GL_UNSIGNED_BYTE, tmpPixelsBuffer);
            string file2 = "C:/Users/czzzz/Desktop/skybox/" + storepath + "_" + to_string(leve) + "/skybox" + to_string(i) + ".tga";
            stbi_write_tga(file2.c_str(), mapsize, mapsize, 3, tmpPixelsBuffer);
            std::cout << "  第" + to_string(i + 1) + "张图片成功生成。\n";
        }
    }
    cout << storepath << "图片全部生成成功" << "\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}