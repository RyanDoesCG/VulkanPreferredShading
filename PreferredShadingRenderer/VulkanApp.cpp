//
//  VulkanApp.cpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//
#include <glm/gtc/matrix_transform.hpp>

#include "ErrorHandler.hpp"
#include "VulkanHelpers.hpp"
#include "VulkanShaders.hpp"
#include "VulkanDebug.hpp"
#include "VulkanApp.hpp"

#include "MeshIO.hpp"

glm::vec3 lightPosition = { 2.0f, -3.0f, 1.0f };
glm::vec3 eyePosition = { 0.0f, 5.0f, 0.0f };

//
//  The vulkan API allows us to opt in or out of
//  it's validation suite and offers good granularity
//  for which (if any) validation layers we want to
//  activate.
//
//  DEBUG is a constant available in most IDE's that
//  is defined when an executable is built as debug.
//  We use it to set a global boolean to avoid using
//  the macro definition too much and overworking the
//  preprocessor
//
#ifdef NDEBUG
	const bool DEBUG_MODE = false;
	const std::vector<const char*> requestedValidation =
		{    };
#else
	const bool DEBUG_MODE = true;
	const std::vector<const char*> requestedValidation =
		{ "VK_LAYER_LUNARG_standard_validation" };
#endif

uint8_t forwards   = 0;
uint8_t backwards  = 0;

uint8_t left       = 0;
uint8_t right      = 0;

uint8_t up         = 0;
uint8_t down       = 0;

bool animateLights = true;

uint32_t reset = 0;

bool regenerateMaterials = false;

float mouseX = 0.0f;
float mouseY = 0.0f;

float lastMouseX = 0.0f;
float lastMouseY = 0.0f;

float dragStartX = 0.0f;
float dragStartY = 0.0f;

bool leftMouseButton  = false;
bool rightMouseButton = false;

static void mouseScrollCallback (GLFWwindow* window, double xoffset, double yoffset)
	{

	eyePosition.y += yoffset * 0.1f;
	eyePosition.x += xoffset * 0.1f;

	}

static void mouseButtonCallback (GLFWwindow* window, int button, int action, int mods)
	{

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
		if (action == GLFW_PRESS)
			{
			dragStartX = mouseX;
			dragStartY = mouseY;
			rightMouseButton = true;
			}

		if (action == GLFW_RELEASE)
			{
			dragStartX = 0;
			dragStartY = 0;
			rightMouseButton = false;
			}
		}

	if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
		if (action == GLFW_PRESS)
			{
			dragStartX = mouseX;
			dragStartY = mouseY;
			leftMouseButton = true;
			}
		if (action == GLFW_RELEASE)
			{
			dragStartX = 0;
			dragStartY = 0;
			leftMouseButton = false;
			}
		}

	}

static void mouseMovementCallback (GLFWwindow* window, double xpos, double ypos)
	{

	lastMouseX = mouseX;
	lastMouseY = mouseY;

	mouseX = -1.0 + ((xpos / (float)1080) * 2);
	mouseY =  1.0 - ((ypos / (float)1080) * 2);

	mouseX = dragStartX - mouseX;
	mouseY = dragStartY - mouseY;

	}

void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mods)
    {
    if (key == GLFW_KEY_W)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) forwards = 1;
        if (action == GLFW_RELEASE) forwards = 0;
        }

    if (key == GLFW_KEY_S)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) backwards = 1;
        if (action == GLFW_RELEASE) backwards = 0;
        }
        
    if (key == GLFW_KEY_A)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) left = 1;
        if (action == GLFW_RELEASE) left = 0;
        }

    if (key == GLFW_KEY_D)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) right = 1;
        if (action == GLFW_RELEASE) right = 0;
        }

    if (key == GLFW_KEY_Q)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) up = 1;
        if (action == GLFW_RELEASE)  up = 0;
        }

    if (key == GLFW_KEY_E)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) down = 1;
        if (action == GLFW_RELEASE) down = 0;
        }

	if (key == GLFW_KEY_F)
		{
		if (action == GLFW_PRESS) regenerateMaterials = true;
		if (action == GLFW_RELEASE) regenerateMaterials = false;
		}
        
    if (key == GLFW_KEY_SPACE)
        {
		if (action == GLFW_PRESS) animateLights = !animateLights;
        }

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		reset = (reset + 1) % 3;
    }



