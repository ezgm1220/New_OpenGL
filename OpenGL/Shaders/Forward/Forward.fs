#version 410 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
// 光照视角下的坐标
in vec4 LightPos;


// material
uniform bool haveTexture;
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// LTC
uniform sampler2D LTC_1Map;
uniform sampler2D LTC_2Map;
uniform bool HaveAreaLight;

// shadow
uniform sampler2D ShadowMap;
uniform int Shadow_type;
uniform vec3 ShadowDirection;
// shadow-CSM
layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform sampler2DArray ShadowMap_CSM;
uniform float cascadePlaneDistances[16];
uniform int cascadeCount; 
uniform mat4 view;
uniform float farPlane;

//  没有纹理时的材质信息
uniform vec3 _Albedo;
uniform float _Metallic;
uniform float _Roughness;
uniform float _Ao;

const float PI = 3.14159265359;
const int SUM_LIGHTS = 32;
const float LUT_SIZE  = 64.0; // ltc_texture size
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS  = 0.5/LUT_SIZE;

struct Light {
    
    int type;
    vec3 Position;
    vec3 Color;
    vec3 Direction;
    vec3 points[4];
    bool twoface;
    
    float Intensity;
    float Linear;
    float Quadratic;
    float Radius;
    float CutOff;
    float outerCutOff;
};


uniform Light lights[SUM_LIGHTS];
uniform vec3 camPos;
uniform int lights_size;
uniform bool OpenSpot;


float rcp(float f)
{
    return 1.0 / f;
}
float Pow5(float f)
{
    float f2 = f*f;
	return f2 * f2 * f;
}

vec3 Diffuse_Burley( vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH );
vec3 Diffuse_OrenNayar( vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH );
float D_GGX( float a2, float NoH );
float Vis_Smith( float a2, float NoV, float NoL );
vec3 F_Schlick( vec3 SpecularColor, float VoH );
vec3 fresnelSchlickRoughness( vec3 SpecularColor, float VoH, float roughness );
vec3 getNormalFromMap();
float getShadow(vec3 N, vec3 L);
float getShadow_CSM(vec3 N, vec3 L);

vec3 get_BRDF(vec3 N, vec3 L, vec3 H,vec3 V, vec3 F0,vec3 albedo, float roughness, float metallic);

vec3 PointLight(int id, vec3 WorldPos, vec3 N, vec3 L);
vec3 DirectionLight(int id, vec3 N);
vec3 SpotLight(int id, vec3 WorldPos, vec3 N, vec3 L);


vec3 IntegrateEdgeVec(vec3 v1, vec3 v2);
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided);
vec3 AreaLight(int id, vec3 N, vec3 V, vec3 P, mat3 Minv, vec2 t2, vec3 albedo, vec3 F0);


