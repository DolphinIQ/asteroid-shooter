#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

typedef unsigned int entity_eid;

// ULTILITY
void print_vec2( Vector2 * vec2, bool should_add_newline ) {
    printf( "Vec2{ %.2f, %.2f } ", vec2->x, vec2->y );
    if ( should_add_newline ) printf( "\n" );
}

// Returns the angle between two 2D positions in radians
float Vector2AngleMine( Vector2 * v1, Vector2 * v2 ) {

    // Calculate the difference between the two positions
    float delta_x = v2->x - v1->x;
    float delta_y = v2->y - v1->y;
    // Calculate the angle using arctangent (in radians)
    float angle = atan2f( delta_y, delta_x );
    return angle;
}

// Loops the value t, so that it is never larger than length and never smaller than 0.
// Ported from Unity https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Math/Mathf.cs)
static inline float Repeat( float t, float length ) {
    return Clamp(
        t - floorf( t / length ) * length,
        0.0f, length
    );
}
// Same as Lerp but makes sure the values interpolate correctly when they wrap around 360 degrees.
// Ported from Unity https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Math/Mathf.cs)
static inline float LerpAngle( float a, float b, float t ) {
    float delta = Repeat( ( b - a ), 360.0f );
    if ( delta > 180 ) delta -= 360.0f;
    return a + delta * Clamp( t, 0.0f, 1.0f );
}

// Global const
const unsigned int screenWidth = 1600;
const unsigned int screenHeight = 900;
Vector2 screenCenter;

float deltaT = 0.0, elapsedT = 0.0;
unsigned int score = 0;


typedef struct laser {
    Vector2 position;
    Vector2 velocity;
    float lifeTime;
    bool isDead;
} laser;

#define DEFINE_LIST_STRUCT( type )  \
typedef struct type##_list {        \
    type * data;                    \
    size_t length;                  \
    size_t capacity                 \
} type##_list;                      \

// printf( "EXCEEDED %s_list CAPACITY !\n", #type );
#define DEFINE_LIST_IMPLEMENTATION( type )                  \
type##_list * type##_list_make( size_t capacity ) {         \
    type##_list * list = malloc( sizeof( type##_list ) );   \
    list->capacity = capacity;                              \
    list->length = 0;                                       \
    list->data = malloc( sizeof( type ) * capacity );       \
    return list;                                            \
}                                                           \
void type##_list_push( type##_list * list, type item ) {    \
    if ( list->length == list->capacity ) {                 \
        return;                                             \
    }                                                       \
    list->data[ list->length ] = item;                      \
    list->length ++;                                        \
}                                                           \
void type##_list_remove( type##_list * list, size_t index ) {   \
    list->data[ index ] = list->data[ list->length - 1 ];       \
    list->length --;                                            \
}                                                               \

DEFINE_LIST_STRUCT( laser );
DEFINE_LIST_IMPLEMENTATION( laser );

DEFINE_LIST_STRUCT( entity_eid );
DEFINE_LIST_IMPLEMENTATION( entity_eid );

// PLAYER
typedef struct Player {
    Vector2 position;
    float rotation;
    float radius;
    Texture2D sprite;

    float speed;
    Vector2 velocity;

    float hp;
    float dmg;

    laser_list * laser_list;
} Player;


// ASTEROIDS
#define ASTEROIDS_MAX_COUNT 100
#define ASTEROID_COLOR (Color) { 255, 60, 60, 255 }
#define ASTEROID_ROTATION_MIN_SPEED 80
#define ASTEROID_ROTATION_MAX_SPEED 180
#define ASTEROID_MIN_VELOCITY 50
// #define ASTEROID_MAX_VELOCITY 300
#define ASTEROID_MAX_VELOCITY 60
#define ASTEROID_BASE_RADIUS 15.0f
#define ASTEROID_RADIUS_FACTOR 0.9f
#define ASTEROID_BASE_HEALTH 100.0f
#define ASTEROID_LIFE_FRAME_COUNT 300
#define ASTEROID_DMG 100.0f
// #define ASTEROID_DMG 1.0f

