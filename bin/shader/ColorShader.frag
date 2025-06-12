#version 460 core

layout(location = 0) in vec2 fragTexCoord;//输入纹理坐标
layout(location = 1) in vec3 inColor;//输入颜色
layout(location = 2) in vec3 intNormal;//法线
layout(location = 3) in vec4 inTexcoords;//阴影贴图深度
layout(location = 4) in vec3 inLight;//光源位置
layout(location = 5) in vec3 inColor_pos;//片段位置
layout(location = 6) in vec3 inView_pos;//相机位置
//推送常量
layout(push_constant) uniform  Push_Constants{
    vec3 ka;//环境光颜色
    vec3 kd;//漫反射光颜色
    vec3 ks;//镜面光颜色
    vec3 Ns_Ni_d;//反射高光度_折射值_不透明度
    uint index;
}push_Constants;

//组合图像采样器
layout (constant_id = 0) const uint TexSampler_count = 1;
layout(set=1,binding = 1) uniform sampler2D texSampler[TexSampler_count];
//阴影贴图
layout(set=2,binding = 2) uniform sampler2D ShadowSampler;
//layout(binding = 2) uniform sampler2D ShadowSampler;

layout(location = 0) out vec4 outColor;

//阴影映射
float shadowMap();
const vec3 light_color = vec3(1.0,1.0,1.0);
void main() {
    vec4 Color = texture(texSampler[0], fragTexCoord);
    //环境颜色
    float wigth = 0.50;
    const float shadow = Color.a * shadowMap();
    //漫反射
    vec3 light = normalize(inLight - inColor_pos);
    wigth = wigth + max(dot(intNormal,light),0.0) * shadow;
    //镜面反射
    vec3 viewDir = normalize(inView_pos - inColor_pos.xyz);
    vec3 halfwayDir = normalize(light + viewDir); 
    wigth = wigth +  pow(max(dot(intNormal, halfwayDir),0.0),32.0) * shadow;
    outColor = vec4(Color.rgb * light_color * wigth,Color.a);
}

float shadowMap(){
    const vec2 coord = vec2(inTexcoords.xy * 0.5000 + 0.5000);
    float shadow = 0.0;
    const vec2 shadow_image_xy = vec2(1.0/3840.0000,1.0/2160.0000) * 0.0080;
    for (int y = -150; y <= 150; y += 25 ){
        for (int x = 150; x >= -150; x -= 25 ){
            shadow += texture(ShadowSampler,vec2(coord + vec2(x, y) * shadow_image_xy)).r > inTexcoords.z ? 1.0:0.0;
        }
    }
    return shadow / 144.0;
}