void main(){

 
    vec3 albedo, normal;
    float roughness, metallic, ao;
    if(haveTexture){
        albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
        metallic = texture(metallicMap, TexCoords).r;
        roughness = texture(roughnessMap, TexCoords).r;
        ao = texture(aoMap, TexCoords).r;
        normal = getNormalFromMap();
    }else{
        albedo = _Albedo;
        normal = Normal;
        roughness = _Roughness;
        metallic = _Metallic;
        ao = _Ao;
    }

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);// 混合计算出F0

    // LTC相关参数
    float dotNV = clamp(dot(N, V), 0.0f, 1.0f);
    vec2 uv;
    vec4 t1,t2;
    mat3 Minv;
    if(HaveAreaLight){
        uv = vec2(0.2, sqrt(1.0f - dotNV));
        uv = uv*LUT_SCALE + LUT_BIAS;
        // get 4 parameters for inverse_M
        t1 = texture(LTC_1Map, uv);
        // Get 2 菲涅尔计算参数
        t2 = texture(LTC_2Map, uv);
        // M矩阵
        Minv = mat3(
            vec3(t1.x, 0, t1.y),
            vec3(  0,  1,    0),
            vec3(t1.z, 0, t1.w)
        );
    }

    vec3 Lo = vec3(0.0);
    vec3 DirLo = vec3(0.0);

    for(int i=0; i < lights_size; i++)
    {
        // radiance:
        vec3 H;
        vec3 L;
        vec3 radiance;
        if(lights[i].type == 0){
            L = normalize(lights[i].Position - WorldPos);
            H = normalize(V + L);
            radiance = PointLight(i, WorldPos, N, L);
            vec3 brdf = get_BRDF(N, L, H, V, F0, albedo, roughness, metallic);
            radiance = radiance * brdf;
        }
        else if(lights[i].type == 1){
            radiance = DirectionLight(i,N);
            L = -lights[i].Direction;
            H = normalize(V + L);
            vec3 brdf = get_BRDF(N, L, H, V, F0, albedo, roughness, metallic);
            radiance = radiance * brdf;
            DirLo += radiance;
        }
        else if(lights[i].type == 2){
            if(OpenSpot){
                L = -lights[i].Direction;
                H = normalize(V + L);
                radiance = SpotLight(i, WorldPos, N, L);
                vec3 brdf = get_BRDF(N, L, H, V, F0, albedo, roughness, metallic);
                radiance = radiance * brdf;
            }
        }
        else if(lights[i].type == 3){
            radiance = AreaLight(i, N, V, WorldPos, Minv, vec2(t2.rg), albedo, F0);
        }
        
        // cos值:
        float NdotL = max(dot(N, L), 0.0);        
        // brdf:
        
        // 累加各个光源的贡献

        Lo += radiance ;
    }

    // 环境光照:

    // 这里面的F是为将kd提出时简化过的

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    vec3 F = fresnelSchlickRoughness(F0, NoV,roughness);
    
    // kd:
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	 

    // 漫反射:
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // 镜面反射:

    const float MAX_REFLECTION_LOD = 4.0;// 0, 0.25, 0.5, 0.75
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient;
    vec3 color;
    float shadow;

    if(Shadow_type == 1){
        shadow = getShadow(N,ShadowDirection);
        shadow = (1.0 - shadow);
        
    }
    else if(Shadow_type == 2){
        shadow = getShadow_CSM(N,ShadowDirection);
        shadow = (1.0 - shadow);
    }
    else{
       shadow = 1.0;
    }

    ambient = (kD * diffuse + specular) * ao;

    
    color =  ambient + (Lo - DirLo) + DirLo * shadow;
    //color = DirLo;
    //color =  vec3(getShadow_CSM(N,ShadowDirection));
    //=color = ambient;

    vec4 fragPosViewSpace = view * vec4(WorldPos, 1.0); // 转换到相机视角中
    float depthValue = abs(fragPosViewSpace.z); 

    int layer = -1;// 分5级 0-4
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    //color = vec3((layer*1.0) / cascadeCount);

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    //color = vec3(texture(ShadowMap,TexCoords).r);
    //color = texture(LTC_2Map,TexCoords).rgb;
    //color = vec3(metallic);

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(0.3,0.2,0.3,1.0);
}

vec3 SpotLight(int id, vec3 WorldPos, vec3 N, vec3 L){
    vec3 radiance = vec3(0, 0, 0);

    float distance = length(lights[id].Position - WorldPos);
    if(distance > lights[id].Radius) return radiance;
    else{
        float attenuation = 1.0 / (1.0 + lights[id].Linear * distance + lights[id].Quadratic * distance * distance);
        radiance = lights[id].Color * attenuation;
        float theta = dot(L, normalize(lights[id].Position - WorldPos)); 
        float epsilon = (lights[id].CutOff - lights[id].outerCutOff);
        float intensity = clamp((theta - lights[id].outerCutOff) / epsilon, 0.0, 1.0);
        intensity = intensity * intensity;
        radiance = radiance * intensity;
    }

    return radiance;
}
vec3 DirectionLight(int id, vec3 N){

    float NdotL = max(dot(N, -lights[id].Direction), 0.0);
    vec3 radiance = lights[id].Color * NdotL;
    return radiance;
}
vec3 PointLight(int id, vec3 WorldPos, vec3 N, vec3 L){

    float distance = length(lights[id].Position - WorldPos);
    vec3 radiance= vec3(0,0,0);
    float NdotL = max(dot(N, L), 0.0);
    if(distance > lights[id].Radius) return radiance;
    else{
        float attenuation = 1.0 / (1.0 + lights[id].Linear * distance + lights[id].Quadratic * distance * distance);
        radiance = lights[id].Color * attenuation;
        return radiance * NdotL;
    }

}
vec3 AreaLight(int id, vec3 N, vec3 V, vec3 P, mat3 Minv, vec2 t2, vec3 albedo, vec3 F0){
    vec3 radiance = vec3 (0,0,0);

    // Evaluate LTC shading
    vec3 diffuse = LTC_Evaluate(N, V, P, mat3(1), lights[id].points, lights[id].twoface);
	vec3 specular = LTC_Evaluate(N, V, P, Minv, lights[id].points, lights[id].twoface);
    // 菲涅尔项的计算:
    //F0 = pow(vec3(0.23f, 0.23f, 0.23f),vec3(2.2));

	specular *= F0*t2.x + (1.0f - F0) * t2.y;

    radiance = lights[id].Color * lights[id].Intensity * (specular + albedo * diffuse);

    //radiance = lights[id].Color * lights[id].Intensity * specular;
    //radiance = lights[id].Color * (specular);
    //radiance = lights[id].Color;

    return radiance;
}

