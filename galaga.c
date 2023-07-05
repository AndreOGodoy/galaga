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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void debug(const char* text) {
    printf("[DEBUG] %s\n", text);
}

void setup_game();

#define BUFF_SIZE 2048
#define MAX_BULLETS 512

#define MAX_ENEMIES 16 // Potencias de 2 preferencialmente
int enemies_alive = MAX_ENEMIES;
int max_enemies_going_down;

#define ENEMIES_HEIGHT 60.0f
#define ENEMIES_WIDTH 60.0f

int END_GAME = 0;
int ENEMIES_CAN_SHOT = 1;

unsigned int VAO, VBO;

int shoot;
double pause_start_time;
double pause_x_cursor_pos, pause_y_cursor_pos;

int screen_width, screen_height;

typedef struct {
    float x, y;
    float velocity;
    int is_active;
    float width, height;
    GLuint sprite;
} Entity;

typedef struct {
    double fire_rate;
    double last_shoot_time;
    GLuint bullet_sprite;
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

int PAUSE_GAME = 0;
int DEBUG_MODE = 0;

Spaceship spaceship = { .entity = { .x=400.0f, .y=100.0f, .velocity=5.0f, .width=50.0f, .height=50.0f }, .ship = { .fire_rate=0.7, .last_shoot_time=-1 } };

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


void move_cursor_to_middle(GLFWwindow *window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    // Move cursor to the middle of the window
    glfwSetCursorPos(window, windowWidth / 2.0, windowHeight / 2.0);
}

void pause(GLFWwindow *window) {
    PAUSE_GAME = !PAUSE_GAME;
    if (PAUSE_GAME) {
        // Record when the pause started
        pause_start_time = glfwGetTime();
	glfwGetCursorPos(window, &pause_x_cursor_pos, &pause_y_cursor_pos);
    } else {
        // Calculate how long the game was paused
        double pause_duration = glfwGetTime() - pause_start_time;

        // Adjust the enemies' last shoot time
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            enemies[i].ship.last_shoot_time += pause_duration;
        }

        // Also adjust the player's last shoot time
        spaceship.ship.last_shoot_time += pause_duration;
	glfwSetCursorPos(window, pause_x_cursor_pos, pause_y_cursor_pos);
    }
}

void restart(GLFWwindow* window) {
    move_cursor_to_middle(window);
    setup_game();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        restart(window);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && !PAUSE_GAME) {
	if (action == GLFW_PRESS)
	    shoot = 1;
	else if (action == GLFW_RELEASE) {
	    shoot = 0;
	}
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        // toggle pause
        if (action == GLFW_PRESS)
	    pause(window);
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        // toggle debug mode
        if (action == GLFW_PRESS) {
            DEBUG_MODE = !DEBUG_MODE;
            PAUSE_GAME = DEBUG_MODE;
        }
        if (DEBUG_MODE) {
	}
	//print_game_objects();
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (PAUSE_GAME)
        return;

    spaceship.entity.x = xpos - spaceship.entity.width / 2.0;
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

GLuint load_texture(char const * path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 4);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Add this line
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        printf("Texture failed to load at path: %s", path);
    }
    stbi_image_free(data);
    return textureID;
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetInputMode(*window, GLFW_REPEAT, GLFW_FALSE);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
}

