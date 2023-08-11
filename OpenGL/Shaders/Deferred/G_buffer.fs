#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gMaterial;
layout (location = 3) out vec3 gAlbedo;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

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
//vec3 othernormal(){
//    vec3 nn;
//    vec4 normalInput;
//    normalInput = vec4(texture(normalMap,TexCoords).rgb,1.0);
//    nn.xy = normalInput.ag * 2.0 - 1.0;
//    nn.z = sqrt(max(1.0 - dot(nn.xy, nn.xy), 0.0));
//
//    /return normal;
//}
//
void main(){
	vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao = texture(aoMap, TexCoords).r;

    gPosition = WorldPos;
    gNormal = getNormalFromMap();
    gMaterial = vec3(metallic, roughness, ao);
    gAlbedo = albedo;


}