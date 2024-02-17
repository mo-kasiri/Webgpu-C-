#include "webgpu-utils.h"

#include "glfw3webgpu.h"
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <iostream>
#include <cassert>

// The very first thing to do with WebGPU is to get the adapter.
// The adapter is used to access the capabilities of the customer’s hardware, which are used to select the behavior of your application among very different code paths
// Adapter:
//
//The adapter represents the interface to the underlying hardware capabilities of the user's machine. It provides information about the available features, performance characteristics, and limitations of the graphics hardware.
//By querying the adapter, the application can gather information about the hardware environment it's running on, allowing it to make informed decisions about which features to use and how to optimize performance.
//Different machines may have different adapters with varying capabilities. Therefore, by querying the adapter, the application can adapt its behavior to the specific hardware it's running on.
/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
#define UNUSED(x) (void)x;

int main (int, char**) {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        return 1;
    }

    std::cout << "Requesting adapter..." << std::endl;
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = glfwGetWGPUSurface(instance, window);
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

    inspectDevice(device);

    // Get the main and only command queue used to send instructions to the GPU
    WGPUQueue queue = wgpuDeviceGetQueue(device);

#ifdef WEBGPU_BACKEND_DAWN
    // Add a callback to monitor the moment queued work winished
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
	    std::cout << "Queued work finished with status: " << status << std::endl;
	};
	wgpuQueueOnSubmittedWorkDone(queue, 0 /* non standard argument for Dawn */, onQueueWorkDone, nullptr /* pUserData */);
#endif // WEBGPU_BACKEND_DAWN

    // We create a command encoder to be able to create command buffers
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    // Encode some mock commands
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

    // Encode commands into a command buffer
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder); // release encoder after it's finished

    // Finally submit the command queue
    std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command); // release command buffer once submitted

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
    //wgpuSurfaceRelease(surface);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;

}