#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// G-buffer
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gMaterial;
uniform sampler2D gAlbedo;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// LTC
uniform sampler2D LTC_1Map;
uniform sampler2D LTC_2Map;

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
uniform bool Have_AreaLight;

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

vec3 get_BRDF(vec3 N, vec3 L, vec3 H,vec3 V, vec3 F0,vec3 albedo, float roughness, float metallic);

vec3 PointLight(int id, vec3 WorldPos, vec3 N, vec3 L);
vec3 DirectionLight(int id, vec3 N);
vec3 SpotLight(int id, vec3 WorldPos, vec3 N, vec3 L);

vec3 IntegrateEdgeVec(vec3 v1, vec3 v2);
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided);
vec3 AreaLight(int id, vec3 N, vec3 V, vec3 P, mat3 Minv, vec2 t2, vec3 albedo, vec3 F0);

void main(){

    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    float metallic = texture(gMaterial, TexCoords).r;
    float roughness = texture(gMaterial, TexCoords).g;
    float ao = texture(gMaterial, TexCoords).b;
    
    //albedo =vec3(0.7f, 0.8f, 0.96f);


    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);// ��ϼ����F0

    // LTC��ز���
    float dotNV = clamp(dot(N, V), 0.0f, 1.0f);
    vec2 uv;
    vec4 t1,t2;
    mat3 Minv;
    if(Have_AreaLight){
        uv = vec2(0.2, sqrt(1.0f - dotNV));
        uv = uv*LUT_SCALE + LUT_BIAS;
        // get 4 parameters for inverse_M
        t1 = texture(LTC_1Map, uv);
        // Get 2 �������������
        t2 = texture(LTC_2Map, uv);
        // M����
        Minv = mat3(
            vec3(t1.x, 0, t1.y),
            vec3(  0,  1,    0),
            vec3(t1.z, 0, t1.w)
        );
    }
    // reflectance equation
    vec3 Lo = vec3(0.0);

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

        // �ۼӸ�����Դ�Ĺ���
        Lo +=   radiance ;

    }

    // ��������:

    // �������F��Ϊ��kd���ʱ�򻯹���

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    vec3 F = fresnelSchlickRoughness(F0, NoV,roughness);
    
    // kd:
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	 

    // ������:
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // ���淴��:

    const float MAX_REFLECTION_LOD = 4.0;// 0, 0.25, 0.5, 0.75
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    vec3 color =  ambient + Lo;
    //color = Lo;


    // HDR tonemapping
    //color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    //color = texture(LTC_1Map,TexCoords).rgb;

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
    // ��������ļ���:
    F0 = pow(vec3(0.23f, 0.23f, 0.23f),vec3(2.2));

	//specular *= F0*t2.x + (1.0f - F0) * t2.y;

    radiance = lights[id].Color * lights[id].Intensity * (specular + albedo * diffuse);

    //radiance = lights[id].Color * lights[id].Intensity * specular;
    //radiance = lights[id].Color * (specular);
    //radiance = lights[id].Color;

    return radiance;
}
// unrealBRDF:
vec3 get_BRDF(vec3 N, vec3 L, vec3 H,vec3 V, vec3 F0, vec3 albedo, float roughness, float metallic){
    
    float VoH = clamp(dot(V, H), 0.0, 1.0);
	float NoH = clamp(dot(N, H), 0.0, 1.0);
	float NoL = clamp(dot(N, L), 0.0, 1.0);
	float NoV = clamp(dot(N, V), 0.0, 1.0);

    float D = D_GGX(roughness * roughness, NoH);
    float Vis = Vis_Smith(roughness, NoV, NoL);
    vec3 F = F_Schlick(F0, VoH);

    vec3 specularFactor = D * F * Vis;

    vec3 KD = vec3(1.0) - F;
	KD *= 1.0 - metallic;
	albedo *= KD;

    //vec3 diffuseFactor = Diffuse_OrenNayar(albedo, roughness, NoV, NoL, VoH);
    vec3 diffuseFactor = Diffuse_Burley(albedo, roughness, NoV, NoL, VoH);

    return diffuseFactor + specularFactor;
}
// LTC:
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    // ʹ������ acos() �����ᵼ��ȱ�� 
    // ʹ����Ͻ������ acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // ����TBN���������������
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // ����TBN������ת��Դ
    Minv = Minv * transpose(mat3(T1, T2, N));

    // ������ĸ�����
    vec3 L[4];

    // ͨ����任���󽫶���任�� ��Լ���ҷֲ� ��
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // use tabulated horizon-clipped sphere
    // �ж���ɫ���Ƿ�λ�ڹ�Դ֮��
    vec3 dir = points[0] - P; // LTC �ռ�
    vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) < 0.0);

    // ͶӰ����λ������
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // ��Ե����
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);

    // ������������������Ҫ�ĵĲ���
    float len = length(vsum);

    float z = vsum.z/len;
    if (behind)
        z = -z;

    vec2 uv = vec2(z*0.5f + 0.5f, len); // range [0, 1]
    uv = uv*LUT_SCALE + LUT_BIAS;

    // ͨ��������ü���˥��ϵ��
    float scale = texture(LTC_2Map, uv).w;

    float sum = len*scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // ���
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}

// [Burley 2012��"��ʿ������������ɫ"����
vec3 Diffuse_Burley( vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
	float FD90 = 0.5 + 2.0 * VoH * VoH * Roughness;
	float FdV = 1.0 + (FD90 - 1.0) * Pow5( 1.0 - NoV );
	float FdL = 1.0 + (FD90 - 1.0) * Pow5( 1.0 - NoL );
	return DiffuseColor * ( (1.0 / PI) * FdV * FdL );
}
// [Gotanda 2012��"��Խ��������ļ򵥲���-��ģ�͵�ʵʱ��"]��
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
	
	// С�� 2% ���������ǲ����ܵģ���˱���Ϊ��Ӱ
	return clamp( 50.0 * SpecularColor.g, 0.0, 1.0) * Fc + (1.0 - Fc) * SpecularColor;
	
}
vec3 fresnelSchlickRoughness( vec3 F0, float cosTheta, float roughness )
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}