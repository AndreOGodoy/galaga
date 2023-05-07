#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int heigth) {
  glViewport(0, 0, width, heigth);
}

void configure_window(GLFWwindow **window) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  *window = glfwCreateWindow(800, 600, "LearningGL", NULL, NULL);
  if (*window == NULL) {
    printf("[ERROR] Failed to create window\n");
    glfwTerminate();
    exit(1);
  }

  if (*window == NULL) {
    printf("[ERROR] Failed to create window\n");
    glfwTerminate();
    exit(1);
  }

  glfwMakeContextCurrent(*window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("[ERROR] Failed to initialize GLAD\n");
    glfwTerminate();
    exit(1);
  }

  glViewport(0, 0, 800, 600);
  glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);

  return;
}


int main() {
  GLFWwindow *window;
  configure_window(&window);

  while(glfwGetKey(window, GLFW_KEY_ESCAPE) != !glfwWindowShouldClose(window)) {
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}



