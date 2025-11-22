#include <M5Unified.h>
#include <cmath>
#include <vector>
#include <random>

// Глобальные переменные
bool game_over = false;
bool game_over_played = false;
bool in_menu = true;
bool screen_dimmed = false;
bool Sound = false;
int menu_selection = 0;

// Таймер бездействия
unsigned long last_activity_time = 0;
const unsigned long inactivity_timeout = 30000;

// Корабль
float ship_x = 0;
float ship_y = 0;
const float ship_size = 15;
float ship_rotation = 0;
float ship_rotation_speed = 0;
float ship_speed_x = 0;
float ship_speed_y = 0;
const float ship_acceleration = 0.2;
const float ship_max_speed = 3.0;
const float ship_friction = 0.95;
bool ship_moving = false;

// Топливная система
float ship_fuel = 100.0f;
const float max_fuel = 100.0f;
const float fuel_consumption_move = 0.1f;  // Расход за кадр при движении
const float fuel_consumption_bullet = 2.0f; // Расход за выстрел
const float fuel_consumption_missile = 10.0f; // Расход за ракету
const float fuel_refill_amount = 30.0f;    // Количество топлива в баке

// Структуры для объектов
struct Particle {
    float x, y;
    float vx, vy;
    int size;
    int life;
    uint32_t color;
};

struct Bullet {
    float x, y;
    float vx, vy;
    int life;
};

struct Missile {
    float x, y;
    float vx, vy;
    float rotation;
    float speed;
    int life;
    std::vector<std::pair<float, float>> trail;
};

struct Asteroid {
    float x, y;
    float vx, vy;
    int size;
    float rotation;
    float rotation_speed;
    float orbit_angle; // Для астероидов-защитников
    float orbit_distance; // Для астероидов-защитников
    bool is_guardian; // Флаг астероида-защитника
};

struct Fragment {
    float x, y;
    float vx, vy;
    int size;
    float rotation;
    float rotation_speed;
    int life;
    uint32_t color;
};

struct Explosion {
    float x, y;
    float size;
    float max_size;
    int life;
    float damage_radius; // Радиус урона для взрыва ракеты
};

// Топливный бак
struct FuelTank {
    float x, y;
    int size;
    bool active;
    std::vector<Asteroid> guardian_asteroids; // Астероиды-защитники
};

FuelTank fuel_tank = {0, 0, 12, false, {}};

// Контейнеры объектов
std::vector<Particle> exhaust_particles;
std::vector<Bullet> bullets;
std::vector<Missile> missiles;
std::vector<Asteroid> asteroids;
std::vector<Asteroid> menu_asteroids;
std::vector<Fragment> fragments;
std::vector<Explosion> explosions;

// Таймеры
int bullet_cooldown = 0;
int missile_cooldown = 0;
int asteroid_spawn_timer = 0;
const int asteroid_spawn_delay = 60;

// Константы
const int world_width = 500;
const int world_height = 500;
const int bullet_speed = 5;
const int bullet_size = 3;
const float missile_base_speed = 2.0;
const float missile_acceleration = 0.05;
const float missile_max_speed = 5.0;
const float missile_turn_rate = 0.1;
const int missile_size = 4;
const float missile_max_turn_angle = 30;
const int explosion_max_size = 25;
const int explosion_lifetime = 30;
const float asteroid_min_speed = 0.5;
const float asteroid_max_speed = 2.0;
const int fragment_lifetime = 40;
const int exhaust_particle_lifetime = 15;
const float missile_explosion_radius = 40.0f; // Радиус урона взрыва ракеты

// Камера
float camera_x = 0;
float camera_y = 0;
const float camera_follow_speed = 0.1;

// Прототипы функций
void update_activity();
float random_float(float min, float max);
void rotate_point(float x, float y, float angle_degrees, float& out_x, float& out_y);
void world_to_screen(float world_x, float world_y, int& screen_x, int& screen_y);
bool is_on_screen(float x, float y, int margin = 50);
void play_game_over_melody();
void create_menu_asteroids();
void update_menu_asteroids();
Asteroid* find_nearest_asteroid(float x, float y);
void create_explosion(float x, float y, float size, bool is_missile_explosion = false);
void create_exhaust_particle();
void update_exhaust_particles();
void btnA_wasClicked();
void btnA_wasHold();
void btnB_wasClicked();
void btnPWR_wasClicked();
void btnPWR_wasHold();
void reset_game();
float update_accelerometer();
void update_ship_physics();
void update_camera();
void draw_world_border();
void draw_background_grid();
void update_missiles();
void update_bullets();
void create_fragments(const Asteroid& asteroid);
void update_explosions();
void spawn_asteroid();
void update_asteroids();
void update_fragments();
void check_missile_collisions();
void check_bullet_collisions();
void check_explosion_collisions();
void check_guardian_collisions();
std::vector<std::pair<float, float>> calculate_ship_vertices();
std::vector<std::pair<float, float>> calculate_missile_vertices(const Missile& missile);
std::vector<std::pair<float, float>> calculate_asteroid_vertices(const Asteroid& asteroid);
std::vector<std::pair<float, float>> calculate_fragment_vertices(const Fragment& fragment);
void draw_game_over();
void draw_menu();
void check_inactivity();
void spawn_fuel_tank();
void update_fuel_tank();
void draw_fuel_gauge();
bool consume_fuel(float amount);
void refill_fuel(float amount);
void create_guardian_asteroids();
void update_guardian_asteroids();

// Вспомогательные функции
void update_activity() {
    last_activity_time = millis();
    if (screen_dimmed) {
        screen_dimmed = false;
        M5.Lcd.setBrightness(100);
    }
}

float random_float(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(max-min)));
}

void rotate_point(float x, float y, float angle_degrees, float& out_x, float& out_y) {
    float angle_radians = angle_degrees * M_PI / 180.0;
    float cos_angle = cos(angle_radians);
    float sin_angle = sin(angle_radians);
    
    out_x = x * cos_angle - y * sin_angle;
    out_y = x * sin_angle + y * cos_angle;
}

void world_to_screen(float world_x, float world_y, int& screen_x, int& screen_y) {
    screen_x = static_cast<int>(world_x - camera_x);
    screen_y = static_cast<int>(world_y - camera_y);
}

