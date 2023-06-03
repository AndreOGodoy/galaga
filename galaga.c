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
  float velocity;
  int is_active;
  float width, height;
} Entity;

typedef struct {
  Entity entity;
} Enemy;

typedef struct {
  Entity entity;
  int from_enemy;
} Bullet;

typedef struct {
  Entity entity;
} Spaceship;

Enemy enemies[MAX_ENEMIES];
Bullet bullets[MAX_BULLETS];
int curr_bullet = 0;

Spaceship spaceship = { .entity = { .x=0.0f, .y=-0.5f, .velocity=0.04f }};

// TODO: Make more efficient
int get_available_bullet() {
  for (int i = 0; i < MAX_BULLETS; ++i) {
	if (!bullets[i].entity.is_active) {
	  return i;
	}
  }

  return -1;
}

int check_collision(Entity *a, Entity *b) {
  return
	a->x <= b->x + b->width / 2.0f &&
    a->x >= b->x - b->width / 2.0f &&
    a->y >= b->y - b->height / 2.0f &&
	a->y >= b->y + b->height / 2.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE)
	spaceship.entity.x -= spaceship.entity.velocity;
  else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE)
	spaceship.entity.x += spaceship.entity.velocity;
  else if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
	spaceship.entity.y += spaceship.entity.velocity;
  else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
	spaceship.entity.y -= spaceship.entity.velocity;
  else if (key == GLFW_KEY_SPACE && action != GLFW_RELEASE) {
	int bullet_index = get_available_bullet();
	if (bullet_index < 0) return;

	Bullet *bullet = &bullets[bullet_index];

	bullet->entity.x = spaceship.entity.x;
	bullet->entity.y = spaceship.entity.y;
	bullet->entity.velocity = 0.02;
	bullet->from_enemy = 0;
	bullet->entity.is_active = 1;
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
	enemy.entity.x -  enemy.entity.width / 2.0, enemy.entity.y, 0.0f,
	enemy.entity.x +  enemy.entity.width / 2.0, enemy.entity.y, 0.0f,
	enemy.entity.x, enemy.entity.y - enemy.entity.height, 0.0f,
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
	enemies[i].entity.x = -1 + i * (0.060 + 0.15) + 0.060;
	enemies[i].entity.y = 0.7;
	enemies[i].entity.height = ENEMIES_HEIGHT;
	enemies[i].entity.width = ENEMIES_WIDTH;

	if (enemies[i].entity.x < 1 && enemies[i].entity.x > -1) {
	  enemies[i].entity.is_active = 1;
	}
  }

  for (int i = 0; i < MAX_BULLETS; ++i) {
	bullets[i].entity.is_active = 0;
	bullets[i].entity.velocity = 0.01f;
	bullets[i].from_enemy = 0;
  }
}

void draw_enemies() {
  for (int i = 0; i < MAX_ENEMIES; ++i)
	if (enemies[i].entity.is_active)
	  draw_enemy(enemies[i]);
}

void enemy_shot(Enemy enemy) {
  int bullet_index = get_available_bullet();
  if (bullet_index < 0) return;

  Bullet *bullet = &bullets[bullet_index];

  bullet->entity.x = enemy.entity.x;
  bullet->entity.y = enemy.entity.y;
  bullet->entity.is_active = 1;
  bullet->from_enemy = 1;
  bullet->entity.velocity = -0.01;
}


void update_bullets() {
  for (int i = 0; i < MAX_BULLETS; ++i) {
	if (bullets[i].entity.is_active) {
	  bullets[i].entity.y += bullets[i].entity.velocity;
	  draw_bullet(bullets[i].entity.x, bullets[i].entity.y);

	  if (bullets[i].entity.y >= 1 || bullets[i].entity.y <= -1) {
		bullets[i].entity.is_active = 0;
	  }

	  if (bullets[i].from_enemy == 1) {
		continue;
	  }

	  for (int j = 0; j < MAX_ENEMIES; ++j) {
		if (enemies[j].entity.is_active) {
		  if (bullets[i].entity.x <= enemies[j].entity.x + enemies[j].entity.width / 2.0 &&
			  bullets[i].entity.x >= enemies[j].entity.x - enemies[j].entity.width / 2.0 &&
			  bullets[i].entity.y >= enemies[j].entity.y - enemies[j].entity.height) {
			enemies[j].entity.is_active = 0;
			bullets[i].entity.is_active = 0;
		  }
		}
	  }
	}
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
  unsigned ticks = 0;
  while (!glfwWindowShouldClose(window)) {
	glClear(GL_COLOR_BUFFER_BIT);

	draw_spaceship(spaceship.entity.x, spaceship.entity.y);
	draw_enemies();

	if (ticks >= 60) {
	  for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (enemies[i].entity.is_active) {
		  enemy_shot(enemies[i]);
		}
	  }

	  ticks = 0;
	}

	update_bullets();

	glfwSwapBuffers(window);
	glfwPollEvents();
	ticks += 1;
  }

  glfwTerminate();
  return 0;
}

