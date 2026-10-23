#include <stddef.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "game.h"

#define SCREEN_W 640
#define SCREEN_H 448
#define PAD_BUFFER_SIZE 256

#define COLOR(r, g, b) GS_SETREG_RGBAQ((r), (g), (b), 0x80, 0x00)

static GSGLOBAL *gs_global;
static unsigned char pad_buffer[PAD_BUFFER_SIZE] __attribute__((aligned(64)));

/* Five-pixel-wide uppercase font for a self-contained, art-free prototype.
 * A production build will replace this with the approved original title/UI art. */
static const unsigned char font_az[26][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {15,16,16,16,16,16,15}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {15,16,16,19,17,17,15}, {17,17,17,31,17,17,17},
    {31,4,4,4,4,4,31}, {1,1,1,1,17,17,14},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
};

static void draw_rect(int x, int y, int width, int height, u64 color)
{
    if (width <= 0 || height <= 0 || x >= SCREEN_W || y >= SCREEN_H ||
        x + width <= 0 || y + height <= 0)
        return;
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_W) width = SCREEN_W - x;
    if (y + height > SCREEN_H) height = SCREEN_H - y;
    gsKit_prim_sprite(gs_global, (float)x, (float)y,
                      (float)(x + width), (float)(y + height), 1, color);
}

static void draw_text(int x, int y, const char *text, int scale, u64 color)
{
    int cursor = x;
    while (*text) {
        int row, col;
        char c = *text++;
        if (c == ' ') {
            cursor += 4 * scale;
            continue;
        }
        if (c < 'A' || c > 'Z') {
            cursor += 6 * scale;
            continue;
        }
        for (row = 0; row < 7; ++row) {
            for (col = 0; col < 5; ++col) {
                if (font_az[c - 'A'][row] & (1 << (4 - col)))
                    draw_rect(cursor + col * scale, y + row * scale,
                              scale, scale, color);
            }
        }
        cursor += 6 * scale;
    }
}

static u64 pickup_color(GamePower power)
{
    switch (power) {
        case GAME_POWER_TURBO: return COLOR(255, 190, 30);
        case GAME_POWER_SHIELD: return COLOR(40, 180, 255);
        case GAME_POWER_REPAIR: return COLOR(70, 230, 90);
        case GAME_POWER_NAILGUN: return COLOR(245, 80, 60);
        default: return COLOR(160, 160, 160);
    }
}

static void draw_bike(const GameBike *bike, float camera_x, u64 body_color)
{
    int x = (int)(bike->x - camera_x);
    int y = (int)bike->y;
    u64 wheel = COLOR(20, 20, 24);

    draw_rect(x - 20, y - 10, 13, 13, wheel);
    draw_rect(x + 10, y - 10, 13, 13, wheel);
    draw_rect(x - 17, y - 30, 40, 16, body_color);
    draw_rect(x - 4, y - 48, 12, 20, COLOR(225, 210, 185));
    if (bike->shield_time > 0.0f)
        draw_rect(x - 27, y - 57, 60, 4, COLOR(70, 210, 255));
}

static void draw_terrain(float camera_x)
{
    int screen_x;
    for (screen_x = 0; screen_x < SCREEN_W; screen_x += 8) {
        float world_x = camera_x + (float)screen_x;
        int ground = (int)game_ground_height(world_x);
        draw_rect(screen_x, ground, 10, SCREEN_H - ground, COLOR(67, 59, 44));
        draw_rect(screen_x, ground, 10, 4, COLOR(135, 117, 73));
    }
}

static void draw_hud(const GameWorld *world)
{
    int player_bar = (int)(world->player.health * 1.1f);
    int diesel_bar = (int)(world->diesel.health * 1.1f);
    draw_text(16, 15, "HEALTH", 2, COLOR(240, 240, 230));
    draw_rect(16, 34, 112, 10, COLOR(70, 24, 28));
    draw_rect(16, 34, player_bar, 10, COLOR(60, 210, 75));
    draw_rect(SCREEN_W - 128, 34, 112, 10, COLOR(70, 24, 28));
    draw_rect(SCREEN_W - 128, 34, diesel_bar, 10, COLOR(240, 75, 55));
    draw_text(16, 55, "POWER", 1, pickup_color(world->player.power));
    draw_text(16, 67, game_power_name(world->player.power), 1,
              pickup_color(world->player.power));
}