//
//  constructor
//
//      width   - horizontal size of the window
//      height  - vertical size of the window
//      title   - string to display in menu bar
//      clear   - the colour to clear the screen with each frame
//
//  creates a window with a vulkan context configured for a forward
//  shading architecture
//
VulkanApp::VulkanApp (uint32_t width, uint32_t height, std::string title, uint32_t objects, uint32_t interval, uint32_t resolution, uint32_t id, glm::vec3 clear):
        WINDOW_WIDTH  (width),
        WINDOW_HEIGHT (height),
        WINDOW_TITLE  (title),
        WINDOW_CLEAR  ({ clear.x, clear.y, clear.z, 1.0f }),
        window        (nullptr),
		timing        (MAX_FPS),
		nObjects      (objects)
    { // VulkanApp :: VulkanApp

	shading.BUFFER_SIZE = resolution;
	shading.interval = interval;
    
	runID = id;
	timing.id = runID;

    // Initial Setup
    if (createWindow                () != vk::Result::eSuccess) ErrorHandler::fatal    ("GLFW Window Creation failure");
    if (createSceneMesh             () != vk::Result::eSuccess) ErrorHandler::fatal    ("Failed to prepare a mesh");
    if (createInstance              () != vk::Result::eSuccess) ErrorHandler::fatal    ("Vulkan instance creation failure");
    if (createDebugCallback         () != vk::Result::eSuccess) ErrorHandler::nonfatal ("Validation disabled");
    if (createSurface               () != vk::Result::eSuccess) ErrorHandler::fatal    ("Surface KHR creation failed");
    if (createDevice                () != vk::Result::eSuccess) ErrorHandler::fatal    ("Device creation failure");
    if (createSwapChain             () != vk::Result::eSuccess) ErrorHandler::fatal    ("Swapchain Creation failure");
    if (createDepthBuffer           () != vk::Result::eSuccess) ErrorHandler::fatal    ("Depth Buffer Creation failure");
	if (createCommandPool           () != vk::Result::eSuccess) ErrorHandler::fatal("Command Pool Creation Failure");
    if (createShadingResources      () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Resource Creation Failure");
    
    // Uniform Buffers
    if (createGeometryUniformBuffer () != vk::Result::eSuccess) ErrorHandler::fatal    ("Geometry Uniform Buffer Creationn failure");
    if (createShadingUniformBuffer  () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Uniform Buffer Creationn failure");
    if (createRasterUniformBuffer   () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster Uniform Buffer Creationn failure");
    
    // Descriptor Sets
    if (createDescriptorPool        () != vk::Result::eSuccess) ErrorHandler::fatal    ("Descriptor Pool Creation Failure");
    if (createGeometryDescriptorSet () != vk::Result::eSuccess) ErrorHandler::fatal    ("Geometry Descriptor Set Creation Failure");
    if (createShadingDescriptorSet  () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Descriptor Set Creation failure");
    if (createRasterDescriptorSet   () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster Descriptor Set Creation failure");
    
    if (createSemaphores            () != vk::Result::eSuccess) ErrorHandler::fatal    ("Semaphore creation failure");
    
    if (createShadingRenderPass     () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Render Pass Creation");
    if (createRasterRenderPass      () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster Render Pass Creation failure");
    
    if (createShadingFrameBuffer    () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading framebuffer creation failure");
    if (createRasterFrameBuffers    () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster framebuffer Creation failure");
    
    if (createVertexBuffers         () != vk::Result::eSuccess) ErrorHandler::fatal    ("Vertex Buffer Creation failure");
    if (createIndexBuffers          () != vk::Result::eSuccess) ErrorHandler::fatal    ("Index Buffer Creation failure");

    if (createGeometryPipeline      () != vk::Result::eSuccess) ErrorHandler::fatal    ("Geometry Graphics Pipeline Creation Failure");
    if (createShadingPipeline       () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Graphics Pipeline Creation Failure");
    if (createRasterPipeline        () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster Pipeline Creation failure");
    
    if (createShadingCommandBuffers () != vk::Result::eSuccess) ErrorHandler::fatal    ("Shading Command Pool/Buffer creation failure");
    if (createRasterCommandBuffers  () != vk::Result::eSuccess) ErrorHandler::fatal    ("Raster Command Pool/Buffer creation failure");

	createPhysicsState();

	arrangeObjects();

    loop ();
    
    } // VulkanApp :: VulkanApp


//
//  desctructor
//
//  destroys the vulkan instance and deallocates it's state
//
VulkanApp::~VulkanApp ()
    { // VulkanApp :: ~VulkanApp
    
    // destroy graphics pipeline
    core.logicalDevice.destroyPipeline(pipelines.raster.pipeline);
    
    // destroy semaphores
    core.logicalDevice.destroySemaphore(semaphores.presentReady);
    core.logicalDevice.destroySemaphore(semaphores.renderComplete);
	core.logicalDevice.destroySemaphore(semaphores.shadingComplete);
	core.logicalDevice.destroySemaphore(semaphores.rasterComplete);

    // destroy vertex buffer
    core.logicalDevice.destroyBuffer(buffers.sceneVertex.buffer);
    core.logicalDevice.freeMemory(buffers.sceneVertex.memory);

    core.logicalDevice.destroyBuffer(buffers.quadVertex.buffer);
    core.logicalDevice.freeMemory(buffers.quadVertex.memory);

    // destroy index buffer
    core.logicalDevice.destroyBuffer(buffers.sceneIndex.buffer);
    core.logicalDevice.freeMemory(buffers.sceneIndex.memory);

    core.logicalDevice.destroyBuffer(buffers.quadIndex.buffer);
    core.logicalDevice.freeMemory(buffers.quadIndex.memory);

    // destroy framebuffers
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        core.logicalDevice.destroyFramebuffer(swapchain.framebuffers[i]);
    
    // destroy render pass
    core.logicalDevice.destroyRenderPass(pipelines.raster.renderPass);
    
    // destroy descriptor pool
    core.logicalDevice.destroyDescriptorPool(pipelines.descriptorPool);
    
    // destroy pipeline layout
    core.logicalDevice.destroyDescriptorSetLayout(pipelines.raster.descriptorLayout);
    core.logicalDevice.destroyPipelineLayout(pipelines.raster.layout);
    
    // destroy uniform buffer
    core.logicalDevice.destroyBuffer(buffers.shadingUniform.buffer);
    core.logicalDevice.destroyBuffer(buffers.rasterUniform.buffer);
	core.logicalDevice.destroyBuffer(buffers.geometryUniform.buffer);

	core.logicalDevice.freeMemory(buffers.shadingUniform.memory);
	core.logicalDevice.freeMemory(buffers.rasterUniform.memory);
    core.logicalDevice.freeMemory(buffers.geometryUniform.memory);
    
    // destroy depth buffer
    core.logicalDevice.destroyImageView(depth.view);
    core.logicalDevice.destroyImage(depth.image);
    core.logicalDevice.freeMemory(depth.memory);
    
    // destroy swap chain
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        core.logicalDevice.destroyImageView(swapchain.views[i]);
    core.logicalDevice.destroySwapchainKHR(swapchain.swapchain);
    
    // destroy command pool
    core.logicalDevice.destroyCommandPool(command.pool, nullptr);
    
    VulkanDebug::DestroyDebugReportCallbackEXT((VkInstance)core.instance, callback, nullptr);

	core.logicalDevice.destroy();
	core.instance.destroy();
    
    // destroy glfw stuff
    glfwDestroyWindow(window);
    glfwTerminate();
    
    } // VulkanApp :: ~VulkanApp


//
//  createWindow
//
//  uses the constants defined at creation to
//  initialize a window using glfw
//
vk::Result VulkanApp::createWindow ()
    { // VulkanApp :: createWindow
    
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        WINDOW_TITLE.c_str(),
        nullptr, nullptr);
        
    glfwSetWindowUserPointer(window, this);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouseMovementCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);

    if (window == nullptr)
        return vk::Result::eIncomplete;
    else
        return vk::Result::eSuccess;
    
    } // VulkanApp :: createWindow


//
//  createSceneMesh
//
//  initializes an indexed mesh to render
//
vk::Result VulkanApp::createSceneMesh ()
    { // VulkanApp :: createSceneMesh
    
    // we need a quad mesh for passing over the geometry
    // buffer to compute lighting at each pixel
    meshes.quad.vertices = {
		{{-1.0f, 0.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, 0},
        {{-1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, 0},
        {{ 1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, 0},
        {{ 1.0f, 0.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, 0}};
        
    meshes.quad.indices = {
        0, 1, 2,
        2, 3, 0};
    
	std::uniform_int_distribution<int> dist(0, 3);

	// the four meshes are pre-loaded and selected from
	// at random for each object, the selected mesh is then
	// batched into the render mesh
	std::vector<std::vector<Vertex>>   vBuffers(1);
	std::vector<std::vector<uint32_t>> iBuffers(1);

	for (uint32_t i = 0; i < 1; ++i)
		{ // for each mesh

		std::string path = "models/bust_";
		path += std::to_string(i);
		path += ".mesh";

		MeshIO::readMeshFile(path.c_str(), vBuffers[i], iBuffers[i]);

		} // for each mesh

	for (uint32_t i = 0; i < nObjects; ++i)
		{ // for each objectssss

		uint32_t model = 0;
		MeshIO::assign(vBuffers[model], i);
		MeshIO::merge(meshes.scene.vertices, meshes.scene.indices, vBuffers[model], iBuffers[model]);

		} // for each object

	MeshIO::atlas (meshes.scene.vertices, nObjects, shading.BUFFER_SIZE);

	return vk::Result::eSuccess;
    
    } // VulkanApp :: createSceneMesh


//
//  createInstance
//
//  constructs an instance of the Vulkan API with
//  the appropriate extensions, validation layers
//  and the versions of both the API and the app
//  itself
//
vk::Result VulkanApp::createInstance ()
    { // VulkanApp :: createInstance
    vk::Result result = vk::Result::eSuccess;
    
    // our windowing framework will/may want some extentions
    // to be enabled and lets us query it for these C style
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);
    
    // we move the extentions requested by the windowing
    // library to a dynamic array and append our own
    // desired extensions
    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back (VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    // vulkan will want to know what version of the API it
    // should, and some semantic information about the app
    vk::ApplicationInfo appInfo          = { };
        appInfo.pApplicationName             = WINDOW_TITLE.c_str();
        appInfo.applicationVersion           = 1;
        appInfo.pEngineName                  = "No Engine";
        appInfo.engineVersion                = 1;
        appInfo.apiVersion                   = VK_API_VERSION_1_0;
    
    // we package everything the instance will want to know
    // into it's creation information structure and call
    // the initialization function
    vk::InstanceCreateInfo instanceInfo  = { };
        instanceInfo.pApplicationInfo        = &appInfo;
        instanceInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        instanceInfo.ppEnabledExtensionNames = extensions.data();
        instanceInfo.enabledLayerCount       = static_cast<uint32_t>(requestedValidation.size());
        instanceInfo.ppEnabledLayerNames     = requestedValidation.data();

    result = vk::createInstance(&instanceInfo, nullptr, &core.instance);
    
    if (result != vk::Result::eSuccess)
        ErrorHandler::nonfatal("error code: " + to_string(result));

    return result;

    } // VulkanApp :: createInstance


//
//  createDebugCallback
//
//  sets up a user-defined error reporting function
//
vk::Result VulkanApp::createDebugCallback ()
    { // VulkanApp :: createDebugCallback
    if (!DEBUG_MODE) return vk::Result::eNotReady;
    
    vk::Result result = vk::Result::eSuccess;

    VkDebugReportCallbackCreateInfoEXT createInfo = { };
        createInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = VulkanDebug::debugCallback;

    result = static_cast<vk::Result>(VulkanDebug::CreateDebugReportCallbackEXT((VkInstance)core.instance, &createInfo, nullptr, &callback));

    return result;
        
    } // VulkanApp :: createDebugCallback


//
//  createSurface
//
//  we have to break the c++ bindings to get glfw
//  to create a surface for us. It's ugly, but it's
//  concise compared to manually sorting the surface
//
vk::Result VulkanApp::createSurface ()
    { // VulkanApp :: createSurface
    vk::Result result = vk::Result::eSuccess;

    VkSurfaceKHR surf = VkSurfaceKHR(swapchain.surface);
    result = static_cast<vk::Result>(glfwCreateWindowSurface(
        static_cast<VkInstance>(core.instance),
        window,
        nullptr,
        &surf));
    swapchain.surface = (vk::SurfaceKHR)surf;
    
    return result;
    
    } // VulkanApp :: createSurface


//
//  createDevice
//
//  sets up the physical and logical vulkan devices, as
//  well as populating the queue indices object with the
//  index of relevant queues on the physical device
//
vk::Result VulkanApp::createDevice ()
    { // VulkanApp :: createDevice
    vk::Result result = vk::Result::eSuccess;
    
    // Get and check the physical devices on the system
    // but we'll just use the first device we find (for now)
    std::vector<vk::PhysicalDevice> candidateDevices =
        core.instance.enumeratePhysicalDevices();
        
    if (candidateDevices.size() < 1)
        return vk::Result::eIncomplete;

    core.physicalDevice = candidateDevices[0];
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        core.physicalDevice.getQueueFamilyProperties();
        
    // we populate a struct with data about the queues
    // we'll want to use for our application
    vk::DeviceQueueCreateInfo graphicsCreateInfo = { };
    vk::DeviceQueueCreateInfo computeCreateInfo  = { };
    vk::DeviceQueueCreateInfo transferCreateInfo = { };
    vk::DeviceQueueCreateInfo presentCreateInfo  = { };
    
    vk::Bool32 graphicsFound = VK_FALSE;
    vk::Bool32 computeFound  = VK_FALSE;
    vk::Bool32 transferFound = VK_FALSE;
    vk::Bool32 presentFound  = VK_FALSE;
    
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
        { // for each queue family
        
        // check if the queue supports graphics
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && !graphicsFound)
            { // found graphics queue family
            graphicsCreateInfo.queueFamilyIndex = i;
            queues.graphicsIndex                = i;
            graphicsFound                       = VK_TRUE;
            } // found graphics queue family
            
        // check if the queue supports compute
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && !computeFound)
            { // found compute queue family
            computeCreateInfo.queueFamilyIndex = i;
            queues.computeIndex                = i;
            computeFound                       = VK_TRUE;
            } // found compute queue family
            
        // check if the queue supports transfer
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) && !transferFound)
            { // found transfer queue family
            transferCreateInfo.queueFamilyIndex = i;
            queues.transferIndex                = i;
            transferFound                       = VK_TRUE;
            } // found transfer queue family
         
        // check if the queue supports present
        core.physicalDevice.getSurfaceSupportKHR(i, swapchain.surface, &presentFound);
        if (queueFamilyProperties[i].queueCount >= 0 && presentFound)
            { // found a present queue family
            presentCreateInfo.queueFamilyIndex = i;
            queues.presentIndex                = i;
            } // found a present queue family
        
        if (graphicsFound && computeFound && transferFound && presentFound)
            break;
         
        } // for each queue family
    
    // report which queues are missing from the physical device
    if (DEBUG_MODE)
        {
        if (queues.graphicsIndex == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find graphics queue");
        if (queues.computeIndex  == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find compute queue");
        if (queues.transferIndex == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find transfer queue");
        if (queues.presentIndex  == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find present queue");
        }
        
    // by default we'll always want the graphics queue to be
    // created. However, the other queues should only be created
    // if a queue with that family index has not already been created
    std::vector<vk::DeviceQueueCreateInfo> queueCreationInfos = { graphicsCreateInfo };
    //if (queues.computeIndex  != queues.graphicsIndex) queueCreationInfos.push_back(computeCreateInfo);
    //if (queues.transferIndex != queues.graphicsIndex) queueCreationInfos.push_back(transferCreateInfo);
    //if (queues.presentIndex  != queues.graphicsIndex) queueCreationInfos.push_back(presentCreateInfo);
    
    // it's sensible to only create one queue per family based on
    // the face current vulkan implementations that may not support
    // multiple queues in a single family
    for (vk::DeviceQueueCreateInfo& info : queueCreationInfos)
        { // for each queue creation struct
        info.queueCount = 1;
        info.pQueuePriorities = std::array<float, 1>{ 0.0f }.data();
        } // for each queue creation struct
    
    // the only extension we need to worry about at present is the ability
    // to use the basic Khronos (KHR) swapchain implementation
    const std::vector<const char*> extensions =
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        
    vk::PhysicalDeviceFeatures features = { };
        features.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo deviceCreateInfo = { };
        deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreationInfos.size());
        deviceCreateInfo.pQueueCreateInfos       = queueCreationInfos.data();
        deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
        deviceCreateInfo.enabledLayerCount       = 0;
        deviceCreateInfo.ppEnabledLayerNames     = nullptr;
        deviceCreateInfo.pEnabledFeatures        = &features;

    result = candidateDevices[0].createDevice(&deviceCreateInfo, nullptr, &core.logicalDevice);

    core.logicalDevice.getQueue(queues.graphicsIndex, 0, &queues.graphics);
    core.logicalDevice.getQueue(queues.presentIndex, 0, &queues.present);

    return result;
        
    } // VulkanApp :: createDevice


//
//  createSwapChain
//
//  sets up a basic khronos swap chain, as well as images and
//  views for the various buffers (however many there should be)
//
vk::Result VulkanApp::createSwapChain ()
    { // VulkanApp :: createSwapChain
    vk::Result result = vk::Result::eSuccess;

    // query the physical device
    core.physicalDevice.getSurfaceCapabilitiesKHR (swapchain.surface, &swapchain.supported.capabilities);
    swapchain.supported.formats      = core.physicalDevice.getSurfaceFormatsKHR      (swapchain.surface);
    swapchain.supported.presentModes = core.physicalDevice.getSurfacePresentModesKHR (swapchain.surface);

    // Choose a swap surface present format
    vk::SurfaceFormatKHR format      = VulkanHelpers::querySwapChainSurfaceFormat(swapchain.supported.formats);
    vk::PresentModeKHR   presentMode = VulkanHelpers::querySwapChainPresentMode(swapchain.supported.presentModes);
    vk::Extent2D         extent      = VulkanHelpers::querySwapChainExtents(swapchain.supported.capabilities, window);

    // Set an image count
    swapchain.nImages = swapchain.supported.capabilities.minImageCount + 1;
    if (swapchain.supported.capabilities.maxImageCount > 0 && swapchain.nImages > swapchain.supported.capabilities.maxImageCount)
        swapchain.nImages = swapchain.supported.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo = { };
        createInfo.surface          = swapchain.surface;
        createInfo.minImageCount    = swapchain.nImages;
        createInfo.imageFormat      = format.format;
        createInfo.imageColorSpace  = format.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
    
    // this should be handled better now that the queue family indexing
    // code has been renovated to assemble all the family types available
    // in the API
    uint32_t queueFamilyIndices[] = { queues.graphicsIndex, queues.presentIndex };
    if (queues.graphicsIndex != queues.presentIndex)
        {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
    else
        {
        createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
        }

        createInfo.preTransform   = swapchain.supported.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;

    result = core.logicalDevice.createSwapchainKHR(&createInfo, nullptr, &swapchain.swapchain);
    
    core.logicalDevice.getSwapchainImagesKHR(swapchain.swapchain, &swapchain.nImages, nullptr);
    swapchain.images.resize(swapchain.nImages);
    core.logicalDevice.getSwapchainImagesKHR(swapchain.swapchain, &swapchain.nImages, swapchain.images.data());
    
    swapchain.format = format.format;
    swapchain.extent = extent;
    
    pipelines.raster.pixelFormat = swapchain.format;
    
    swapchain.views.resize(swapchain.nImages);
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image view
        
        vk::ImageViewCreateInfo createInfo = { };
            createInfo.image                            = swapchain.images[i];
            createInfo.viewType                         = vk::ImageViewType::e2D;
            createInfo.format                           = pipelines.raster.pixelFormat;
            createInfo.subresourceRange.aspectMask      = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel    = 0;
            createInfo.subresourceRange.levelCount      = 1;
            createInfo.subresourceRange.baseArrayLayer  = 0;
            createInfo.subresourceRange.layerCount      = 1;
            
        result = core.logicalDevice.createImageView(&createInfo, nullptr, &swapchain.views[i]);
        
        if (result != vk::Result::eSuccess)
            return result;
        
        } // for each swapchain image view
    
    return result;
    
    } // VulkanApp :: createSwapChain


//
//
//
vk::Result VulkanApp::createDepthBuffer ()
    { // VulkanApp :: createDepthBuffer
    vk::Result result = vk::Result::eSuccess;
    
    pipelines.raster.depthFormat = vk::Format::eD16Unorm;
    
    vk::FormatProperties depthProperties;
    core.physicalDevice.getFormatProperties (pipelines.raster.depthFormat, &depthProperties);

    vk::ImageCreateInfo createInfo = { };
    if (depthProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        createInfo.tiling = vk::ImageTiling::eLinear;
    else if (depthProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        createInfo.tiling = vk::ImageTiling::eOptimal;
    else ErrorHandler::fatal("depth format unsupported");
        createInfo.imageType             = vk::ImageType::e2D;
        createInfo.format                = pipelines.raster.depthFormat;
        createInfo.extent.width          = swapchain.extent.width;
        createInfo.extent.height         = swapchain.extent.height;
        createInfo.extent.depth          = 1;
        createInfo.mipLevels             = 1;
        createInfo.arrayLayers           = 1;
        createInfo.samples               = vk::SampleCountFlagBits::e1;
        createInfo.initialLayout         = vk::ImageLayout::eUndefined;
        createInfo.usage                 = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
        createInfo.sharingMode           = vk::SharingMode::eExclusive;
    
    vk::ImageViewCreateInfo viewCreateInfo = { };
        viewCreateInfo.format                          = pipelines.raster.depthFormat;
        viewCreateInfo.components.r                    = vk::ComponentSwizzle::eR;
        viewCreateInfo.components.g                    = vk::ComponentSwizzle::eG;
        viewCreateInfo.components.b                    = vk::ComponentSwizzle::eB;
        viewCreateInfo.components.a                    = vk::ComponentSwizzle::eA;
        viewCreateInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
        viewCreateInfo.subresourceRange.baseMipLevel   = 0;
        viewCreateInfo.subresourceRange.levelCount     = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount     = 1;
        viewCreateInfo.viewType                        = vk::ImageViewType::e2D;
        
    result = core.logicalDevice.createImage(&createInfo, nullptr, &depth.image);

    if (result != vk::Result::eSuccess)
        return result;
    
    vk::MemoryRequirements memoryRequirements = { };
        core.logicalDevice.getImageMemoryRequirements(depth.image, &memoryRequirements);
        
    vk::MemoryAllocateInfo allocationInfo = { };
        allocationInfo.allocationSize  = memoryRequirements.size;
        allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(core.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &depth.memory);
    
    if (result != vk::Result::eSuccess)
        return result;

    core.logicalDevice.bindImageMemory(depth.image, depth.memory, 0);
    
    viewCreateInfo.image = depth.image;
    core.logicalDevice.createImageView(&viewCreateInfo, nullptr, &depth.view);
    
    return result;
    
    } // VulkanApp :: createDepthBuffer

//
//
//
vk::Result VulkanApp::createCommandPool ()
	{ // VulkanApp :: createCommandPool

	vk::Result result = vk::Result::eSuccess;

	vk::CommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.queueFamilyIndex = queues.graphicsIndex;

	result = core.logicalDevice.createCommandPool(&poolCreateInfo, nullptr, &command.pool);

	if (result != vk::Result::eSuccess)
		return result;
	
	} // VulkanApp :: createCommandPool

//
//
//
vk::Result VulkanApp::createShadingResources ()
    { // VulkanApp :: createShadingResources
    vk::Result result = vk::Result::eSuccess;

    result = shading.position.init(core.logicalDevice, core.physicalDevice, pipelines.shading.format, shading.BUFFER_SIZE);
    if (result != vk::Result::eSuccess) 
		return result;

    result = shading.normal.init(core.logicalDevice, core.physicalDevice, pipelines.shading.format, shading.BUFFER_SIZE);
    if (result != vk::Result::eSuccess)
		return result;

    result = shading.color.init(core.logicalDevice, core.physicalDevice, pipelines.shading.format, shading.BUFFER_SIZE);
    if (result != vk::Result::eSuccess) 
		return result;

    result = shading.result.init(core.logicalDevice, core.physicalDevice, pipelines.shading.format, shading.BUFFER_SIZE);
    if (result != vk::Result::eSuccess) 
		return result;

	vk::CommandBuffer commandBuffer = VulkanHelpers::beginSingleUseCommand(core.logicalDevice, command.pool);
	shading.position .transition(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal);
	shading.normal   .transition(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal);
	shading.color    .transition(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal);
	shading.result   .transition(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal);
	VulkanHelpers::endSingleUseCommand(core.logicalDevice, command.pool, commandBuffer, queues.graphics);

    return result;
    } // VulkanApp :: createShadingResources


//
//
//
vk::Result VulkanApp::createGeometryUniformBuffer ()
    { // VulkanApp :: createGeometryUniformBuffer
    vk::Result result = vk::Result::eSuccess;

    // then once we have an acceptable default we can set up
    // a buffer that we'll use to pass the data to the GPU
    vk::DeviceSize size = sizeof(UniformBufferObjects::GeometryUBO);
    
    vk::BufferCreateInfo bufferCreateInfo = { };
        bufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eUniformBuffer;
        bufferCreateInfo.size                  = size;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices   = nullptr;
        bufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
        
    result = core.logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffers.geometryUniform.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    // once we've created the buffer we can allocate some VRAM
    // to store our uniform data
    vk::MemoryRequirements bufferRequirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.geometryUniform.buffer, &bufferRequirements);

    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.allocationSize  = bufferRequirements.size;
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        bufferRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.geometryUniform.memory);
    if (result != vk::Result::eSuccess)
        return result;
    
    // finally we can map the data across to the GPU
    void* data;

    result = core.logicalDevice.mapMemory(buffers.geometryUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
    if (result != vk::Result::eSuccess) return result;

    memcpy(data, &ubo.geometry, (size_t)size);

    core.logicalDevice.unmapMemory(buffers.geometryUniform.memory);
    core.logicalDevice.bindBufferMemory(buffers.geometryUniform.buffer, buffers.geometryUniform.memory, 0);

    return result;
    } // VulkanApp :: createGeometryUniformBuffer


//
//
//
vk::Result VulkanApp::createShadingUniformBuffer ()
    { // VulkanApp :: createShadingUniformBuffer
    vk::Result result = vk::Result::eSuccess;
    
		eyePosition.y = sqrt(nObjects) * 2.0f;

    ubo.shading.lightPosition = glm::vec4(lightPosition.x, lightPosition.y, lightPosition.z, 1.0);
    ubo.shading.eyePosition   = glm::vec4(eyePosition.x, eyePosition.y, eyePosition.z, 1.0);

	std::uniform_real_distribution<float> dist (0.0, 1.0);
	rng.seed(time(0));
	for (uint32_t i = 0; i < nObjects; ++i)
		ubo.shading.materials[i] = { dist(rng), dist(rng), dist(rng), dist(rng) };
    
    // then once we have an acceptable default we can set up
    // a buffer that we'll use to pass the data to the GPU
    vk::DeviceSize size = sizeof(UniformBufferObjects::ShadingUBO);
    
    vk::BufferCreateInfo bufferCreateInfo = { };
        bufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eUniformBuffer;
        bufferCreateInfo.size                  = size;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices   = nullptr;
        bufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
        
    result = core.logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffers.shadingUniform.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    // once we've created the buffer we can allocate some VRAM
    // to store our uniform data
    vk::MemoryRequirements bufferRequirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.shadingUniform.buffer, &bufferRequirements);

    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.allocationSize  = bufferRequirements.size;
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        bufferRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.shadingUniform.memory);
    if (result != vk::Result::eSuccess)
        return result;
    
    // finally we can map the data across to the GPU
    void* data;

    result = core.logicalDevice.mapMemory(buffers.shadingUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
    if (result != vk::Result::eSuccess) return result;

    memcpy(data, &ubo.shading, (size_t)size);

    core.logicalDevice.unmapMemory(buffers.shadingUniform.memory);
    core.logicalDevice.bindBufferMemory(buffers.shadingUniform.buffer, buffers.shadingUniform.memory, 0);

    return result;
    } // VulkanApp :: createShadingUniformBuffer


//
//
//
vk::Result VulkanApp::createRasterUniformBuffer ()
    { // VulkanApp :: createRasterUniformBuffer
    vk::Result result = vk::Result::eSuccess;

    ubo.raster.view = glm::lookAt(
        eyePosition,                      // position
        glm::vec3 { 0.0f, 0.0f, 0.00f },  // center
        glm::vec3 { 0.0f, 0.0f, 1.00f }); // world up

    ubo.raster.proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.01f, 100.0f);
    ubo.raster.proj[1][1] *= -1;
    
    // then once we have an acceptable default we can set up
    // a buffer that we'll use to pass the data to the GPU
    vk::DeviceSize size = sizeof(UniformBufferObjects::RasterUBO);
    
    vk::BufferCreateInfo bufferCreateInfo = { };
        bufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eUniformBuffer;
        bufferCreateInfo.size                  = size;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices   = nullptr;
        bufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
        
    result = core.logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffers.rasterUniform.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    // once we've created the buffer we can allocate some VRAM
    // to store our uniform data
    vk::MemoryRequirements bufferRequirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.rasterUniform.buffer, &bufferRequirements);

    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.allocationSize  = bufferRequirements.size;
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        bufferRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.rasterUniform.memory);
    if (result != vk::Result::eSuccess)
        return result;
    
    // finally we can map the data across to the GPU
    void* data;

    result = core.logicalDevice.mapMemory(buffers.rasterUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
    if (result != vk::Result::eSuccess) return result;

    memcpy(data, &ubo.raster, (size_t)size);

    core.logicalDevice.unmapMemory(buffers.rasterUniform.memory);
    core.logicalDevice.bindBufferMemory(buffers.rasterUniform.buffer, buffers.rasterUniform.memory, 0);

    return result;
    } // VulkanApp :: createRasterUniformBuffer

//
//
//
vk::Result VulkanApp::createDescriptorPool ()
    { // VulkanApp :: createDescriptorPool
    vk::Result result = vk::Result::eSuccess;
    
    vk::DescriptorPoolSize sizes [3];
        sizes[0].type             = vk::DescriptorType::eUniformBuffer;
        sizes[0].descriptorCount  = 3;
        
        sizes[1].type             = vk::DescriptorType::eInputAttachment;
        sizes[1].descriptorCount  = 3;

		sizes[2].type             = vk::DescriptorType::eCombinedImageSampler;
		sizes[2].descriptorCount  = 1;
        
    vk::DescriptorPoolCreateInfo poolCreateInfo = { };
        poolCreateInfo.poolSizeCount = 3;
        poolCreateInfo.pPoolSizes    = sizes;
        poolCreateInfo.maxSets       = 3;
        
    result = core.logicalDevice.createDescriptorPool (
        &poolCreateInfo,
        nullptr,
        &pipelines.descriptorPool);
        
    if (result != vk::Result::eSuccess)
        { // failed to create pool
        std::cout << "Failed to create descriptor pool" << std::endl;
        return result;
        } // failed to create pool
    
    return result;
    } // VulkanApp :: createDescriptorPool

//
//
//
vk::Result VulkanApp::createGeometryDescriptorSet ()
    { // VulkanApp :: createGeometryDescriptorSet
    vk::Result result = vk::Result::eSuccess;
    
    // The geometry pipeline will have only a uniform
    // buffer containing model transform data
    vk::DescriptorSetLayoutBinding layoutBindings [1];
        layoutBindings[0].binding             = 0;
        layoutBindings[0].descriptorCount     = 1;
        layoutBindings[0].descriptorType      = vk::DescriptorType::eUniformBuffer;
        layoutBindings[0].stageFlags          = vk::ShaderStageFlagBits::eVertex;
        layoutBindings[0].pImmutableSamplers  = nullptr;
    
    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = { };
        layoutCreateInfo.bindingCount  = 1;
        layoutCreateInfo.pBindings     = layoutBindings;
        
    result = core.logicalDevice.createDescriptorSetLayout(
        &layoutCreateInfo,
        nullptr,
        &pipelines.shading.geometryDescriptorlayout);
        
    if (result != vk::Result::eSuccess)
        { // failed to create layout
        std::cout << "Failed to create geometry descriptor set layout" << std::endl;
        return result;
        } // failed to create layout
    
    // Once we've created a layout for the set we can allocate space from
    // the pool to accomodate the layout
    vk::DescriptorSetAllocateInfo allocationInfo = { };
        allocationInfo.descriptorPool      = pipelines.descriptorPool;
        allocationInfo.descriptorSetCount  = 1;
        allocationInfo.pSetLayouts         = &pipelines.shading.geometryDescriptorlayout;
        
    result = core.logicalDevice.allocateDescriptorSets(&allocationInfo, &pipelines.shading.geometryDescriptorSet);
    if (result != vk::Result::eSuccess)
        { // failed to allocate set
        std::cout << "Failed to create geometry descriptor set layout" << std::endl;
        return result;
        } // failed to allocate set
        
    // following the allocation we can create the descriptor set
    vk::DescriptorBufferInfo bufferInfo = { };
        bufferInfo.buffer = buffers.geometryUniform.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObjects::GeometryUBO);
        
    vk::WriteDescriptorSet descriptorWrite = { };
        descriptorWrite.dstSet           = pipelines.shading.geometryDescriptorSet;
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        
    core.logicalDevice.updateDescriptorSets (1, &descriptorWrite, 0, nullptr);
        
    return result;
    } // VulkanApp :: createGeometryDescriptorSet

//
//
//
vk::Result VulkanApp::createShadingDescriptorSet ()
    { // VulkanApp :: createShadingDescriptorSet
    vk::Result result = vk::Result::eSuccess;
    
    // The shading pipeline will need 3 samplers and a uniform
    // buffer to compute the shading results
    vk::DescriptorSetLayoutBinding layoutBindings [4];
    
        // Uniform Buffer
        layoutBindings[0].binding             = 0;
        layoutBindings[0].descriptorCount     = 1;
        layoutBindings[0].descriptorType      = vk::DescriptorType::eUniformBuffer;
        layoutBindings[0].stageFlags          = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        layoutBindings[0].pImmutableSamplers  = nullptr;

        // Position Buffer
        layoutBindings[1].binding             = 1;
        layoutBindings[1].descriptorCount     = 1;
        layoutBindings[1].descriptorType      = vk::DescriptorType::eInputAttachment;
        layoutBindings[1].stageFlags          = vk::ShaderStageFlagBits::eFragment;
        layoutBindings[1].pImmutableSamplers  = nullptr;

        // Normal Buffer
        layoutBindings[2].binding             = 2;
        layoutBindings[2].descriptorCount     = 1;
        layoutBindings[2].descriptorType      = vk::DescriptorType::eInputAttachment;
        layoutBindings[2].stageFlags          = vk::ShaderStageFlagBits::eFragment;
        layoutBindings[2].pImmutableSamplers  = nullptr;
        
        // Color Buffer
        layoutBindings[3].binding             = 3;
        layoutBindings[3].descriptorCount     = 1;
        layoutBindings[3].descriptorType      = vk::DescriptorType::eInputAttachment;
        layoutBindings[3].stageFlags          = vk::ShaderStageFlagBits::eFragment;
        layoutBindings[3].pImmutableSamplers  = nullptr;

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = { };
        layoutCreateInfo.bindingCount  = 4;
        layoutCreateInfo.pBindings     = layoutBindings;
        
    result = core.logicalDevice.createDescriptorSetLayout(
        &layoutCreateInfo,
        nullptr,
        &pipelines.shading.shadingDescriptorLayout);
        
    if (result != vk::Result::eSuccess)
        { // failed to create layout
        std::cout << "Failed to create shading descriptor set layout" << std::endl;
        return result;
        } // failed to create layout
    
    // Once we've created a layout for the set we can allocate space from
    // the pool to accomodate the layout
    vk::DescriptorSetAllocateInfo allocationInfo = { };
        allocationInfo.descriptorPool      = pipelines.descriptorPool;
        allocationInfo.descriptorSetCount  = 1;
        allocationInfo.pSetLayouts         = &pipelines.shading.shadingDescriptorLayout;
        
    result = core.logicalDevice.allocateDescriptorSets(&allocationInfo, &pipelines.shading.shadingDescriptorSet);
    if (result != vk::Result::eSuccess)
        { // failed to allocate set
        std::cout << "Failed to create shading descriptor set" << std::endl;
        return result;
        } // failed to allocate set
        
    // following the allocation we can create the descriptor set
    vk::DescriptorBufferInfo bufferInfo = { };
        bufferInfo.buffer = buffers.shadingUniform.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObjects::ShadingUBO);
        
    vk::DescriptorImageInfo imageInfo [3];
        
        // Position Buffer
        imageInfo[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo[0].imageView   = shading.position.view;
        imageInfo[0].sampler     = shading.position.sampler;
        
        // Normal Buffer
        imageInfo[1].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo[1].imageView   = shading.normal.view;
        imageInfo[1].sampler     = shading.normal.sampler;
        
        // Albedo Buffer
        imageInfo[2].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo[2].imageView   = shading.color.view;
        imageInfo[2].sampler     = shading.color.sampler;
        
    vk::WriteDescriptorSet descriptorWrites [4];
    
        // Uniform Buffer
        descriptorWrites[0].dstSet           = pipelines.shading.shadingDescriptorSet;
        descriptorWrites[0].dstBinding       = 0;
        descriptorWrites[0].dstArrayElement  = 0;
        descriptorWrites[0].descriptorType   = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount  = 1;
        descriptorWrites[0].pBufferInfo      = &bufferInfo;

        // Position Buffer
        descriptorWrites[1].dstSet           = pipelines.shading.shadingDescriptorSet;
        descriptorWrites[1].dstBinding       = 1;
        descriptorWrites[1].dstArrayElement  = 0;
        descriptorWrites[1].descriptorType   = vk::DescriptorType::eInputAttachment;
        descriptorWrites[1].descriptorCount  = 1;
        descriptorWrites[1].pImageInfo       = &imageInfo[0];
        
        // Normal Buffer
        descriptorWrites[2].dstSet           = pipelines.shading.shadingDescriptorSet;
        descriptorWrites[2].dstBinding       = 2;
        descriptorWrites[2].dstArrayElement  = 0;
        descriptorWrites[2].descriptorType   = vk::DescriptorType::eInputAttachment;
        descriptorWrites[2].descriptorCount  = 1;
        descriptorWrites[2].pImageInfo       = &imageInfo[1];
        
        // Color Buffer
        descriptorWrites[3].dstSet           = pipelines.shading.shadingDescriptorSet;
        descriptorWrites[3].dstBinding       = 3;
        descriptorWrites[3].dstArrayElement  = 0;
        descriptorWrites[3].descriptorType   = vk::DescriptorType::eInputAttachment;
        descriptorWrites[3].descriptorCount  = 1;
        descriptorWrites[3].pImageInfo       = &imageInfo[2];
    
    core.logicalDevice.updateDescriptorSets (4, descriptorWrites, 0, nullptr);
    
    return result;
    } // VulkanApp :: createShadingDescriptorSet

//
//
//
vk::Result VulkanApp::createRasterDescriptorSet ()
    { // VulkanApp :: createRasterDescriptorSet
    vk::Result result = vk::Result::eSuccess;
    
    // The raster pipeline will need 1 sampler and a uniform
    // buffer to compute the screen-space positions
    vk::DescriptorSetLayoutBinding layoutBindings [2];
    
        // Uniform Buffer
        layoutBindings[0].binding             = 0;
        layoutBindings[0].descriptorCount     = 1;
        layoutBindings[0].descriptorType      = vk::DescriptorType::eUniformBuffer;
        layoutBindings[0].stageFlags          = vk::ShaderStageFlagBits::eVertex;
        layoutBindings[0].pImmutableSamplers  = nullptr;

        // G-Buffers
        layoutBindings[1].binding             = 1;
        layoutBindings[1].descriptorCount     = 1;
        layoutBindings[1].descriptorType      = vk::DescriptorType::eCombinedImageSampler;
        layoutBindings[1].stageFlags          = vk::ShaderStageFlagBits::eFragment;
        layoutBindings[1].pImmutableSamplers  = nullptr;

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = { };
        layoutCreateInfo.bindingCount  = 2;
        layoutCreateInfo.pBindings     = layoutBindings;
        
    result = core.logicalDevice.createDescriptorSetLayout(
        &layoutCreateInfo,
        nullptr,
        &pipelines.raster.descriptorLayout);
        
    if (result != vk::Result::eSuccess)
        { // failed to create layout
        std::cout << "Failed to create raster descriptor set layout" << std::endl;
        return result;
        } // failed to create layout
    
    // Once we've created a layout for the set we can allocate space from
    // the pool to accomodate the layout
    vk::DescriptorSetAllocateInfo allocationInfo = { };
        allocationInfo.descriptorPool      = pipelines.descriptorPool;
        allocationInfo.descriptorSetCount  = 1;
        allocationInfo.pSetLayouts         = &pipelines.raster.descriptorLayout;
        
    result = core.logicalDevice.allocateDescriptorSets(&allocationInfo, &pipelines.raster.descriptorSet);
    if (result != vk::Result::eSuccess)
        { // failed to allocate set
        std::cout << "Failed to create raster descriptor set" << std::endl;
        return result;
        } // failed to allocate set
        
    // following the allocation we can create the descriptor set
    vk::DescriptorBufferInfo bufferInfo = { };
        bufferInfo.buffer = buffers.rasterUniform.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObjects::RasterUBO);
        
    vk::DescriptorImageInfo imageInfo = { };
        
        // Position Buffer
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView   = shading.result.view;
        imageInfo.sampler     = shading.result.sampler;

        
    vk::WriteDescriptorSet descriptorWrites [2];
    
        // Uniform Buffer
        descriptorWrites[0].dstSet           = pipelines.raster.descriptorSet;
        descriptorWrites[0].dstBinding       = 0;
        descriptorWrites[0].dstArrayElement  = 0;
        descriptorWrites[0].descriptorType   = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount  = 1;
        descriptorWrites[0].pBufferInfo      = &bufferInfo;

        // Image Samplers
        descriptorWrites[1].dstSet           = pipelines.raster.descriptorSet;
        descriptorWrites[1].dstBinding       = 1;
        descriptorWrites[1].dstArrayElement  = 0;
        descriptorWrites[1].descriptorType   = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount  = 1;
        descriptorWrites[1].pImageInfo       = &imageInfo;

    core.logicalDevice.updateDescriptorSets (2, descriptorWrites, 0, nullptr);
    
    return result;
    } // VulkanApp :: createRasterDescriptorSet


//
//
//
vk::Result VulkanApp::createSemaphores ()
    { // VulkanApp :: createSemaphores
    vk::Result result = vk::Result::eSuccess;
    
    vk::SemaphoreCreateInfo semaphoreCreateInfo = { };
		semaphoreCreateInfo.flags = vk::SemaphoreCreateFlagBits{};

    result = core.logicalDevice.createSemaphore (&semaphoreCreateInfo, nullptr, &semaphores.presentReady);
    if (result != vk::Result::eSuccess)
        return result;

	result = core.logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.renderComplete);
	if (result != vk::Result::eSuccess)
		return result;

	result = core.logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.shadingComplete);
	if (result != vk::Result::eSuccess)
		return result;
        
	result = core.logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphores.rasterComplete);
	if (result != vk::Result::eSuccess)
		return result;

    return result;
    
    } // VulkanApp :: createSemaphores


//
//
//
vk::Result VulkanApp::createShadingRenderPass ()
    { // VulkanApp :: createShadingRenderPass
    vk::Result result = vk::Result::eSuccess;
    
    // each attachment used by the render pass will require a
    // description regardless of if it's used for reading, writing
    // or both
    vk::AttachmentDescription attachmentDescriptions[4];
        
        // position buffer attachment
        attachmentDescriptions[0].format         = pipelines.shading.format;
        attachmentDescriptions[0].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[0].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[0].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[0].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[0].finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
        
        // normal buffer attachment
        attachmentDescriptions[1].format         = pipelines.shading.format;
        attachmentDescriptions[1].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[1].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[1].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[1].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[1].finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
        
        // color buffer attachment
        attachmentDescriptions[2].format         = pipelines.shading.format;
        attachmentDescriptions[2].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[2].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[2].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[2].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[2].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[2].finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
        
        // lighting buffer attachment
        attachmentDescriptions[3].format         = pipelines.shading.format;
        attachmentDescriptions[3].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[3].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[3].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[3].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[3].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[3].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[3].finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;
    
    // our shading pipeline will have 2 render passes. One to populate the
    // geometry buffers with information about the mesh and one to run the
    // lighting computation and fill in the shading buffer
    vk::SubpassDescription subpasses [2];
    
    // Subpass One: Populate Geometry Buffers
    vk::AttachmentReference geometryBufferAttachments [3];
        geometryBufferAttachments[0] = { 0, vk::ImageLayout::eColorAttachmentOptimal };
        geometryBufferAttachments[1] = { 1, vk::ImageLayout::eColorAttachmentOptimal };
        geometryBufferAttachments[2] = { 2, vk::ImageLayout::eColorAttachmentOptimal };

        subpasses[0].pipelineBindPoint        = vk::PipelineBindPoint::eGraphics;
        subpasses[0].colorAttachmentCount     = 3;
        subpasses[0].pColorAttachments        = geometryBufferAttachments;
        subpasses[0].pDepthStencilAttachment  = nullptr;
    
    // Subpass Two: Compute Shading Results
    vk::AttachmentReference shadingBufferAttachment = { };
        shadingBufferAttachment.attachment = 3;
        shadingBufferAttachment.layout     = vk::ImageLayout::eColorAttachmentOptimal;
     
    vk::AttachmentReference inputBufferAttachments [3];
        inputBufferAttachments[0] = { 0, vk::ImageLayout::eShaderReadOnlyOptimal };
        inputBufferAttachments[1] = { 1, vk::ImageLayout::eShaderReadOnlyOptimal };
        inputBufferAttachments[2] = { 2, vk::ImageLayout::eShaderReadOnlyOptimal };
        
        subpasses[1].pipelineBindPoint        = vk::PipelineBindPoint::eGraphics;
        subpasses[1].colorAttachmentCount     = 1;
        subpasses[1].pColorAttachments        = &shadingBufferAttachment;
        subpasses[1].pDepthStencilAttachment  = nullptr;
        subpasses[1].inputAttachmentCount     = 3;
        subpasses[1].pInputAttachments        = inputBufferAttachments;
        
    // we then need to define the dependencies between the individual
    // subpasses to ensure they're executed in the correct order taking
    // the correct precautions
    vk::SubpassDependency dependencies [3];
    
        // the dependency for entry into the renderpass
        dependencies[0].srcSubpass     = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass     = 0;
        dependencies[0].srcStageMask   = vk::PipelineStageFlagBits::eTopOfPipe;
        dependencies[0].dstStageMask   = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask  = vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dstAccessMask  = vk::AccessFlagBits::eShaderRead;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;
        
        // the dependency between the subpasses to make the geometry
        // buffer attachments shader readable for the lighting pass
        dependencies[1].srcSubpass       = 0;
        dependencies[1].dstSubpass       = 1;
        dependencies[1].srcStageMask     = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask     = vk::PipelineStageFlagBits::eFragmentShader;
        dependencies[1].srcAccessMask    = vk::AccessFlagBits::eShaderRead;
        dependencies[1].dstAccessMask    = vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dependencyFlags  = vk::DependencyFlagBits::eByRegion;
     
        // the dependency for exit out of the render pass
        dependencies[2].srcSubpass       = 1;
        dependencies[2].dstSubpass       = VK_SUBPASS_EXTERNAL;
        dependencies[2].srcStageMask     = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[2].dstStageMask     = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[2].srcAccessMask    = vk::AccessFlagBits { };
        //    vk::AccessFlagBits::eColorAttachmentRead |
        //    vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[2].dstAccessMask = vk::AccessFlagBits { };
        dependencies[2].dependencyFlags  = vk::DependencyFlagBits::eByRegion;
    
    vk::RenderPassCreateInfo createInfo = { };
        createInfo.attachmentCount  = 4;
        createInfo.pAttachments     = attachmentDescriptions;
        createInfo.subpassCount     = 2;
        createInfo.pSubpasses       = subpasses;
        createInfo.dependencyCount  = 3;
        createInfo.pDependencies    = dependencies;
        
    result = core.logicalDevice.createRenderPass(&createInfo, nullptr, &pipelines.shading.renderPass);
    
    if (result != vk::Result::eSuccess)
        std::cout << "Shading RenderPass ERROR: " << vk::to_string(result);
    
    return result;
    } // VulkanApp :: createShadingRenderPass


//
//
//
vk::Result VulkanApp::createRasterRenderPass ()
    { // VulkanApp :: createRasterRenderPass
    vk::Result result = vk::Result::eSuccess;
    
    // each attachment used by the render pass will require a
    // description regardless of if it's used for reading, writing
    // or both
    vk::AttachmentDescription attachmentDescriptions[3];
    
        // shading buffer attachment
        attachmentDescriptions[0].format          = pipelines.shading.format;
        attachmentDescriptions[0].samples         = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[0].loadOp          = vk::AttachmentLoadOp::eLoad;
        attachmentDescriptions[0].storeOp         = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[0].stencilLoadOp   = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[0].stencilStoreOp  = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[0].initialLayout   = vk::ImageLayout::eColorAttachmentOptimal;
        attachmentDescriptions[0].finalLayout     = vk::ImageLayout::eColorAttachmentOptimal;
    
        // frame buffer attachment
        attachmentDescriptions[1].format          = pipelines.raster.pixelFormat;
        attachmentDescriptions[1].samples         = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[1].loadOp          = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[1].storeOp         = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[1].stencilLoadOp   = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[1].stencilStoreOp  = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[1].initialLayout   = vk::ImageLayout::eUndefined;
        attachmentDescriptions[1].finalLayout     = vk::ImageLayout::ePresentSrcKHR;
        
        // depth buffer attachment
        attachmentDescriptions[2].format          = pipelines.raster.depthFormat;
        attachmentDescriptions[2].samples         = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[2].loadOp          = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[2].storeOp         = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[2].stencilLoadOp   = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[2].stencilStoreOp  = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[2].initialLayout   = vk::ImageLayout::eUndefined;
        attachmentDescriptions[2].finalLayout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    
    // the raster pipeline only contains one subpass to rasterize to the screen
    vk::SubpassDescription subpass [1];
    
    // Subpass One: rasterize geometry and lookup shading values
    vk::AttachmentReference rasterOutputAttachments [2];
        rasterOutputAttachments[0] = { 1, vk::ImageLayout::eColorAttachmentOptimal };        // Frame Buffer
        rasterOutputAttachments[1] = { 2, vk::ImageLayout::eDepthStencilAttachmentOptimal }; // Depth Buffer
        
    vk::AttachmentReference rasterInputAttachments [1];
        rasterInputAttachments[0] = { 0, vk::ImageLayout::eShaderReadOnlyOptimal };          // Shading Buffer
        
    subpass[0].pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass[0].colorAttachmentCount    = 1;
    subpass[0].pColorAttachments       = &rasterOutputAttachments[0];
    subpass[0].pDepthStencilAttachment = &rasterOutputAttachments[1];
    subpass[0].inputAttachmentCount    = 1;
    subpass[0].pInputAttachments       = &rasterInputAttachments[0];
        
    // we can now define the subpass dependency graph
    vk::SubpassDependency dependencies = { };
        dependencies.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies.dstSubpass    = 0;
        dependencies.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies.srcAccessMask = vk::AccessFlagBits { };
        dependencies.dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite;
     
    // then finally by the big daddy render pass
    vk::RenderPassCreateInfo createInfo = { };
        createInfo.attachmentCount = 3;
        createInfo.pAttachments    = attachmentDescriptions;
        createInfo.subpassCount    = 1;
        createInfo.pSubpasses      = &subpass[0];
        createInfo.dependencyCount = 1;
        createInfo.pDependencies   = &dependencies;
        
    result = core.logicalDevice.createRenderPass(&createInfo, nullptr, &pipelines.raster.renderPass);
    
    if (result != vk::Result::eSuccess)
        std::cout << "ERROR: " << vk::to_string(result);

    return result;
    
    } // VulkanApp :: createRasterRenderPass


//
//
//
vk::Result VulkanApp::createShadingFrameBuffer ()
    { // VulkanApp :: createShadingFrameBuffer
    vk::Result result = vk::Result::eSuccess;
    
    vk::ImageView attachments [4] = {
        shading.position.view,
        shading.normal.view,
        shading.color.view,
        shading.result.view};
    
    vk::FramebufferCreateInfo framebufferCreateInfo = { };
        framebufferCreateInfo.renderPass = pipelines.shading.renderPass;
        framebufferCreateInfo.attachmentCount = 4;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = shading.BUFFER_SIZE;
        framebufferCreateInfo.height = shading.BUFFER_SIZE;
        framebufferCreateInfo.layers = 1;

        result = core.logicalDevice.createFramebuffer (
            &framebufferCreateInfo,
            nullptr,
            &shading.framebuffer);
        
        if (result != vk::Result::eSuccess)
            return result;

    return result;
    
    } // VulkanApp :: createShadingFrameBuffer


//
//
//
vk::Result VulkanApp::createRasterFrameBuffers ()
    { // VulkanApp :: createRasterFrameBuffers
    vk::Result result = vk::Result::eSuccess;
    
    // we create a framebuffer in memory for every
    // swap chain image. this will represent the
    // image in VRAM and allow us to render to it
    swapchain.framebuffers.resize(swapchain.nImages);
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image
        std::array<vk::ImageView, 3> attachmentViews;
            attachmentViews[0] = shading.result.view;
            attachmentViews[1] = swapchain.views[i];
            attachmentViews[2] = depth.view;
        vk::FramebufferCreateInfo framebufferCreateInfo = { };
            framebufferCreateInfo.renderPass      = pipelines.raster.renderPass;
            framebufferCreateInfo.attachmentCount = attachmentViews.size();
            framebufferCreateInfo.pAttachments    = attachmentViews.data();
            framebufferCreateInfo.width           = swapchain.extent.width;
            framebufferCreateInfo.height          = swapchain.extent.height;
            framebufferCreateInfo.layers          = 1;
        result = core.logicalDevice.createFramebuffer (
            &framebufferCreateInfo,
            nullptr,
            &swapchain.framebuffers[i]);
            
        if (result != vk::Result::eSuccess)
            return result;

        } // for each swapchain image

    return result;
    
    } // VulkanApp :: createRasterFrameBuffers


//
//
//
vk::Result VulkanApp::createVertexBuffers ()
    { // VulkanApp :: createVertexBuffers
    vk::Result result = vk::Result::eSuccess;
    
    //  for the preferred shading architecture we'll need two sets
    //  of vertex data. One for the scene itself and one for the
    //  quad we'll use to traverse our geometry buffers in the
    //  lighting segment of the shading pass
    
    // first we create a buffer for the vertices so
    // we can get them onto VRAM / device memory
    vk::DeviceSize sceneBufferSize = sizeof(Vertex) * meshes.scene.vertices.size ();
    vk::DeviceSize quadBufferSize  = sizeof(Vertex) * meshes.quad.vertices.size ();
    
    vk::BufferCreateInfo sceneBufferCreateInfo = { };
        sceneBufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eVertexBuffer;
        sceneBufferCreateInfo.size                  = sceneBufferSize;
        sceneBufferCreateInfo.queueFamilyIndexCount = 0;
        sceneBufferCreateInfo.pQueueFamilyIndices   = nullptr;
        sceneBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(&sceneBufferCreateInfo, nullptr, &buffers.sceneVertex.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    vk::BufferCreateInfo quadBufferCreateInfo = { };
        quadBufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eVertexBuffer;
        quadBufferCreateInfo.size                  = quadBufferSize;
        quadBufferCreateInfo.queueFamilyIndexCount = 0;
        quadBufferCreateInfo.pQueueFamilyIndices   = nullptr;
        quadBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;

    result = core.logicalDevice.createBuffer(&quadBufferCreateInfo, nullptr, &buffers.quadVertex.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    // we query the device for the requirements of
    // our buffers' memory including size and type
    vk::MemoryRequirements sceneMemoryRequirements = { };
    vk::MemoryRequirements quadMemoryRequirements  = { };
    
    core.logicalDevice.getBufferMemoryRequirements(buffers.sceneVertex.buffer, &sceneMemoryRequirements);
    core.logicalDevice.getBufferMemoryRequirements(buffers.quadVertex.buffer,  &quadMemoryRequirements);

    // once we have a buffer and know about the memory
    // we're going to allocate we can perform the VRAM
    // allocation and crash on failure
    vk::MemoryAllocateInfo sceneAllocationInfo = { };
        sceneAllocationInfo.allocationSize  = sceneMemoryRequirements.size;
        sceneAllocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
            core.physicalDevice,
            sceneMemoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&sceneAllocationInfo, nullptr, &buffers.sceneVertex.memory);
    if (result != vk::Result::eSuccess)
        return result;
    
    vk::MemoryAllocateInfo quadAllocationInfo  = { };
        quadAllocationInfo.allocationSize  = quadMemoryRequirements.size;
        quadAllocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
            core.physicalDevice,
            quadMemoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&quadAllocationInfo, nullptr, &buffers.quadVertex.memory);
    if (result != vk::Result::eSuccess)
        return result;


    // now the VRAM on the graphics card is sitting
    // allocated and empty we can copy our vertex
    // data across, once again crashing on failure
    void* sceneData;
    void* quadData;
    
    result = core.logicalDevice.mapMemory(buffers.sceneVertex.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &sceneData);
    if (result != vk::Result::eSuccess)
        return result;
        
    result = core.logicalDevice.mapMemory(buffers.quadVertex.memory,  0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &quadData);
    if (result != vk::Result::eSuccess)
        return result;
        
    memcpy(sceneData, meshes.scene.vertices.data(), (size_t)sceneBufferSize);
    memcpy(quadData,  meshes.quad.vertices.data(),  (size_t)quadBufferSize);

    core.logicalDevice.unmapMemory(buffers.sceneVertex.memory);
    core.logicalDevice.unmapMemory(buffers.quadVertex.memory);

    core.logicalDevice.bindBufferMemory(buffers.sceneVertex.buffer, buffers.sceneVertex.memory, 0);
    core.logicalDevice.bindBufferMemory(buffers.quadVertex.buffer,  buffers.quadVertex.memory,  0);

    return result;
    
    } // VulkanApp :: createVertexBuffers


//
//
//
vk::Result VulkanApp::createIndexBuffers ()
    { // VulkanApp :: createIndexBuffers
    vk::Result result = vk::Result::eSuccess;
    
    //  for the preferred shading architecture we'll need two sets
    //  of vertex data. One for the scene itself and one for the
    //  quad we'll use to traverse our geometry buffers in the
    //  lighting segment of the shading pass
    
    // first we create a buffer for the vertices so
    // we can get them onto VRAM / device memory
    vk::DeviceSize sceneBufferSize = sizeof(uint32_t) * meshes.scene.indices.size ();
    vk::DeviceSize quadBufferSize  = sizeof(uint32_t) * meshes.quad.indices.size ();
    
    vk::BufferCreateInfo sceneBufferCreateInfo = { };
        sceneBufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eIndexBuffer;
        sceneBufferCreateInfo.size                  = sceneBufferSize;
        sceneBufferCreateInfo.queueFamilyIndexCount = 0;
        sceneBufferCreateInfo.pQueueFamilyIndices   = nullptr;
        sceneBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(&sceneBufferCreateInfo, nullptr, &buffers.sceneIndex.buffer);
    if (result != vk::Result::eSuccess)
        return result;
    
    vk::BufferCreateInfo quadBufferCreateInfo = { };
        quadBufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eIndexBuffer;
        quadBufferCreateInfo.size                  = quadBufferSize;
        quadBufferCreateInfo.queueFamilyIndexCount = 0;
        quadBufferCreateInfo.pQueueFamilyIndices   = nullptr;
        quadBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(&quadBufferCreateInfo, nullptr, &buffers.quadIndex.buffer);
    if (result != vk::Result::eSuccess)
        return result;

    // we query the device for the requirements of
    // our buffer's memory including size and type
    vk::MemoryRequirements sceneMemoryRequirements = { };
    vk::MemoryRequirements quadMemoryRequirements  = { };
    
    core.logicalDevice.getBufferMemoryRequirements(buffers.sceneIndex.buffer, &sceneMemoryRequirements);
    core.logicalDevice.getBufferMemoryRequirements(buffers.quadIndex.buffer,  &quadMemoryRequirements);

    // once we have a buffer and know about the memory
    // we're going to allocate we can perform the VRAM
    // allocation and crash on failure
    vk::MemoryAllocateInfo sceneAllocationInfo = { };
        sceneAllocationInfo.allocationSize  = sceneMemoryRequirements.size;
        sceneAllocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
            core.physicalDevice,
            sceneMemoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&sceneAllocationInfo, nullptr, &buffers.sceneIndex.memory);
    if (result != vk::Result::eSuccess)
        return result;
    
    vk::MemoryAllocateInfo quadAllocationInfo = { };
        quadAllocationInfo.allocationSize  = quadMemoryRequirements.size;
        quadAllocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
            core.physicalDevice,
            quadMemoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        
    result = core.logicalDevice.allocateMemory(&quadAllocationInfo, nullptr, &buffers.quadIndex.memory);
    if (result != vk::Result::eSuccess)
        return result;

    // now the VRAM on the graphics card is sitting
    // allocated and empty we can copy our index
    // data across, once again crashing on failure
    void* sceneData;
    void* quadData;
    
    result = core.logicalDevice.mapMemory(buffers.sceneIndex.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags {}, &sceneData);
    if (result != vk::Result::eSuccess)
        return result;
    
    result = core.logicalDevice.mapMemory(buffers.quadIndex.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags {}, &quadData);
    if (result != vk::Result::eSuccess)
        return result;

    memcpy(sceneData, meshes.scene.indices.data(), (size_t)sceneBufferSize);
    memcpy(quadData,  meshes.quad.indices.data(),  (size_t)quadBufferSize);
    
    core.logicalDevice.unmapMemory(buffers.sceneIndex.memory);
    core.logicalDevice.unmapMemory(buffers.quadIndex.memory);
    
    core.logicalDevice.bindBufferMemory(buffers.sceneIndex.buffer, buffers.sceneIndex.memory, 0);
    core.logicalDevice.bindBufferMemory(buffers.quadIndex.buffer, buffers.quadIndex.memory, 0);

    return result;
    
    } // VulkanApp :: createIndexBuffers


//
//
//
vk::Result VulkanApp::createGeometryPipeline ()
    { // VulkanApp :: createGeometryPipeline
    
    vk::Result result = vk::Result::eSuccess;
    
    vk::PipelineShaderStageCreateInfo shaderStages [] =
        {
        VulkanShaders::loadShader(core.logicalDevice, "shaders/geometry.vert.spv", vk::ShaderStageFlagBits::eVertex),
        VulkanShaders::loadShader(core.logicalDevice, "shaders/geometry.frag.spv", vk::ShaderStageFlagBits::eFragment)
        };
    
    vk::VertexInputBindingDescription inputBinding = { };
        inputBinding.binding    = 0;
        inputBinding.stride     = sizeof(Vertex);
        inputBinding.inputRate  = vk::VertexInputRate::eVertex;
        
    std::array<vk::VertexInputAttributeDescription, 5> attributes = Vertex::attributeDescriptions();
        
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = { };
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexBindingDescriptions      = &inputBinding;
        vertexInputInfo.pVertexAttributeDescriptions    = attributes.data();
        
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
        inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = { };
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)shading.BUFFER_SIZE;
        viewport.height   = (float)shading.BUFFER_SIZE;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = { };
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = (float)shading.BUFFER_SIZE;
        scissor.extent.height = (float)shading.BUFFER_SIZE;
        
    vk::PipelineViewportStateCreateInfo viewportCreateInfo = { };
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports    = &viewport;
        viewportCreateInfo.scissorCount  = 1;
        viewportCreateInfo.pScissors     = &scissor;
        
    vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = { };
        rasterizationCreateInfo.depthClampEnable        = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode             = vk::PolygonMode::eFill;
        rasterizationCreateInfo.lineWidth               = 1.0f;
        rasterizationCreateInfo.cullMode                = vk::CullModeFlagBits::eBack;
        rasterizationCreateInfo.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizationCreateInfo.depthBiasEnable         = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp          = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor    = 0.0f;
        
    vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = { };
        multisampleCreateInfo.sampleShadingEnable   = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples  = vk::SampleCountFlagBits::e1;
        multisampleCreateInfo.minSampleShading      = 1.0f;
        multisampleCreateInfo.pSampleMask           = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable      = VK_FALSE;
        
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState[3];
    
    for (uint32_t i = 0; i < 3; ++i)
        {
        colorBlendAttachmentState[i].colorWriteMask       =
                  vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState[i].blendEnable          = VK_TRUE;
        colorBlendAttachmentState[i].srcColorBlendFactor  = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState[i].dstColorBlendFactor  = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachmentState[i].colorBlendOp         = vk::BlendOp::eAdd;
        colorBlendAttachmentState[i].srcAlphaBlendFactor  = vk::BlendFactor::eOne;
        colorBlendAttachmentState[i].dstAlphaBlendFactor  = vk::BlendFactor::eZero;
        colorBlendAttachmentState[i].alphaBlendOp         = vk::BlendOp::eAdd;
        }
        
    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = { };
        colorBlendCreateInfo.logicOpEnable     = VK_FALSE;
        colorBlendCreateInfo.logicOp           = vk::LogicOp::eCopy;
        colorBlendCreateInfo.attachmentCount   = 3;
        colorBlendCreateInfo.pAttachments      = colorBlendAttachmentState;
        colorBlendCreateInfo.blendConstants[0] = 0.0f;
        colorBlendCreateInfo.blendConstants[1] = 0.0f;
        colorBlendCreateInfo.blendConstants[2] = 0.0f;
        colorBlendCreateInfo.blendConstants[3] = 0.0f;
        
    vk::DynamicState dynamicStates[] =
        {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
        };
        
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = { };
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;
        
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { };
        depthStencilCreateInfo.depthTestEnable       = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable      = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp        = vk::CompareOp::eGreater;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilCreateInfo.minDepthBounds        = 0.0f;
        depthStencilCreateInfo.maxDepthBounds        = 1.0f;
        depthStencilCreateInfo.stencilTestEnable     = VK_FALSE;
        
    vk::PipelineLayoutCreateInfo layoutCreateInfo = { };
        layoutCreateInfo.setLayoutCount = 1;
        layoutCreateInfo.pSetLayouts    = &pipelines.shading.geometryDescriptorlayout;

    result = core.logicalDevice.createPipelineLayout(&layoutCreateInfo, nullptr, &pipelines.shading.geometryLayout);
    
    if (result != vk::Result::eSuccess)
        { // failed to create pipeline layout
        std::cout << "Failed to create shading pipeline layout" << std::endl;
        return result;
        } // failed to create pipeline layout
    
     vk::GraphicsPipelineCreateInfo pipelineCreateInfo = { };
        pipelineCreateInfo.stageCount           = 2;
        pipelineCreateInfo.pStages              = shaderStages;
        pipelineCreateInfo.pVertexInputState    = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState  = &inputAssemblyInfo;
        pipelineCreateInfo.pViewportState       = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState  = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState    = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState   = &depthStencilCreateInfo;
        pipelineCreateInfo.pColorBlendState     = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState        = nullptr; // Optional
        pipelineCreateInfo.layout               = pipelines.shading.geometryLayout;
        pipelineCreateInfo.renderPass           = pipelines.shading.renderPass;
        pipelineCreateInfo.subpass              = 0;

    return core.logicalDevice.createGraphicsPipelines(vk::PipelineCache {}, 1, &pipelineCreateInfo, nullptr, &pipelines.shading.geometryPipeline);

    } // VulkanApp :: createGeometryPipeline


//
//
//
vk::Result VulkanApp::createShadingPipeline ()
    { // VulkanApp :: createShadingPipeline
    
    vk::Result result = vk::Result::eSuccess;
    
    vk::PipelineShaderStageCreateInfo shaderStages [] =
        {
        VulkanShaders::loadShader(core.logicalDevice, "shaders/lighting.vert.spv", vk::ShaderStageFlagBits::eVertex),
        VulkanShaders::loadShader(core.logicalDevice, "shaders/lighting.frag.spv", vk::ShaderStageFlagBits::eFragment)
        };
    
    vk::VertexInputBindingDescription inputBinding = { };
        inputBinding.binding    = 0;
        inputBinding.stride     = sizeof(Vertex);
        inputBinding.inputRate  = vk::VertexInputRate::eVertex;
        
    std::array<vk::VertexInputAttributeDescription, 5> attributes = Vertex::attributeDescriptions();
        
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = { };
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexBindingDescriptions      = &inputBinding;
        vertexInputInfo.pVertexAttributeDescriptions    = attributes.data();
        
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
        inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = { };
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)shading.BUFFER_SIZE;
        viewport.height   = (float)shading.BUFFER_SIZE;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = { };
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = (float)shading.BUFFER_SIZE;
        scissor.extent.height = (float)shading.BUFFER_SIZE;
        
    vk::PipelineViewportStateCreateInfo viewportCreateInfo = { };
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports    = &viewport;
        viewportCreateInfo.scissorCount  = 1;
        viewportCreateInfo.pScissors     = &scissor;
        
    vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = { };
        rasterizationCreateInfo.depthClampEnable        = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode             = vk::PolygonMode::eFill;
        rasterizationCreateInfo.lineWidth               = 1.0f;
        rasterizationCreateInfo.cullMode                = vk::CullModeFlagBits::eBack;
        rasterizationCreateInfo.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizationCreateInfo.depthBiasEnable         = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp          = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor    = 0.0f;
        
    vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = { };
        multisampleCreateInfo.sampleShadingEnable   = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples  = vk::SampleCountFlagBits::e1;
        multisampleCreateInfo.minSampleShading      = 1.0f;
        multisampleCreateInfo.pSampleMask           = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable      = VK_FALSE;
        
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState[1];
    
    for (uint32_t i = 0; i < 1; ++i)
        {
        colorBlendAttachmentState[i].colorWriteMask       =
                  vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState[i].blendEnable          = VK_FALSE;
        colorBlendAttachmentState[i].srcColorBlendFactor  = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState[i].dstColorBlendFactor  = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachmentState[i].colorBlendOp         = vk::BlendOp::eAdd;
        colorBlendAttachmentState[i].srcAlphaBlendFactor  = vk::BlendFactor::eOne;
        colorBlendAttachmentState[i].dstAlphaBlendFactor  = vk::BlendFactor::eZero;
        colorBlendAttachmentState[i].alphaBlendOp         = vk::BlendOp::eAdd;
        }
        
    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = { };
        colorBlendCreateInfo.logicOpEnable     = VK_FALSE;
        colorBlendCreateInfo.logicOp           = vk::LogicOp::eCopy;
        colorBlendCreateInfo.attachmentCount   = 1;
        colorBlendCreateInfo.pAttachments      = colorBlendAttachmentState;
        colorBlendCreateInfo.blendConstants[0] = 0.0f;
        colorBlendCreateInfo.blendConstants[1] = 0.0f;
        colorBlendCreateInfo.blendConstants[2] = 0.0f;
        colorBlendCreateInfo.blendConstants[3] = 0.0f;
        
    vk::DynamicState dynamicStates[] =
        {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
        };
        
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = { };
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;
        
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { };
        depthStencilCreateInfo.depthTestEnable       = VK_FALSE;
        depthStencilCreateInfo.depthWriteEnable      = VK_FALSE;
        depthStencilCreateInfo.depthCompareOp        = vk::CompareOp::eLess;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilCreateInfo.minDepthBounds        = 0.0f;
        depthStencilCreateInfo.maxDepthBounds        = 1.0f;
        depthStencilCreateInfo.stencilTestEnable     = VK_FALSE;
        
    vk::PipelineLayoutCreateInfo layoutCreateInfo = { };
        layoutCreateInfo.setLayoutCount = 1;
        layoutCreateInfo.pSetLayouts    = &pipelines.shading.shadingDescriptorLayout;
        
    result = core.logicalDevice.createPipelineLayout(&layoutCreateInfo, nullptr, &pipelines.shading.shadingLayout);
    
    if (result != vk::Result::eSuccess)
        { // failed to create pipeline layout
        std::cout << "Failed to create shading pipeline layout" << std::endl;
        return result;
        } // failed to create pipeline layout
    
     vk::GraphicsPipelineCreateInfo pipelineCreateInfo = { };
        pipelineCreateInfo.stageCount           = 2;
        pipelineCreateInfo.pStages              = shaderStages;
        pipelineCreateInfo.pVertexInputState    = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState  = &inputAssemblyInfo;
        pipelineCreateInfo.pViewportState       = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState  = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState    = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState   = &depthStencilCreateInfo;
        pipelineCreateInfo.pColorBlendState     = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState        = nullptr; // Optional
        pipelineCreateInfo.layout               = pipelines.shading.shadingLayout;
        pipelineCreateInfo.renderPass           = pipelines.shading.renderPass;
        pipelineCreateInfo.subpass              = 1;

    return core.logicalDevice.createGraphicsPipelines(vk::PipelineCache {}, 1, &pipelineCreateInfo, nullptr, &pipelines.shading.shadingPipeline);

    } // VulkanApp :: createShadingPipeline


//
//
//
vk::Result VulkanApp::createRasterPipeline ()
    { // VulkanApp :: createRasterPipeline
    vk::Result result = vk::Result::eSuccess;
    
    vk::PipelineShaderStageCreateInfo shaderStages[] =
        {
        VulkanShaders::loadShader(core.logicalDevice, "shaders/raster.vert.spv", vk::ShaderStageFlagBits::eVertex),
        VulkanShaders::loadShader(core.logicalDevice, "shaders/raster.frag.spv", vk::ShaderStageFlagBits::eFragment)
        };
        
     vk::VertexInputBindingDescription inputBinding = { };
        inputBinding.binding    = 0;
        inputBinding.stride     = sizeof(Vertex);
        inputBinding.inputRate  = vk::VertexInputRate::eVertex;
        
    std::array<vk::VertexInputAttributeDescription, 5> attributes = Vertex::attributeDescriptions();
        
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = { };
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        
        vertexInputInfo.pVertexBindingDescriptions      = &inputBinding;
        vertexInputInfo.pVertexAttributeDescriptions    = attributes.data();
        
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
        inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = { };
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)swapchain.extent.width;
        viewport.height   = (float)swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = { };
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = swapchain.extent.width;
        scissor.extent.height = swapchain.extent.height;
        
    vk::PipelineViewportStateCreateInfo viewportCreateInfo = { };
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports    = &viewport;
        viewportCreateInfo.scissorCount  = 1;
        viewportCreateInfo.pScissors     = &scissor;
        
    vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = { };
        rasterizationCreateInfo.depthClampEnable        = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode             = vk::PolygonMode::eFill;
        rasterizationCreateInfo.lineWidth               = 1.0f;
        rasterizationCreateInfo.cullMode                = vk::CullModeFlagBits::eBack;
        rasterizationCreateInfo.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizationCreateInfo.depthBiasEnable         = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp          = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor    = 0.0f;
        
    vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = { };
        multisampleCreateInfo.sampleShadingEnable   = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples  = vk::SampleCountFlagBits::e1;
        multisampleCreateInfo.minSampleShading      = 1.0f;
        multisampleCreateInfo.pSampleMask           = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable      = VK_FALSE;
        
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = { };
        colorBlendAttachmentState.colorWriteMask       =
                  vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState.blendEnable          = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor  = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState.dstColorBlendFactor  = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachmentState.colorBlendOp         = vk::BlendOp::eAdd;
        colorBlendAttachmentState.srcAlphaBlendFactor  = vk::BlendFactor::eOne;
        colorBlendAttachmentState.dstAlphaBlendFactor  = vk::BlendFactor::eZero;
        colorBlendAttachmentState.alphaBlendOp         = vk::BlendOp::eAdd;
        
    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = { };
        colorBlendCreateInfo.logicOpEnable     = VK_FALSE;
        colorBlendCreateInfo.logicOp           = vk::LogicOp::eCopy;
        colorBlendCreateInfo.attachmentCount   = 1;
        colorBlendCreateInfo.pAttachments      = &colorBlendAttachmentState;
        colorBlendCreateInfo.blendConstants[0] = 0.0f;
        colorBlendCreateInfo.blendConstants[1] = 0.0f;
        colorBlendCreateInfo.blendConstants[2] = 0.0f;
        colorBlendCreateInfo.blendConstants[3] = 0.0f;
        
    vk::DynamicState dynamicStates[] =
        {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
        };
        
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = { };
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;
        
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { };
        depthStencilCreateInfo.depthTestEnable       = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable      = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp        = vk::CompareOp::eLess;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilCreateInfo.minDepthBounds        = 0.0f;
        depthStencilCreateInfo.maxDepthBounds        = 1.0f;
        depthStencilCreateInfo.stencilTestEnable     = VK_FALSE;
        
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
        pipelineLayoutCreateInfo.setLayoutCount         = 1;
        pipelineLayoutCreateInfo.pSetLayouts            = &pipelines.raster.descriptorLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
        
    result = core.logicalDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &pipelines.raster.layout);
    
    if (result != vk::Result::eSuccess)
        return result;
        
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo = { };
        pipelineCreateInfo.stageCount           = 2;
        pipelineCreateInfo.pStages              = shaderStages;
        pipelineCreateInfo.pVertexInputState    = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState  = &inputAssemblyInfo;
        pipelineCreateInfo.pViewportState       = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState  = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState    = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState   = &depthStencilCreateInfo;
        pipelineCreateInfo.pColorBlendState     = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState        = nullptr; // Optional
        pipelineCreateInfo.layout               = pipelines.raster.layout;
        pipelineCreateInfo.renderPass           = pipelines.raster.renderPass;
        pipelineCreateInfo.subpass              = 0;

    result = core.logicalDevice.createGraphicsPipelines(vk::PipelineCache {}, 1, &pipelineCreateInfo, nullptr, &pipelines.raster.pipeline);

    if (result != vk::Result::eSuccess)
        return result;
        
    VulkanShaders::tidy(core.logicalDevice);

    return result;
    } // VulkanApp :: createRasterPipeline


