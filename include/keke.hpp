#ifndef KEKE_HPP
#define KEKE_HPP "keke"

#define SHADOW_W 1920U * 2U
#define SHADOW_H 1080U * 2U


#ifndef __cplusplus
    extern "C++"{
#endif

#define VK_USE_PLATFORM_WIN32_KHR //bing win32
#include "include\vulkan\vulkan.h"//https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/index.html
#pragma comment(lib,"../keke/lib/vulkan/vulkan-1")
#include "include/shader/VertexFrom.hpp"
#include <memory>

VkPhysicalDevice enumeratePhysicalDevice(VkInstance instance){
    uint32_t gpu_count;
    vkEnumeratePhysicalDevices(instance,&gpu_count,nullptr);
    VkPhysicalDevice *pGpus=new VkPhysicalDevice[gpu_count];
    vkEnumeratePhysicalDevices(instance,&gpu_count,pGpus);

    VkPhysicalDevice GPU=VK_NULL_HANDLE;
    GPU=pGpus[0];
    
    //优先使用独立显卡
    for(uint32_t i=0U;i<gpu_count;i++){
        VkPhysicalDeviceProperties gpu_info{};
        vkGetPhysicalDeviceProperties(pGpus[i],&gpu_info);
        if(gpu_info.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
            GPU=pGpus[i];
        }
    }
    delete[] pGpus;
    return GPU;
}

typedef struct vk_device_swapchain{
    VkDevice device;
    VkQueue queue;
    VkSwapchainKHR swapchain;
    static const uint32_t Image_count=2U;
    VkImage images[Image_count];
    VkImageView views[Image_count];
}vk_device_swapchain;

std::shared_ptr<vk_device_swapchain> keke(VkInstance instance,VkPhysicalDevice GPU,VkSurfaceKHR surface,uint32_t surface_w,uint32_t surface_h,VkFormat surface_format,VkColorSpaceKHR surface_colorSpace){
    std::shared_ptr<vk_device_swapchain>device_swapchain(new vk_device_swapchain({}),[](vk_device_swapchain *p){
        for(uint32_t i=0U;i<p->Image_count;i++)
            if(p->views[i]) vkDestroyImageView(p->device,p->views[i],nullptr);
        if(p->swapchain) vkDestroySwapchainKHR(p->device,p->swapchain,nullptr);
        if(p->device) vkDestroyDevice(p->device,nullptr);
        delete p;
    });
    
    [GPU,instance](VkDevice *pDevice,VkQueue *pQueue){
        VkDeviceQueueCreateInfo q={};
        q.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.pNext=nullptr;
        q.flags=0U;
        q.queueFamilyIndex=0U;
        q.queueCount=1U;
        const float QueuePriorities[]={1.0F};
        q.pQueuePriorities=QueuePriorities;

        VkDeviceCreateInfo info={};
        info.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext=nullptr;
        info.flags=0U;
        info.enabledLayerCount=0U;
        info.ppEnabledLayerNames=nullptr;
        const char* Externs[]={
            "VK_KHR_swapchain"
        };
        info.enabledExtensionCount=1U;
        info.ppEnabledExtensionNames=Externs;
        
        VkPhysicalDeviceFeatures _deviceFeatures{};
        vkGetPhysicalDeviceFeatures(GPU,&_deviceFeatures);
        VkPhysicalDeviceFeatures deviceFeatures{};
        if(_deviceFeatures.samplerAnisotropy) deviceFeatures.samplerAnisotropy = VK_TRUE;//开启各项异性过滤
        if(_deviceFeatures.sampleRateShading) deviceFeatures.sampleRateShading = VK_TRUE;//开启样品采样
        info.pEnabledFeatures=&deviceFeatures;
        
        info.queueCreateInfoCount=1U;
        info.pQueueCreateInfos=&q;
        (((PFN_vkCreateDevice) vkGetInstanceProcAddr(instance,"vkCreateDevice")) (GPU,&info,nullptr,pDevice));
        vkGetDeviceQueue(*pDevice,0U,0U,pQueue);
    }(&device_swapchain->device,&device_swapchain->queue);

    [=]()mutable{
        VkSwapchainCreateInfoKHR info={};
        info.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.pNext=nullptr;
        info.flags=0U;
        info.surface=surface;
        info.minImageCount=device_swapchain->Image_count;
        info.imageExtent.width=surface_w;
        info.imageExtent.height=surface_h;
        info.imageFormat= surface_format;
        info.imageColorSpace=surface_colorSpace;
        info.imageArrayLayers=1U;
        info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.presentMode=VK_PRESENT_MODE_FIFO_KHR;
        VkSurfaceCapabilitiesKHR c;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU, surface, &c);
        info.preTransform=c.currentTransform;
        info.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0U;
        info.pQueueFamilyIndices=nullptr;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.clipped=VK_TRUE;
        info.oldSwapchain=VK_NULL_HANDLE;

        vkCreateSwapchainKHR(device_swapchain->device, &info, nullptr, &device_swapchain->swapchain);
    }();
    //创建交换链图像视图
    uint32_t swapchain_image_count=device_swapchain->Image_count;
    vkGetSwapchainImagesKHR(device_swapchain->device,device_swapchain->swapchain,&swapchain_image_count,device_swapchain->images);
    for(uint32_t i=0U;i<device_swapchain->Image_count;i++){
        VkImageViewCreateInfo info={};
        info.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext=nullptr;
        info.flags=0U;
        info.image=device_swapchain->images[i];
        info.format=surface_format;
        info.viewType=VK_IMAGE_VIEW_TYPE_2D;
        info.components={
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        info.subresourceRange={
            VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1
        };

        vkCreateImageView(device_swapchain->device,&info,nullptr,&device_swapchain->views[i]);
    }

    return device_swapchain;
}

typedef struct vk_render_instances{
    static const uint32_t Shader_Count=4U;
    static const uint32_t Set_Layout_Count=3U;
    VkShaderModule shaders[Shader_Count];
    VkShaderStageFlagBits shader_stages[Shader_Count];
    VkRenderPass pass,shadow_map_pass;
    VkDescriptorSetLayout setLayouts[Set_Layout_Count];
    VkPipelineLayout layout;
    VkPipeline shadow_map_pipline,pipline;
    VkFramebuffer freames[vk_device_swapchain::Image_count],shadow_map_freames[vk_device_swapchain::Image_count];
}vk_render_instances;

std::shared_ptr<vk_render_instances> createRenderInstance(const std::shared_ptr<vk_device_swapchain> &Device_swapchain,uint32_t surface_w,uint32_t surface_h,VkFormat surface_format,VkFormat depth_format,VkImageView depth_image_view,VkImageView colorResources_view, VkSampleCountFlagBits numSamples,const VkImageView *pShadow_views,VkPhysicalDevice GPU){
    std::shared_ptr<vk_render_instances> render_instance(new vk_render_instances({}),[&](vk_render_instances *p){
        if(p->shadow_map_pipline) vkDestroyPipeline(Device_swapchain->device,p->shadow_map_pipline,nullptr);
        if(p->pipline) vkDestroyPipeline(Device_swapchain->device,p->pipline,nullptr);
        if(p->layout) vkDestroyPipelineLayout(Device_swapchain->device,p->layout,nullptr);
        for(uint32_t i=0U;i<p->Set_Layout_Count;i++)
            if(p->setLayouts[i]) vkDestroyDescriptorSetLayout(Device_swapchain->device,p->setLayouts[i],nullptr);
        for(uint32_t i=0U;i<vk_device_swapchain::Image_count;i++){
            if(p->shadow_map_freames[i]) vkDestroyFramebuffer(Device_swapchain->device,p->shadow_map_freames[i],nullptr);
            if(p->freames[i]) vkDestroyFramebuffer(Device_swapchain->device,p->freames[i],nullptr);
        }
        if(p->pass) vkDestroyRenderPass(Device_swapchain->device,p->pass,nullptr);
        if(p->shadow_map_pass) vkDestroyRenderPass(Device_swapchain->device,p->shadow_map_pass,nullptr);
        for(uint32_t i=0U;i<p->Shader_Count;i++)
            if(p->shaders[i]) vkDestroyShaderModule(Device_swapchain->device,p->shaders[i],nullptr);
        delete p;
    });
    VkPhysicalDeviceFeatures gpu_eatures{};
    vkGetPhysicalDeviceFeatures(GPU, &gpu_eatures);
    
    VkShaderModuleCreateInfo shader_info={};
    shader_info.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.pNext=nullptr;
    shader_info.flags=0U;
    //创建顶点着色器模块
    const uint32_t VertexShaderIndex = 0U;
    render_instance->shader_stages[VertexShaderIndex] = VK_SHADER_STAGE_VERTEX_BIT;
    shader_info.pCode = (uint32_t*)getShaderSPVcode(VERTEX_SHADER_FINE_1, (uint64_t*)&shader_info.codeSize);
    vkCreateShaderModule(Device_swapchain->device, &shader_info, nullptr, &render_instance->shaders[VertexShaderIndex]);
    delete[] shader_info.pCode;
    //创建颜色着色器模块
    const uint32_t ColorShaderIndex = 1U;
    render_instance->shader_stages[ColorShaderIndex] = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_info.pCode = (uint32_t*)getShaderSPVcode(COLOR_SHADER_FINE_1, (uint64_t*)&shader_info.codeSize);
    vkCreateShaderModule(Device_swapchain->device, &shader_info, nullptr, &render_instance->shaders[ColorShaderIndex]);
    delete[] shader_info.pCode;
    //创建阴影贴图顶点着色器模块
    const uint32_t ShadowVertexShaderIndex = 2U;
    render_instance->shader_stages[ShadowVertexShaderIndex] = VK_SHADER_STAGE_VERTEX_BIT;
    shader_info.pCode = (uint32_t*)getShaderSPVcode(SHADOW_VERTEX_SHADER_FINE_1, (uint64_t*)&shader_info.codeSize);
    vkCreateShaderModule(Device_swapchain->device, &shader_info, nullptr, &render_instance->shaders[ShadowVertexShaderIndex]);
    delete[] shader_info.pCode;
    //创建阴影贴图颜色着色器模块
    const uint32_t ShadowColorShaderIndex = 3U;
    render_instance->shader_stages[ShadowColorShaderIndex] = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_info.pCode = (uint32_t*)getShaderSPVcode(SHADOW_COLOR_SHADER_FINE_1, (uint64_t*)&shader_info.codeSize);
    vkCreateShaderModule(Device_swapchain->device, &shader_info, nullptr, &render_instance->shaders[ShadowColorShaderIndex]);
    delete[] shader_info.pCode;
    //创建管道时用到的着色器信息
    VkPipelineShaderStageCreateInfo shaderStages[vk_render_instances::Shader_Count] = { {},{} };
    for(uint32_t i=0U;i<vk_render_instances::Shader_Count;i++){
        shaderStages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].pNext=nullptr;
        shaderStages[i].flags=0U;
        shaderStages[i].module = render_instance->shaders[i];
        shaderStages[i].stage = render_instance->shader_stages[i];
        shaderStages[i].pName = "main\0";
        shaderStages[i].pSpecializationInfo = nullptr;
    }
    //顶点输入信息
    const uint32_t CertexBindingDescriptions_Count = 1U;
    VkVertexInputBindingDescription vertexBindingDescriptions[CertexBindingDescriptions_Count];
    vertexBindingDescriptions[0].binding = 0U;
    vertexBindingDescriptions[0].stride =sizeof(Vertex_1);
    vertexBindingDescriptions[0].inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
    const uint32_t VertexAttributeDescriptions_Count = 4U;
    VkVertexInputAttributeDescription vertexAttributeDescriptions[VertexAttributeDescriptions_Count];
    vertexAttributeDescriptions[0].binding = 0U;
    vertexAttributeDescriptions[0].location = 0U;
    vertexAttributeDescriptions[0].offset = offsetof(Vertex_1,pos);
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].binding = 0U;
    vertexAttributeDescriptions[1].location = 1U;
    vertexAttributeDescriptions[1].offset = offsetof(Vertex_1,pos_vn);
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[2].binding=0U;
    vertexAttributeDescriptions[2].location=2U;
    vertexAttributeDescriptions[2].format=VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[2].offset=offsetof(Vertex_1,color);
    vertexAttributeDescriptions[3].binding=0U;
    vertexAttributeDescriptions[3].location=3U;
    vertexAttributeDescriptions[3].format=VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescriptions[3].offset=offsetof(Vertex_1,texCoord);
    VkPipelineVertexInputStateCreateInfo vertexInputInfo={};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = CertexBindingDescriptions_Count;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = VertexAttributeDescriptions_Count;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions; // Optional
    //顶点装配信息
    VkPipelineInputAssemblyStateCreateInfo inputAssembly={};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    //观景港和剪刀
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)surface_w;
    viewport.height = (float)surface_h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {surface_w,surface_h};
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    //光栅化配置
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    //多采样信息
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = gpu_eatures.sampleRateShading;
    multisampling.rasterizationSamples = numSamples;
    multisampling.minSampleShading = .1; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
    //深度缓冲区
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
    //颜色混合子附件
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //颜色混合
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional
    //动态状态
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext=nullptr;
    dynamicState.flags=0U;
    dynamicState.dynamicStateCount = 0U;
    dynamicState.pDynamicStates = nullptr;
    //管道描述符创建信息
    const uint32_t UboLayoutBindingCount=3U;
    VkDescriptorSetLayoutBinding uboLayoutBindings[UboLayoutBindingCount]{};
    //变换矩阵
    uboLayoutBindings[0].binding = 0;
    uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindings[0].descriptorCount = 1;
    uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBindings[0].pImmutableSamplers = nullptr;
    //图像采样器
    uboLayoutBindings[1].binding=1U;
    uboLayoutBindings[1].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uboLayoutBindings[1].descriptorCount= 1U;
    uboLayoutBindings[1].pImmutableSamplers=nullptr;
    uboLayoutBindings[1].stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
    //阴影贴图
    uboLayoutBindings[2].binding = 2U;
    uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uboLayoutBindings[2].descriptorCount = 1U;
    uboLayoutBindings[2].pImmutableSamplers = nullptr;
    uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo set_Layout_infos [ vk_render_instances::Set_Layout_Count ] {};
    set_Layout_infos[0].sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_Layout_infos[0].pNext=nullptr;
    set_Layout_infos[0].flags=0;
    set_Layout_infos[0].bindingCount=1;
    set_Layout_infos[0].pBindings=uboLayoutBindings;
    set_Layout_infos[1].sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_Layout_infos[1].pNext=nullptr;
    set_Layout_infos[1].flags=0;
    set_Layout_infos[1].bindingCount=1;
    set_Layout_infos[1].pBindings=uboLayoutBindings + 1;
    set_Layout_infos[2].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_Layout_infos[2].pNext = nullptr;
    set_Layout_infos[2].flags = 0;
    set_Layout_infos[2].bindingCount = 1;
    set_Layout_infos[2].pBindings = uboLayoutBindings + 2;
    for(uint32_t i=0U;i<vk_render_instances::Set_Layout_Count;i++)
        vkCreateDescriptorSetLayout(Device_swapchain->device,set_Layout_infos + i,nullptr,( render_instance -> setLayouts + i ) );
    //管道推送常量
    const uint32_t PushConstantRange_Count=1U;
    VkPushConstantRange pushConstantRanges[PushConstantRange_Count];
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset=0;
    pushConstantRanges[0].size=sizeof(Push_Constant);
    //管道布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = vk_render_instances :: Set_Layout_Count; // Optional
    pipelineLayoutInfo.pSetLayouts = render_instance -> setLayouts; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = PushConstantRange_Count; // Optional
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges; // Optional
    vkCreatePipelineLayout(Device_swapchain->device,&pipelineLayoutInfo,nullptr,&render_instance->layout);
    //渲染通行证
    const uint32_t Attachments_Count=3U;
    VkAttachmentDescription attachments[Attachments_Count]{};
    //多重采样颜色附件
    attachments[0].format=surface_format;
    attachments[0].samples=numSamples;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //深度模板附件
    attachments[1].format = depth_format;
    attachments[1].samples = numSamples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //交换链附件
    attachments[2].format=surface_format;
    attachments[2].samples=VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //颜色附件数量
    const uint32_t ColorAttachmentRef_Count=1U;
    //多重采样颜色附件
    VkAttachmentReference colorAttachmentRefs[ColorAttachmentRef_Count]{};
    colorAttachmentRefs[0].attachment = 0;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //深度模板子附件
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //解析附件目标->交换链接图像
    VkAttachmentReference colorResources;
    colorResources.attachment=2;
    colorResources.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //子通行证描述
    const uint32_t Subpass_Count=1U;
    VkSubpassDescription subpass[Subpass_Count]{};
    subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass[0].colorAttachmentCount = ColorAttachmentRef_Count;
    subpass[0].pColorAttachments =colorAttachmentRefs;
    subpass[0].pDepthStencilAttachment=&depthAttachmentRef;
    subpass[0].pResolveAttachments= &colorResources;
    //子通行证依赖性描述
    const uint32_t DependencyCount=1U;
    VkSubpassDependency dependencys[DependencyCount]{};
    dependencys[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencys[0].dstSubpass = 0;
    dependencys[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencys[0].srcAccessMask = 0;
    dependencys[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencys[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    //通行证描述
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = Attachments_Count;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = Subpass_Count;
    renderPassInfo.pSubpasses = subpass;
    renderPassInfo.dependencyCount=DependencyCount;
    renderPassInfo.pDependencies=dependencys;
    vkCreateRenderPass(Device_swapchain->device,&renderPassInfo,nullptr,&render_instance->pass);
    //阴影贴图附件
    VkAttachmentDescription shadow_attachment{};
    shadow_attachment.format = depth_format;
    shadow_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    shadow_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadow_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadow_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadow_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    shadow_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadow_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkAttachmentReference shadow_Reference{};
    shadow_Reference.attachment=0;
    shadow_Reference.layout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription shadow_subass{};
    shadow_subass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    shadow_subass.pDepthStencilAttachment = &shadow_Reference;
    VkSubpassDependency shadow_deoendencys[2]={{},{}};
    shadow_deoendencys[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    shadow_deoendencys[0].dstSubpass = 0;
    shadow_deoendencys[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    shadow_deoendencys[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    shadow_deoendencys[0].srcAccessMask =  VK_ACCESS_SHADER_READ_BIT;
    shadow_deoendencys[0].dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadow_deoendencys[0].dependencyFlags =  VK_DEPENDENCY_BY_REGION_BIT;
    shadow_deoendencys[1].srcSubpass = 0;
    shadow_deoendencys[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    shadow_deoendencys[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    shadow_deoendencys[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    shadow_deoendencys[1].srcAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadow_deoendencys[1].dstAccessMask =  VK_ACCESS_SHADER_READ_BIT;
    shadow_deoendencys[1].dependencyFlags =  VK_DEPENDENCY_BY_REGION_BIT;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &shadow_attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &shadow_subass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = shadow_deoendencys;
    vkCreateRenderPass(Device_swapchain->device,&renderPassInfo,nullptr,&render_instance->shadow_map_pass);
    
    //创建图形管道
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout=render_instance->layout;
    pipelineInfo.renderPass = render_instance->pass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    pipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(Device_swapchain->device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&render_instance->pipline);
    //创建阴影贴图管道
    scissor.extent.width = viewport.width = SHADOW_W;
    scissor.extent.height = viewport.height = SHADOW_H;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    multisampling.sampleShadingEnable=VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    colorBlending.attachmentCount = 0U;
    //colorBlendAttachment.blendEnable=VK_FALSE;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages + 2;
    pipelineInfo.renderPass = render_instance->shadow_map_pass;
    pipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(Device_swapchain->device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&render_instance->shadow_map_pipline);

    //创建帧缓冲区
    VkFramebufferCreateInfo framebufferInfo{};
    for(uint32_t i=0U;i<Device_swapchain->Image_count;i++){
        framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_instance->pass;
        VkImageView view[Attachments_Count]={colorResources_view,depth_image_view,Device_swapchain->views[i]};
        framebufferInfo.attachmentCount = Attachments_Count;
        framebufferInfo.pAttachments =view;
        framebufferInfo.width = surface_w;
        framebufferInfo.height = surface_h;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(Device_swapchain->device, &framebufferInfo, nullptr, render_instance->freames + i);

        framebufferInfo.renderPass=render_instance->shadow_map_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.height = SHADOW_H;
        framebufferInfo.width = SHADOW_W;
        framebufferInfo.pAttachments = pShadow_views + i;
        vkCreateFramebuffer(Device_swapchain->device, &framebufferInfo, nullptr, render_instance->shadow_map_freames + i);
    }
    
    return render_instance;
}

VkCommandBuffer beginBuffer(const VkDevice&, VkCommandPool*);
void endSubmitBuffer(const VkQueue&, const VkCommandBuffer&);

typedef struct CommandBuffer{
    VkCommandPool pool;
    static const uint32_t BufCount = vk_device_swapchain::Image_count;
    VkCommandBuffer bufs[BufCount];
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> imageDescriptorSets;
    std::vector<VkDescriptorSet> uniformDescriptorSets;
    VkDescriptorSet shadow_DescriptorSets[vk_device_swapchain::Image_count];
}CommandBuffer;
//记录命令缓冲区
std::shared_ptr<CommandBuffer> getCommandBuffer(const VkDevice &device,VkQueue queue,const VkRenderPass &pass,const VkRenderPass &Shadow_pass,const VkFramebuffer *pFreams,const VkFramebuffer *pShadow_frames,
                                                const uint32_t &surface_w,const uint32_t &surface_h,const VkPipeline &pipline,const VkPipeline &Shadow_map_pipline,
                                                const VkPipelineLayout &pipline_layout,const VkDescriptorSetLayout *pSetLayouts,
                                                VkBuffer *pUniform_buffers,VkBuffer *pVertex_buffers,const VkDeviceSize * pVertex_buffer_vertex_counts,
                                                VkBuffer* pVertex_index_buffers, const std::vector<uint64_t> &index_instances,
                                                const std::vector<VkImageView> &image_views,const std::vector<uint32_t> &Image_indexs,const VkImageView *pShadow_map_views,const VkSampler *pSampler,
                                                VkBuffer *pUniform_srcs,VkBuffer *pUniform_dsts,std::vector<Push_Constant> &Push_Constants
){
    std::shared_ptr<CommandBuffer> cmd(new CommandBuffer, [=](CommandBuffer* p) {
        vkFreeCommandBuffers(device,p->pool, p->BufCount, p->bufs);
        if(p->pool) vkDestroyCommandPool(device, p->pool, nullptr);
        if(p->descriptorPool) vkDestroyDescriptorPool(device,p->descriptorPool,nullptr);
        delete p;
    });
    cmd->pool = VK_NULL_HANDLE;
    *cmd->bufs= {VK_NULL_HANDLE};
    cmd->descriptorPool=VK_NULL_HANDLE;
    //创建命令池
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex =0;
    poolInfo.flags = 0; // Optional
    vkCreateCommandPool(device,&poolInfo,nullptr,&cmd->pool);
    //分配命令缓冲区
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmd->pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = CommandBuffer::BufCount;
    vkAllocateCommandBuffers(device,&allocInfo,cmd->bufs);
    //创建描述符池
    const uint32_t Pool_Size_Count=2U;
    VkDescriptorPoolSize descripto_poolSizes[Pool_Size_Count]{};
    descripto_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descripto_poolSizes[0].descriptorCount = 1;
    descripto_poolSizes[1].type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descripto_poolSizes[1].descriptorCount = image_views.size() + 2;
    VkDescriptorPoolCreateInfo descriptor_poolInfo{};
    descriptor_poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_poolInfo.poolSizeCount = Pool_Size_Count;
    descriptor_poolInfo.pPoolSizes = descripto_poolSizes;
    descriptor_poolInfo.maxSets = image_views.size() + 3;
    vkCreateDescriptorPool(device,&descriptor_poolInfo,nullptr,&cmd->descriptorPool);
    
    //分配描述符集
    VkDescriptorSetAllocateInfo descriptor_allocInfo{};
    descriptor_allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_allocInfo.pNext=nullptr;
    descriptor_allocInfo.descriptorPool = cmd->descriptorPool;
    descriptor_allocInfo.descriptorSetCount = 1;
    descriptor_allocInfo.pSetLayouts = pSetLayouts + 1; 
    for(uint32_t i=0U;i<image_views.size();i++){
        cmd->imageDescriptorSets.push_back(VK_NULL_HANDLE);
        if(vkAllocateDescriptorSets(device,&descriptor_allocInfo,cmd->imageDescriptorSets.data() + i) != VK_SUCCESS) abort();
    }
    //更新描述符
    for (uint32_t i = 0; i < image_views.size(); i++) {
        //图像组合图像采样器
        VkWriteDescriptorSet imageDescriptorWrite{};
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = image_views[Image_indexs[i]];
        imageInfo.sampler = pSampler[0];
        imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageDescriptorWrite.dstSet = cmd->imageDescriptorSets[i];
        imageDescriptorWrite.dstBinding = 1;
        imageDescriptorWrite.dstArrayElement = 0;
        imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageDescriptorWrite.descriptorCount = 1;
        imageDescriptorWrite.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &imageDescriptorWrite, 0, nullptr);
    }
    //uniform缓冲区
    descriptor_allocInfo.descriptorSetCount =1;
    descriptor_allocInfo.pSetLayouts =pSetLayouts;
    cmd->uniformDescriptorSets.push_back(VK_NULL_HANDLE);
    if(vkAllocateDescriptorSets(device,&descriptor_allocInfo,cmd->uniformDescriptorSets.data()) != VK_SUCCESS) abort();
    
    const uint32_t Uniform_Buffer_Count=1U;
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = *pUniform_buffers;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(uinformForm_1);
    VkWriteDescriptorSet uniformDescriptorWrite{};
    uniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformDescriptorWrite.dstSet = cmd->uniformDescriptorSets[0];
    uniformDescriptorWrite.dstBinding = 0;
    uniformDescriptorWrite.dstArrayElement = 0;
    uniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformDescriptorWrite.descriptorCount = 1U;
    uniformDescriptorWrite.pBufferInfo = &uniformBufferInfo;
    uniformDescriptorWrite.pImageInfo = nullptr; // Optional
    uniformDescriptorWrite.pTexelBufferView = nullptr; // Optional
    vkUpdateDescriptorSets(device,1,&uniformDescriptorWrite, 0, nullptr);
    //阴影贴图
    for(uint32_t i=0U;i<vk_device_swapchain::Image_count;i++){
        descriptor_allocInfo.descriptorSetCount =1;
        descriptor_allocInfo.pSetLayouts =pSetLayouts + 2;
        if(vkAllocateDescriptorSets(device,&descriptor_allocInfo,cmd->shadow_DescriptorSets + i) != VK_SUCCESS) abort();
        VkWriteDescriptorSet imageDescriptorWrite{};
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = pShadow_map_views[i];
        imageInfo.sampler = pSampler[0];
        imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageDescriptorWrite.dstSet = cmd->shadow_DescriptorSets[i];
        imageDescriptorWrite.dstBinding = 2;
        imageDescriptorWrite.dstArrayElement = 0;
        imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageDescriptorWrite.descriptorCount = 1;
        imageDescriptorWrite.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &imageDescriptorWrite, 0, nullptr);
    }
    //记录命令缓冲区
    VkBufferCopy csize;
    csize.srcOffset = 0U;
    csize.dstOffset = 0U;
    csize.size = sizeof(uinformForm_1);
    for (uint32_t i = 0U; i < CommandBuffer::BufCount; i++) {
        VkCommandBufferBeginInfo cbeg{};
        cbeg.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 
        vkBeginCommandBuffer(cmd->bufs[i],&cbeg);
            vkCmdCopyBuffer(cmd->bufs[i], *pUniform_srcs, *pUniform_dsts, 1U, &csize);
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = Shadow_pass;
            renderPassInfo.framebuffer = pShadow_frames[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {SHADOW_W,SHADOW_H};
            VkClearValue shadowClear;
            shadowClear.depthStencil = {1.0F,0};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &shadowClear;
            //渲染阴影贴图
            vkCmdBeginRenderPass(cmd->bufs[i],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
                VkDeviceSize vertex_offsets[]= {0U};
                //glm::vec4 light_pos = glm::vec4(-25.0F,-25.0F,0.0F,1.0F);
                VkDescriptorSet bindDescriptorSets[] = {cmd->uniformDescriptorSets[0],cmd->imageDescriptorSets[Image_indexs[0]],cmd->shadow_DescriptorSets[i]};
                
                //vkCmdPushConstants(cmd->bufs[i], pipline_layout,VK_SHADER_STAGE_VERTEX_BIT, 0 , sizeof(light_pos), &light_pos);
                vkCmdBindPipeline(cmd->bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS,Shadow_map_pipline);
                vkCmdBindVertexBuffers(cmd->bufs[i], 0U, 1U, pVertex_buffers, vertex_offsets);
                vkCmdBindIndexBuffer(cmd->bufs[i], pVertex_index_buffers[0], 0U, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(cmd->bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipline_layout, 0,1,bindDescriptorSets, 0, nullptr);
                vkCmdDrawIndexed(cmd->bufs[i],index_instances.back(), 1, 0, 0, 0);
            vkCmdEndRenderPass(cmd->bufs[i]);
            //正常渲染场景
            renderPassInfo.renderPass = pass;
            renderPassInfo.framebuffer = pFreams[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {surface_w,surface_h};
            const uint32_t ClearColor_Count=2U;
            VkClearValue clearColors[ClearColor_Count];
            clearColors[0].color= {{.5f, .5f, .5f, 1.f}};
            clearColors[1].depthStencil = {1.0f,0};
            renderPassInfo.clearValueCount = ClearColor_Count;
            renderPassInfo.pClearValues = clearColors;
            vkCmdBeginRenderPass(cmd->bufs[i],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(cmd->bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipline);
                vkCmdBindVertexBuffers(cmd->bufs[i], 0U, 1U, pVertex_buffers, vertex_offsets);
                vkCmdBindIndexBuffer(cmd->bufs[i], pVertex_index_buffers[0], 0U, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(cmd->bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipline_layout, 0, 3,bindDescriptorSets, 0, nullptr);
                vkCmdDrawIndexed(cmd->bufs[i], index_instances[0], 1, 0, 0, 0);//第一个实例单独从零开始
                for(uint32_t set_i=1;set_i<index_instances.size(); set_i++){
                    Push_Constant *pPush_Constant = Push_Constants.data() + Image_indexs[set_i];
                    pPush_Constant->image_index = Image_indexs[set_i];
                    vkCmdPushConstants(cmd->bufs[i], pipline_layout,VK_SHADER_STAGE_FRAGMENT_BIT, 0 , sizeof(Push_Constant),pPush_Constant);
                    //bindDescriptorSets[1] = cmd->imageDescriptorSets[Image_indexs[set_i]];
                    vkCmdBindDescriptorSets(cmd->bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipline_layout,1,1,cmd->imageDescriptorSets.data() + pPush_Constant->image_index, 0, nullptr);
                    //索引数量是现在的索引实例减去前一个索引实例的最后一个位置,起始位置是前一个索引实例的最后一个索引的位置
                    vkCmdDrawIndexed(cmd->bufs[i], index_instances[set_i] - index_instances[set_i-1], 1  , index_instances[set_i-1] , 0, 0);
                }
                //std::cout<<"Infex:"<<pVertex_buffer_index_counts[0]<<"Count:"<<pVertex_buffer_vertex_counts[0];
                //vkCmdDraw(cmd->bufs[i],pVertex_buffer_vertex_counts[0],pVertex_buffer_vertex_counts[0]/3,0,0);
            vkCmdEndRenderPass(cmd->bufs[i]);
        vkEndCommandBuffer(cmd->bufs[i]);
    }
    return cmd;
}

VkCommandBuffer beginBuffer(const VkDevice &Device,VkCommandPool *pPool) {
    //创建命令池
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = 0U;
    vkCreateCommandPool(Device, &pool_info, nullptr,pPool);

    //分配命令缓冲区
    VkCommandBufferAllocateInfo call{};
    call.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    call.commandPool = *pPool;
    call.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    call.commandBufferCount = 1U;

    VkCommandBuffer cmdBuffers[1] = { VK_NULL_HANDLE };
    vkAllocateCommandBuffers(Device, &call, cmdBuffers);

    //记录命令缓冲区
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuffers[0], &begin);
    
    return cmdBuffers[0];
}

void endSubmitBuffer(const VkQueue &Queue,const VkCommandBuffer &cmdBuffer) {
    vkEndCommandBuffer(cmdBuffer);
    //提交命令缓冲区
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmdBuffer;
    vkQueueSubmit(Queue, 1U, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(Queue);
}

/*将缓冲区内存拷贝到设备本地（原来的缓冲区和内存会删除和替换,确保源缓冲区创建时支持的 usage 包括传输命令的源*/
void copyBufferToLocal(VkBuffer *pSrcBuffer,VkDeviceMemory *pSrcMemory,VkBufferUsageFlags dstUsages,VkDeviceSize copySize,const VkDevice &Device,const VkQueue &Queue,const VkPhysicalDevice &GPU){
    //创建设备本地缓冲区
    VkBufferCreateInfo buf{};
    buf.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    buf.size= copySize;
    buf.usage=dstUsages;

    VkBuffer dstBuffer = VK_NULL_HANDLE;
    vkCreateBuffer(Device,&buf,nullptr,&dstBuffer);
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device, dstBuffer, &memRequirements);
    VkPhysicalDeviceMemoryProperties memory_info;
    vkGetPhysicalDeviceMemoryProperties(GPU,&memory_info);
    
    //找到设备本地内存索引
    VkMemoryAllocateInfo all{};
    VkMemoryPropertyFlagBits dst_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t destMemoryIndex;
    for (uint32_t i = 0U; i < memory_info.memoryTypeCount; i++)
        if ((memRequirements.memoryTypeBits & (1 << i)) && ((memory_info.memoryTypes[i].propertyFlags & dst_flags) == dst_flags))
            destMemoryIndex = i;
    all.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    all.memoryTypeIndex=destMemoryIndex;
    all.allocationSize=memRequirements.size;
    //分配和绑定设备本地内存
    VkDeviceMemory dstMemory = VK_NULL_HANDLE;
    vkAllocateMemory(Device,&all,nullptr,&dstMemory);
    vkBindBufferMemory(Device, dstBuffer, dstMemory,0U);

    VkCommandPool pool;
    VkCommandBuffer buffer = beginBuffer(Device, &pool);
        VkBufferCopy csize;
        csize.srcOffset = 0U;
        csize.dstOffset = 0U;
        csize.size = copySize;
        vkCmdCopyBuffer(buffer, *pSrcBuffer, dstBuffer, 1U, &csize);
    endSubmitBuffer(Queue, buffer);
    //销毁命令池和释放缓冲区
    vkFreeCommandBuffers(Device, pool, 1U, &buffer);
    vkDestroyCommandPool(Device, pool, nullptr);
    
    //销毁原来的缓冲区和内存
    vkDestroyBuffer(Device, *pSrcBuffer, nullptr);
    vkFreeMemory(Device, *pSrcMemory, nullptr);
    *pSrcBuffer = dstBuffer;
    *pSrcMemory = dstMemory;
}

typedef struct vk_buffer {
    static const uint32_t Uniform_Buffer_Count=1U;
    VkBuffer uniform_buffers[Uniform_Buffer_Count];
    VkBuffer uniform_buffer_temps[Uniform_Buffer_Count];
    VkDeviceMemory uniform_memorys[Uniform_Buffer_Count];
    VkDeviceMemory uniform_memorys_temps[Uniform_Buffer_Count];
    static const uint32_t Vertex_Buffer_Count = 1U;
    VkDeviceSize vertex_buffers_vertex_counts[Vertex_Buffer_Count];
    VkBuffer vertex_buffers[Vertex_Buffer_Count];
    VkDeviceMemory vertex_memorys[Vertex_Buffer_Count];
    static const uint32_t Vertex_index_buffer_Count = 1U;
    VkBuffer vertex_index_buffers[Vertex_index_buffer_Count];
    VkDeviceMemory vertex_index_memorys[Vertex_index_buffer_Count];
    std::vector<uint64_t> index_instances;/*每个索引实例的最后一个索引的位置*/
    std::vector<std::string> vertex_map_urls;/*每个索引实例的材质链接*/
    std::vector<uint32_t> image_indexs;/*每个索引实例的纹理在纹理缓冲区中的索引*/
    std::vector<Push_Constant> push_Constants;//每个纹理的属性
}vk_buffer;
//创建缓冲区 
std::shared_ptr<vk_buffer> createBuffer(const VkDevice &Device,const VkQueue &Queue,const VkPhysicalDevice &GPU){
    std::shared_ptr<vk_buffer>buffer(new vk_buffer(),[=](vk_buffer *p){
        for(uint32_t i=0U;i<p->Uniform_Buffer_Count;i++){
            if(p->uniform_buffer_temps[i]) vkDestroyBuffer(Device,p->uniform_buffer_temps[i],nullptr);
            if(p->uniform_memorys_temps[i]) vkFreeMemory(Device,p->uniform_memorys_temps[i],nullptr);
            if(p->uniform_buffers[i]) vkDestroyBuffer(Device,p->uniform_buffers[i],nullptr);
            if(p->uniform_memorys[i]) vkFreeMemory(Device,p->uniform_memorys[i],nullptr);
        }
        for (uint32_t i = 0U; i < p->Vertex_Buffer_Count; i++) {
            if (p->vertex_buffers[i]) vkDestroyBuffer(Device, p->vertex_buffers[i], nullptr);
            if (p->vertex_memorys[i]) vkFreeMemory(Device, p->vertex_memorys[i], nullptr);
        }
        for (uint32_t i = 0U; i < p->Vertex_index_buffer_Count; i++) {
            if (p->vertex_index_buffers[i]) vkDestroyBuffer(Device, p->vertex_index_buffers[i], nullptr);
            if (p->vertex_index_memorys[i]) vkFreeMemory(Device, p->vertex_index_memorys[i], nullptr);
        }
        delete p;
    });
    //创建 uniform 缓冲区
    VkBufferCreateInfo buf{};
    VkMemoryRequirements memRequirements{};
    VkPhysicalDeviceMemoryProperties memory_info{};
    VkMemoryAllocateInfo all{};
    for(uint32_t i=0U;i< buffer->Uniform_Buffer_Count;i++){
        buf.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        buf.size = sizeof(uinformForm_1);
        buf.usage= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkCreateBuffer(Device,&buf,nullptr,&buffer->uniform_buffers[i]);
        buf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkCreateBuffer(Device,&buf,nullptr,&buffer->uniform_buffer_temps[i]);
        
        vkGetBufferMemoryRequirements(Device,buffer->uniform_buffers[i], &memRequirements);
        vkGetPhysicalDeviceMemoryProperties(GPU,&memory_info);
        VkMemoryPropertyFlagBits dst_mor_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        for (uint32_t i = 0U; i < memory_info.memoryTypeCount; i++)
            if ((memRequirements.memoryTypeBits & (1 << i)) && ((memory_info.memoryTypes[i].propertyFlags & dst_mor_flags) == dst_mor_flags))
                all.memoryTypeIndex = i;
        all.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        all.allocationSize = memRequirements.size;
        vkAllocateMemory(Device,&all,nullptr,&buffer->uniform_memorys[i]);
        vkBindBufferMemory(Device,buffer->uniform_buffers[i],buffer->uniform_memorys[i],0U);
        
        dst_mor_flags= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for (uint32_t i = 0U; i < memory_info.memoryTypeCount; ++i)
            if ((memRequirements.memoryTypeBits & (1 << i)) && ((memory_info.memoryTypes[i].propertyFlags & dst_mor_flags) == dst_mor_flags))
                all.memoryTypeIndex = i;
        vkAllocateMemory(Device,&all,nullptr,&buffer->uniform_memorys_temps[i]);
        vkBindBufferMemory(Device,buffer->uniform_buffer_temps[i],buffer->uniform_memorys_temps[i],0U);
    }
    //创建顶点缓冲区
    inVertexForm_1 inVertexForm;
    inVertexForm.loader(buffer->index_instances,buffer->vertex_map_urls,buffer->image_indexs,buffer->push_Constants);
    void* pMapMemory=nullptr;
    for (uint32_t i = 0U; i < buffer->Vertex_Buffer_Count;i++) {
        buffer->vertex_buffers_vertex_counts[i] = inVertexForm.vertices.size() * 3;
        all.allocationSize = buf.size = inVertexForm.vertices.size() * sizeof(Vertex_1);
        buf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkCreateBuffer(Device, &buf, nullptr, &buffer->vertex_buffers[i]);
        vkAllocateMemory(Device, &all, nullptr, &buffer->vertex_memorys[i]);
        vkBindBufferMemory(Device, buffer->vertex_buffers[i], buffer->vertex_memorys[i], 0U);
        
        vkMapMemory(Device, buffer->vertex_memorys[i], 0U, all.allocationSize, 0U, &pMapMemory);
        memcpy(pMapMemory, inVertexForm.vertices.data(), all.allocationSize);
        vkUnmapMemory(Device, buffer->vertex_memorys[i]);
        copyBufferToLocal(buffer.get()->vertex_buffers + i, buffer.get()->vertex_memorys + i, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, all.allocationSize,Device, Queue, GPU);
    }

    for (uint32_t i = 0U; i < buffer->Vertex_index_buffer_Count; i++) {
        //创建索引缓冲区
        all.allocationSize = buf.size = inVertexForm.indices.size() * sizeof(uint32_t);
        buf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkCreateBuffer(Device, &buf, nullptr, &buffer->vertex_index_buffers[i]);
        vkAllocateMemory(Device, &all, nullptr, &buffer->vertex_index_memorys[i]);
        
        vkBindBufferMemory(Device, buffer->vertex_index_buffers[i], buffer->vertex_index_memorys[i], 0U);

        vkMapMemory(Device, buffer->vertex_index_memorys[i], 0U, all.allocationSize, 0U, &pMapMemory);
        memcpy(pMapMemory, inVertexForm.indices.data(), all.allocationSize);
        vkUnmapMemory(Device, buffer->vertex_index_memorys[i]);
        copyBufferToLocal(buffer.get()->vertex_index_buffers + i, buffer.get()->vertex_index_memorys + i, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, all.allocationSize, Device, Queue, GPU);
    }
    return buffer;
}
//过渡图像布局
void transitionImageLayout(VkCommandBuffer commandBuffer,VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,uint32_t mipLevels) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else {
        abort();
    }

    
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage /* TODO */, destinationStage /* TODO */,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}
//生成 mipmap 
void generateMipmaps(VkCommandBuffer cmd,VkImage image,int32_t w,int32_t h,uint32_t mipLevels){
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { w, h, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { w > 1 ? w / 2 : 1, h > 1 ? h / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (w > 1) w /= 2;
        if (h > 1) h /= 2;
    }
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}
//将临时内存拷贝到图形本地内存
void copyBufferToImage(const std::shared_ptr<vk_device_swapchain> &Device_swapchain,VkBuffer srcBuffer,VkImage dstImage, uint32_t width, uint32_t height,uint32_t mipLevels=1U) {
    VkCommandPool pool;
    VkCommandBuffer commandBuffer = beginBuffer(Device_swapchain->device, &pool);
        transitionImageLayout(commandBuffer, dstImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                width,
                height,
                1
            };
            vkCmdCopyBufferToImage(
                commandBuffer,
                srcBuffer,
                dstImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        transitionImageLayout(commandBuffer,dstImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,mipLevels);
        generateMipmaps(commandBuffer,dstImage,width,height,mipLevels);
    endSubmitBuffer(Device_swapchain->queue, commandBuffer);
    //销毁命令池和释放缓冲区
    vkFreeCommandBuffers(Device_swapchain->device, pool, 1U, &commandBuffer);
    vkDestroyCommandPool(Device_swapchain->device, pool, nullptr);
}

//创建纹理缓冲区
typedef struct vk_image_buffer {
    static const uint32_t Depth_Image_count=1U;
    VkImage shadow_images[vk_device_swapchain::Image_count],colorResources_Image,depth_images[Depth_Image_count];
    VkImageView shadow_views[vk_device_swapchain::Image_count],colorResources_Image_view,depth_image_views[Depth_Image_count];
    VkDeviceMemory shadow_memorys[vk_device_swapchain::Image_count],colorResources_Image_memory,depth_image_memorys[Depth_Image_count];
    std::vector<VkImage> images;
    std::vector<VkDeviceMemory> memorys;
    std::vector<VkImageView> views;
    std::vector<VkSampler> Samplers;
}vk_image_buffer;

std::shared_ptr<vk_image_buffer> createImageBuffer(const std::vector<std::string> &vertex_map_urls,const std::shared_ptr<vk_device_swapchain>& Device_swapchain,const VkPhysicalDevice &GPU) {
    std::shared_ptr<vk_image_buffer> images(new vk_image_buffer({}), [=](vk_image_buffer *p) {
            for(uint32_t i=0U;i<Device_swapchain->Image_count;i++){
                if(p->shadow_views[i]) vkDestroyImageView(Device_swapchain->device,p->shadow_views[i],nullptr);
                if(p->shadow_images[i]) vkDestroyImage(Device_swapchain->device,p->shadow_images[i],nullptr);
                if(p->shadow_memorys[i]) vkFreeMemory(Device_swapchain->device,p->shadow_memorys[i],nullptr);
            }
            if(p->colorResources_Image_view) vkDestroyImageView(Device_swapchain->device,p->colorResources_Image_view,nullptr);
            if(p->colorResources_Image) vkDestroyImage(Device_swapchain->device,p->colorResources_Image,nullptr);
            if(p->colorResources_Image_memory) vkFreeMemory(Device_swapchain->device,p->colorResources_Image_memory,nullptr);
            for(uint32_t i=0U;i<p->Depth_Image_count;i++){
                if(p->depth_image_views[i]) vkDestroyImageView(Device_swapchain->device,p->depth_image_views[i],nullptr);
                if(p->depth_images[i]) vkDestroyImage(Device_swapchain->device,p->depth_images[i],nullptr);
                if(p->depth_image_memorys[i]) vkFreeMemory(Device_swapchain->device,p->depth_image_memorys[i],nullptr);
            }
            for(const auto &i:p->Samplers) if(i) vkDestroySampler(Device_swapchain->device,i,nullptr);
            for (uint32_t i = 0U; i < p->images.size(); i++) {
                if(p->views[i]) vkDestroyImageView(Device_swapchain->device,p->views[i],nullptr);
                if(p->images[i]) vkDestroyImage(Device_swapchain->device, p->images[i], nullptr);
                if(p->memorys[i]) vkFreeMemory(Device_swapchain->device, p->memorys[i], nullptr);
            }
            delete p;
        }
    );
    
    //获得外部图形数据
    int texWidth=0,//宽度
        texHeight=0,//高度
        texChannel=0;//通道数
    stbi_uc* pixeles=nullptr;//图形数据
    VkDeviceSize imageSize=0U;//图形的总像素数量
    uint32_t mipLevels;//图像 mipmap 的最大级别数
    VkBufferCreateInfo buf{};
    VkMemoryRequirements memRequirements{};
    VkPhysicalDeviceMemoryProperties memory_info{};
    vkGetPhysicalDeviceMemoryProperties(GPU, &memory_info);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(GPU, &properties);
    VkMemoryAllocateInfo all{};
    
    for (uint32_t i = 0U; i < vertex_map_urls.size(); i++) {
        images->images.push_back(VK_NULL_HANDLE);
        images->memorys.push_back(VK_NULL_HANDLE);
        images->views.push_back(VK_NULL_HANDLE);
        
        pixeles = stbi_load(vertex_map_urls[i].c_str(), &texWidth, &texHeight, &texChannel, STBI_rgb_alpha);
        if (pixeles == nullptr) abort();//图片打开失败
        //VkDeviceSize size=0U;
        //stbi_image_free(pixeles);
        //pixeles = (stbi_uc*)getShaderSPVcode((vertex_map_urls[i] + ".astc").c_str(),&size);
        //texWidth = (texWidth + 8 - 1) / 8;
	    //texHeight = (texHeight + 8 - 1) / 8;
        imageSize = texWidth * texHeight * 4;
        
        //计算mip最大级别
        mipLevels=static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1U;
        
        //创建临时缓冲区
        VkBuffer temp_buffer=VK_NULL_HANDLE;
        VkDeviceMemory temp_memory = VK_NULL_HANDLE;
        buf.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf.size = imageSize;
        buf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;//传输命令的源
        vkCreateBuffer(Device_swapchain->device, &buf, nullptr, &temp_buffer);
        vkGetBufferMemoryRequirements(Device_swapchain->device,temp_buffer, &memRequirements);
        for (uint32_t i = 0U; i < memory_info.memoryTypeCount; i++)
            if ((memRequirements.memoryTypeBits & (1 << i)) && 
                (   
                    (memory_info.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == 
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                )
            )
                all.memoryTypeIndex = i;

        all.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        all.allocationSize = memRequirements.size;
        vkAllocateMemory(Device_swapchain->device, &all, nullptr, &temp_memory);
        vkBindBufferMemory(Device_swapchain->device, temp_buffer, temp_memory, 0U);
        void* data = nullptr;
        vkMapMemory(Device_swapchain->device, temp_memory, 0, all.allocationSize, 0, &data);
        memcpy(data, pixeles, static_cast<size_t>(all.allocationSize));
        vkUnmapMemory(Device_swapchain->device, temp_memory);
        //删除图片
        stbi_image_free(pixeles);
        pixeles = nullptr;
        
        ///创建纹理缓冲区
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(texWidth);
        imageInfo.extent.height = static_cast<uint32_t>(texHeight);
        imageInfo.extent.depth = 1U;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1U;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;//传输源和目的地 and 着色器可以访问
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        vkCreateImage(Device_swapchain->device,&imageInfo,nullptr, images -> images.data() + i);
        vkGetImageMemoryRequirements(Device_swapchain->device, images->images[i], &memRequirements);
        all.allocationSize = memRequirements.size;
        for (uint32_t i = 0U; i < memory_info.memoryTypeCount; i++)
            if ((memRequirements.memoryTypeBits & (1 << i)) && (memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                all.memoryTypeIndex = memory_info.memoryTypes[i].heapIndex;
        vkAllocateMemory(Device_swapchain->device, &all, nullptr, images->memorys.data() + i);
        vkBindImageMemory(Device_swapchain->device, images->images[i], images->memorys[i],0U);
        //拷贝到设备本地
        copyBufferToImage(Device_swapchain, temp_buffer,images->images[i], imageInfo.extent.width, imageInfo.extent.height,mipLevels);
        //销毁临时缓冲区
        vkDestroyBuffer(Device_swapchain->device, temp_buffer, nullptr);
        vkFreeMemory(Device_swapchain->device, temp_memory, nullptr);
        //创建图像视图
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(Device_swapchain->device,&viewInfo,nullptr,images->views.data() + i);
    }
        //创建图像采样器
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0F;
        samplerInfo.maxLod = 1024.0;
        
        images->Samplers.push_back(VK_NULL_HANDLE);
        vkCreateSampler(Device_swapchain->device,&samplerInfo,nullptr,images->Samplers.data());
    
    return images;
}

//创建深度缓冲区
void createDepthBuffer(const VkPhysicalDevice &GPU,const VkDevice &device,VkQueue queue,std::shared_ptr<vk_image_buffer> &depth_buffer,const uint32_t &Width, const uint32_t &Height,VkFormat depthFormat,VkFormat colorResources_Image_format,VkSampleCountFlagBits numSamples,VkImageTiling tiling, VkImageUsageFlags usage){
    VkPhysicalDeviceMemoryProperties memory_info{};
    VkMemoryRequirements memRequirements;
    vkGetPhysicalDeviceMemoryProperties(GPU, &memory_info); 
    VkImageCreateInfo image{};
    VkImageViewCreateInfo view{};
    VkMemoryAllocateInfo all{};
    for(uint32_t i=0U;i<vk_image_buffer::Depth_Image_count;i++){
        image.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent.width = Width;
        image.extent.height = Height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.format =depthFormat;
        image.tiling = tiling;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.usage = usage;
        image.samples = numSamples;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(device, &image, nullptr,depth_buffer->depth_images + i);
        vkGetImageMemoryRequirements(device,depth_buffer->depth_images[i], &memRequirements);

        all.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        all.allocationSize = memRequirements.size;
        for (uint32_t i = 0U; i < memory_info.memoryTypeCount; i++)
                if ((memRequirements.memoryTypeBits & (1 << i)) && (memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                    all.memoryTypeIndex = i;
	
        vkAllocateMemory(device, &all, nullptr,depth_buffer->depth_image_memorys + i);
        vkBindImageMemory(device,depth_buffer->depth_images[i],depth_buffer->depth_image_memorys[i],0);

        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = depth_buffer->depth_images[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = image.format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &view, nullptr, depth_buffer->depth_image_views);
    }
    //创建多采样图像
    image.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.format= colorResources_Image_format;
    vkCreateImage(device, &image, nullptr,&depth_buffer->colorResources_Image);
    vkAllocateMemory(device, &all, nullptr,&depth_buffer->colorResources_Image_memory);
    vkBindImageMemory(device,depth_buffer->colorResources_Image,depth_buffer->colorResources_Image_memory,0);
    view.format=colorResources_Image_format;
    view.image=depth_buffer->colorResources_Image;
    view.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
    vkCreateImageView(device, &view, nullptr, &depth_buffer->colorResources_Image_view);
    //创建阴影贴图纹理
    for(uint32_t i=0U;i<vk_device_swapchain::Image_count;i++){
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.format = depthFormat;
        image.extent.height = SHADOW_H;
        image.extent.width = SHADOW_W;
        image.samples=VK_SAMPLE_COUNT_1_BIT;

        vkCreateImage(device, &image, nullptr,depth_buffer->shadow_images + i);
        vkGetImageMemoryRequirements(device,depth_buffer->shadow_images[i], &memRequirements);
        all.allocationSize = memRequirements.size;
        vkAllocateMemory(device, &all, nullptr,depth_buffer->shadow_memorys + i);
        vkBindImageMemory(device,depth_buffer->shadow_images[i],depth_buffer->shadow_memorys[i],0);
        view.format = depthFormat;
        view.image=depth_buffer->shadow_images[i];
        view.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(device, &view, nullptr,depth_buffer->shadow_views + i);
    }
}
//获取最大采样数量
VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice &PhysicalDevice) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(PhysicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

#ifndef __cplusplus
    }
#endif
#endif //KEKE_HPP
