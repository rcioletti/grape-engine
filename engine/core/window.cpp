#include "window.hpp"
#include <stdexcept>
#include <stb_image.h> // Include the image loading library

namespace grape {

    Window::Window(int w, int h, std::string name) : width{ w }, height{ h }, windowName{ name } {
        initWindow();
    }

    Window::~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto grapeWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        grapeWindow->frameBufferResized = true;
        grapeWindow->width = width;
        grapeWindow->height = height;
    }

    void setWindowIcons(GLFWwindow* window, const char* icon16, const char* icon32, const char* icon48) {
        GLFWimage images[3];
        int width, height, channels;

        // Load 16x16 icon
        images[0].pixels = stbi_load(icon16, &width, &height, &channels, 4);
        if (!images[0].pixels) {
            throw std::runtime_error("Failed to load 16x16 icon");
        }
        images[0].width = width;
        images[0].height = height;

        // Load 32x32 icon
        images[1].pixels = stbi_load(icon32, &width, &height, &channels, 4);
        if (!images[1].pixels) {
            stbi_image_free(images[0].pixels);
            throw std::runtime_error("Failed to load 32x32 icon");
        }
        images[1].width = width;
        images[1].height = height;

        // Load 48x48 icon
        images[2].pixels = stbi_load(icon48, &width, &height, &channels, 4);
        if (!images[2].pixels) {
            stbi_image_free(images[0].pixels);
            stbi_image_free(images[1].pixels);
            throw std::runtime_error("Failed to load 48x48 icon");
        }
        images[2].width = width;
        images[2].height = height;

        // Set the window icons
        glfwSetWindowIcon(window, 3, images);

        // Free the image data
        stbi_image_free(images[0].pixels);
        stbi_image_free(images[1].pixels);
        stbi_image_free(images[2].pixels);
    }


    void Window::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        
        glfwMaximizeWindow(window);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        // Call the new function to set the window icon
        // Make sure "path/to/your/icon.png" is a valid path to your icon file
        setWindowIcons(window, "../resources/grape_logo_16.png", "../resources/grape_logo_32.png", "../resources/grape_logo_48.png");
        
        glfwPollEvents();
    }

    void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface");
        }
    }
}