vec3 get_BRDF(vec3 N, vec3 L, vec3 H,vec3 V, vec3 F0, vec3 albedo, float roughness, float metallic){
    
    float VoH = clamp(dot(V, H), 0.0, 1.0);
	float NoH = clamp(dot(N, H), 0.0, 1.0);
	float NoL = clamp(dot(N, L), 0.0, 1.0);
	float NoV = clamp(dot(N, V), 0.0, 1.0);

    float D = D_GGX(roughness * roughness, NoH);
    float Vis = Vis_Smith(roughness, NoV, NoL);
    vec3 F = F_Schlick(F0, VoH);
    //vec3 F = F_Schlick(vec3(0.04),VoH);

    vec3 specularFactor = D * F * Vis;

    vec3 KD = vec3(1.0) - F;
	KD *= 1.0 - metallic;
	albedo *= KD;

    //vec3 diffuseFactor = Diffuse_OrenNayar(albedo, roughness, NoV, NoL, VoH);
    vec3 diffuseFactor = Diffuse_Burley(albedo, roughness, NoV, NoL, VoH);

    //return F;
    //return vec3(D);
    return diffuseFactor + specularFactor;
}
 //[Burley 2012，"迪士尼基于物理的着色"。］

vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // 构建TBN矩阵的三个基向量
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // 依据TBN矩阵旋转光源
    Minv = Minv * transpose(mat3(T1, T2, N));

    // 多边形四个顶点
    vec3 L[4];

    // 通过逆变换矩阵将顶点变换于 受约余弦分布 中
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // use tabulated horizon-clipped sphere
    // 判断着色点是否位于光源之后
    vec3 dir = points[0] - P; // LTC 空间
    vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) < 0.0);

    // 投影至单位球面上
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // 边缘积分
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);

    // 计算正半球修正所需要的的参数
    float len = length(vsum);

    float z = vsum.z/len;
    if (behind)
        z = -z;

    vec2 uv = vec2(z*0.5f + 0.5f, len); // range [0, 1]
    uv = uv*LUT_SCALE + LUT_BIAS;

    // 通过参数获得几何衰减系数
    float scale = texture(LTC_2Map, uv).w;

    float sum = len*scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // 输出
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    // 使用内置 acos() 函数会导致缺陷 
    // 使用拟合结果计算 acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}