//
//  createShadingCommandBuffers
//
//  sets up a graphics command buffer for fulfilling
//  our rendering needs, if we require compute we'll
//  need (ideally) another command buffer
//
vk::Result VulkanApp::createShadingCommandBuffers ()
    { // VulkanApp :: createShadingCommandBuffers
    vk::Result result = vk::Result::eSuccess;

    vk::CommandBufferAllocateInfo allocationInfo = { };
        allocationInfo.commandPool        = command.pool;
        allocationInfo.level              = vk::CommandBufferLevel::ePrimary;
        allocationInfo.commandBufferCount = 1;
  
    result = core.logicalDevice.allocateCommandBuffers(&allocationInfo, &shading.commandBuffer);
  
    if (result != vk::Result::eSuccess)
        return result;
    
    vk::CommandBufferBeginInfo beginInfo = { };
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        beginInfo.pInheritanceInfo = nullptr;

    // we define a clear value for our colour buffer and our
    // stencil buffer so they can be reset at the start of render
    vk::ClearColorValue color = { WINDOW_CLEAR };
    vk::ClearDepthStencilValue depth = { 1.0f, 0 };
    
    std::array<vk::ClearValue, 4> clearValues = {};
        clearValues[0].color        = color;
        clearValues[1].color        = color;
        clearValues[2].color        = color;
        clearValues[3].color        = color;

    vk::RenderPassBeginInfo renderPassBeginInfo = { };
        renderPassBeginInfo.renderPass        = pipelines.shading.renderPass;
        renderPassBeginInfo.framebuffer       = shading.framebuffer;
        renderPassBeginInfo.renderArea.offset = vk::Offset2D { 0, 0 };
        renderPassBeginInfo.renderArea.extent.width = shading.BUFFER_SIZE;
		renderPassBeginInfo.renderArea.extent.height = shading.BUFFER_SIZE;
        renderPassBeginInfo.clearValueCount   = 4;
        renderPassBeginInfo.pClearValues      = clearValues.data();
    
    vk::DeviceSize offsets[] = { 0 };
    
    shading.commandBuffer.begin(&beginInfo);
    shading.commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
    
        //  Subpass One: Populate geometry buffers in preperation for lighting computation
        shading.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.shading.geometryPipeline);
        shading.commandBuffer.bindVertexBuffers(0, 1, &buffers.sceneVertex.buffer, offsets);
        shading.commandBuffer.bindIndexBuffer(buffers.sceneIndex.buffer, 0, vk::IndexType::eUint32);
        shading.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.shading.geometryLayout, 0, 1, &pipelines.shading.geometryDescriptorSet, 0, nullptr);
        shading.commandBuffer.drawIndexed(static_cast<uint32_t>(meshes.scene.indices.size()), 1, 0, 0, 0);

		shading.commandBuffer.nextSubpass(vk::SubpassContents::eInline);
       
        //  Subpass Two: Compute lighting and store in buffer
        shading.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.shading.shadingPipeline);
        shading.commandBuffer.bindVertexBuffers(0, 1, &buffers.quadVertex.buffer, offsets);
        shading.commandBuffer.bindIndexBuffer(buffers.quadIndex.buffer, 0, vk::IndexType::eUint32);
        shading.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.shading.shadingLayout, 0, 1, &pipelines.shading.shadingDescriptorSet, 0, nullptr);
        shading.commandBuffer.drawIndexed(static_cast<uint32_t>(meshes.quad.indices.size()), 1, 0, 0, 0);

    shading.commandBuffer.endRenderPass();
    shading.commandBuffer.end();

    return result;
        
    } // VulkanApp :: createShadingCommandBuffers