bool is_on_screen(float x, float y, int margin) {
    int screen_x, screen_y;
    world_to_screen(x, y, screen_x, screen_y);
    return (screen_x >= -margin && screen_x <= 135 + margin && 
            screen_y >= -margin && screen_y <= 240 + margin);
}

// Функции топливной системы
bool consume_fuel(float amount) {
    if (ship_fuel >= amount) {
        ship_fuel -= amount;
        ship_fuel = std::max(0.0f, ship_fuel);
        return true;
    }
    return false;
}

void refill_fuel(float amount) {
    ship_fuel += amount;
    ship_fuel = std::min(max_fuel, ship_fuel);
    if (Sound) {
        M5.Speaker.tone(1000, 100); // Звук заправки
    }
}

void create_guardian_asteroids() {
    fuel_tank.guardian_asteroids.clear();
    int num_guardians = 4 + rand() % 3; // 4-6 астероидов
    
    for (int i = 0; i < num_guardians; i++) {
        Asteroid asteroid;
        asteroid.orbit_distance = 30 + rand() % 30; // Дистанция от бака
        asteroid.orbit_angle = random_float(0, 360);
        asteroid.size = 4 + rand() % 6; // Меньшие астероиды
        asteroid.rotation = rand() % 360;
        asteroid.rotation_speed = random_float(-3, 3);
        asteroid.is_guardian = true;
        
        // Начальная позиция на орбите
        asteroid.x = fuel_tank.x + cos(asteroid.orbit_angle * M_PI / 180.0) * asteroid.orbit_distance;
        asteroid.y = fuel_tank.y + sin(asteroid.orbit_angle * M_PI / 180.0) * asteroid.orbit_distance;
        
        fuel_tank.guardian_asteroids.push_back(asteroid);
    }
}

void update_guardian_asteroids() {
    for (auto& asteroid : fuel_tank.guardian_asteroids) {
        // Вращение по орбите
        asteroid.orbit_angle += 1.0f; // Скорость вращения
        if (asteroid.orbit_angle > 360) asteroid.orbit_angle -= 360;
        
        // Обновление позиции на орбите
        asteroid.x = fuel_tank.x + cos(asteroid.orbit_angle * M_PI / 180.0) * asteroid.orbit_distance;
        asteroid.y = fuel_tank.y + sin(asteroid.orbit_angle * M_PI / 180.0) * asteroid.orbit_distance;
        
        asteroid.rotation += asteroid.rotation_speed;
        if (asteroid.rotation > 360) asteroid.rotation -= 360;
        else if (asteroid.rotation < 0) asteroid.rotation += 360;
    }
}

void spawn_fuel_tank() {
    fuel_tank.x = random_float(50, world_width - 50);
    fuel_tank.y = random_float(50, world_height - 50);
    fuel_tank.active = true;
    create_guardian_asteroids();
}

void update_fuel_tank() {
    if (!fuel_tank.active) {
        // Спавним новый бак каждые 10 секунд, если его нет
        static unsigned long last_spawn_check = 0;
        if (millis() - last_spawn_check > 10000) {
            spawn_fuel_tank();
            last_spawn_check = millis();
        }
        return;
    }
    
    // Обновляем астероиды-защитники
    update_guardian_asteroids();
    
    // Проверяем столкновение с кораблем
    float dx = fuel_tank.x - ship_x;
    float dy = fuel_tank.y - ship_y;
    float distance = sqrt(dx*dx + dy*dy);
    
    if (distance < (fuel_tank.size + ship_size)) {
        refill_fuel(fuel_refill_amount);
        fuel_tank.active = false;
        fuel_tank.guardian_asteroids.clear();
    }
}

void draw_fuel_gauge() {
    // Рисуем индикатор топлива в левом верхнем углу
    int gauge_width = 40;
    int gauge_height = 6;
    int gauge_x = 5;
    int gauge_y = 5;
    
    // Фон индикатора
    M5.Lcd.drawRect(gauge_x, gauge_y, gauge_width, gauge_height, 0x666666);
    
    // Заполненная часть
    float fuel_percent = ship_fuel / max_fuel;
    int fill_width = static_cast<int>(gauge_width * fuel_percent);
    
    uint32_t fuel_color;
    if (fuel_percent > 0.6f) {
        fuel_color = 0x00FF00; // Зеленый
    } else if (fuel_percent > 0.3f) {
        fuel_color = 0xFFFF00; // Желтый
    } else {
        fuel_color = 0xFF0000; // Красный
    }
    
    if (fill_width > 0) {
        M5.Lcd.fillRect(gauge_x, gauge_y, fill_width, gauge_height, fuel_color);
    }
    
    // Мигание при критическом уровне топлива
    if (fuel_percent < 0.2f && (millis() / 500) % 2 == 0) {
        M5.Lcd.drawRect(gauge_x, gauge_y, gauge_width, gauge_height, 0xFF0000);
    }
}

// Функции игры
void play_game_over_melody() {
    if (Sound) {
        M5.Speaker.tone(300, 300);
        delay(350);
        M5.Speaker.tone(250, 400);
        delay(450);
        M5.Speaker.tone(200, 500);
    }
}

void create_menu_asteroids() {
    menu_asteroids.clear();
    for (int i = 0; i < 8; i++) {
        Asteroid asteroid;
        asteroid.x = rand() % 135;
        asteroid.y = rand() % 240;
        asteroid.size = 10 + rand() % 15;
        asteroid.rotation = rand() % 360;
        asteroid.rotation_speed = random_float(-1, 1);
        menu_asteroids.push_back(asteroid);
    }
}

void update_menu_asteroids() {
    for (auto& asteroid : menu_asteroids) {
        asteroid.rotation += asteroid.rotation_speed;
        if (asteroid.rotation > 360) asteroid.rotation -= 360;
        else if (asteroid.rotation < 0) asteroid.rotation += 360;
    }
}

