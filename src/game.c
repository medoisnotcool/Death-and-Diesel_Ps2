#include "game.h"

/* This is a deliberately small vertical slice: side-on bike physics, an AI
 * opponent, four representative pickup behaviours, projectiles and race win
 * conditions. It is a clean recreation scaffold, not a Flash runtime. */

#define COURSE_LENGTH       6200.0f
#define RACE_TIME_LIMIT       75.0f
#define MAX_SPEED            300.0f
#define ACCELERATION         245.0f
#define COAST_DECEL           38.0f
#define BRAKE_DECEL          410.0f
#define GRAVITY              720.0f
#define JUMP_SPEED           315.0f
#define PICKUP_RADIUS         34.0f
#define BIKE_HIT_RADIUS       27.0f

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float absf(float value)
{
    return value < 0.0f ? -value : value;
}

static float wrap_course(float value, float length)
{
    while (value >= length) value -= length;
    while (value < 0.0f) value += length;
    return value;
}

/* A repeatable junkyard profile. y grows toward the bottom of the screen. */
float game_ground_height(float world_x)
{
    const float x = wrap_course(world_x, 960.0f);

    if (x < 160.0f) return 395.0f - x * 0.34f;
    if (x < 330.0f) return 340.6f + (x - 160.0f) * 0.18f;
    if (x < 520.0f) return 371.2f - (x - 330.0f) * 0.42f;
    if (x < 670.0f) return 291.4f + (x - 520.0f) * 0.19f;
    if (x < 810.0f) return 319.9f - (x - 670.0f) * 0.12f;
    return 303.1f + (x - 810.0f) * 0.48f;
}

const char *game_power_name(GamePower power)
{
    switch (power) {
        case GAME_POWER_TURBO: return "TURBO";
        case GAME_POWER_SHIELD: return "SHIELD";
        case GAME_POWER_REPAIR: return "REPAIR";
        case GAME_POWER_NAILGUN: return "NAILGUN";
        default: return "EMPTY";
    }
}

static void set_bike(GameBike *bike, float x)
{
    bike->x = x;
    bike->y = game_ground_height(x);
    bike->vx = 0.0f;
    bike->vy = 0.0f;
    bike->health = 100.0f;
    bike->shield_time = 0.0f;
    bike->turbo_time = 0.0f;
    bike->invulnerable_time = 0.0f;
    bike->grounded = 1;
    bike->power = GAME_POWER_NONE;
}

static void reset_race(GameWorld *world)
{
    static const float locations[GAME_MAX_PICKUPS] = {
        620.0f, 1160.0f, 1720.0f, 2470.0f,
        3190.0f, 3980.0f, 4740.0f, 5520.0f
    };
    static const GamePower powers[GAME_MAX_PICKUPS] = {
        GAME_POWER_TURBO, GAME_POWER_SHIELD, GAME_POWER_NAILGUN,
        GAME_POWER_REPAIR, GAME_POWER_TURBO, GAME_POWER_NAILGUN,
        GAME_POWER_SHIELD, GAME_POWER_REPAIR
    };
    int i;

    set_bike(&world->player, 120.0f);
    set_bike(&world->diesel, 55.0f);
    for (i = 0; i < GAME_MAX_PICKUPS; ++i) {
        world->pickups[i].x = locations[i];
        world->pickups[i].power = powers[i];
        world->pickups[i].active = 1;
    }
    for (i = 0; i < GAME_MAX_PROJECTILES; ++i)
        world->projectiles[i].active = 0;

    world->elapsed = 0.0f;
    world->course_finish = COURSE_LENGTH;
    world->winner = -1;
    world->state = GAME_RACE;
}

void game_reset(GameWorld *world)
{
    reset_race(world);
    world->state = GAME_TITLE;
    world->previous_input = 0;
}

static void damage_bike(GameBike *bike, float damage)
{
    if (bike->shield_time > 0.0f || bike->invulnerable_time > 0.0f)
        return;
    bike->health -= damage;
    bike->invulnerable_time = 0.24f;
    if (bike->health < 0.0f)
        bike->health = 0.0f;
}

static void spawn_nail(GameWorld *world, const GameBike *bike, int owner)
{
    int i;
    for (i = 0; i < GAME_MAX_PROJECTILES; ++i) {
        GameProjectile *shot = &world->projectiles[i];
        if (!shot->active) {
            shot->active = 1;
            shot->owner = owner;
            shot->x = bike->x + 28.0f;
            shot->y = bike->y - 32.0f;
            shot->vx = 510.0f + bike->vx;
            return;
        }
    }
}

static void use_power(GameWorld *world, GameBike *bike, int owner)
{
    switch (bike->power) {
        case GAME_POWER_TURBO:
            bike->turbo_time = 1.65f;
            break;
        case GAME_POWER_SHIELD:
            bike->shield_time = 3.0f;
            break;
        case GAME_POWER_REPAIR:
            bike->health = clampf(bike->health + 30.0f, 0.0f, 100.0f);
            break;
        case GAME_POWER_NAILGUN:
            spawn_nail(world, bike, owner);
            break;
        default:
            return;
    }
    bike->power = GAME_POWER_NONE;
}