//
//  createRasterCommandBuffers
//
//  sets up a graphics command buffer for fulfilling
//  our rendering needs, if we require compute we'll
//  need (ideally) another command buffer
//
vk::Result VulkanApp::createRasterCommandBuffers ()
    { // VulkanApp :: createRasterCommandBuffers
    vk::Result result = vk::Result::eSuccess;

    swapchain.commandBuffers.resize(swapchain.nImages);
    
    vk::CommandBufferAllocateInfo allocationInfo = { };
        allocationInfo.commandPool        = command.pool;
        allocationInfo.level              = vk::CommandBufferLevel::ePrimary;
        allocationInfo.commandBufferCount = swapchain.nImages;
  
    result = core.logicalDevice.allocateCommandBuffers(&allocationInfo, swapchain.commandBuffers.data());
  
    if (result != vk::Result::eSuccess)
        return result;
        
    // once we allocate our memory we can initialize a command
    // buffer for each swapchain image
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image
        
        vk::CommandBufferBeginInfo beginInfo = { };
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
            beginInfo.pInheritanceInfo = nullptr;
        swapchain.commandBuffers[i].begin(&beginInfo);

        // we define a clear value for our colour buffer and our
        // stencil buffer so they can be reset at the start of render
        vk::ClearColorValue color = { WINDOW_CLEAR };
        vk::ClearDepthStencilValue depth = { 1.0f, 0 };
        
        std::array<vk::ClearValue, 3> clearValues = {};
            clearValues[0].color        = color;
            clearValues[1].color        = color;
            clearValues[2].depthStencil = depth;

        vk::RenderPassBeginInfo renderPassBeginInfo = { };
            renderPassBeginInfo.renderPass        = pipelines.raster.renderPass;
            renderPassBeginInfo.framebuffer       = swapchain.framebuffers[i];
            renderPassBeginInfo.renderArea.offset = vk::Offset2D { 0, 0 };
            renderPassBeginInfo.renderArea.extent = swapchain.extent;
            renderPassBeginInfo.clearValueCount   = 3;
            renderPassBeginInfo.pClearValues      = clearValues.data();
        
        vk::DeviceSize offsets[] = { 0 };
        
        swapchain.commandBuffers[i].beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
        
            swapchain.commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.raster.pipeline);
            swapchain.commandBuffers[i].bindVertexBuffers(0, 1, &buffers.sceneVertex.buffer, offsets);
            swapchain.commandBuffers[i].bindIndexBuffer(buffers.sceneIndex.buffer, 0, vk::IndexType::eUint32);
            swapchain.commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.raster.layout, 0, 1, &pipelines.raster.descriptorSet, 0, nullptr);
            swapchain.commandBuffers[i].drawIndexed(static_cast<uint32_t>(meshes.scene.indices.size()), 1, 0, 0, 0);
    
        swapchain.commandBuffers[i].endRenderPass();
        swapchain.commandBuffers[i].end();

        } // for each swapchain image
        
    return result;
        
    } // VulkanApp :: createRasterCommandBuffers


