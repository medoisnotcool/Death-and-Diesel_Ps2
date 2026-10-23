#include <assert.h>
#include <stdio.h>

#include "game.h"

static void run_for(GameWorld *world, unsigned int input, int frames)
{
    int i;
    for (i = 0; i < frames; ++i)
        game_step(world, input, 1.0f / 60.0f);
}

int main(void)
{
    GameWorld world;
    float start_x;
    float start_y;
    int i;

    game_reset(&world);
    assert(world.state == GAME_TITLE);

    /* Start the race using the same Cross action used on PS2. Release it so a
       held menu press does not act as a continuous jump. */
    game_step(&world, GAME_INPUT_JUMP, 1.0f / 60.0f);
    game_step(&world, 0, 1.0f / 60.0f);
    assert(world.state == GAME_RACE);

    start_x = world.player.x;
    run_for(&world, GAME_INPUT_ACCEL, 120);
    assert(world.player.x > start_x + 20.0f);
    assert(world.player.vx > 0.0f);

    /* A jump must put a grounded bike above its terrain contact point. */
    world.player.y = game_ground_height(world.player.x);
    world.player.vy = 0.0f;
    world.player.grounded = 1;
    start_y = world.player.y;
    game_step(&world, GAME_INPUT_JUMP, 1.0f / 60.0f);
    run_for(&world, 0, 5);
    assert(world.player.y < start_y);

    /* Place the rider on a pickup; the native gameplay layer must award it. */
    world.player.x = world.pickups[0].x;
    world.player.y = game_ground_height(world.player.x);
    world.player.vx = 0.0f;
    game_step(&world, 0, 1.0f / 60.0f);
    assert(!world.pickups[0].active);
    assert(world.player.power == GAME_POWER_TURBO);

    /* The awarded turbo must be consumed and have a timed effect. */
    game_step(&world, GAME_INPUT_POWER, 1.0f / 60.0f);
    assert(world.player.power == GAME_POWER_NONE);
    assert(world.player.turbo_time > 1.0f);

    /* Advance for a moment to cover projectile/update loops deterministically. */
    for (i = 0; i < 30; ++i)
        game_step(&world, GAME_INPUT_ACCEL, 1.0f / 60.0f);

    puts("game logic tests passed");
    return 0;
}
