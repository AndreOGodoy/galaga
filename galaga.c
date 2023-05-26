#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define BUFF_SIZE 2048
unsigned int VAO, VBO;

float spaceshipX = 0.0f;
float spaceshipY = 0.0f;
float spaceshipSpeed = 0.05f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE)
	spaceshipX -= spaceshipSpeed;
    else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE)
	spaceshipX += spaceshipSpeed;
    else if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
	spaceshipY += spaceshipSpeed;
    else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
	spaceshipY -= spaceshipSpeed;
}

int read_file(const char* file_name, char* buffer) {
    FILE* file = fopen(file_name, "r");
    if (!file) {
	printf("[ERROR] Faniled to open file: %s\n", file_name);
	return -1;
    }

    size_t read_size = fread((void*)buffer, sizeof(char), BUFF_SIZE, file);
    if (read_size <= 0) {
	fclose(file);
	return -1;
    }

    buffer[read_size] = '\0';
    fclose(file);
    return 1;
}

int compile_shaders(unsigned int *vertex_shader, unsigned int *fragment_shader, unsigned int *shader_program) {
    char vertex_source[BUFF_SIZE];
    read_file("vertex_shader.glsl", vertex_source);

    const char* vertex_sources[] = { vertex_source };
    glShaderSource(*vertex_shader, 1, vertex_sources, NULL);
    glCompileShader(*vertex_shader);

    int success;
    char infoLog[512];
    glGetShaderiv(*vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
	glGetShaderInfoLog(*vertex_shader, BUFF_SIZE, NULL, infoLog);
	printf("[ERRO]: vertex_shader compilation failed: %s\n", infoLog);
	return success;
    }

    char fragment_source[BUFF_SIZE];
    read_file("fragment_shader.glsl", fragment_source);

    const char* fragment_sources[] = { fragment_source };
    glShaderSource(*fragment_shader, 1, fragment_sources, NULL);
    glCompileShader(*fragment_shader);

    glGetShaderiv(*fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
	glGetShaderInfoLog(*fragment_shader, BUFF_SIZE, NULL, infoLog);
	printf("[ERRO]: fragment_shader compilation failed: %s\n", infoLog);
	return success;
    }

    glAttachShader(*shader_program, *vertex_shader);
    glAttachShader(*shader_program, *fragment_shader);
    glLinkProgram(*shader_program);
    glGetShaderiv(*shader_program, GL_LINK_STATUS, &success);
    if (!success) {
	glGetShaderInfoLog(*shader_program, BUFF_SIZE, NULL, infoLog);
	printf("[ERRO]: shader_program linkage failed: %s\n", infoLog);
	return success;
    }

    return success;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
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

    glfwMakeContextCurrent(*window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	printf("[ERROR] Failed to initialize GLAD\n");
	glfwTerminate();
	exit(1);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
}

void draw_spaceship(float x, float y) {
    float halfWidth = 0.025f;  // Half of the width of the triangle
    float height = 0.05f;     // Height of the triangle

    float vertices[] = {
	x -  halfWidth, y - height, 0.0f,
	x +  halfWidth, y - height, 0.0f,
	x, y, 0.0f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
    GLFWwindow *window;
    configure_window(&window);

    glClearColor(13.0f/255.0f, 93.0f/255.0f, 143.0f/255.0f, 1.0f);

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    unsigned int shader_program = glCreateProgram();
    compile_shaders(&vertex_shader, &fragment_shader, &shader_program);

    glfwSetKeyCallback(window, key_callback);
    while (!glfwWindowShouldClose(window)) {
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shader_program);
	glBindVertexArray(VAO);
	draw_spaceship(spaceshipX, spaceshipY);

	glfwSwapBuffers(window);
	glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

