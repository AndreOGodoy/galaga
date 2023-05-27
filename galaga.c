#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define BUFF_SIZE 2048

#define MAX_BULLETS 128

#define MAX_ENEMIES 128
#define ENEMIES_HEIGHT 0.07f
#define ENEMIES_WIDTH 0.06f

unsigned int VAO, VBO;

typedef struct {
    float x, y;
    int is_active;
    float width, height;
} Enemy;

typedef struct {
    float x, y;
    float velocity;
    int is_active;
} Bullet;

typedef struct {
    float x, y;
    float velocity;
} Spaceship;

Enemy enemies[MAX_ENEMIES];
Bullet bullets[MAX_BULLETS];
size_t curr_bullet = 0;

Spaceship spaceship = { .x=0.0f, .y=-0.5f, .velocity=0.02f };

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE)
	spaceship.x -= spaceship.velocity;
    else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE)
	spaceship.x += spaceship.velocity;
    else if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
	spaceship.y += spaceship.velocity;
    else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
	spaceship.y -= spaceship.velocity;
    else if (key == GLFW_KEY_SPACE && action != GLFW_RELEASE) {
	bullets[curr_bullet].x = spaceship.x;
	bullets[curr_bullet].y = spaceship.y;
	bullets[curr_bullet].is_active = 1;
 	curr_bullet = (curr_bullet + 1) % MAX_BULLETS;
    }
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

void draw_enemy(Enemy enemy) {
    float vertices[] = {
	enemy.x -  enemy.width / 2.0, enemy.y, 0.0f,
	enemy.x +  enemy.width / 2.0, enemy.y, 0.0f,
	enemy.x, enemy.y - enemy.height, 0.0f,
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void draw_bullet(float x, float y) {
    float halfWidth = 0.01f;
    float height = 0.02f;

    float vertices[] = {
	x -  halfWidth, y, 0.0f,
	x +  halfWidth, y, 0.0f,
	x, y - height, 0.0f,
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void setup_game() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
	enemies[i].x = -1 + i * (0.060 + 0.15) + 0.060;
	enemies[i].y = 0.7;
	enemies[i].is_active = 1;
	enemies[i].height = ENEMIES_HEIGHT;
	enemies[i].width = ENEMIES_WIDTH;
    }

    for (int i = 0; i < MAX_BULLETS; ++i) {
	bullets[i].is_active = 0;
	bullets[i].velocity = 0.01f;
    }
}

int main() {
    GLFWwindow *window;
    configure_window(&window);

    glClearColor(13.0f/255.0f, 93.0f/255.0f, 143.0f/255.0f, 1.0f);

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    unsigned int shader_program = glCreateProgram();
    compile_shaders(&vertex_shader, &fragment_shader, &shader_program);

    setup_game();
    glUseProgram(shader_program);

    glfwSetKeyCallback(window, key_callback);
    while (!glfwWindowShouldClose(window)) {
	glClear(GL_COLOR_BUFFER_BIT);

	draw_spaceship(spaceship.x, spaceship.y);

	for (int i = 0; i < MAX_ENEMIES; ++i)
	    if (enemies[i].is_active)
		draw_enemy(enemies[i]);

	for (int i = 0; i < MAX_BULLETS; ++i) {
	    if (bullets[i].is_active) {
		bullets[i].y += bullets[i].velocity;
		draw_bullet(bullets[i].x, bullets[i].y);

		if (bullets[i].y >= 1) {
		    bullets[i].is_active = 0;
		}

		for (int j = 0; j < MAX_ENEMIES; ++j) {
		    if (enemies[j].is_active) {
			if (bullets[i].x <= enemies[j].x + enemies[j].width / 2.0 &&
			    bullets[i].x >= enemies[j].x - enemies[j].width / 2.0 &&
			    bullets[i].y >= enemies[j].y - enemies[j].height) {

			    enemies[j].is_active = 0;
			    bullets[i].is_active = 0;
			}
		    }
		}
	    }
	}

	glfwSwapBuffers(window);
	glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

