#include <cglm/cam.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>
#include <cglm/mat4.h>

void debug(const char* text) {
    printf("[DEBUG] %s\n", text);
}

#define BUFF_SIZE 2048
#define MAX_BULLETS 512

#define MAX_ENEMIES 10
int enemies_alive = MAX_ENEMIES;

#define ENEMIES_HEIGHT 60.0f
#define ENEMIES_WIDTH 60.0f

int END_GAME = 0;
int ENEMIES_CAN_SHOT = 1;

unsigned int VAO, VBO;

int screen_width, screen_height;

typedef struct {
    float x, y;
    float velocity;
    int is_active;
    float width, height;
} Entity;

typedef struct {
    double fire_rate;
    double last_shoot_time;
} Ship;

typedef struct {
    Entity entity;
    Ship ship;
} Enemy;

typedef struct {
    Entity entity;
    int from_enemy;
} Bullet;

typedef struct {
    Entity entity;
    Ship ship;
} Spaceship;

Enemy enemies[MAX_ENEMIES];
Bullet bullets[MAX_BULLETS];
int curr_bullet = 0;

Spaceship spaceship = { .entity = { .x=400.0f, .y=100.0f, .velocity=5.0f, .width=60.0f, .height=60.0f }, .ship = { .fire_rate=0.2, .last_shoot_time=-1 } };

int next_bullet = 0;
int get_available_bullet() {
    int available_bullet = next_bullet;
    next_bullet = (next_bullet + 1) % MAX_BULLETS;
    return available_bullet;
}

int check_collision(Entity *a, Entity *b) {
    return
	a->x + a->width / 2.0f >= b->x - b->width / 2.0f &&
	a->x - a->width / 2.0f <= b->x + b->width / 2.0f &&
	a->y + a->height / 2.0f >= b->y - b->height / 2.0f &&
	a->y - a->height / 2.0f <= b->y + b->height / 2.0f;
}

int moveLeft = 0;
int moveRight = 0;
int moveUp = 0;
int moveDown = 0;
int shoot = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
	if (action == GLFW_PRESS)
	    moveLeft = 1;
	else if (action == GLFW_RELEASE)
	    moveLeft = 0;
    }
    else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
	if (action == GLFW_PRESS)
	    moveRight = 1;
	else if (action == GLFW_RELEASE)
	    moveRight = 0;
    }
    else if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
	if (action == GLFW_PRESS)
	    moveUp = 1;
	else if (action == GLFW_RELEASE)
	    moveUp = 0;
    }
    else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
	if (action == GLFW_PRESS)
	    moveDown = 1;
	else if (action == GLFW_RELEASE)
	    moveDown = 0;
    }
    else if (key == GLFW_KEY_SPACE || key ==  GLFW_KEY_LEFT_CONTROL) {
	if (action == GLFW_PRESS)
	    shoot = 1;
	else if (action == GLFW_RELEASE)
	    shoot = 0;
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
    screen_width = width;
    screen_height = height;
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

    glfwSetInputMode(*window, GLFW_REPEAT, GLFW_FALSE);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
}

void draw_spaceship(float x, float y) {
    float vertices[] = {
	x -  spaceship.entity.width / 2, y - spaceship.entity.height, 0.0f,
	x +  spaceship.entity.width / 2, y - spaceship.entity.height, 0.0f,
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
    float halfWidth = 10.0;
    float height = 10.0;

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

void draw_enemies() {
    for (int i = 0; i < MAX_ENEMIES; ++i)
	if (enemies[i].entity.is_active)
	    draw_enemy(enemies[i]);
}

void enemy_shot(Enemy *enemy) {
    double curr_time = glfwGetTime();
    double curr_shoot_delay = curr_time - enemy->ship.last_shoot_time;

    if (ENEMIES_CAN_SHOT && curr_shoot_delay >= enemy->ship.fire_rate) {
	enemy->ship.last_shoot_time = curr_time;
	int bullet_index = get_available_bullet();
	if (bullet_index < 0) return;

	Bullet *bullet = &bullets[bullet_index];

	bullet->entity.x = enemy->entity.x;
	bullet->entity.y = enemy->entity.y;
	bullet->entity.is_active = 1;
	bullet->from_enemy = 1;
	bullet->entity.velocity = -5.0;
    }
}


void update_bullets() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
	if (bullets[i].entity.is_active) {
	    bullets[i].entity.y += bullets[i].entity.velocity;
	    draw_bullet(bullets[i].entity.x, bullets[i].entity.y);

	    if (bullets[i].entity.y >= screen_height || bullets[i].entity.y <= 0) {
		bullets[i].entity.is_active = 0;
	    }

	    if (bullets[i].from_enemy == 1) {
		if (check_collision(&bullets[i].entity, &spaceship.entity)) {
		    debug("Entrou no collision");
		    END_GAME = 1;
		    return;
		}
		continue;
	    }

	    for (int j = 0; j < MAX_ENEMIES; ++j) {
		if (enemies[j].entity.is_active) {
		    if (check_collision(&bullets[i].entity, &enemies[j].entity)) {
			enemies[j].entity.is_active = 0;
			bullets[i].entity.is_active = 0;

			enemies_alive--;
		    }
		}
	    }
	}
    }
}

