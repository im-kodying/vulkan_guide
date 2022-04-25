
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <VkBootstrap.h>
#include <cctype>
#include <clocale>

#include <vk_types.h>
#include <vk_initializers.h>

#include <iostream>

//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

void VulkanEngine::init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);

    //load the core Vulkan structures
    init_vulkan();

    //create the swapchain
    init_swapchain();

	//everything went fine
	_isInitialized = true;
}

void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;

    unsigned int count;
    if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr)) cerr << "EXTENSION 1 ERROR" << "\n";

    std::vector<const char*> extensions = {

    };
    size_t additional_extension_count = extensions.size();
    extensions.resize(additional_extension_count + count);

    if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, extensions.data() + additional_extension_count)) cerr << "Extension 2 ERROR" << "\n";


    //make the Vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger();

    for(int i = 0; i<count; i++){
        inst_ret.enable_extension(extensions[i]);
    }

    auto build = inst_ret.build();
    if(!build) cerr << "INST_RET " << build.error().message() << "\n";
    vkb::Instance vkb_inst = build.value();
    //store the instance
    _instance = vkb_inst.instance;
    //store the debug messenger
    _debug_messenger = vkb_inst.debug_messenger;

    // get the surface of the window we opened with SDL
    if(!SDL_Vulkan_CreateSurface(_window, _instance, &_surface)) cerr << "SURFACE CREATION" << "\n";

    //use vkbootstrap to select a GPU.
    //We want a GPU that can write to the SDL surface and supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto maybe = selector
            .set_minimum_version(1, 1)
            .set_surface(_surface)
            .select();
    if(!maybe) cerr << "Maybe " << maybe.error().message() << "\n";
    vkb::PhysicalDevice physicalDevice = maybe.value();
    //create the final Vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;


}

void VulkanEngine::init_swapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
                    //use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(_windowExtent.width, _windowExtent.height)
            .build()
            .value();

    //store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

    _swapchainImageFormat = vkbSwapchain.image_format;
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        //destroy swapchain resources
        for (int i = 0; i < _swapchainImageViews.size(); i++) {

            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }
}

void VulkanEngine::draw()
{
	//nothing yet
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;
		}

		draw();
	}
}