static void draw_race(const GameWorld *world)
{
    float camera_x = world->player.x - 145.0f;
    int i;

    if (camera_x < 0.0f) camera_x = 0.0f;
    draw_terrain(camera_x);

    for (i = 0; i < GAME_MAX_PICKUPS; ++i) {
        const GamePickup *pickup = &world->pickups[i];
        int x;
        int y;
        if (!pickup->active) continue;
        x = (int)(pickup->x - camera_x);
        y = (int)game_ground_height(pickup->x) - 39;
        draw_rect(x - 10, y - 10, 21, 21, COLOR(25, 25, 28));
        draw_rect(x - 6, y - 6, 13, 13, pickup_color(pickup->power));
    }
    for (i = 0; i < GAME_MAX_PROJECTILES; ++i) {
        const GameProjectile *shot = &world->projectiles[i];
        if (shot->active)
            draw_rect((int)(shot->x - camera_x), (int)shot->y, 12, 3,
                      COLOR(250, 230, 110));
    }

    if (world->course_finish - camera_x < (float)SCREEN_W) {
        int flag_x = (int)(world->course_finish - camera_x);
        int ground = (int)game_ground_height(world->course_finish);
        draw_rect(flag_x, ground - 115, 4, 115, COLOR(230, 230, 220));
        draw_rect(flag_x + 4, ground - 115, 44, 24, COLOR(235, 205, 45));
    }

    draw_bike(&world->diesel, camera_x, COLOR(206, 65, 46));
    draw_bike(&world->player, camera_x, COLOR(58, 103, 220));
    draw_hud(world);
}

static void draw_frame(const GameWorld *world)
{
    gsKit_clear(gs_global, COLOR(23, 27, 37));
    if (world->state == GAME_TITLE) {
        draw_text(70, 112, "DIESEL AND DEATH", 4, COLOR(240, 158, 36));
        draw_text(153, 210, "PS TWO PROTOTYPE", 2, COLOR(225, 225, 218));
        draw_text(128, 292, "PRESS START OR CROSS", 2, COLOR(110, 205, 245));
        draw_text(72, 355, "RIGHT ACCEL LEFT BRAKE CROSS JUMP", 1,
                  COLOR(205, 205, 200));
        draw_text(125, 374, "CIRCLE OR R ONE USE POWER", 1,
                  COLOR(205, 205, 200));
    } else {
        draw_race(world);
        if (world->state == GAME_GAME_OVER) {
            draw_rect(120, 168, 400, 104, COLOR(20, 20, 25));
            if (world->winner == 0)
                draw_text(213, 191, "YOU WIN", 4, COLOR(80, 230, 120));
            else
                draw_text(178, 191, "DIESEL WINS", 4, COLOR(240, 85, 64));
            draw_text(178, 246, "PRESS START OR CROSS", 2,
                      COLOR(225, 225, 215));
        }
    }
    gsKit_queue_exec(gs_global);
    gsKit_sync_flip(gs_global);
}

static unsigned int read_controller(void)
{
    struct padButtonStatus buttons;
    unsigned int input = 0;
    int state = padGetState(0, 0);

    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1)
        return 0;
    if (padRead(0, 0, &buttons) == 0)
        return 0;
    if (!(buttons.btns & PAD_RIGHT)) input |= GAME_INPUT_ACCEL;
    if (!(buttons.btns & PAD_LEFT)) input |= GAME_INPUT_BRAKE;
    if (!(buttons.btns & PAD_CROSS)) input |= GAME_INPUT_JUMP;
    if (!(buttons.btns & PAD_CIRCLE) || !(buttons.btns & PAD_R1))
        input |= GAME_INPUT_POWER;
    if (!(buttons.btns & PAD_START)) input |= GAME_INPUT_START;
    return input;
}

static void initialise_ps2(void)
{
    SifInitRpc(0);
    SifLoadFileInit();
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    padPortOpen(0, 0, pad_buffer);

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gs_global = gsKit_init_global();
    gs_global->Mode = GS_MODE_NTSC;
    gs_global->Interlace = GS_INTERLACED;
    gs_global->Field = GS_FIELD;
    gs_global->DoubleBuffering = GS_SETTING_ON;
    gs_global->ZBuffering = GS_SETTING_OFF;
    gsKit_init_screen(gs_global);
}

int main(int argc, char *argv[])
{
    GameWorld world;
    (void)argc;
    (void)argv;

    initialise_ps2();
    game_reset(&world);
    while (1) {
        game_step(&world, read_controller(), 1.0f / 60.0f);
        draw_frame(&world);
    }
    return 0;
}