void update_enemies() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
	if (enemies[i].entity.is_active) {
	    enemy_shot(&enemies[i]);
	}
    }
}

void handle_movement() {
    if (moveLeft)
	spaceship.entity.x -= spaceship.entity.velocity;
    if (moveRight)
	spaceship.entity.x += spaceship.entity.velocity;
    if (moveUp)
	spaceship.entity.y += spaceship.entity.velocity;
    if (moveDown)
	spaceship.entity.y -= spaceship.entity.velocity;


    spaceship.entity.x = fmod(spaceship.entity.x + screen_width, screen_width);

    double curr_time = glfwGetTime();
    double curr_shoot_delay = curr_time - spaceship.ship.last_shoot_time;
    if (shoot && curr_shoot_delay >= spaceship.ship.fire_rate) {
	int bullet_index = get_available_bullet();
	if (bullet_index < 0) return;

	Bullet* bullet = &bullets[bullet_index];

	bullet->entity.x = spaceship.entity.x;
	bullet->entity.y = spaceship.entity.y;
	bullet->entity.velocity = 10.0f;
	bullet->from_enemy = 0;
	bullet->entity.is_active = 1;

	spaceship.ship.last_shoot_time = curr_time;
    }
}

void create_next_phase() {
    spaceship.entity.x = screen_width / 2.0;
    spaceship.entity.y = screen_height * 0.10;

    spaceship.entity.height *= 1.15;
    spaceship.entity.width *= 1.15;
    spaceship.entity.velocity *= 0.95;

    for (int i = 0; i < MAX_ENEMIES; ++i) {
	enemies[i].ship.fire_rate *= 0.75;
	enemies[i].entity.is_active = 1;
    }

    float arc_radius = screen_width * 0.5; // Adjust this value to change the size of the arc
    float arc_center_x = screen_width / 2.0;
    float arc_center_y = screen_height * 0.25; // Adjust this value to change the vertical position of the arc

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        float angle = (M_PI / MAX_ENEMIES) * i;
        enemies[i].entity.x = arc_center_x + arc_radius * cos(angle);
        enemies[i].entity.y = arc_center_y + arc_radius * sin(angle);

        enemies[i].ship.fire_rate *= 0.75;
        enemies[i].entity.is_active = 1;
    }

    enemies_alive = MAX_ENEMIES;
    ENEMIES_CAN_SHOT = 1;
}

void setup_game() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
	enemies[i].entity.x = (i + 0.5) * screen_width / MAX_ENEMIES;
	enemies[i].entity.y = screen_height * 0.90f;
	enemies[i].entity.height = ENEMIES_HEIGHT;
	enemies[i].entity.width = ENEMIES_WIDTH;

	enemies[i].ship.fire_rate = 2;
	enemies[i].ship.last_shoot_time = -1;

	/* if (enemies[i].entity.x >= 0 && enemies[i].entity.x <= screen_width) { */
	/*     enemies[i].entity.is_active = 1; */
	/* } */
	enemies[i].entity.is_active = 1;
    }

    create_next_phase();

    for (int i = 0; i < MAX_BULLETS; ++i) {
	bullets[i].entity.is_active = 0;
	bullets[i].entity.velocity = 0.0;
	bullets[i].from_enemy = 0;
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

    glUseProgram(shader_program);

    glfwGetWindowSize(window, &screen_width, &screen_height);
    mat4 projection;
    glm_ortho(0, (float) screen_width, 0, (float) screen_height, -1.0, 1.0, projection);
    int transform_loc = glGetUniformLocation(shader_program, "transform");

    glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &projection[0][0]);

    glfwSetKeyCallback(window, key_callback);
    setup_game();
    int ticks = 0;
    int next_phase_countdown = 0;
    while (!glfwWindowShouldClose(window)) {
	glClear(GL_COLOR_BUFFER_BIT);

	draw_spaceship(spaceship.entity.x, spaceship.entity.y);
	draw_enemies();

	update_enemies();
	update_bullets();

	if (END_GAME) {
	    break;
	}

	if (enemies_alive == 0) {
	    if (next_phase_countdown == 1 && ticks == 0) {
		create_next_phase();
		next_phase_countdown = 0;
	    }

	    else if (ticks == 0) {
		ticks = 120;
		next_phase_countdown = 1;
	    }
	}

	handle_movement();
	glfwSwapBuffers(window);
	glfwPollEvents();

	if (ticks > 0)
	    ticks--;
    }

    glfwTerminate();
    return 0;
}

