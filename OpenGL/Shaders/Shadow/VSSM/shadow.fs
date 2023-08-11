#version 330 core
void main()
{             
     float depth = gl_FragCoord.z;
     //if(gl_FragCoord.x<500 && gl_FragCoord.y<500)depth = 0.00000;
     //else depth = 0.00001;
     //depth = 0.0011;
     float depth_2 = depth * depth;
     gl_FragColor = vec4(depth, depth_2, 0.0, 0.0);
}