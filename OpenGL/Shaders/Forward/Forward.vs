#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
// 光照视角下的坐标
out vec4 LightPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;

// Shadow
uniform mat4 LightSpace;
uniform bool HaveShadow;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;  
    if(HaveShadow){
        LightPos = LightSpace * vec4(WorldPos, 1.0);
    }

    gl_Position =  projection * view * vec4(WorldPos, 1.0);

}