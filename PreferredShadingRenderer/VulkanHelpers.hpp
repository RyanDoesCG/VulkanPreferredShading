//
//  VulkanHelpers.hpp
//  ForwardRenderer
//
//  Created by macbook on 19/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanHelpers_hpp
#define VulkanHelpers_hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>


#include <iostream>

struct VulkanHelpers
    {  // VulkanHelpers struct
    
    //
    //
    //
    static vk::SurfaceFormatKHR querySwapChainSurfaceFormat (std::vector<vk::SurfaceFormatKHR>& available)
        { // VulkanHelpers :: querySwapChainSurfaceFormat
        if (available.size() == 1 && available[0].format == vk::Format::eUndefined)
            return {
            vk::Format::eB8G8R8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear
            };
            
        for (const vk::SurfaceFormatKHR& format : available)
            { // for each available format
            if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                return format;
            } // for each available format
        
        return available.front ();
        } // VulkanHelpers :: querySwapChainSurfaceFormat
        
    //
    //
    //
    static vk::PresentModeKHR querySwapChainPresentMode (std::vector<vk::PresentModeKHR>& available)
        { // VulkanHelpers :: querySwapChainPresentMode
        
        vk::PresentModeKHR ideal = vk::PresentModeKHR::eFifo;
        
        for (const vk::PresentModeKHR& mode : available)
            { // for each available present mode
            if (mode == vk::PresentModeKHR::eMailbox)
                return mode;
            if (mode == vk::PresentModeKHR::eImmediate)
                ideal = mode;
            } // for each available present mode

        return ideal;
        
        } // VulkanHelpers :: querySwapChainPresentMode
    
    //
    //
    //
    static vk::Extent2D querySwapChainExtents (vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
        { // VulkanHelpers :: querySwapChainExtents
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;
        else
            {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            uint32_t w = width, h = height;
            vk::Extent2D actualExtent = { w, h };
            return actualExtent;
            }
        } // VulkanHelpers :: querySwapChainExtents
    

    //
    //
    //
    static uint32_t findMemoryType (vk::PhysicalDevice& physical, uint32_t filter, vk::MemoryPropertyFlags properties)
        { // VulkanHelpers :: findMemoryType
        
        vk::PhysicalDeviceMemoryProperties memoryProperties;
        physical.getMemoryProperties(&memoryProperties);
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
			if ((filter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
        
		std::cout << "failed to find memory type" << std::endl;

        return 0;
        
        } // VulkanHelpers :: findMemoryType
    
	//
	//
	//
    static inline bool hasStencilComponent (vk::Format format)
        { // VulkanHelpers :: hasStencilComponent
        return
            format == vk::Format::eD32Sfloat ||
            format == vk::Format::eD24UnormS8Uint;
        } // VulkanHelpers :: hasStencilComponent

	//
	//
	//
	static vk::CommandBuffer beginSingleUseCommand (vk::Device& device, vk::CommandPool& pool)
		{ // VulkanHelpers :: beginSingleUseCommand

		vk::CommandBufferAllocateInfo allocationInfo = { };
			allocationInfo.level               = vk::CommandBufferLevel::ePrimary;
			allocationInfo.commandPool         = pool;
			allocationInfo.commandBufferCount  = 1;

		vk::CommandBuffer commandBuffer = { };
			device.allocateCommandBuffers(&allocationInfo, &commandBuffer);

		vk::CommandBufferBeginInfo beginInfo = { };
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		commandBuffer.begin(&beginInfo);

		return commandBuffer;

		} // VulkanHelpers :: beginSingleUseCommand

	//
	//
	//
	static void endSingleUseCommand (vk::Device& device, vk::CommandPool& pool, vk::CommandBuffer& buffer, vk::Queue& queue)
		{ // VulkanHelpers :: endSingleUseCommand

		buffer.end();

		vk::SubmitInfo submitInfo = { };
			submitInfo.commandBufferCount  = 1;
			submitInfo.pCommandBuffers     = &buffer;

		queue.submit(submitInfo, vk::Fence { });
		queue.waitIdle();

		device.freeCommandBuffers(pool, 1, &buffer);
			
		} // VulkanHelpers :: endSingleUseCommand
    
    }; // VulkanHelpers struct


#endif /* VulkanHelpers_hpp */
