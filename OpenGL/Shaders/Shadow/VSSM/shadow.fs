#version 330 core
//out vec4 FragColor;
void main()
{             
     //gl_FragDepth = gl_FragCoord.z;
     //FragColor = vec4(gl_FragCoord.z);
     //FragColor = vec4(1,1,1,1.0);
     float depth = gl_FragCoord.z;
     float depth_2 = depth * depth;
     //store shadow map and square shadow map in one texture
     // R channel for shadow map, G channel for square shadow map
     gl_FragColor = vec4(depth, depth_2, 0.0, 0.0);
}