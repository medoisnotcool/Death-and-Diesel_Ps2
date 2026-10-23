#ifndef DIESEL_DEATH_GAME_H
#define DIESEL_DEATH_GAME_H

/* Platform-neutral prototype rules. This module is intentionally free of PS2SDK
 * headers so the gameplay can be unit-tested on a normal host compiler. */

enum {
    GAME_INPUT_ACCEL = 1u << 0,
    GAME_INPUT_BRAKE = 1u << 1,
    GAME_INPUT_JUMP  = 1u << 2,
    GAME_INPUT_POWER = 1u << 3,
    GAME_INPUT_START = 1u << 4
};

typedef enum {
    GAME_TITLE = 0,
    GAME_RACE,
    GAME_GAME_OVER
} GameState;

typedef enum {
    GAME_POWER_NONE = 0,
    GAME_POWER_TURBO,
    GAME_POWER_SHIELD,
    GAME_POWER_REPAIR,
    GAME_POWER_NAILGUN
} GamePower;

typedef struct {
    float x;
    float y;       /* Bike wheel/ground contact point in world pixels. */
    float vx;
    float vy;
    float health;
    float shield_time;
    float turbo_time;
    float invulnerable_time;
    int grounded;
    GamePower power;
} GameBike;

typedef struct {
    float x;
    GamePower power;
    int active;
} GamePickup;

typedef struct {
    float x;
    float y;
    float vx;
    int owner;     /* 0 = player, 1 = Diesel. */
    int active;
} GameProjectile;

#define GAME_MAX_PICKUPS 8
#define GAME_MAX_PROJECTILES 8

typedef struct {
    GameState state;
    GameBike player;
    GameBike diesel;
    GamePickup pickups[GAME_MAX_PICKUPS];
    GameProjectile projectiles[GAME_MAX_PROJECTILES];
    float elapsed;
    float course_finish;
    unsigned int previous_input;
    int winner;    /* 0 = Death/player, 1 = Diesel, -1 = no winner yet. */
} GameWorld;

void game_reset(GameWorld *world);
void game_step(GameWorld *world, unsigned int input, float dt);
float game_ground_height(float world_x);
const char *game_power_name(GamePower power);

#endif