typedef enum AsteroidType {
    ASTEROID_SMALL = 1,
    ASTEROID_MEDIUM = 2,
    ASTEROID_LARGE = 4
} AsteroidType;

Vector2* position_c;
Vector2* velocity_c;
float* rotation_c;
float* rotation_speed_c;
float* radius_c;
float* hp_c;
AsteroidType* type_c;
bool* is_active_c;

unsigned int asteroid_count = 0;
entity_eid asteroid_eids[ ASTEROIDS_MAX_COUNT ];
entity_eid_list * free_eids;

void create_asteroid() {

    const int outerSpawnMargin = screenWidth / 4;

    // Determine spawn position outside the screen bounds
    const float randomPositionSeed = (float)GetRandomValue( 0, 100 );
    Vector2 randomPos = {
        sinf( randomPositionSeed ) * outerSpawnMargin + screenCenter.x,
        cosf( randomPositionSeed ) * outerSpawnMargin + screenCenter.y
    };

    // Determine target for direction
    const int maxCenterOffset = 300;
    const float randomTargetSeed = (float)GetRandomValue( 0, 100 );
    Vector2 target = {
        sinf( randomTargetSeed ) * maxCenterOffset + screenCenter.x,
        cosf( randomTargetSeed ) * maxCenterOffset + screenCenter.y
    };
    
    // Use target to create velocity
    Vector2 randomVelocity = { target.x - randomPos.x, target.y - randomPos.y };
    randomVelocity = Vector2Normalize( randomVelocity );
    randomVelocity = Vector2Scale( randomVelocity, (float)GetRandomValue( ASTEROID_MIN_VELOCITY, ASTEROID_MAX_VELOCITY ) );
    //Vector2 randomVelocity = Vector2{ 0, 0 };

    entity_eid eid = free_eids->data[ free_eids->length - 1 ];
    free_eids->length -= 1;
    // for ( size_t i = free_eids.length - 1; i >= 0; i-- ) {
    //     eid 
    // }
    // entity_eid eid = asteroid_count;
    asteroid_eids[ asteroid_count ] = eid;
    
    position_c[ eid ] = randomPos;
    velocity_c[ eid ] = randomVelocity;
    rotation_c[ eid ] = 0.0f;
    rotation_speed_c[ eid ] = (float)GetRandomValue( ASTEROID_ROTATION_MIN_SPEED, ASTEROID_ROTATION_MAX_SPEED );
    // type_c[ eid ] = ASTEROID_SMALL;
    type_c[ eid ] = ASTEROID_MEDIUM;
    radius_c[ eid ] = ASTEROID_BASE_RADIUS * type_c[ eid ];
    hp_c[ eid ] = ASTEROID_BASE_HEALTH * type_c[ eid ];
    is_active_c[ eid ] = true;

    // position_c[ eid ] = (Vector2) { 400.0f, 400.0f };
    // velocity_c[ eid ] = (Vector2) { 4.0f, 4.0f };
    // rotation_c[ eid ] = 0.0f;
    // rotation_speed_c[ eid ] = (float)GetRandomValue( ASTEROID_ROTATION_MIN_SPEED, ASTEROID_ROTATION_MAX_SPEED );
    // type_c[ eid ] = ASTEROID_SMALL;
    // radius_c[ eid ] = ASTEROID_BASE_RADIUS * type_c[ eid ];
    // hp_c[ eid ] = ASTEROID_BASE_HEALTH * type_c[ eid ];
    // is_active_c[ eid ] = true;

    asteroid_count ++;
}

void asteroid_update_rotation( entity_eid eid, float deltaT ) {

    rotation_c[ eid ] = rotation_c[ eid ] + rotation_speed_c[ eid ] * deltaT;
}

