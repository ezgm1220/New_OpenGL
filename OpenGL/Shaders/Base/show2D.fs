#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;
uniform sampler2D tex11;
uniform sampler2D tex12;

uniform sampler2DArray depthMap;
uniform int layer;

void main()
{
	float depthValue = texture(depthMap, vec3(TexCoords, 2)).r;
    //FragColor = vec4(vec3(depthValue), 13);
	FragColor = vec4(0.3,0.2,0.3,1.0);
}