Asteroid* find_nearest_asteroid(float x, float y) {
    Asteroid* nearest = nullptr;
    float min_distance = std::numeric_limits<float>::max();
    
    // Ищем среди обычных астероидов
    for (auto& asteroid : asteroids) {
        float dx = asteroid.x - x;
        float dy = asteroid.y - y;
        float distance = sqrt(dx*dx + dy*dy);
        
        if (distance < min_distance) {
            min_distance = distance;
            nearest = &asteroid;
        }
    }
    
    // Ищем среди астероидов-защитников
    for (auto& asteroid : fuel_tank.guardian_asteroids) {
        float dx = asteroid.x - x;
        float dy = asteroid.y - y;
        float distance = sqrt(dx*dx + dy*dy);
        
        if (distance < min_distance) {
            min_distance = distance;
            nearest = &asteroid;
        }
    }
    
    return nearest;
}

void create_explosion(float x, float y, float size, bool is_missile_explosion) {
    Explosion explosion;
    explosion.x = x;
    explosion.y = y;
    explosion.size = size;
    explosion.max_size = size;
    explosion.life = explosion_lifetime;
    explosion.damage_radius = is_missile_explosion ? missile_explosion_radius : 0;
    explosions.push_back(explosion);
}

void create_exhaust_particle() {
    if (!ship_moving) return;
    
    float direction_x = cos((ship_rotation - 90) * M_PI / 180.0);
    float direction_y = sin((ship_rotation - 90) * M_PI / 180.0);
    
    float base_x = 0;
    float base_y = ship_size * 0.8;
    
    float rotated_x, rotated_y;
    rotate_point(base_x, base_y, ship_rotation, rotated_x, rotated_y);
    float particle_x = ship_x + rotated_x;
    float particle_y = ship_y + rotated_y;
    
    float speed = random_float(1.0, 2.5);
    float particle_vx = -direction_x * speed + random_float(-0.3, 0.3);
    float particle_vy = -direction_y * speed + random_float(-0.3, 0.3);
    
    if (random_float(0, 1) < 0.5) {
        Particle particle;
        particle.x = particle_x;
        particle.y = particle_y;
        particle.vx = particle_vx;
        particle.vy = particle_vy;
        particle.size = 1 + rand() % 2;
        particle.life = exhaust_particle_lifetime;
        particle.color = 0x888888;
        exhaust_particles.push_back(particle);
    }
}

void update_exhaust_particles() {
    std::vector<Particle> new_particles;
    
    for (auto& particle : exhaust_particles) {
        particle.x += particle.vx;
        particle.y += particle.vy;
        particle.life--;
        
        if (particle.life > 0) {
            new_particles.push_back(particle);
        }
    }
    
    exhaust_particles = new_particles;
}

void btnA_wasClicked() {
    update_activity();
    if (in_menu) {
        in_menu = false;
        reset_game();
    } else if (!game_over && bullet_cooldown <= 0) {
        // Проверяем топливо для выстрела
        if (!consume_fuel(fuel_consumption_bullet)) {
            if (Sound) {
                M5.Speaker.tone(200, 100); // Звук отсутствия топлива
            }
            return;
        }
        
        // Расчет вершин корабля для пули
        std::vector<std::pair<float, float>> ship_vertices = calculate_ship_vertices();
        float front_x = ship_vertices[0].first;
        float front_y = ship_vertices[0].second;
        
        float direction_x = cos((ship_rotation - 90) * M_PI / 180.0);
        float direction_y = sin((ship_rotation - 90) * M_PI / 180.0);
        
        Bullet bullet;
        bullet.x = front_x;
        bullet.y = front_y;
        bullet.vx = direction_x * bullet_speed;
        bullet.vy = direction_y * bullet_speed;
        bullet.life = 100;
        bullets.push_back(bullet);
        
        bullet_cooldown = 10;
        if (Sound) {
            M5.Speaker.tone(3000, 30);
        }
    }
}

void btnA_wasHold() {
    update_activity();
    if (!game_over && missile_cooldown <= 0) {
        // Проверяем топливо для ракеты
        if (!consume_fuel(fuel_consumption_missile)) {
            if (Sound) {
                M5.Speaker.tone(200, 100); // Звук отсутствия топлива
            }
            return;
        }
        
        std::vector<std::pair<float, float>> ship_vertices = calculate_ship_vertices();
        float front_x = ship_vertices[0].first;
        float front_y = ship_vertices[0].second;
        
        float direction_x = cos((ship_rotation - 90) * M_PI / 180.0);
        float direction_y = sin((ship_rotation - 90) * M_PI / 180.0);
        
        Missile missile;
        missile.x = front_x;
        missile.y = front_y;
        missile.vx = direction_x * missile_base_speed;
        missile.vy = direction_y * missile_base_speed;
        missile.rotation = ship_rotation;
        missile.speed = missile_base_speed;
        missile.life = 200;
        missiles.push_back(missile);
        
        missile_cooldown = 60;
        if (Sound) {
            M5.Speaker.tone(1500, 100);
        }
    }
}

void btnB_wasClicked() {
    update_activity();
    if (game_over) {
        in_menu = true;
        reset_game();
    }
}

void btnPWR_wasClicked() {
    update_activity();
    if (!in_menu && !game_over) {
        ship_moving = !ship_moving;
        if (Sound) {
            if (ship_moving) {
                M5.Speaker.tone(1200, 80);
            } else {
                M5.Speaker.tone(800, 80);
            }
        }
    }
}

void btnPWR_wasHold() {
    update_activity();
    if (!Sound) {
        Sound = true;
        M5.Speaker.tone(2000, 100);
    } else {
        M5.Speaker.tone(3000, 100);
        Sound = false;
    }
}

void reset_game() {
    update_activity();
    game_over = false;
    game_over_played = false;
    asteroids.clear();
    bullets.clear();
    missiles.clear();
    fragments.clear();
    explosions.clear();
    exhaust_particles.clear();
    ship_rotation = 0;
    ship_speed_x = 0;
    ship_speed_y = 0;
    ship_moving = false;
    ship_fuel = max_fuel;
    asteroid_spawn_timer = 30;
    
    fuel_tank.active = false;
    fuel_tank.guardian_asteroids.clear();
    
    ship_x = world_width / 2;
    ship_y = world_height / 2;
    camera_x = ship_x - 67;
    camera_y = ship_y - 120;
}

