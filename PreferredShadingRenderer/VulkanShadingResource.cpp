//
//  VulkanShadingResource.cpp
//  PreferredRenderer
//
//  Created by macbook on 19/06/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#include "VulkanShadingResource.hpp"
#include "VulkanHelpers.hpp"
#include <iostream>

//
//
//
vk::Result VulkanShadingResource::init (vk::Device& logical, vk::PhysicalDevice& physical, vk::Format format, uint32_t _size)
    { // VulkanShadingResource :: init
    vk::Result result = vk::Result::eSuccess;
        
    vk::FormatProperties properties = physical.getFormatProperties(format);

	layout = vk::ImageLayout::eUndefined;
    size = _size;
    
    // create image handle
    vk::ImageCreateInfo imageCreateInfo = { };
        imageCreateInfo.imageType             = vk::ImageType::e2D;
        imageCreateInfo.format                = format;
        imageCreateInfo.extent.width          = size;
        imageCreateInfo.extent.height         = size;
        imageCreateInfo.extent.depth          = 1;
        imageCreateInfo.mipLevels             = 1;
        imageCreateInfo.arrayLayers           = 1;
        imageCreateInfo.samples               = vk::SampleCountFlagBits::e1;
        imageCreateInfo.tiling                = vk::ImageTiling::eOptimal;
        imageCreateInfo.initialLayout         = vk::ImageLayout::eUndefined;
        imageCreateInfo.usage                 =
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eInputAttachment |
            vk::ImageUsageFlagBits::eSampled;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices   = nullptr;
        imageCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
        imageCreateInfo.flags                 = vk::ImageCreateFlagBits {};
    
    result = logical.createImage(&imageCreateInfo, nullptr, &image);
    if (result != vk::Result::eSuccess) return result;
    
    vk::MemoryRequirements memoryRequirements = { };
    logical.getImageMemoryRequirements(image, &memoryRequirements);

	// allocate and map device memory
	vk::MemoryAllocateInfo allocationInfo = {};
		allocationInfo.allocationSize  = memoryRequirements.size;
		allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
				physical,
				memoryRequirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal);
        
    result = logical.allocateMemory(&allocationInfo, nullptr, &memory);
    if (result != vk::Result::eSuccess) return result;
    
    logical.bindImageMemory(image, memory, 0);

    // set up a sampler for use in our shader code
    vk::SamplerCreateInfo samplerCreateInfo = { };
        samplerCreateInfo.minFilter        = vk::Filter::eLinear;
        samplerCreateInfo.magFilter        = vk::Filter::eLinear;
        samplerCreateInfo.mipmapMode       = vk::SamplerMipmapMode::eNearest;
        samplerCreateInfo.addressModeU     = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeV     = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeW     = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.mipLodBias       = 0.0f;
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy    = 16.0f;
        samplerCreateInfo.compareEnable    = VK_FALSE;
        samplerCreateInfo.compareOp        = vk::CompareOp::eNever;
        samplerCreateInfo.minLod           = 0.0f;
        samplerCreateInfo.maxLod           = 0.0f;
        samplerCreateInfo.borderColor      = vk::BorderColor::eFloatOpaqueWhite;
        
        result = logical.createSampler(&samplerCreateInfo, nullptr, &sampler);
        if (result != vk::Result::eSuccess) return result;
    
    // set up an image view
    vk::ImageViewCreateInfo viewCreateInfo = { };
        viewCreateInfo.image                           = image;
        viewCreateInfo.viewType                        = vk::ImageViewType::e2D;
        viewCreateInfo.format                          = format;
        viewCreateInfo.components.r                    = vk::ComponentSwizzle::eR;
        viewCreateInfo.components.g                    = vk::ComponentSwizzle::eG;
        viewCreateInfo.components.b                    = vk::ComponentSwizzle::eB;
        viewCreateInfo.components.a                    = vk::ComponentSwizzle::eA;
        viewCreateInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        viewCreateInfo.subresourceRange.baseMipLevel   = 0;
        viewCreateInfo.subresourceRange.levelCount     = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount     = 1;
        
        result = logical.createImageView(&viewCreateInfo, nullptr, &view);
        if (result != vk::Result::eSuccess) return result;
        
    return result;
    
    } // VulkanShadingResource :: init

//
//
//
vk::Result VulkanShadingResource::tidy (vk::Device& logical)
    { // VulkanShadingResource :: tidy
    
    logical.destroyImageView (view);
    logical.destroySampler   (sampler);
    logical.destroyImage     (image);
	logical.freeMemory       (memory);
    
    return vk::Result::eSuccess;
    
    } // VulkanShadingResource :: tidy

//
//
//
vk::Result VulkanShadingResource::transition (vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout)
	{ // VulkanShadingResource :: transition
	vk::Result result = vk::Result::eSuccess;

	vk::ImageMemoryBarrier memoryBarrier = { };
		memoryBarrier.oldLayout                       = layout;
		memoryBarrier.newLayout                       = newLayout;
		memoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image                           = image;
		memoryBarrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
		memoryBarrier.subresourceRange.baseMipLevel   = 0;
		memoryBarrier.subresourceRange.levelCount     = 1;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.subresourceRange.layerCount     = 1;

	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;

	if (layout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal)
		{ // initialisation transition

		memoryBarrier.srcAccessMask = vk::AccessFlagBits { };
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		// this transition must happen before any rendering is done as 
		// the image is to be used as a render target for the g-buffers
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		} // initialisation transition

	else if (layout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{ // post-render transition

		memoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		// this transition must happen AFTER the current subpass operation has
		// finished but BEFORE the next fragment shading operation
		srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;

		} // post-render transition

	else if (layout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eColorAttachmentOptimal)
		{ // pre-render transition

		memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
		memoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		// this transition must happen AFTER the current subpass operation has
		// finished but BEFORE the next fragment shading operation
		srcStage = vk::PipelineStageFlagBits::eFragmentShader;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		} // pre-render transition

	else std::cout << "transition not recognised" << std::endl;

	commandBuffer.pipelineBarrier(
		srcStage, dstStage, 
		vk::DependencyFlagBits { }, 
		0, nullptr,						// memory barriers
		0, nullptr,						// buffer memory barriers
		1, &memoryBarrier);				// image memory barriers

	return result;
	} // VulkanShadingResource :: transition