//
//
//
void VulkanApp::arrangeObjects ()
	{ // VulkanApp :: arrangeObjects

	if (!arrangement.arranged)
		{
		int m = sqrt(nObjects);

		arrangement.translations.resize(nObjects);
		arrangement.centre = { 0.0f, 0.0f, 0.0f };

		for (Vertex& v : meshes.scene.vertices)
			{ // for each vertex

			arrangement.translations[v.id] = {
				(v.id % m) * offset,
				0.0f, 
				(v.id / m) * offset};

			} // for each vertex

		for (uint32_t i = 0; i < nObjects; ++i)
			arrangement.centre += arrangement.translations[i];

		arrangement.centre /= nObjects;

		arrangement.arranged = true;

		}


	for (uint32_t i = 0; i < nObjects; ++i)
		{ // for each object
		
		ubo.geometry.model[i] = glm::mat4(1.0f);
		ubo.geometry.model[i] = glm::translate(ubo.geometry.model[i], arrangement.translations[i] - arrangement.centre);
		ubo.geometry.model[i] = glm::scale(ubo.geometry.model[i], glm::vec3(0.5f, 0.5f, 0.5f));
		ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		ubo.raster.model[i] = ubo.geometry.model[i];

		simulation.positions[i] = arrangement.translations[i] - arrangement.centre;

		} // for each object

	} // VulkanApp :: arrangeObjects