float update_accelerometer() {
    float ax, ay, az;
    if (M5.Imu.getAccel(&ax, &ay, &az)) {
        float rotation_control = ax * 3.0;
        if (abs(rotation_control) > 0.1) {
            update_activity();
        }
        return rotation_control;
    }
    return 0;
}

void update_ship_physics() {
    float rotation_control = update_accelerometer();
    ship_rotation_speed = rotation_control * 2.0;
    ship_rotation_speed = std::max(-4.0f, std::min(4.0f, ship_rotation_speed));
    ship_rotation += ship_rotation_speed;
    
    if (ship_rotation > 360) ship_rotation -= 360;
    else if (ship_rotation < 0) ship_rotation += 360;
    
    if (ship_moving) {
        // Проверяем топливо для движения
        if (!consume_fuel(fuel_consumption_move)) {
            ship_moving = false; // Автоматически выключаем двигатель при отсутствии топлива
            return;
        }
        
        float direction_x = cos((ship_rotation - 90) * M_PI / 180.0);
        float direction_y = sin((ship_rotation - 90) * M_PI / 180.0);
        
        ship_speed_x += direction_x * ship_acceleration;
        ship_speed_y += direction_y * ship_acceleration;
        
        float speed = sqrt(ship_speed_x*ship_speed_x + ship_speed_y*ship_speed_y);
        if (speed > ship_max_speed) {
            ship_speed_x = ship_speed_x / speed * ship_max_speed;
            ship_speed_y = ship_speed_y / speed * ship_max_speed;
        }
        
        create_exhaust_particle();
    }
    
    ship_speed_x *= ship_friction;
    ship_speed_y *= ship_friction;
    
    ship_x += ship_speed_x;
    ship_y += ship_speed_y;
    
    ship_x = std::max(ship_size, std::min(world_width - ship_size, ship_x));
    ship_y = std::max(ship_size, std::min(world_height - ship_size, ship_y));
}

void update_camera() {
    float target_x = ship_x - 67;
    float target_y = ship_y - 120;
    
    camera_x += (target_x - camera_x) * camera_follow_speed;
    camera_y += (target_y - camera_y) * camera_follow_speed;
    
    camera_x = std::max(0.0f, std::min(world_width - 135.0f, camera_x));
    camera_y = std::max(0.0f, std::min(world_height - 240.0f, camera_y));
}

void draw_world_border() {
    const int border_margin = 20;
    bool near_left = ship_x < 100;
    bool near_right = ship_x > world_width - 100;
    bool near_top = ship_y < 100;
    bool near_bottom = ship_y > world_height - 100;
    
    uint32_t color = 0xFFFF00;
    if (ship_x < 50 || ship_x > world_width - 50 || ship_y < 50 || ship_y > world_height - 50) {
        color = 0xFF0000;
    }
    
    if (near_left) {
        for (int i = 0; i < 5; i++) {
            M5.Lcd.fillRect(2, 40 + i * 40, 4, 20, color);
        }
    }
    if (near_right) {
        for (int i = 0; i < 5; i++) {
            M5.Lcd.fillRect(129, 40 + i * 40, 4, 20, color);
        }
    }
    if (near_top) {
        for (int i = 0; i < 5; i++) {
            M5.Lcd.fillRect(40 + i * 40, 2, 20, 4, color);
        }
    }
    if (near_bottom) {
        for (int i = 0; i < 5; i++) {
            M5.Lcd.fillRect(40 + i * 40, 234, 20, 4, color);
        }
    }
}

void draw_background_grid() {
    const int grid_size = 40;
    const uint32_t grid_color = 0x222222;
    
    int offset_x = static_cast<int>(camera_x * 0.3) % grid_size;
    int offset_y = static_cast<int>(camera_y * 0.3) % grid_size;
    
    for (int x = -offset_x; x < 135; x += grid_size) {
        M5.Lcd.drawLine(x, 0, x, 240, grid_color);
    }
    
    for (int y = -offset_y; y < 240; y += grid_size) {
        M5.Lcd.drawLine(0, y, 135, y, grid_color);
    }
}

void update_missiles() {
    if (missile_cooldown > 0) {
        missile_cooldown--;
    }
    
    std::vector<Missile> new_missiles;
    for (auto& missile : missiles) {
        missile.trail.push_back(std::make_pair(missile.x, missile.y));
        if (missile.trail.size() > 5) {
            missile.trail.erase(missile.trail.begin());
        }
        
        if (missile.speed < missile_max_speed) {
            missile.speed += missile_acceleration;
        }
        
        Asteroid* target = find_nearest_asteroid(missile.x, missile.y);
        
        if (target) {  // Добавляем проверку на nullptr
            float dx = target->x - missile.x;
            float dy = target->y - missile.y;
            
            float desired_angle = atan2(dy, dx) * 180.0 / M_PI + 90;
            if (desired_angle < 0) desired_angle += 360;
            
            float current_angle = missile.rotation;
            float angle_diff = desired_angle - current_angle;
            
            if (angle_diff > 180) angle_diff -= 360;
            else if (angle_diff < -180) angle_diff += 360;
            
            float max_turn = missile_max_turn_angle;
            float turn_amount = std::max(-max_turn, std::min(max_turn, angle_diff * missile_turn_rate));
            
            missile.rotation += turn_amount;
            if (missile.rotation > 360) missile.rotation -= 360;
            else if (missile.rotation < 0) missile.rotation += 360;
        }
        
        float direction_x = cos((missile.rotation - 90) * M_PI / 180.0);
        float direction_y = sin((missile.rotation - 90) * M_PI / 180.0);
        
        missile.vx = missile.vx * 0.7 + direction_x * missile.speed * 0.3;
        missile.vy = missile.vy * 0.7 + direction_y * missile.speed * 0.3;
        
        missile.x += missile.vx;
        missile.y += missile.vy;
        missile.life--;
        
        if (missile.x >= 0 && missile.x <= world_width && missile.y >= 0 && missile.y <= world_height && 
            missile.life > 0 && is_on_screen(missile.x, missile.y, 100)) {
            new_missiles.push_back(missile);
        }
    }
    
    missiles = new_missiles;
}

