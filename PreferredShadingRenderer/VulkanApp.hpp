//
//  VulkanApp.hpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanApp_hpp
#define VulkanApp_hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <array>
#include <vector>
#include <cstring>
#include <random>
#include <string>
#include <chrono>
#include <thread>

#include "VulkanVertex.hpp"
#include "VulkanShadingResource.hpp"
#include "Timer.hpp"

class VulkanApp
    {  // VulkanApp
    public:
    VulkanApp (uint32_t width, uint32_t height, std::string title, uint32_t objects, uint32_t interval, uint32_t resolution, uint32_t id, glm::vec3 clear = { 
		0.12156862745098039f, 
		0.12156862745098039f, 
		0.12156862745098039f });
   ~VulkanApp ();
    
    protected:
    const uint32_t            WINDOW_WIDTH;
    const uint32_t            WINDOW_HEIGHT;
    const std::string         WINDOW_TITLE;
    const std::array<float,4> WINDOW_CLEAR;
	const uint32_t			  MAX_FPS = 120;
    GLFWwindow*               window;
    
	Timer timing;

	uint32_t runID;

    private:

    // Initial Setup
    vk::Result createWindow                 ();
    vk::Result createSceneMesh              ();
    vk::Result createInstance               ();
    vk::Result createDebugCallback          ();
    vk::Result createSurface                ();
    vk::Result createDevice                 ();
    vk::Result createSwapChain              ();
    vk::Result createDepthBuffer            ();
	vk::Result createCommandPool            ();
    vk::Result createShadingResources       ();
    
    // Uniform Buffers
    vk::Result createGeometryUniformBuffer  ();
    vk::Result createShadingUniformBuffer   ();
    vk::Result createRasterUniformBuffer    ();
    
    // Descriptor Sets
    vk::Result createDescriptorPool         ();
    vk::Result createGeometryDescriptorSet  ();
    vk::Result createShadingDescriptorSet   ();
    vk::Result createRasterDescriptorSet    ();
    
    vk::Result createSemaphores             ();
    
    vk::Result createShadingRenderPass      ();
    vk::Result createRasterRenderPass       ();
    
    vk::Result createShadingFrameBuffer     ();
    vk::Result createRasterFrameBuffers     ();
    
    vk::Result createVertexBuffers          ();
    vk::Result createIndexBuffers           ();
    
    vk::Result createGeometryPipeline       ();
    vk::Result createShadingPipeline        ();
    vk::Result createRasterPipeline         ();
    
    vk::Result createShadingCommandBuffers  ();
    vk::Result createRasterCommandBuffers   ();

	void arrangeObjects          ();

	void createPhysicsState      ();
	void updatePhysicsState      ();
    
    void updateGeometryUniforms  ();
    void updateShadingUniforms   ();
    void updateRasterUniforms    ();
    
	void report     ();

	void fullRender ();
	void halfRender ();
    void loop       ();
    
    struct VulkanCore {
        vk::Instance       instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device         logicalDevice;
    } core;
    
    struct VulkanCommandState {
        vk::CommandPool   pool;
    } command;
    
    struct VulkanQueues {
        vk::Queue graphics; uint32_t graphicsIndex = UINT32_MAX;
        vk::Queue compute;  uint32_t computeIndex  = UINT32_MAX;
        vk::Queue transfer; uint32_t transferIndex = UINT32_MAX;
        vk::Queue present;  uint32_t presentIndex  = UINT32_MAX;
    } queues;

    struct VulkanSwapChain {
        vk::SurfaceKHR   surface;
        vk::SwapchainKHR swapchain;
        vk::Extent2D     extent;
        vk::Format       format;
        
        std::vector<vk::CommandBuffer> commandBuffers;
        std::vector<vk::Framebuffer>   framebuffers;
        std::vector<vk::ImageView>     views;
        std::vector<vk::Image>         images;
        
        uint32_t currentImage = 0;
        uint32_t nImages      = 0;

        struct Support {
            vk::SurfaceCapabilitiesKHR      capabilities;
            std::vector<vk::SurfaceFormatKHR>    formats;
            std::vector<vk::PresentModeKHR> presentModes;
        } supported;
    } swapchain;
    
    struct VulkanPipelines {
    
        vk::DescriptorPool descriptorPool;
    
        struct VulkanShadingPipeline {
            vk::RenderPass renderPass;
            
            vk::Format format = vk::Format::eR16G16B16A16Sfloat;
            
            vk::DescriptorSetLayout geometryDescriptorlayout;
            vk::DescriptorSetLayout shadingDescriptorLayout;
            
            vk::DescriptorSet  geometryDescriptorSet;
            vk::DescriptorSet  shadingDescriptorSet;
            
            vk::PipelineLayout geometryLayout;
            vk::PipelineLayout shadingLayout;
            
            vk::Pipeline       geometryPipeline;
            vk::Pipeline       shadingPipeline;
            
        } shading;
    
        struct VulkanRasterPipeline {
            vk::RenderPass renderPass;
            
            vk::Format pixelFormat;
            vk::Format depthFormat;
        
            vk::DescriptorSetLayout descriptorLayout;
            
            vk::DescriptorSet       descriptorSet;
            
            vk::PipelineLayout      layout;
            vk::Pipeline            pipeline;
            
        } raster;
    
    } pipelines;

    
    struct VulkanShaderModules {
        vk::ShaderModule vertex;
        vk::ShaderModule fragment;
    } shaders;
    
    struct VulkanDepthBuffer {
        vk::Image          image;
        vk::DeviceMemory   memory;
        vk::ImageView      view;
    } depth;
    
    struct VulkanSemaphores {
        vk::Semaphore presentReady;
		vk::Semaphore renderComplete;

		vk::Semaphore shadingComplete;
        vk::Semaphore rasterComplete;
    } semaphores;
    
    struct VulkanBuffers {
        struct VulkanBuffer {
            vk::Buffer       buffer;
            vk::DeviceMemory memory;
        };
        
        VulkanBuffer geometryUniform;
        VulkanBuffer shadingUniform;
        VulkanBuffer rasterUniform;
        
        VulkanBuffer sceneVertex;
        VulkanBuffer quadVertex;
        
        VulkanBuffer sceneIndex;
        VulkanBuffer quadIndex;
    } buffers;

    VkDebugReportCallbackEXT callback;

    static constexpr uint32_t maxObjects = 64;
	const uint32_t nObjects;
	static constexpr float offset = 2.5f;

    struct UniformBufferObjects {
        struct GeometryUBO {
            glm::mat4 model[maxObjects];
        } geometry;
    
        struct ShadingUBO {
            glm::vec4 lightPosition;
            glm::vec4 eyePosition;
			glm::vec4 materials[maxObjects];

        } shading;
        
        struct RasterUBO {
            glm::mat4 model[maxObjects];
            glm::mat4 view;
            glm::mat4 proj;
        } raster;
    } ubo;
    
    struct VulkanMeshes {
        struct Mesh {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t>  indices;
        };
        
        Mesh scene;
        
        Mesh quad;
    
    } meshes;

	struct PhysicsData {
		std::vector<glm::vec3> positions;  // bounding sphere centroids
		std::vector<glm::vec3> velocities; // derivatives of the motion

		std::vector<glm::vec3> orientations; //
		std::vector<glm::vec3> rotations;    // angular velocities
		float bounds = 0.0f;                          // size of the bounding spheres
	} simulation;

	struct InputParameters {
		float movementSpeed = 0.1f;
	} parameters;

	struct ArrangementData {
		std::vector<glm::vec3> translations;
		glm::vec3 centre;
		bool arranged = false;
	} arrangement;
    
    struct VulkanShadingState {

		uint32_t interval = 3;
    
        vk::CommandBuffer commandBuffer;
        vk::Framebuffer framebuffer;
    
		uint32_t BUFFER_SIZE = 2560;

        VulkanShadingResource position;
        VulkanShadingResource normal;
        VulkanShadingResource color;
        
        // the shaded scene data
        VulkanShadingResource result;
        
        enum Attachments {
            ePosition,
            eNormal,
            eColor,
            eLighting
        };
        
        enum Subpasses {
            eGenerateGeometryBuffers,
            eGenerateLightingBuffer
        };
        
    } shading;
    
    void createBuffer (
        vk::DeviceSize          size,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags properties,
        vk::Buffer&             buffer,
        vk::DeviceMemory        memory);
    
    std::default_random_engine rng;
    
    }; // VulkanApp

#endif /* VulkanApp_hpp */