//
//
//
void VulkanApp::createPhysicsState ()
	{ // VulkanApp :: createPhysicsState
		
	std::uniform_real_distribution<float> posDist (-0.01, 0.01);
	std::uniform_real_distribution<float> angDist(-10.0, 10.0);

	simulation.positions.resize(nObjects);
	simulation.velocities.resize(nObjects);
	simulation.orientations.resize(nObjects);
	simulation.rotations.resize(nObjects);

	for (uint32_t i = 0; i < nObjects; ++i)
		{ // for each object

		simulation.velocities[i] = glm::vec3(posDist(rng), posDist(rng), posDist(rng));
		simulation.rotations[i] = glm::vec3(angDist(rng), angDist(rng), angDist(rng));
		simulation.orientations[i] = glm::vec3(0.0f, 0.0f, 0.0f);
		simulation.bounds = 2.0f;

		} // for each object



	} // VulkanApp :: createPhysicsState


//
//
//
void VulkanApp::updatePhysicsState ()
	{ // VulkanApp :: updatePhysicsState

	// advance the simulation
	for (uint32_t i = 0; i < nObjects; ++i)
		{
		simulation.positions[i] += simulation.velocities[i] * (float)timing.delta * 0.05f;
		simulation.orientations[i] += simulation.rotations[i] * (float)timing.delta * 0.005f;
		}

	// collide with boundary
	for (uint32_t i = 0; i < nObjects; ++i)
		if (glm::length(simulation.positions[i]) > 12.0f)
			{ 
			//simulation.positions[i] = glm::normalize(simulation.positions[i]) * (nObjects * 0.9f);
			simulation.velocities[i] = glm::normalize(simulation.positions[i]) * -0.01f;
			}


	// collide with eachother
	for (uint32_t i = 0; i < nObjects; ++i)
		for (uint32_t j = 0; j < nObjects; ++j)
			{
			if (i == j) continue;

			float minDist = simulation.bounds;
			float actDist = glm::length(simulation.positions[i] - simulation.positions[j]);
			if (actDist < minDist)
				{ 
				simulation.velocities[i] = glm::normalize(simulation.positions[i] - simulation.positions[j]) * 0.01f;
				simulation.velocities[j] = glm::normalize(simulation.positions[j] - simulation.positions[i]) * 0.01f;
				}
			}

	} // VulkanApp :: updatePhysicsState