void asteroid_update_position( entity_eid eid, float deltaT ) {

    position_c[ eid ] = Vector2Add( position_c[ eid ], Vector2Scale( velocity_c[ eid ], deltaT ) );

}

void asteroid_update_damage_vs_player( entity_eid eid, Player * player, float deltaT ) {

    if ( Vector2Distance( position_c[ eid ], player->position ) < radius_c[ eid ] * ASTEROID_RADIUS_FACTOR + player->radius * 0.6 ) {
        player->hp -= ASTEROID_DMG * deltaT;
    }
}
void asteroid_update_damage_vs_asteroids( entity_eid eid, float deltaT ) {

    // int count = 0;
    for ( size_t i = 0; i < ASTEROIDS_MAX_COUNT; i++ ) {
        const entity_eid collider_eid = asteroid_eids[ i ];
        if ( collider_eid == eid || is_active_c[ collider_eid ] == false ) {
            continue; // Skip if found itself, or when the collider isnt active
        }

        // count ++;
        // printf( "asteroid_update_damage_vs_asteroids: %d, eid: %d, active: %d \n", count, collider_eid, is_active_c[ collider_eid ] );  

        const float collision_dist_sq = powf( radius_c[ eid ] + radius_c[ collider_eid ], 2.0f );
        if ( Vector2DistanceSqr( position_c[ eid ], position_c[ collider_eid ] ) < collision_dist_sq ) {
            hp_c[ collider_eid ] -= ASTEROID_DMG * deltaT;
        }
        // const float collision_dist = radius_c[ eid ] + radius_c[ collider_eid ];
        // if ( Vector2Distance( position_c[ eid ], position_c[ collider_eid ] ) < collision_dist ) {
        //     hp_c[ collider_eid ] -= ASTEROID_DMG * deltaT;
        // }
    }
}
void asteroid_update_state( entity_eid eid ) {

    if ( hp_c[ eid ] <= 0.0 ) {
        if ( is_active_c[ eid ] == true ) { // Just died
            score += type_c[ eid ];
            // Spawn mini asteroids
            if ( type_c[ eid ] == ASTEROID_MEDIUM ) {
                
            }
        }
        is_active_c[ eid ] = false;
    }
}

void asteroid_draw( entity_eid eid ) {

    DrawPolyLinesEx( position_c[ eid ], 5, radius_c[ eid ], rotation_c[ eid ], 2.0f, ASTEROID_COLOR );
}
void asteroid_draw_radius( entity_eid eid ) {

    DrawCircleLinesV( position_c[ eid ], radius_c[ eid ], RAYWHITE );
}

// LASERS
#define LASER_LENGTH 35.0f
#define LASER_LIFETIME 2.0f // in seconds
#define LASER_SPEED 17.0f // pixels per second
#define LASER_DMG 100.0f // after colliding
#define LASER_COLOR (Color) { 50, 255, 30, 255 }

laser laser_create( Vector2 playerPos, Vector2 mousePos ) {

    // float rotation = Vector2AngleMine( &player.position, &mousePos ) * RAD2DEG;

    Vector2 velocity = { mousePos.x - playerPos.x, mousePos.y - playerPos.y };
    velocity = Vector2Normalize( velocity );
    velocity = Vector2Scale( velocity, LASER_SPEED );

    laser l = {
        .position = playerPos,
        .velocity = velocity
    };
    return l;
}

void laser_update( laser * l, float deltaT ) {

    l->position = Vector2Add( l->position, l->velocity );

    for ( size_t i = 0; i < ASTEROIDS_MAX_COUNT; i++ ) {
        const entity_eid asteroid_eid = asteroid_eids[ i ];
        if ( is_active_c[ asteroid_eid ] == false ) {
            continue; // Skip when the collider isnt active
        }

        // count ++;
        // printf( "asteroid_update_damage_vs_asteroids: %d, eid: %d, active: %d \n", count, asteroid_eid, is_active_c[ asteroid_eid ] );  

        const float collision_dist_sq = powf( radius_c[ asteroid_eid ] * ASTEROID_RADIUS_FACTOR, 2.0f );
        if ( Vector2DistanceSqr( l->position, position_c[ asteroid_eid ] ) < collision_dist_sq ) {
            hp_c[ asteroid_eid ] -= LASER_DMG;
            l->isDead = true;
            return;
        }
    }

    l->lifeTime += deltaT;
    if ( l->lifeTime > LASER_LIFETIME ) {
        l->isDead = true;
    }
}

