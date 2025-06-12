#ifndef VERTEX_FROM_HPP
#define VERTEX_FROM_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_LEFT_HANDED
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE //透视投影矩阵将使用 vulkan 的深度范围 
#include "include/glm/glm.hpp"
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include "include/glm/gtx/hash.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "include/stbs/stb_image.h"


//#define TINYOBJLOADER_USE_DOUBLE
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_DONOT_INCLUDE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include "include/loader/tiny_obj_loader.h"

#include <fstream>
VERTEX_FROM_HPP char* getShaderSPVcode(const char*,uint64_t*);
#include <string>
#include <iostream>


#ifndef __cplusplus
    extern "C++"{
#endif

#define VERTEX_SHADER_FINE_1 "E:/keke/include/shader/VertexShader.spv\0"
#define COLOR_SHADER_FINE_1 "E:/keke/include/shader/ColorShader.spv\0"
#define SHADOW_VERTEX_SHADER_FINE_1 "E:/keke/include/shader/ShadowVertexShader.spv\0"
#define SHADOW_COLOR_SHADER_FINE_1 "E:/keke/include/shader/ShadowColorShader.spv\0"
const std::string VERTEX_OPJ_BASE_FINE_1 = "E:/keke/bin/";
const std::string VERTEX_OPJ_FINE_1 = "E:/keke/bin/1.obj";
VERTEX_FROM_HPP typedef struct uinformForm_1{
    alignas(16) glm::mat4 m;//模型矩阵
    alignas(16) glm::mat4 v;//观察视图
    alignas(16) glm::mat4 p;//透视视图
    alignas(16) glm::mat4 sv;//灯光视图
    alignas(16) glm::vec3 Light_pos;//灯光位置
    alignas(16) glm::vec3 view_pos;//相机位置
}uinformForm_1;
//顶点输入格式
VERTEX_FROM_HPP typedef struct Vertex_1 {
    glm::vec3 pos;
    glm::vec3 pos_vn;
    glm::vec3 color;
    glm::vec2 texCoord;
    
    bool operator==(const Vertex_1& other) const {
        return pos == other.pos && pos_vn == other.pos_vn && color == other.color && texCoord == other.texCoord;
    }
}Vertex_1;
//推送常量格式
typedef struct Push_Constant{
    alignas(16) glm::vec3 ka;//环境光颜色
    alignas(16) glm::vec3 kd;//漫反射光颜色
    alignas(16) glm::vec3 ks;//镜面光颜色
    alignas(16) glm::vec3 Ns_Ni_d;//反射高光度_折射值_不透明度
    alignas(16) uint32_t image_index;
}Push_Constant;

#ifdef _UNORDERED_MAP_ 
    namespace std {
        //分离重复顶点
        template<> struct hash<Vertex_1> {
            size_t operator()(Vertex_1 const& vertex) const {
                return ((hash<glm::vec3>()(vertex.pos)  ^ (hash<glm::vec3>()(vertex.pos_vn) << 1) ^ (hash<glm::vec3>()(vertex.color)<< 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
            }
        };
    }
#endif

VERTEX_FROM_HPP typedef struct inVertexForm_1{
    std::vector<Vertex_1> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex_1, uint32_t> uniqueVertices{};
    
    void loader(std::vector<uint64_t> &index_instances,//每个索引实例的最后一个索引的位置
                std::vector< std::string > &vertex_map_urls,//每个索引实例的材质链接
                std::vector<uint32_t> &image_indexs,//每个索引实例的纹理在纹理缓冲区中的索引
                std::vector<Push_Constant> &push_Constants//每个纹理的属性
    ){
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::vector<uint32_t> temp;
        std::string warn, err;
        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,VERTEX_OPJ_FINE_1.c_str(),VERTEX_OPJ_BASE_FINE_1.c_str());
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex_1 vertex{};
                
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.pos_vn = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
                
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0F - attrib.texcoords[2 * index.texcoord_index + 1] //这里反转了纹理坐标
                };

                vertex.color = { 0.5f, 0.5f, 0.5f };
                
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
            for (const auto& id : shape.mesh.material_ids) temp.push_back(id);//把纹理id放到同一个容器中方便遍历.
        }
        for (uint64_t i=0; i < temp.size(); i++) {
            //纹理id不同时记录索引位置
            if (temp[i] == temp[i+1]) continue;
            index_instances.push_back( (i+1) * 3);
            std::cout << index_instances.back()<<" "<< index_instances.size() << std::endl;
        }
        temp.~vector();
        //当只有一个纹理时,实现就简单多了
        if(index_instances.size() == 0) index_instances.push_back(indices.size());

        uint64_t size=0ULL;
        char *p=getShaderSPVcode(VERTEX_OPJ_FINE_1.c_str(),&size);
        for(uint64_t i=0ULL;i<size - 6;i++){
            if(p[i] == 'u' && p[i+1] == 's' && p[i+2] == 'e' && p[i+3] == 'm' && p[i+4] =='t' && p[i+5] == 'l'){
                std::string mtl;
                for(;p[i+7] != '\n';i++) mtl.push_back(p[i+7]);
                for(uint32_t mat_i = 0U;mat_i<materials.size();mat_i++){
                    if(materials[mat_i].name == mtl){
                        if(materials[mat_i].diffuse_texname != ""){
                            
                            VkBool32 flag = VK_TRUE;
                            for (uint32_t i2 = 0; i2<vertex_map_urls.size(); i2++) {
                                //重复图片使用同一个图片索引
                                if(vertex_map_urls[i2] == materials[i2].diffuse_texname){
                                    image_indexs.push_back(i2);
                                    flag = VK_FALSE;
                                    break;
                                }
                            }
                            //如果不重复添加新的图片和索引
                            if(flag){
                                image_indexs.push_back(vertex_map_urls.size());
                                vertex_map_urls.push_back(VERTEX_OPJ_BASE_FINE_1 + materials[mat_i].diffuse_texname);
                                const Push_Constant image_info ={
                                    glm::vec3(materials[mat_i].ambient[0],materials[mat_i].ambient[1],materials[mat_i].ambient[2]),
                                    glm::vec3(materials[mat_i].diffuse[0],materials[mat_i].diffuse[1],materials[mat_i].diffuse[2]),
                                    glm::vec3(materials[mat_i].specular[0],materials[mat_i].specular[1],materials[mat_i].specular[2]),
                                    glm::vec3(materials[mat_i].shininess,materials[mat_i].ior,materials[mat_i].dissolve)
                                };
                                push_Constants.push_back(image_info);
                            }
                            std::cout << image_indexs.back() << " " << image_indexs.size() << std::endl;
                        
                        }else {
                            abort();
                        }
                        break;
                    }
                }
            }
        }
        delete[] p;
        if(index_instances.size() != image_indexs.size()) abort();
        if (vertex_map_urls.size() == 0) abort();

        //Shader_in_mesh mesh = {};
        //load3DFile("E:/keke/bin/", "E:/keke/bin/1.obj",&mesh);

        //index_instances = v_data->mesh_counts;
        //image_indexs = v_data->image_indexs;
        //vertex_map_urls = v_data->image_paths;
    }
}inVertexForm_1;

VERTEX_FROM_HPP char* getShaderSPVcode(const char *pFLIE,uint64_t *pSize){
    std::ifstream in(pFLIE,std::ios::ate | std::ios::binary | std::ios::in);
    *pSize=in.tellg();
    in.seekg(0,std::ios::beg);

    char *pDatas=new char[*pSize];
    in.read(pDatas,*pSize);
    in.close();
    return pDatas;
}

#ifndef __cplusplus
    }
#endif
#endif //VERTEX_FROM_HPP