//
//
//
void VulkanApp::updateGeometryUniforms ()
    { // VulkanApp :: updateGeometryUniforms

	for (uint32_t i = 0; i < nObjects; ++i)
        {

		ubo.geometry.model[i] = glm::mat4(1.0f);
		ubo.geometry.model[i] = glm::translate(ubo.geometry.model[i], simulation.positions[i]);
		ubo.geometry.model[i] = glm::scale(ubo.geometry.model[i], glm::vec3(0.5f, 0.5f, 0.5f));
		ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(simulation.orientations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(simulation.orientations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.geometry.model[i] = glm::rotate(ubo.geometry.model[i], glm::radians(simulation.orientations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));

        }

	void* data;
	core.logicalDevice.mapMemory(buffers.geometryUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags{}, &data);
	memcpy(data, &ubo.geometry, sizeof(UniformBufferObjects::GeometryUBO));
	core.logicalDevice.unmapMemory(buffers.geometryUniform.memory);

    } // VulkanApp :: updateGeometryUniforms


//
//
//
void VulkanApp::updateShadingUniforms ()
    { // VulkanApp :: updateShadingUniforms

	if (animateLights)
        {
        ubo.shading.lightPosition.x = sin (timing.timer * 0.1f) * 4.0f;
        ubo.shading.lightPosition.y = cos (timing.timer * 0.1f) * 4.0f;
        }

	if (regenerateMaterials)
		{
		rng.seed(time(0));
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		for (uint32_t i = 0; i < nObjects; ++i)
			ubo.shading.materials[i] =
				{ dist(rng), dist(rng), dist(rng), dist(rng) };

		regenerateMaterials = false;
		}

	void* data;
	core.logicalDevice.mapMemory(buffers.shadingUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags{}, &data);
	memcpy(data, &ubo.shading, sizeof(UniformBufferObjects::ShadingUBO));
	core.logicalDevice.unmapMemory(buffers.shadingUniform.memory);

    } // VulkanApp :: updateShadingUniforms


//
//
//
void VulkanApp::updateRasterUniforms ()
    { // VulkanApp :: updateRasterUniforms

	for (uint32_t i = 0; i < nObjects; ++i)
		{
		ubo.raster.model[i] = ubo.geometry.model[i];
		}

	ubo.raster.proj = glm::perspective((float)(WINDOW_WIDTH / WINDOW_HEIGHT), 1.0f, 0.01f, 100.0f);
	ubo.raster.proj[1][1] *= -1;
	ubo.raster.view = glm::lookAt(
		eyePosition,                      // position
		eyePosition + glm::vec3{ 0.0f, -1.0f, 0.0f },  // center
		glm::vec3{ 0.0f, 0.0f, 1.00f }); // world up

	void* data;
	core.logicalDevice.mapMemory(buffers.rasterUniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags{}, &data);
	memcpy(data, &ubo.raster, sizeof(UniformBufferObjects::RasterUBO));
	core.logicalDevice.unmapMemory(buffers.rasterUniform.memory);

    } // VulkanApp :: updateRasterUniforms


#include <windows.h>

//
//
//
void VulkanApp::report ()
	{ // VulkanApp :: report


	if ((timing.frame % 120) != 0) return;

	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	cellCount = csbi.dwSize.X *csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR) ' ',
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Move the cursor home */
	SetConsoleCursorPosition(hStdOut, homeCoords);

	uint32_t meshMemoryOccupation = ((meshes.scene.vertices.size() * sizeof(Vertex)) / 1000) / 1000;
	meshMemoryOccupation += ((meshes.scene.indices.size() * sizeof(uint32_t)) / 1000) / 1000;

	uint32_t depthBufferMemorySize = ((WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint32_t) / 1000) / 1000);
	uint32_t frameBufferMemorySize = ((WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(glm::vec3) / 1000) / 1000);
	uint32_t shadingBufferMemorySize = ((shading.BUFFER_SIZE * shading.BUFFER_SIZE * sizeof(glm::vec4) / 1000) / 1000) * 4;
	
	uint32_t textureMemoryOccupation = (depthBufferMemorySize + frameBufferMemorySize) * swapchain.nImages;
	textureMemoryOccupation += shadingBufferMemorySize;

	std::cout << std::endl;
	std::cout << "  average fps    : " << timing.fps << std::endl;
	std::cout << "  mesh memory    : " << meshMemoryOccupation << "mb" << std::endl;
	std::cout << "  texture memory : " << textureMemoryOccupation << "mb" << std::endl;
	std::cout << "  object count   : " << nObjects << std::endl;

	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << meshes.scene.vertices.size();

	std::locale::global(std::locale(""));
	std::cout << "  vertex count   : " << ss.str() << std::endl;
	std::cout << std::endl;

	} // VulkanApp :: report