void laser_draw( laser * l ) {

    Vector2 direction = Vector2Normalize( l->velocity );
    const float laser_length_half = LASER_LENGTH * 0.5f;
    Vector2 offset = Vector2Scale( direction, laser_length_half );
    Vector2 laser_start = Vector2Add( l->position, offset );
    Vector2 laser_end = Vector2Subtract( l->position, offset );

    // DrawCircleV( l->position, LASER_SIZE, LASER_COLOR );

    DrawLineEx( laser_start, laser_end, 4.0f, LASER_COLOR );
}

// Globals
Player player;
Vector2 mouse_coords;
bool GAME_OVER = false;


void processInput() {

    // Mouse
    mouse_coords = GetMousePosition();
    DrawPolyLinesEx( mouse_coords, 4, 8, 0, 2.0f, BLUE );

    if ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) ) {
        // printf("FIRE!");
        laser_list_push( player.laser_list, laser_create( player.position, mouse_coords ) );
        // laser_list_push( player.laser_list, (laser) {
        //     .position = player.position,
        //     .isDead = false
        // });
    }

    // Keyboard
    Vector2 direction = { 0 };
    if ( IsKeyDown( KEY_W ) ) {
        direction.y -= 1.0f;
    }
    if ( IsKeyDown( KEY_S ) ) {
        direction.y += 1.0f;
    }
    if ( IsKeyDown( KEY_A ) ) {
        direction.x -= 1.0f;
    }
    if ( IsKeyDown( KEY_D ) ) {
        direction.x += 1.0f;
    }
    direction = Vector2Scale( Vector2Normalize( direction ), player.speed );

    player.velocity = Vector2Add( player.velocity, direction );
}

void updateLogic() {

    deltaT = GetFrameTime();
    elapsedT = GetTime();

    // Lasers
    for ( size_t i = 0; i < player.laser_list->length; i++ ) {
        laser_update( &(player.laser_list->data[ i ]), deltaT );
    }
    for ( size_t i = 0; i < player.laser_list->length; i++ ) {
        if ( player.laser_list->data[ i ].isDead ) {
            laser_list_remove( player.laser_list, i );
        }
    }

    // Player
    const float FRICTION = 0.95;
    player.velocity = Vector2Scale( player.velocity, FRICTION );
    player.position = Vector2Add( player.position, Vector2Scale( player.velocity, deltaT ) );

    // Player orientation
    float rotation = Vector2AngleMine( &player.position, &mouse_coords ) * RAD2DEG;
    // player.rotation = rotation;
    player.rotation = LerpAngle( player.rotation, rotation, 0.15 );

    // Asteroids
    for ( size_t i = 0; i < asteroid_count; i++ ) {
        entity_eid eid = asteroid_eids[ i ];
        if ( is_active_c[ eid ] == false ) continue;

        asteroid_update_rotation( eid, deltaT );
        asteroid_update_position( eid, deltaT );
        asteroid_update_damage_vs_player( eid, &player, deltaT );
        // asteroid_update_damage_vs_asteroids( eid, deltaT );
        asteroid_update_state( eid );
    }

    // Check Game State
    if ( player.hp <= 0.0 ) {
        GAME_OVER = true;
    }
}

