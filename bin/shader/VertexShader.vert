#version 460 core

layout(location = 0) in vec3 inVer3;//顶点
layout(location = 1) in vec3 inNormal;//顶点法线
layout(location = 2) in vec3 inColor;//顶点颜色
layout(location = 3) in vec2 inFragTexCoord;//纹理坐标
layout(set=0,binding = 0) uniform inMat4_3{
    mat4 m;//局部坐标系->世界坐标系
    mat4 v;//世界坐标系->摄像机坐标系
    mat4 p;//摄像机坐标系 * 投影变换(不规则缩小)
    mat4 sv;//阴影视图矩阵
    vec3 Light_pos;//灯光位置
    vec3 view_pos;//相机位置
}inMats;

layout(location = 0) out vec2 outFragTexCoord;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outTexcoords;
layout(location = 4) out vec3 outLight;
layout(location = 5) out vec3 outColor_pos;
layout(location = 6) out vec3 outView_pos;

void main() {
    vec4 pos = vec4(inVer3, 1.0);
    mat4 pvm_mat = inMats.p * inMats.v * inMats.m;
    mat4 t_normal = transpose(inverse(inMats.m));
    outNormal = normalize(mat3(t_normal) * inNormal);
    outLight = inMats.Light_pos;
    outColor_pos = vec3(inMats.m * pos);
    outTexcoords = inMats.sv * pos;
    outView_pos = vec3(inMats.m * vec4(inMats.view_pos,1.0));
    outFragTexCoord = inFragTexCoord;
    //outColor = inColor;
    //注意矩阵相乘顺序
    gl_Position =  pvm_mat * pos;
}