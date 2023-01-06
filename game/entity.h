#pragma once

struct components {
	unsigned int is_player:1;
	unsigned int is_grass:1;
	unsigned int has_debug:1;
	unsigned int has_render:1;
	unsigned int has_sound:1;
	unsigned int has_movement:1;
	unsigned int has_midi:1;
};

struct entity {
	struct components components;
	vec3 position;
	vec3 direction;
	float move_speed;

	vec3 color;
	enum asset_key mesh;
	enum asset_key shader;
	enum asset_key debug_mesh;
	vec3 debug_scale;
};
typedef uint16_t entity_id;

void entity_step(void);

