//
//  VulkanShadingResource.hpp
//  PreferredRenderer
//
//  Created by macbook on 19/06/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanShadingResource_hpp
#define VulkanShadingResource_hpp

#include <vulkan/vulkan.hpp>

struct VulkanShadingResource
    {
    vk::Image        image;
    vk::ImageView    view;
	vk::ImageLayout  layout;
    vk::DeviceMemory memory;
   
    vk::Sampler sampler;
	uint32_t size;
    
    vk::Result init (vk::Device& logical, vk::PhysicalDevice& physical, vk::Format format, uint32_t _size);
    vk::Result tidy (vk::Device& logical);
    
	vk::Result transition (vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout);

    };

#endif /* VulkanShadingResource_hpp */