void render() {

    BeginDrawing();
    ClearBackground( BLACK );

    // Draw Lasers
    for ( size_t i = 0; i < player.laser_list->length; i++ ) {

        laser_draw( &(player.laser_list->data[ i ]) );
    }

    // Draw player
    DrawPolyLinesEx( player.position, 3, player.radius, player.rotation, 2.0f, RAYWHITE );

    // Draw Asteroids
    for ( size_t i = 0; i < asteroid_count; i++ ) {
        entity_eid eid = asteroid_eids[ i ];
        if ( is_active_c[ eid ] == false ) continue;

        asteroid_draw( eid );
        // asteroid_draw_radius( eid );
    }

    // Debug
    DrawText( TextFormat( "FPS: %d", GetFPS() ), screenWidth - 100, 10, 20, RED );

    DrawText( TextFormat( "Score: %d", score ), 30, screenHeight - 90, 30, YELLOW );
    DrawText( TextFormat( "HP: %d", (int)player.hp ), 30, screenHeight - 50, 30, RED );
    if ( GAME_OVER ) {
        DrawText( "GAME OVER", screenWidth / 2 - 200, 50, 70, RED );
    }

    EndDrawing();
}

int main() {

    InitWindow(screenWidth, screenHeight, "Raylib basic window");
    SetTargetFPS( 120 );
    // SetMouseCursor( MOUSE_CURSOR_CROSSHAIR );
    HideCursor();
    printf("Hi 22222 \n");

    // Screen
    screenCenter = (Vector2) { screenWidth * 0.5, screenHeight * 0.5 };

    // Init Data components
    position_c = malloc( sizeof( Vector2 ) * ASTEROIDS_MAX_COUNT );
    velocity_c = malloc( sizeof( Vector2 ) * ASTEROIDS_MAX_COUNT );
    rotation_c = malloc( sizeof( float ) * ASTEROIDS_MAX_COUNT );
    rotation_speed_c = malloc( sizeof( float ) * ASTEROIDS_MAX_COUNT );
    radius_c = malloc( sizeof( float ) * ASTEROIDS_MAX_COUNT );
    hp_c = malloc( sizeof( float ) * ASTEROIDS_MAX_COUNT );
    type_c = malloc( sizeof( AsteroidType ) * ASTEROIDS_MAX_COUNT );
    is_active_c = malloc( sizeof( bool ) * ASTEROIDS_MAX_COUNT );

    // Init global values
    const int LASER_COUNT = 128;
    player = (Player) {
        .position = (Vector2) { .x = screenWidth * 0.5, .y = screenHeight * 0.5 },
        .radius = 30.0f,
        .speed = 30.0f,
        .hp = 100.0f,
        .dmg = 5.0f,
        .laser_list = laser_list_make( LASER_COUNT )
    };

    printf( "laser list: %lld %lld \n", player.laser_list->length, player.laser_list->capacity );

    // for ( size_t i = 0; i < LASER_COUNT; i++ ) {
    //     laser new_laser = {
    //         .position
    //     };
    //     player.laser_list
    // }

    mouse_coords = (Vector2) { .x = screenWidth * 0.5f, .y = screenHeight * 0.5f };


    free_eids = entity_eid_list_make( ASTEROIDS_MAX_COUNT );
    for ( size_t i = 0; i < ASTEROIDS_MAX_COUNT; i++ ) {
        entity_eid_list_push( free_eids, (entity_eid)i );
    }

    // create_asteroid();
    for ( size_t i = 0; i < ASTEROIDS_MAX_COUNT / 2; i++ ) {
        create_asteroid();
    }

    // printf( "asteroid_eids: [ " );
    // for ( size_t i = 0; i < asteroid_count; i++ ) {
    //     printf( "%d, ", asteroid_eids[ i ] );
    // }
    // printf( "] \n" );

    // Load textures
    {

    }

    while (!WindowShouldClose()) {

        processInput();
        updateLogic();
        render();
    }

    free( position_c );
    free( velocity_c );
    free( rotation_c );
    free( rotation_speed_c );
    free( radius_c );
    free( hp_c );
    free( type_c );
    free( is_active_c );

    CloseWindow();
}
