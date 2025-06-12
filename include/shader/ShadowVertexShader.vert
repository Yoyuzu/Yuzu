#version 460 core

layout(location = 0) in vec3 inVer3;//顶点
layout(set=0,binding = 0) uniform inMat4_3{
    mat4 m;//局部坐标系->世界坐标系
    mat4 v;//世界坐标系->摄像机坐标系
    mat4 p;//摄像机坐标系 * 投影变换(不规则缩小)
    mat4 sv;//阴影视图矩阵
    vec3 Light_pos;//灯光位置
    vec3 view_pos;//相机位置
}inMats;

void main() {
    //注意矩阵相乘顺序
    gl_Position = inMats.sv * vec4(inVer3, 1.0);
}