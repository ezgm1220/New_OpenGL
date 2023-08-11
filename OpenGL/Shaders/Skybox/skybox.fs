#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform sampler2D hdr;
uniform int kk;

void main()
{		
    vec3 envColor = textureLod(environmentMap, WorldPos, 0.0).rgb;
    //HDR tonemap and gamma correct

    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    //if(envColor.r >= 1.0 && envColor.g >= 1.0 && envColor.b >= 1.0)
    //if(envColor.r > 1.0 && envColor.g > 1.0 && envColor.b > 1.0)
//    if(envColor.r == 1.0 && envColor.g == 1.0 && envColor.b == 1.0)
//    envColor=vec3(0,0,0);

    //if(envColor.r > 1 || envColor.g > 1 || envColor.b > 1)envColor = vec3(0,0,0);
    
    FragColor = vec4(envColor, 1.0);
    //FragColor = vec4(othercolor, 1.0);
    //FragColor = vec4(0.2,0.3,0.2,1.0);
}