void update_bullets() {
    if (bullet_cooldown > 0) {
        bullet_cooldown--;
    }
    
    std::vector<Bullet> new_bullets;
    for (auto& bullet : bullets) {
        bullet.x += bullet.vx;
        bullet.y += bullet.vy;
        bullet.life--;
        
        if (bullet.x >= 0 && bullet.x <= world_width && bullet.y >= 0 && bullet.y <= world_height && 
            bullet.life > 0 && is_on_screen(bullet.x, bullet.y, 50)) {
            new_bullets.push_back(bullet);
        }
    }
    
    bullets = new_bullets;
}

void create_fragments(const Asteroid& asteroid) {
    int num_fragments = 4;
    int base_size = std::max(3, asteroid.size / 3);
    
    for (int i = 0; i < num_fragments; i++) {
        float angle = random_float(0, 360);
        float speed = random_float(1.0, 3.0);
        
        Fragment fragment;
        fragment.x = asteroid.x;
        fragment.y = asteroid.y;
        fragment.vx = cos(angle * M_PI / 180.0) * speed + asteroid.vx * 0.5;
        fragment.vy = sin(angle * M_PI / 180.0) * speed + asteroid.vy * 0.5;
        fragment.size = base_size + rand() % 3;
        fragment.rotation = random_float(0, 360);
        fragment.rotation_speed = random_float(-5, 5);
        fragment.life = fragment_lifetime;
        fragment.color = 0x666666;
        
        fragments.push_back(fragment);
    }
}

void update_explosions() {
    std::vector<Explosion> new_explosions;
    for (auto& explosion : explosions) {
        explosion.life--;
        if (explosion.life > 0 && is_on_screen(explosion.x, explosion.y, 50)) {
            float progress = static_cast<float>(explosion.life) / explosion_lifetime;
            explosion.size = explosion.max_size * progress;
            new_explosions.push_back(explosion);
        }
    }
    explosions = new_explosions;
}

void check_explosion_collisions() {
    for (auto& explosion : explosions) {
        if (explosion.damage_radius > 0) {
            // Проверяем столкновение с обычными астероидами
            std::vector<Asteroid> new_asteroids;
            for (auto& asteroid : asteroids) {
                float dx = asteroid.x - explosion.x;
                float dy = asteroid.y - explosion.y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < (asteroid.size + explosion.damage_radius)) {
                    create_fragments(asteroid);
                } else {
                    new_asteroids.push_back(asteroid);
                }
            }
            asteroids = new_asteroids;
            
            // Проверяем столкновение с астероидами-защитниками
            std::vector<Asteroid> new_guardians;
            for (auto& asteroid : fuel_tank.guardian_asteroids) {
                float dx = asteroid.x - explosion.x;
                float dy = asteroid.y - explosion.y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < (asteroid.size + explosion.damage_radius)) {
                    create_fragments(asteroid);
                } else {
                    new_guardians.push_back(asteroid);
                }
            }
            fuel_tank.guardian_asteroids = new_guardians;
        }
    }
}

void check_guardian_collisions() {
    // Проверяем столкновение корабля с астероидами-защитниками
    for (auto& asteroid : fuel_tank.guardian_asteroids) {
        float dx = asteroid.x - ship_x;
        float dy = asteroid.y - ship_y;
        float distance = sqrt(dx*dx + dy*dy);
        
        if (distance < (asteroid.size + ship_size)) {
            game_over = true;
            if (Sound && !game_over_played) {
                play_game_over_melody();
                game_over_played = true;
            }
            return;
        }
    }
    
    // Проверяем столкновение пуль с астероидами-защитниками
    std::vector<Bullet> new_bullets;
    for (auto& bullet : bullets) {
        bool keep_bullet = true;
        for (auto& asteroid : fuel_tank.guardian_asteroids) {
            float dx = asteroid.x - bullet.x;
            float dy = asteroid.y - bullet.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + bullet_size)) {
                keep_bullet = false;
                create_fragments(asteroid);
                break;
            }
        }
        if (keep_bullet) {
            new_bullets.push_back(bullet);
        }
    }
    bullets = new_bullets;
}

void spawn_asteroid() {
    int side = rand() % 4;
    int margin = 50;
    
    float x, y, vx, vy;
    
    switch (side) {
        case 0: // Сверху
            x = random_float(camera_x - margin, camera_x + 135 + margin);
            y = camera_y - margin;
            vx = random_float(-1, 1);
            vy = random_float(asteroid_min_speed, asteroid_max_speed);
            break;
        case 1: // Справа
            x = camera_x + 135 + margin;
            y = random_float(camera_y - margin, camera_y + 240 + margin);
            vx = -random_float(asteroid_min_speed, asteroid_max_speed);
            vy = random_float(-1, 1);
            break;
        case 2: // Снизу
            x = random_float(camera_x - margin, camera_x + 135 + margin);
            y = camera_y + 240 + margin;
            vx = random_float(-1, 1);
            vy = -random_float(asteroid_min_speed, asteroid_max_speed);
            break;
        case 3: // Слева
            x = camera_x - margin;
            y = random_float(camera_y - margin, camera_y + 240 + margin);
            vx = random_float(asteroid_min_speed, asteroid_max_speed);
            vy = random_float(-1, 1);
            break;
    }
    
    x = std::max(0.0f, std::min(static_cast<float>(world_width), x));
    y = std::max(0.0f, std::min(static_cast<float>(world_height), y));
    
    Asteroid asteroid;
    asteroid.x = x;
    asteroid.y = y;
    asteroid.vx = vx;
    asteroid.vy = vy;
    asteroid.size = 8 + rand() % 12;
    asteroid.rotation = rand() % 360;
    asteroid.rotation_speed = random_float(-2, 2);
    asteroid.is_guardian = false;
    
    asteroids.push_back(asteroid);
}