//
//
//
void VulkanApp::fullRender ()
	{ // VulkanApp :: fullRender

	// Shade Scene

	vk::PipelineStageFlags shadingWaitStages[] = { vk::PipelineStageFlagBits::eTopOfPipe };
	vk::Semaphore shadingSignalSemaphore[]     = { semaphores.shadingComplete };
	vk::Semaphore shadingWaitSemaphore[]       = { semaphores.rasterComplete };

	vk::SubmitInfo shadingSubmit = {};
		shadingSubmit.waitSemaphoreCount    = 0;
		shadingSubmit.pWaitSemaphores       = nullptr;
		shadingSubmit.signalSemaphoreCount  = 1;
		shadingSubmit.pSignalSemaphores     = shadingSignalSemaphore;
		shadingSubmit.pWaitDstStageMask     = shadingWaitStages;
		shadingSubmit.commandBufferCount    = 1;
		shadingSubmit.pCommandBuffers       = &shading.commandBuffer;

	vk::Result result = queues.graphics.submit(1, &shadingSubmit, nullptr);

	if (result != vk::Result::eSuccess)
		std::cout << std::endl << "queue submission: " << vk::to_string(result) << std::endl;

	// Raster Scene

	// before we begin rendering we'll want to know
	// which framebuffer in the swapchain we're going
	// to be writing to. We can query the device for
	// the index of this framebuffer
	uint32_t framebufferIndex = UINT32_MAX;
	result = core.logicalDevice.acquireNextImageKHR(
		swapchain.swapchain,
		UINT64_MAX,
		semaphores.presentReady,
		nullptr,
		&framebufferIndex);

	if (result != vk::Result::eSuccess)
		std::cout << std::endl << "framebuffer index query: " << vk::to_string(result) << std::endl;

	// to hand to the API for this frame's render
	vk::PipelineStageFlags rasterWaitStages[] = { vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore rasterSignalSemaphores[]    = { semaphores.renderComplete };
	vk::Semaphore rasterWaitSemaphores[]      = { semaphores.shadingComplete, semaphores.presentReady };

	vk::SubmitInfo rasterSubmit = {};
		rasterSubmit.waitSemaphoreCount    = 2;
		rasterSubmit.pWaitSemaphores       = rasterWaitSemaphores;
		rasterSubmit.signalSemaphoreCount  = 1;
		rasterSubmit.pSignalSemaphores     = rasterSignalSemaphores;
		rasterSubmit.pWaitDstStageMask     = rasterWaitStages;
		rasterSubmit.commandBufferCount    = 1;
		rasterSubmit.pCommandBuffers       = &swapchain.commandBuffers[framebufferIndex];

	result = queues.graphics.submit(1, &rasterSubmit, nullptr);

	if (result != vk::Result::eSuccess)
		std::cout << std::endl << "queue submission: " << vk::to_string(result) << std::endl;

	// after submitting our queue we can present the
	// render on the screen using the KHR functions
	vk::SwapchainKHR swapchains[] = { swapchain.swapchain };

	vk::PresentInfoKHR presentInfo = {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &semaphores.renderComplete;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &framebufferIndex;
	presentInfo.pResults = nullptr;

	queues.present.presentKHR(&presentInfo);
	queues.present.waitIdle();


	} // VulkanApp :: fullRender


//
//
//
void VulkanApp::halfRender ()
	{ // VulkanApp :: halfRender

	
	// before we begin rendering we'll want to know
	// which framebuffer in the swapchain we're going
	// to be writing to. We can query the device for
	// the index of this framebuffer
	uint32_t framebufferIndex = UINT32_MAX;
	vk::Result result = core.logicalDevice.acquireNextImageKHR(
		swapchain.swapchain,
		UINT64_MAX,
		semaphores.presentReady,
		nullptr,
		&framebufferIndex);

	if (result != vk::Result::eSuccess)
		std::cout << std::endl << "framebuffer index query: " << vk::to_string(result) << std::endl;

	// to hand to the API for this frame's render
	vk::PipelineStageFlags rasterWaitStages[] = { vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore rasterSignalSemaphores[]    = { semaphores.renderComplete };
	vk::Semaphore rasterWaitSemaphores[]      = { semaphores.presentReady };

	vk::SubmitInfo rasterSubmit = {};
		rasterSubmit.waitSemaphoreCount    = 1;
		rasterSubmit.pWaitSemaphores       = rasterWaitSemaphores;
		rasterSubmit.signalSemaphoreCount  = 1;
		rasterSubmit.pSignalSemaphores     = rasterSignalSemaphores;
		rasterSubmit.pWaitDstStageMask     = rasterWaitStages;
		rasterSubmit.commandBufferCount    = 1;
		rasterSubmit.pCommandBuffers       = &swapchain.commandBuffers[framebufferIndex];

	result = queues.graphics.submit(1, &rasterSubmit, nullptr);

	if (result != vk::Result::eSuccess)
		std::cout << std::endl << "queue submission: " << vk::to_string(result) << std::endl;

	// after submitting our queue we can present the
	// render on the screen using the KHR functions
	vk::SwapchainKHR swapchains[] = { swapchain.swapchain };

	vk::PresentInfoKHR presentInfo = {};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphores.renderComplete;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &framebufferIndex;
		presentInfo.pResults = nullptr;

	queues.present.presentKHR(&presentInfo);
	queues.present.waitIdle();

	} // VulkanApp :: halfRender


//
//
//
void VulkanApp::loop ()
    { // VulkanApp :: loop
    
    // move into loop when complete

    while ((!glfwWindowShouldClose(window)))
        { // while the window is open
        glfwPollEvents();
	//	report();
    
		if (forwards)  eyePosition.y -= parameters.movementSpeed;
		if (backwards) eyePosition.y += parameters.movementSpeed;
		if (up)        eyePosition.z += parameters.movementSpeed;
		if (down)      eyePosition.z -= parameters.movementSpeed;
		if (left)      eyePosition.x += parameters.movementSpeed;
		if (right)     eyePosition.x -= parameters.movementSpeed;

		timing.update();

		if (animateLights) timing.advance();

		if (reset == 2)
			{
			arrangeObjects();
			createPhysicsState();

			timing.timer = 0.0f;
			reset = 0;
			}

		if (reset == 1)
			updatePhysicsState ();

        updateGeometryUniforms ();
        updateShadingUniforms  ();
        updateRasterUniforms   ();


		if (((timing.frame - 1) % shading.interval) == 0) 
			fullRender();
		else 
			halfRender();

		if (timing.shouldClose)
			glfwSetWindowShouldClose(window, 1);
    
        } // while the window is open
    
    } // VulkanApp :: loop


//
//
//
void VulkanApp::createBuffer (
        vk::DeviceSize          size,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags properties,
        vk::Buffer&             buffer,
        vk::DeviceMemory        memory)
    { // VulkanApp :: createBuffer
    
    vk::Result result = vk::Result::eSuccess;
    
    vk::BufferCreateInfo createInfo = { };
        createInfo.size        = size;
        createInfo.usage       = usage;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(&createInfo, nullptr, &buffer);
    
    if (result != vk::Result::eSuccess)
        ErrorHandler::fatal("Failed to create buffer");
        
    vk::MemoryRequirements requirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffer, &requirements);
    
    vk::MemoryAllocateInfo allocationInfo = { };
        allocationInfo.allocationSize  = requirements.size;
        allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType (
            core.physicalDevice,
            requirements.memoryTypeBits,
            properties);
        
    // allocate buffer memory
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &memory);
    if (result != vk::Result::eSuccess)
        ErrorHandler::fatal("Failed to allocate buffer memory");
        
    // bind buffer memory
    core.logicalDevice.bindBufferMemory(buffer, memory, 0);

    } // VulkanApp :: createBuffer
