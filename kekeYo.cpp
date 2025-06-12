// kekeYo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "include/keke.hpp"

#define SDL_MAIN_HANDLED
#include "include\SDL2\SDL.h"
#include "include\SDL2\SDL_vulkan.h"
#pragma comment(lib,"../keke/lib/SDL2/SDL2")

#include <iostream>
#include <thread> //C++ 0xB
//#include <mutex> //c++ 0xB
#ifdef _THREAD_
    //标识主线程是否结束,主线程即将结束时它为 VK_FALSE
    static VkBool32 MAIN_THREAD = VK_TRUE;
#endif

//int main()
int _stdcall wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
    SDL_Window* window = NULL;
    SDL_Surface* SDL_surface = NULL;

    SDL_Init(SDL_INIT_EVENTS);
    window = SDL_CreateWindow("keke",
        0,0,//这两个是窗口的位置的偏移量 x y
        GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),//窗口的宽度和高度
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN     //可以创建 vulkan surface 了
    );
    SDL_surface = SDL_GetWindowSurface(window);

    VkInstance instance = VK_NULL_HANDLE;
    [](VkInstance* pInstance) {
        const uint32_t extern_count = 2U;
        const char* externs[extern_count] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
        };

        VkApplicationInfo app;
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pNext = nullptr;
        vkEnumerateInstanceVersion(&app.apiVersion);
        app.pApplicationName = "keke\0";
        app.pEngineName = "kekeYo\0";
        app.applicationVersion = 1;
        app.engineVersion = 1;
        VkInstanceCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0U;
        info.enabledLayerCount = 0U;
        info.ppEnabledLayerNames = nullptr;
        info.enabledExtensionCount = extern_count;
        info.ppEnabledExtensionNames = externs;
        info.pApplicationInfo = &app;
        
        (((PFN_vkCreateInstance) vkGetInstanceProcAddr(NULL, "vkCreateInstance")) (&info, nullptr, pInstance));
    }(&instance);

    //create surface
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    SDL_Vulkan_CreateSurface(window, (SDL_vulkanInstance)instance, (SDL_vulkanSurface*)&vk_surface);
    VkPhysicalDevice GPU = enumeratePhysicalDevice(instance);
    //获得表面信息
    uint32_t vk_surface_format_count = 0U;
    vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, vk_surface, &vk_surface_format_count, nullptr);
    VkSurfaceFormatKHR* pVk_surface_formats = new VkSurfaceFormatKHR[vk_surface_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, vk_surface, &vk_surface_format_count, pVk_surface_formats);
    //获得最大采样
    auto maxResources_count = getMaxUsableSampleCount(GPU);
    //创建各种vulkan对象
    std::shared_ptr<vk_device_swapchain> device_swapchain = keke(instance,GPU, vk_surface, SDL_surface->w, SDL_surface->h, VK_FORMAT_R8G8B8A8_SRGB,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    std::shared_ptr<vk_buffer> vertex_buffer = createBuffer(device_swapchain->device, device_swapchain->queue,GPU);
    std::shared_ptr<vk_image_buffer> image_buffer = createImageBuffer(vertex_buffer->vertex_map_urls,device_swapchain,GPU);
    createDepthBuffer(GPU,device_swapchain->device,device_swapchain->queue,image_buffer,SDL_surface->w,SDL_surface->h,VK_FORMAT_D32_SFLOAT, VK_FORMAT_R8G8B8A8_SRGB,maxResources_count,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    std::shared_ptr<vk_render_instances> render_instances = createRenderInstance(device_swapchain, SDL_surface->w, SDL_surface->h, VK_FORMAT_R8G8B8A8_SRGB,VK_FORMAT_D32_SFLOAT,image_buffer->depth_image_views[0],image_buffer->colorResources_Image_view,maxResources_count,image_buffer->shadow_views,GPU);
    delete[] pVk_surface_formats;
    //开始记录命令缓冲区
    /////////////////////////////
    std::shared_ptr<CommandBuffer> commandBuffers = getCommandBuffer(
                                                                    device_swapchain->device, device_swapchain->queue,render_instances->pass,render_instances->shadow_map_pass,render_instances->freames,render_instances->shadow_map_freames,
                                                                    SDL_surface->w, SDL_surface->h, render_instances->pipline,render_instances->shadow_map_pipline,
                                                                    render_instances->layout, render_instances->setLayouts, 
                                                                    vertex_buffer->uniform_buffers, vertex_buffer->vertex_buffers, 
                                                                    vertex_buffer->vertex_buffers_vertex_counts, 
                                                                    vertex_buffer->vertex_index_buffers, vertex_buffer->index_instances,
                                                                    image_buffer->views,vertex_buffer->image_indexs,image_buffer->shadow_views,image_buffer->Samplers.data(),
                                                                    vertex_buffer->uniform_buffer_temps,vertex_buffer->uniform_buffers,vertex_buffer->push_Constants
    );
    vertex_buffer->index_instances.clear();
    vertex_buffer->vertex_map_urls.clear();
    vertex_buffer->image_indexs.clear();
    vertex_buffer->push_Constants.clear();

    const uint32_t Semaphores_count = 2U;
    VkSemaphore semaphores[Semaphores_count];
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0U;
    for (uint32_t i = 0U; i < Semaphores_count; i++)
        vkCreateSemaphore(device_swapchain->device, &semaphore_info, nullptr, semaphores + i);

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = 0U;
    
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device_swapchain->device, &fence_info, nullptr, &fence);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 0U;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = waitStages;
    submit_info.commandBufferCount = 1U;
    submit_info.pCommandBuffers = nullptr;
    submit_info.signalSemaphoreCount = 0U;
    submit_info.pSignalSemaphores = nullptr;

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 0U;
    presentInfo.pWaitSemaphores = nullptr;
    presentInfo.swapchainCount = 1U;
    presentInfo.pSwapchains = &device_swapchain->swapchain;
    presentInfo.pImageIndices = nullptr;
    presentInfo.pResults = nullptr;

    //变换矩阵 随着键盘事件而改变
    glm::vec3 lookAt_eye=glm::vec3(-100.0F,-100.0F,-100.0F),//摄像机位置
                lookAt_center=glm::vec3(0.0F,0.0F,0.0f),//目标位置
                    lookAt_up=glm::vec3(0.0f, 1.0f, 0.0f);//向上方向
    float fov = 45.0F;//投影焦距
    float fovy = 1.0F, aspect = 10000.0F;//摄像机投影长度
    uinformForm_1 mat4_3 = {
        glm::mat4(1.0F),
        //glm::scale(glm::mat4(1.0F),glm::vec3( float(SDL_surface->h * 0.001F) , float(SDL_surface->w * 0.001F),1.0F ) ),//局部坐标系->世界坐标系
        glm::lookAt(lookAt_eye,glm::vec3(0.0F,0.0F,0.0F),lookAt_up),//世界坐标系->摄像机坐标系
        glm::perspective(glm::radians( fov ) , float(float(SDL_surface->w) / float(SDL_surface->h)) , fovy,aspect),//摄像机投影范围
        glm::ortho(3840.0F/4.0F,-2160.0F / 4.0F,3840.0F / 4.0F,-2160.0F / 4.0F,0.0F,1500.0F) * glm::lookAt(glm::vec3(-500.0F,-500.0F,-500.0F),glm::vec3(0.0F,0.0F,0.0F),glm::vec3(0.0F,1.0F,0.0F)),//阴影视图矩阵
        //mat4_3.p * mat4_3.v,
        glm::vec3(-500.0F,-500.0F,-500.0F),//灯光位置
        lookAt_eye//相机位置
    };

    std::thread([&](const uinformForm_1 *pMat4_3)mutable {
        const PFN_vkQueueSubmit _vkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device_swapchain->device, "vkQueueSubmit");
        const PFN_vkAcquireNextImageKHR _vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device_swapchain->device, "vkAcquireNextImageKHR");
        const PFN_vkMapMemory _vkMapMemory = (PFN_vkMapMemory)vkGetDeviceProcAddr(device_swapchain->device, "vkMapMemory");
        const PFN_vkUnmapMemory _vkUnmapMemory = (PFN_vkUnmapMemory)vkGetDeviceProcAddr(device_swapchain->device, "vkUnmapMemory");
        const PFN_vkWaitForFences _vkWaitForFences = (PFN_vkWaitForFences)vkGetDeviceProcAddr(device_swapchain->device, "vkWaitForFences");
        const PFN_vkQueueWaitIdle _vkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkGetDeviceProcAddr(device_swapchain->device, "vkQueueWaitIdle");
        const PFN_vkResetFences _vkResetFences = (PFN_vkResetFences)vkGetDeviceProcAddr(device_swapchain->device, "vkResetFences");
        const PFN_vkQueuePresentKHR _vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device_swapchain->device, "vkQueuePresentKHR");
        const float w_h = float(SDL_surface->w / SDL_surface->h);
        constexpr uint32_t Uniform_Size = sizeof(uinformForm_1);
        void* pMapMemory = nullptr; 
        //渲染主循环
        while (MAIN_THREAD)
        {
            for (uint32_t i = 0U; i < vk_device_swapchain::Image_count; i++) {
                _vkAcquireNextImageKHR(device_swapchain->device, device_swapchain->swapchain, UINT32_MAX, VK_NULL_HANDLE, fence, &i);
                
                    _vkMapMemory(device_swapchain->device, *(vertex_buffer->uniform_memorys_temps), 0U, Uniform_Size, 0U, &pMapMemory);
                        mat4_3.v = glm::lookAt(lookAt_eye, lookAt_eye + lookAt_center, lookAt_up);
                        mat4_3.view_pos = lookAt_eye;
                        //glm::mat4 l_rotate = glm::rotate(glm::mat4(1.0F), glm::radians(-0.25f), glm::vec3(0.0F,1.0F,0.0F));
                        //mat4_3.Light_pos = glm::mat3(l_rotate) * mat4_3.Light_pos;
                        //mat4_3.m = l_rotate * mat4_3.m;
                        //mat4_3.sv = mat4_3.v;
                        //mat4_3.Light_pos = mat4_3.view_pos;
                        //mat4_3.sv = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 100.0F, 500.0F) * glm::lookAt(lookAt_eye, glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
                        //mat4_3.p = glm::perspective(glm::radians(fov),w_h , fovy, aspect);
                        //mat4_3.Light_pos = lookAt_eye;
                        memcpy(pMapMemory, pMat4_3, Uniform_Size);
                    _vkUnmapMemory(device_swapchain->device, *(vertex_buffer->uniform_memorys_temps));
                    
                    submit_info.pCommandBuffers = (commandBuffers->bufs + i);
                    _vkWaitForFences(device_swapchain->device, 1U, &fence, VK_TRUE, UINT32_MAX);
                    _vkQueueSubmit(device_swapchain->queue, 1U, &submit_info, fence);
                    _vkResetFences(device_swapchain->device, 1U, &fence);
                
                presentInfo.pImageIndices = &i;
                _vkQueueWaitIdle(device_swapchain->queue);
                _vkQueuePresentKHR(device_swapchain->queue, &presentInfo);
            }
        }
    },& mat4_3).detach();

    //窗口消息主循环 堵塞主线程
    tagPOINT CaptureMouse_pos = {CaptureMouse_pos.x = 760 , CaptureMouse_pos.y = 440};//鼠标初始坐标
    SetCursorPos(CaptureMouse_pos.x, CaptureMouse_pos.y);//设置鼠标坐标
    float CaptureMouse_size = 0.3F;//鼠标灵敏度
    float keysym_size = 4.00F * 1.0F;//键盘灵敏度;
    float CaptureMouse_y_size = .5F;//缩放灵敏度
    SDL_ShowCursor(SDL_FALSE);//隐藏鼠标
    SDL_CaptureMouse(SDL_TRUE);//追踪全局鼠标位置
    SDL_Event event = {};//事件消息
    float lookAt_center_x = .0F;//将要旋转的水平距离
    float lookAt_center_y = .0F; //将要旋转的垂直距离
    glm::vec3 direction ={}; //最终旋转角度
    SDL_ShowWindow(window);
    while (MAIN_THREAD) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_MOUSEMOTION://如果触发了鼠标移动事件
                    lookAt_center_x = -float(float(CaptureMouse_pos.x - event.motion.x) * CaptureMouse_size * float(fov/45.0F));
                    lookAt_center_y = -float(float(CaptureMouse_pos.y - event.motion.y) * CaptureMouse_size * float(fov/45.0F));
                    //防止摄像头上下颠倒
                    if (lookAt_center_y > 89.0F) lookAt_center_y = 89.0F;
                    if (lookAt_center_y < -89.0F) lookAt_center_y = -89.0F;
                    direction.x = cos(glm::radians( lookAt_center_x )) * cos(glm::radians( lookAt_center_y ));
                    direction.y = sin(glm::radians( lookAt_center_y ));
                    direction.z = sin(glm::radians(lookAt_center_x ) * cos(glm::radians( lookAt_center_y ) ));
                    lookAt_center = glm::normalize( direction );
                    break;

                case SDL_KEYDOWN://如果触发了键盘事件
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            lookAt_eye += keysym_size * lookAt_center;
                            break;
                        case SDLK_s:
                            lookAt_eye -= keysym_size * lookAt_center;
                            break;
                        case SDLK_a:
                            lookAt_eye -= glm::normalize(glm::cross(lookAt_center, lookAt_up)) * keysym_size;
                            break;
                        case SDLK_d:
                            lookAt_eye += glm::normalize(glm::cross(lookAt_center, lookAt_up)) * keysym_size;
                            break;
                        default:
                            break;
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN://如果触发了鼠标点击事件
                    //event.button
                    break;
                
                case SDL_MOUSEWHEEL://如果触发了鼠标滑动事件
                    float tem_fov;
                    tem_fov = (fov - (float)event.wheel.y * CaptureMouse_y_size);
                    if (tem_fov < 1.0F ) tem_fov = 1.0F;
                    if (tem_fov > 45.0F) tem_fov = 45.0F;
                    fov = tem_fov;
                    mat4_3.p = glm::perspective(glm::radians(fov), float(float(SDL_surface->w) / float(SDL_surface->h)) , fovy, aspect);
                    break;
                
                case SDL_QUIT:
                    MAIN_THREAD = VK_FALSE;//主线程即将结束,提醒 所有子线程
                    break;
                
                default:
                    // Do nothing.
                    break;
            }
        }
        SDL_Delay(10);
    }
    SDL_MinimizeWindow(window);
    ////////////////////////
    Sleep(3000);
    commandBuffers.~shared_ptr();
    render_instances.~shared_ptr();
    image_buffer.~shared_ptr();
    vertex_buffer.~shared_ptr();
    vkDestroyFence(device_swapchain->device, fence, nullptr);
    for (uint32_t i = 0U; i < Semaphores_count; i++)
        vkDestroySemaphore(device_swapchain->device, semaphores[i], nullptr);
    device_swapchain.~shared_ptr();
    vkDestroySurfaceKHR(instance, vk_surface, nullptr);
    SDL_DestroyWindow(window); 
	SDL_Quit();
    vkDestroyInstance(instance, nullptr);
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