static void update_bike(GameBike *bike, unsigned int controls, float dt)
{
    float ground;

    if (bike->shield_time > 0.0f) bike->shield_time -= dt;
    if (bike->turbo_time > 0.0f) bike->turbo_time -= dt;
    if (bike->invulnerable_time > 0.0f) bike->invulnerable_time -= dt;
    if (bike->shield_time < 0.0f) bike->shield_time = 0.0f;
    if (bike->turbo_time < 0.0f) bike->turbo_time = 0.0f;
    if (bike->invulnerable_time < 0.0f) bike->invulnerable_time = 0.0f;

    if (controls & GAME_INPUT_ACCEL)
        bike->vx += ACCELERATION * dt;
    else
        bike->vx -= COAST_DECEL * dt;
    if (controls & GAME_INPUT_BRAKE)
        bike->vx -= BRAKE_DECEL * dt;
    if (bike->turbo_time > 0.0f)
        bike->vx += 175.0f * dt;

    bike->vx = clampf(bike->vx, 0.0f, MAX_SPEED + (bike->turbo_time > 0.0f ? 95.0f : 0.0f));
    if ((controls & GAME_INPUT_JUMP) && bike->grounded) {
        bike->vy = -JUMP_SPEED;
        bike->grounded = 0;
    }

    bike->vy += GRAVITY * dt;
    bike->x += bike->vx * dt;
    bike->y += bike->vy * dt;
    ground = game_ground_height(bike->x);
    if (bike->y >= ground) {
        if (!bike->grounded && bike->vy > 420.0f)
            damage_bike(bike, 6.0f);
        bike->y = ground;
        bike->vy = 0.0f;
        bike->grounded = 1;
    } else {
        bike->grounded = 0;
    }
}

static unsigned int diesel_controls(const GameWorld *world)
{
    const GameBike *diesel = &world->diesel;
    const GameBike *player = &world->player;
    unsigned int input = GAME_INPUT_ACCEL;
    int pulse = ((int)(world->elapsed * 10.0f) + (int)(diesel->x / 160.0f)) % 37;

    /* The AI attacks when near, uses turbo to close a large gap, and hops at
       regular terrain breaks. It is intentionally predictable for prototype QA. */
    if (diesel->power == GAME_POWER_TURBO && player->x - diesel->x > 80.0f)
        input |= GAME_INPUT_POWER;
    if (diesel->power == GAME_POWER_NAILGUN && absf(player->x - diesel->x) < 255.0f)
        input |= GAME_INPUT_POWER;
    if (diesel->power == GAME_POWER_REPAIR && diesel->health < 48.0f)
        input |= GAME_INPUT_POWER;
    if (diesel->power == GAME_POWER_SHIELD && absf(player->x - diesel->x) < 180.0f)
        input |= GAME_INPUT_POWER;
    if (diesel->grounded && pulse == 0)
        input |= GAME_INPUT_JUMP;
    return input;
}

static void collect_pickups(GameWorld *world, GameBike *bike)
{
    int i;
    for (i = 0; i < GAME_MAX_PICKUPS; ++i) {
        GamePickup *pickup = &world->pickups[i];
        if (pickup->active && absf(bike->x - pickup->x) < PICKUP_RADIUS) {
            pickup->active = 0;
            bike->power = pickup->power;
        }
    }
}

static void update_projectiles(GameWorld *world, float dt)
{
    int i;
    for (i = 0; i < GAME_MAX_PROJECTILES; ++i) {
        GameProjectile *shot = &world->projectiles[i];
        GameBike *target;
        if (!shot->active)
            continue;
        shot->x += shot->vx * dt;
        target = shot->owner == 0 ? &world->diesel : &world->player;
        if (absf(shot->x - target->x) < BIKE_HIT_RADIUS &&
            absf(shot->y - (target->y - 28.0f)) < 48.0f) {
            damage_bike(target, 17.0f);
            shot->active = 0;
        }
        if (shot->x > COURSE_LENGTH + 120.0f)
            shot->active = 0;
    }
}

static void finish_race(GameWorld *world, int winner)
{
    world->winner = winner;
    world->state = GAME_GAME_OVER;
}

void game_step(GameWorld *world, unsigned int input, float dt)
{
    unsigned int pressed;
    unsigned int ai_input;

    /* The render loop can occasionally miss a frame; do not permit a physics
       leap that could throw a bike through a hill. */
    dt = clampf(dt, 0.0f, 1.0f / 20.0f);
    pressed = input & ~world->previous_input;

    if (world->state == GAME_TITLE) {
        if (pressed & (GAME_INPUT_START | GAME_INPUT_JUMP))
            reset_race(world);
        world->previous_input = input;
        return;
    }
    if (world->state == GAME_GAME_OVER) {
        if (pressed & (GAME_INPUT_START | GAME_INPUT_JUMP))
            reset_race(world);
        world->previous_input = input;
        return;
    }

    world->elapsed += dt;
    ai_input = diesel_controls(world);
    if (pressed & GAME_INPUT_POWER)
        use_power(world, &world->player, 0);
    if (ai_input & GAME_INPUT_POWER)
        use_power(world, &world->diesel, 1);

    update_bike(&world->player, input, dt);
    update_bike(&world->diesel, ai_input, dt);
    collect_pickups(world, &world->player);
    collect_pickups(world, &world->diesel);
    update_projectiles(world, dt);

    if (world->player.health <= 0.0f) finish_race(world, 1);
    else if (world->diesel.health <= 0.0f) finish_race(world, 0);
    else if (world->player.x >= world->course_finish) finish_race(world, 0);
    else if (world->diesel.x >= world->course_finish) finish_race(world, 1);
    else if (world->elapsed >= RACE_TIME_LIMIT)
        finish_race(world, world->player.x >= world->diesel.x ? 0 : 1);

    world->previous_input = input;
}