void update_asteroids() {
    asteroid_spawn_timer--;
    if (asteroid_spawn_timer <= 0 && asteroids.size() < 8) {
        spawn_asteroid();
        asteroid_spawn_timer = asteroid_spawn_delay;
    }
    
    std::vector<Asteroid> new_asteroids;
    for (auto& asteroid : asteroids) {
        asteroid.x += asteroid.vx;
        asteroid.y += asteroid.vy;
        asteroid.rotation += asteroid.rotation_speed;
        
        float dx = asteroid.x - ship_x;
        float dy = asteroid.y - ship_y;
        float distance = sqrt(dx*dx + dy*dy);
        
        if (distance < (asteroid.size + ship_size)) {
            game_over = true;
            if (Sound && !game_over_played) {
                play_game_over_melody();
                game_over_played = true;
            }
        }
        
        if (is_on_screen(asteroid.x, asteroid.y, 100)) {
            new_asteroids.push_back(asteroid);
        }
    }
    
    asteroids = new_asteroids;
}

void update_fragments() {
    std::vector<Fragment> new_fragments;
    for (auto& fragment : fragments) {
        fragment.x += fragment.vx;
        fragment.y += fragment.vy;
        fragment.rotation += fragment.rotation_speed;
        fragment.life--;
        
        if (fragment.life > fragment_lifetime * 0.5) {
            fragment.color = 0x888888;
        } else if (fragment.life > fragment_lifetime * 0.25) {
            fragment.color = 0x666666;
        } else {
            fragment.color = 0x444444;
        }
        
        if (fragment.life > 0 && is_on_screen(fragment.x, fragment.y, 50)) {
            new_fragments.push_back(fragment);
        }
    }
    fragments = new_fragments;
}

void check_missile_collisions() {
    std::vector<Asteroid> new_asteroids;
    std::vector<Missile> new_missiles;
    bool missile_hit = false;
    
    for (auto& asteroid : asteroids) {
        bool hit = false;
        for (auto& missile : missiles) {
            float dx = asteroid.x - missile.x;
            float dy = asteroid.y - missile.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + missile_size)) {
                hit = true;
                missile_hit = true;
                create_fragments(asteroid);
                create_explosion(missile.x, missile.y, explosion_max_size, true);
                break;
            }
        }
        if (!hit) {
            new_asteroids.push_back(asteroid);
        }
    }
    
    // Проверяем столкновение ракет с астероидами-защитниками
    std::vector<Asteroid> new_guardians;
    for (auto& asteroid : fuel_tank.guardian_asteroids) {
        bool hit = false;
        for (auto& missile : missiles) {
            float dx = asteroid.x - missile.x;
            float dy = asteroid.y - missile.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + missile_size)) {
                hit = true;
                missile_hit = true;
                create_fragments(asteroid);
                create_explosion(missile.x, missile.y, explosion_max_size, true);
                break;
            }
        }
        if (!hit) {
            new_guardians.push_back(asteroid);
        }
    }
    fuel_tank.guardian_asteroids = new_guardians;
    
    for (auto& missile : missiles) {
        bool keep_missile = true;
        for (auto& asteroid : asteroids) {
            float dx = asteroid.x - missile.x;
            float dy = asteroid.y - missile.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + missile_size)) {
                keep_missile = false;
                break;
            }
        }
        if (keep_missile) {
            for (auto& asteroid : fuel_tank.guardian_asteroids) {
                float dx = asteroid.x - missile.x;
                float dy = asteroid.y - missile.y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < (asteroid.size + missile_size)) {
                    keep_missile = false;
                    break;
                }
            }
        }
        if (keep_missile) {
            new_missiles.push_back(missile);
        }
    }
    
    asteroids = new_asteroids;
    missiles = new_missiles;
    
    if (missile_hit && Sound) {
        M5.Speaker.tone(800, 100);
    }
}

void check_bullet_collisions() {
    std::vector<Asteroid> new_asteroids;
    std::vector<Bullet> new_bullets;
    bool bullet_hit = false;
    
    for (auto& asteroid : asteroids) {
        bool hit = false;
        for (auto& bullet : bullets) {
            float dx = asteroid.x - bullet.x;
            float dy = asteroid.y - bullet.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + bullet_size)) {
                hit = true;
                bullet_hit = true;
                create_fragments(asteroid);
                break;
            }
        }
        if (!hit) {
            new_asteroids.push_back(asteroid);
        }
    }
    
    for (auto& bullet : bullets) {
        bool keep_bullet = true;
        for (auto& asteroid : asteroids) {
            float dx = asteroid.x - bullet.x;
            float dy = asteroid.y - bullet.y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < (asteroid.size + bullet_size)) {
                keep_bullet = false;
                break;
            }
        }
        if (keep_bullet) {
            new_bullets.push_back(bullet);
        }
    }
    
    asteroids = new_asteroids;
    bullets = new_bullets;
    
    if (bullet_hit && Sound) {
        M5.Speaker.tone(2000, 50);
    }
}

std::vector<std::pair<float, float>> calculate_ship_vertices() {
    std::vector<std::pair<float, float>> base_vertices = {
        {0, -ship_size},
        {-ship_size * 0.7, ship_size * 0.7},
        {ship_size * 0.7, ship_size * 0.7}
    };
    
    std::vector<std::pair<float, float>> rotated_vertices;
    for (auto& vertex : base_vertices) {
        float rotated_x, rotated_y;
        rotate_point(vertex.first, vertex.second, ship_rotation, rotated_x, rotated_y);
        rotated_vertices.push_back({ship_x + rotated_x, ship_y + rotated_y});
    }
    
    return rotated_vertices;
}

std::vector<std::pair<float, float>> calculate_missile_vertices(const Missile& missile) {
    std::vector<std::pair<float, float>> base_vertices = {
        {0, -missile_size},
        {-missile_size * 0.5, missile_size * 0.5},
        {missile_size * 0.5, missile_size * 0.5}
    };
    
    std::vector<std::pair<float, float>> rotated_vertices;
    for (auto& vertex : base_vertices) {
        float rotated_x, rotated_y;
        rotate_point(vertex.first, vertex.second, missile.rotation, rotated_x, rotated_y);
        rotated_vertices.push_back({missile.x + rotated_x, missile.y + rotated_y});
    }
    
    return rotated_vertices;
}