vec3 Diffuse_Burley( vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
	float FD90 = 0.5 + 2.0 * VoH * VoH * Roughness;
	float FdV = 1.0 + (FD90 - 1.0) * Pow5( 1.0 - NoV );
	float FdL = 1.0 + (FD90 - 1.0) * Pow5( 1.0 - NoL );
	return DiffuseColor * ( (1.0 / PI) * FdV * FdL );
}
// [Gotanda 2012，"超越基于物理的简单布林-洪模型的实时性"]。
vec3 Diffuse_OrenNayar( vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
	float a = Roughness * Roughness;
	float s = a;// / ( 1.29 + 0.5 * a );
	float s2 = s * s;
	float VoL = 2.0 * VoH * VoH - 1.0;		// double angle identity
	float Cosri = VoL - NoV * NoL;
	float C1 = 1.0 - 0.5 * s2 / (s2 + 0.33);
	float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * ( Cosri >= 0.0 ? rcp( max( NoL, NoV ) ) : 1.0 );
	return DiffuseColor / PI * ( C1 + C2 ) * ( 1.0 + Roughness * 0.5 );
}
float D_GGX( float a2, float NoH )
{
	a2 = a2 * a2;
	float d = ( NoH * a2 - NoH ) * NoH + 1.0;	// 2 mad
	return a2 / ( PI*d*d );					// 4 mul, 1 rcp
}
float Vis_Smith( float a2, float NoV, float NoL )
{
	float Vis_SmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
	float Vis_SmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
	return rcp( Vis_SmithV * Vis_SmithL );
}
vec3 F_Schlick( vec3 SpecularColor, float VoH )
{
	float Fc = Pow5( 1.0 - VoH );					// 1 sub, 3 mul
	//return Fc + (1 - Fc) * SpecularColor;		// 1 add, 3 mad
	
	// 小于 2% 在物理上是不可能的，因此被视为阴影
	return clamp( 50.0 * SpecularColor.g, 0.0, 1.0) * Fc + (1.0 - Fc) * SpecularColor;
	
}
vec3 fresnelSchlickRoughness( vec3 F0, float cosTheta, float roughness )
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
float getShadow(vec3 N, vec3 L){

    float shadow;

    vec3 projCoords = LightPos.xyz / LightPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z>1.0){
        shadow = 0.0;
    }
    else{
        bool PCF = true;
        float currentDepth = projCoords.z;
        float bias = max(0.05 * (1.0 - max(dot(N, -L),0)), 0.001);
        int PCF_SIZE = 0;
        if(PCF){
            vec2 texelSize = 1.0 / textureSize(ShadowMap,0);
            for(int x = -1; x<=1; x++){
                for(int y = -1; y<=1; y++){
                    float pcfDepth = texture(ShadowMap, projCoords.xy + vec2(x,y)*texelSize).r;
                    shadow += (currentDepth - bias)> pcfDepth  ? 1.0 : 0.0;
                    PCF_SIZE++;
                }
            }
            shadow /= PCF_SIZE;
        }
        else{
            float closestDepth = texture(ShadowMap, projCoords.xy).r; 
            shadow = (currentDepth - bias)> closestDepth  ? 1.0 : 0.0;
        }
    }
    return shadow;
}

float getShadow_CSM(vec3 N, vec3 L){
    float shadow;
    // 选择级联层
    vec4 fragPosViewSpace = view * vec4(WorldPos, 1.0); // 转换到相机视角中
    float depthValue = abs(fragPosViewSpace.z); 

    int layer = -1;// 分5级 0-4
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(WorldPos, 1.0);// 将世界坐标转换到光源空间

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    // 当阴影位于光线的远平面区域之外时，阴影值保持为 0.0。
//    if (currentDepth > 1.0)
//    {
//        return 0.0;
//    }
    // 计算偏差（基于深度图分辨率和坡度）
    vec3 normal = normalize(N);
    float bias = max(0.05 * (1.0 - max(dot(N, -L),0)), 0.001);
    const float biasModifier = 0.5f;
    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    bool PCF = true;
    int PCF_SIZE = 0;

    if(PCF){
            vec2 texelSize = 1.0 / vec2(textureSize(ShadowMap_CSM, 0));
            for(int x = -1; x<=1; x++){
                for(int y = -1; y<=1; y++){
                    float pcfDepth = texture(ShadowMap_CSM, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
                    shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
                    PCF_SIZE++;
                }
            }
            shadow /= PCF_SIZE;

        }
        else{
            float closestDepth = texture(ShadowMap, projCoords.xy).r; 
            shadow = (currentDepth - bias)> closestDepth  ? 1.0 : 0.0;
        }

    //return (layer*1.0) / 4.0;
    float closestDepth = texture(ShadowMap, projCoords.xy).r; 

    return shadow ;
}