void draw_spaceship(float x, float y) {
    glBindTexture(GL_TEXTURE_2D, spaceship.entity.sprite);

    float vertices[] = {
        // positions          // texture coords
        x - spaceship.entity.width / 2.0f, y - spaceship.entity.height / 2.0f, 0.0f, 0.0f, 1.0f,
        x + spaceship.entity.width / 2.0f, y - spaceship.entity.height / 2.0f, 0.0f, 1.0f, 1.0f,
        x - spaceship.entity.width / 2.0f, y + spaceship.entity.height / 2.0f, 0.0f, 0.0f, 0.0f,
        x + spaceship.entity.width / 2.0f, y + spaceship.entity.height / 2.0f, 0.0f, 1.0f, 0.0f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_enemy(Enemy enemy) {
    glBindTexture(GL_TEXTURE_2D, enemy.entity.sprite);

    float vertices[] = {
        // positions          // texture coords
        enemy.entity.x - enemy.entity.width / 2.0f, enemy.entity.y - enemy.entity.height / 2.0f, 0.0f, 0.0f, 0.0f,
        enemy.entity.x + enemy.entity.width / 2.0f, enemy.entity.y - enemy.entity.height / 2.0f, 0.0f, 1.0f, 0.0f,
        enemy.entity.x - enemy.entity.width / 2.0f, enemy.entity.y + enemy.entity.height / 2.0f, 0.0f, 0.0f, 1.0f,
        enemy.entity.x + enemy.entity.width / 2.0f, enemy.entity.y + enemy.entity.height / 2.0f, 0.0f, 1.0f, 1.0f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_bullet(Bullet bullet) {
    float halfWidth = 10.0;
    float height = 10.0;
    glBindTexture(GL_TEXTURE_2D, bullet.entity.sprite);

    float vertices[] = {
        // positions          // texture coords
        bullet.entity.x - bullet.entity.width / 2.0f, bullet.entity.y - bullet.entity.height / 2.0f, 0.0f, 0.0f, 1.0f,
        bullet.entity.x + bullet.entity.width / 2.0f, bullet.entity.y - bullet.entity.height / 2.0f, 0.0f, 1.0f, 1.0f,
        bullet.entity.x - bullet.entity.width / 2.0f, bullet.entity.y + bullet.entity.height / 2.0f, 0.0f, 0.0f, 0.0f,
        bullet.entity.x + bullet.entity.width / 2.0f, bullet.entity.y + bullet.entity.height / 2.0f, 0.0f, 1.0f, 0.0f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void draw_enemies() {
    for (int i = 0; i < MAX_ENEMIES; ++i)
	if (enemies[i].entity.is_active)
	    draw_enemy(enemies[i]);
}

void draw_background(GLFWwindow *window,  GLuint texture) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLfloat vertices[] = {
	0.0f , 0.0f, 0.0f, 0.0f, 1.0f,  // bottom left
        windowWidth, 0.0f, 0.0f, 1.0f, 1.0f,  // bottom right
        0.0f, windowHeight, 0.0f, 0.0f, 0.0f,  // top left
        windowWidth, windowHeight, 0.0f, 1.0f, 0.0f   // top right
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void enemy_shot(Enemy *enemy) {
    double curr_time = glfwGetTime();
    double curr_shoot_delay = curr_time - enemy->ship.last_shoot_time;

    if (ENEMIES_CAN_SHOT && curr_shoot_delay >= enemy->ship.fire_rate && !PAUSE_GAME) {
	enemy->ship.last_shoot_time = curr_time;
	int bullet_index = get_available_bullet();
	if (bullet_index < 0) return;

	Bullet *bullet = &bullets[bullet_index];

	bullet->entity.sprite = enemy->ship.bullet_sprite;
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
	    if (!PAUSE_GAME) {
		bullets[i].entity.y += bullets[i].entity.velocity;
	    }

	    draw_bullet(bullets[i]);

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

    int num_rows = 2; // Number of rows for the enemy formation
    int num_columns = MAX_ENEMIES / num_rows; // Number of enemies per row

    float start_x = (screen_width - num_columns * ENEMIES_WIDTH * 2) / 2.0;
    float start_y = screen_height * 0.9;

    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col < num_columns; ++col) {
            int index = row * num_columns + col;
            if (index >= MAX_ENEMIES) {
                break;
            }

            Enemy* enemy = &enemies[index];
            enemy->entity.x = start_x + col * ENEMIES_WIDTH * 2.0;
            enemy->entity.y = start_y - row * ENEMIES_HEIGHT;
            enemy->entity.is_active = 1;
            enemy->ship.fire_rate *= 0.75;
        }
    }

    max_enemies_going_down += 1;
    enemies_alive = MAX_ENEMIES;
    ENEMIES_CAN_SHOT = 1;
}


void create_enemy_type_one(Enemy *enemy) {
    enemy->entity.velocity = 0.3;
    enemy->ship.fire_rate = 3.0;
    enemy->ship.last_shoot_time = 1 + rand() % 4;

}

void create_enemy_type_two(Enemy *enemy) {
    enemy->entity.velocity = 0.4;
    enemy->ship.fire_rate = 3.5;
    enemy->ship.last_shoot_time = 1 + rand() % 4;
}

void create_enemy_type_three(Enemy *enemy) {
    enemy->entity.velocity = 0.5;
    enemy->ship.fire_rate = 4.5;
    enemy->ship.last_shoot_time = 1 + rand() % 4;
}

void setup_game() {
    spaceship.entity.sprite = load_texture("ship.png");

    int half_enemies = MAX_ENEMIES / 2;
    int quarter_enemies = MAX_ENEMIES / 4;
    GLuint enemy1_sprite = load_texture("./enemy1.png");
    GLuint bullet_enemy1_sprite = load_texture("./bullet_enemy1.png");

    GLuint enemy2_sprite = load_texture("./enemy2.png");
    GLuint bullet_enemy2_sprite = load_texture("./bullet_enemy2.png");

    GLuint enemy3_sprite = load_texture("./enemy3.png");
    GLuint bullet_enemy3_sprite = load_texture("./bullet_enemy3.png");

    for (int i = 0; i < MAX_ENEMIES; ++i) {
	if (i < half_enemies) {
	    create_enemy_type_one(&enemies[i]);
	    enemies[i].entity.sprite = enemy1_sprite;
	    enemies[i].ship.bullet_sprite = bullet_enemy1_sprite;
	} else if (i < half_enemies + quarter_enemies) {
	    create_enemy_type_two(&enemies[i]);
	    enemies[i].entity.sprite = enemy2_sprite;
	    enemies[i].ship.bullet_sprite = bullet_enemy2_sprite;
	} else {
	    create_enemy_type_three(&enemies[i]);
	    enemies[i].entity.sprite = enemy3_sprite;
	    enemies[i].ship.bullet_sprite = bullet_enemy3_sprite;
	}

	enemies[i].entity.x = (i + 0.5) * screen_width / MAX_ENEMIES;
	enemies[i].entity.y = screen_height * 0.90f;
	enemies[i].entity.height = ENEMIES_HEIGHT;
	enemies[i].entity.width = ENEMIES_WIDTH;

	enemies[i].entity.is_active = 1;
    }

    max_enemies_going_down = 0;
    create_next_phase();

    GLuint bullet_sprite = load_texture("bullet.png");
    for (int i = 0; i < MAX_BULLETS; ++i) {
	bullets[i].entity.width = 40.0;
	bullets[i].entity.height = 40.0;
	bullets[i].entity.sprite = bullet_sprite;
	bullets[i].entity.is_active = 0;
	bullets[i].entity.velocity = 0.0;
	bullets[i].from_enemy = 0;

    }
}


int main() {
    GLFWwindow *window;
    configure_window(&window);

    srand(time(NULL));
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    setup_game();
    int ticks = 0;
    int next_phase_countdown = 0;

    GLuint background_texture = load_texture("bg.png");
    while (!glfwWindowShouldClose(window)) {
	glClear(GL_COLOR_BUFFER_BIT);
	draw_background(window, background_texture);

	draw_spaceship(spaceship.entity.x, spaceship.entity.y);
	draw_enemies();
	update_enemies();
	update_bullets();
	handle_movement();

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

	glfwSwapBuffers(window);
	glfwPollEvents();

	if (ticks > 0)
	    ticks--;
    }

    glfwTerminate();
    return 0;
}