std::vector<std::pair<float, float>> calculate_asteroid_vertices(const Asteroid& asteroid) {
    std::vector<std::pair<float, float>> vertices;
    float size = asteroid.size;
    float rotation = asteroid.rotation;
    
    for (int i = 0; i < 6; i++) {
        float angle = i * 60;
        float irregularity = 0.8 + (i * 0.05);
        float x = cos(angle * M_PI / 180.0) * size * irregularity;
        float y = sin(angle * M_PI / 180.0) * size * irregularity;
        
        float rotated_x, rotated_y;
        rotate_point(x, y, rotation, rotated_x, rotated_y);
        vertices.push_back({asteroid.x + rotated_x, asteroid.y + rotated_y});
    }
    
    return vertices;
}

std::vector<std::pair<float, float>> calculate_fragment_vertices(const Fragment& fragment) {
    std::vector<std::pair<float, float>> vertices;
    float size = fragment.size;
    float rotation = fragment.rotation;
    
    for (int i = 0; i < 3; i++) {
        float angle = i * 120;
        float x = cos(angle * M_PI / 180.0) * size;
        float y = sin(angle * M_PI / 180.0) * size;
        
        float rotated_x, rotated_y;
        rotate_point(x, y, rotation, rotated_x, rotated_y);
        vertices.push_back({fragment.x + rotated_x, fragment.y + rotated_y});
    }
    
    return vertices;
}

void draw_game_over() {
    int center_x, center_y;
    world_to_screen(ship_x, ship_y, center_x, center_y);
    
    M5.Lcd.drawLine(center_x - 30, center_y - 30, center_x + 30, center_y + 30, 0xFF0000);
    M5.Lcd.drawLine(center_x + 30, center_y - 30, center_x - 30, center_y + 30, 0xFF0000);
    M5.Lcd.drawCircle(center_x, center_y, 40, 0xFF0000);
    M5.Lcd.fillCircle(center_x, center_y + 80, 6, 0x00FF00);
    M5.Lcd.drawCircle(center_x, center_y + 80, 8, 0x00FF00);
}

void draw_menu() {
    for (auto& asteroid : menu_asteroids) {
        auto vertices = calculate_asteroid_vertices(asteroid);
        for (size_t i = 0; i < vertices.size(); i++) {
            auto start_vertex = vertices[i];
            auto end_vertex = vertices[(i + 1) % vertices.size()];
            int start_x, start_y, end_x, end_y;
            world_to_screen(start_vertex.first, start_vertex.second, start_x, start_y);
            world_to_screen(end_vertex.first, end_vertex.second, end_x, end_y);
            M5.Lcd.drawLine(start_x, start_y, end_x, end_y, 0x444444);
        }
    }
    
    int menu_ship_x = 67;
    int menu_ship_y = 60;
    int menu_ship_size = 20;
    
    M5.Lcd.drawTriangle(
        menu_ship_x, menu_ship_y - menu_ship_size,
        menu_ship_x - menu_ship_size * 0.7, menu_ship_y + menu_ship_size * 0.7,
        menu_ship_x + menu_ship_size * 0.7, menu_ship_y + menu_ship_size * 0.7,
        0x00FF00
    );
    
    for (int i = 0; i < 8; i++) {
        float angle = i * 45;
        int ray_length = 30;
        int end_x = menu_ship_x + cos(angle * M_PI / 180.0) * ray_length;
        int end_y = menu_ship_y + sin(angle * M_PI / 180.0) * ray_length;
        M5.Lcd.drawLine(menu_ship_x, menu_ship_y, end_x, end_y, 0x00FF00);
    }
    
    // Упрощенное меню - только кнопка "Играть"
    int menu_y = 120;
    
    M5.Lcd.fillTriangle(40, menu_y, 50, menu_y + 15, 30, menu_y + 15, 0x00FF00);
    
    M5.Lcd.fillCircle(100, menu_y, 8, 0xFF0000);
    M5.Lcd.drawLine(110, menu_y, 120, menu_y, 0xFFFFFF);
    M5.Lcd.drawLine(117, menu_y - 3, 120, menu_y, 0xFFFFFF);
    M5.Lcd.drawLine(117, menu_y + 3, 120, menu_y, 0xFFFFFF);
    
    // Текст "PLAY"
    M5.Lcd.setCursor(50, 160);
    M5.Lcd.setTextColor(0xFFFFFF);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("PLAY");
}

void check_inactivity() {
    unsigned long current_time = millis();
    if (current_time - last_activity_time > inactivity_timeout && !screen_dimmed) {
        M5.Lcd.setBrightness(0);
        screen_dimmed = true;
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(0x000000);
    
    // Начальная позиция
    ship_x = world_width / 2;
    ship_y = world_height / 2;
    camera_x = ship_x - 67;
    camera_y = ship_y - 120;
    
    game_over = false;
    game_over_played = false;
    Sound = false;
    asteroid_spawn_timer = 30;
    in_menu = true;
    ship_fuel = max_fuel;
    
    last_activity_time = millis();
    screen_dimmed = false;
    
    create_menu_asteroids();
    
    // Инициализация случайных чисел
    srand(millis());
}

void loop() {
    M5.update();
    delay(30);
    
    // Обработка кнопок в C++ стиле
    if (M5.BtnA.wasPressed()) {
        btnA_wasClicked();
    }
    if (M5.BtnA.pressedFor(1000)) {
        btnA_wasHold();
    }
    if (M5.BtnB.wasPressed()) {
        btnB_wasClicked();
    }
    if (M5.BtnPWR.wasPressed()) {
        btnPWR_wasClicked();
    }
    if (M5.BtnPWR.pressedFor(1000)) {
        btnPWR_wasHold();
    }
    
    check_inactivity();
    
    if (screen_dimmed) {
        return;
    }
    
    M5.Lcd.fillScreen(0x000000);
    
    if (in_menu) {
        update_menu_asteroids();
        draw_menu();
    } else if (!game_over) {
        update_ship_physics();
        update_camera();
        update_bullets();
        update_missiles();
        update_asteroids();
        update_fragments();
        update_explosions();
        update_exhaust_particles();
        update_fuel_tank();
        check_bullet_collisions();
        check_missile_collisions();
        check_explosion_collisions();
        check_guardian_collisions(); // Добавляем проверку столкновений с защитниками
        
        draw_background_grid();
        
        // Рисуем корабль
        auto ship_vertices = calculate_ship_vertices();
        int screen_vertices[3][2];
        for (int i = 0; i < 3; i++) {
            world_to_screen(ship_vertices[i].first, ship_vertices[i].second, screen_vertices[i][0], screen_vertices[i][1]);
        }
        
        M5.Lcd.drawTriangle(
            screen_vertices[0][0], screen_vertices[0][1],
            screen_vertices[1][0], screen_vertices[1][1],
            screen_vertices[2][0], screen_vertices[2][1],
            0x00FF00
        );
        
        // Рисуем частицы выхлопа
        for (auto& particle : exhaust_particles) {
            int screen_x, screen_y;
            world_to_screen(particle.x, particle.y, screen_x, screen_y);
            M5.Lcd.fillCircle(screen_x, screen_y, particle.size, particle.color);
        }
        
        // Рисуем снаряды
        for (auto& bullet : bullets) {
            int screen_x, screen_y;
            world_to_screen(bullet.x, bullet.y, screen_x, screen_y);
            M5.Lcd.fillCircle(screen_x, screen_y, bullet_size, 0xFFFFFF);
        }
        
        // Рисуем ракеты
        for (auto& missile : missiles) {
            for (size_t i = 0; i < missile.trail.size(); i++) {
                int screen_x, screen_y;
                world_to_screen(missile.trail[i].first, missile.trail[i].second, screen_x, screen_y);
                int alpha = 255 * (i / missile.trail.size());
                uint32_t color = (alpha << 16) | (alpha << 8) | alpha;
                M5.Lcd.fillCircle(screen_x, screen_y, 1, color);
            }
            
            auto missile_vertices = calculate_missile_vertices(missile);
            int screen_vertices[3][2];
            for (int i = 0; i < 3; i++) {
                world_to_screen(missile_vertices[i].first, missile_vertices[i].second, screen_vertices[i][0], screen_vertices[i][1]);
            }
            
            M5.Lcd.drawTriangle(
                screen_vertices[0][0], screen_vertices[0][1],
                screen_vertices[1][0], screen_vertices[1][1],
                screen_vertices[2][0], screen_vertices[2][1],
                0xFFA500
            );
        }
        
        // Рисуем астероиды
        for (auto& asteroid : asteroids) {
            auto asteroid_vertices = calculate_asteroid_vertices(asteroid);
            for (size_t i = 0; i < asteroid_vertices.size(); i++) {
                int start_x, start_y, end_x, end_y;
                world_to_screen(asteroid_vertices[i].first, asteroid_vertices[i].second, start_x, start_y);
                world_to_screen(asteroid_vertices[(i + 1) % asteroid_vertices.size()].first, 
                              asteroid_vertices[(i + 1) % asteroid_vertices.size()].second, end_x, end_y);
                M5.Lcd.drawLine(start_x, start_y, end_x, end_y, 0x888888);
            }
        }
        
        // Рисуем астероиды-защитники (серым цветом)
        for (auto& asteroid : fuel_tank.guardian_asteroids) {
            auto asteroid_vertices = calculate_asteroid_vertices(asteroid);
            for (size_t i = 0; i < asteroid_vertices.size(); i++) {
                int start_x, start_y, end_x, end_y;
                world_to_screen(asteroid_vertices[i].first, asteroid_vertices[i].second, start_x, start_y);
                world_to_screen(asteroid_vertices[(i + 1) % asteroid_vertices.size()].first, 
                              asteroid_vertices[(i + 1) % asteroid_vertices.size()].second, end_x, end_y);
                M5.Lcd.drawLine(start_x, start_y, end_x, end_y, 0x666666); // Серый цвет для защитников
            }
        }
        
        // Рисуем осколки
        for (auto& fragment : fragments) {
            auto fragment_vertices = calculate_fragment_vertices(fragment);
            int screen_vertices[3][2];
            for (int i = 0; i < 3; i++) {
                world_to_screen(fragment_vertices[i].first, fragment_vertices[i].second, screen_vertices[i][0], screen_vertices[i][1]);
            }
            
            M5.Lcd.drawTriangle(
                screen_vertices[0][0], screen_vertices[0][1],
                screen_vertices[1][0], screen_vertices[1][1],
                screen_vertices[2][0], screen_vertices[2][1],
                fragment.color
            );
        }
        
        // Рисуем взрывы
        for (auto& explosion : explosions) {
            int screen_x, screen_y;
            world_to_screen(explosion.x, explosion.y, screen_x, screen_y);
            uint32_t color;
            if (explosion.life > explosion_lifetime * 0.7) {
                color = 0xFFFF00;
            } else if (explosion.life > explosion_lifetime * 0.4) {
                color = 0xFF8800;
            } else {
                color = 0xFF0000;
            }
            M5.Lcd.drawCircle(screen_x, screen_y, explosion.size, color);
            
            // Рисуем радиус урона для взрывов ракет
            if (explosion.damage_radius > 0) {
                M5.Lcd.drawCircle(screen_x, screen_y, explosion.damage_radius, 0xFF0000);
            }
        }
        
        // Рисуем топливный бак
        if (fuel_tank.active) {
            int screen_x, screen_y;
            world_to_screen(fuel_tank.x, fuel_tank.y, screen_x, screen_y);
            
            // Анимированное мигание
            if ((millis() / 300) % 2 == 0) {
                M5.Lcd.fillRect(screen_x - fuel_tank.size/2, screen_y - fuel_tank.size/2, 
                               fuel_tank.size, fuel_tank.size, 0x00AAFF);
            } else {
                M5.Lcd.drawRect(screen_x - fuel_tank.size/2, screen_y - fuel_tank.size/2, 
                               fuel_tank.size, fuel_tank.size, 0x00AAFF);
            }
        }
        
        draw_world_border();
        draw_fuel_gauge();
        
        // Индикатор двигателя
        uint32_t engine_color = ship_moving ? 0x00FF00 : 0x666666;
        M5.Lcd.fillCircle(120, 220, 4, engine_color);
        M5.Lcd.drawCircle(120, 220, 6, 0xFFFFFF);
        
    } else {
        draw_game_over();
    }
}