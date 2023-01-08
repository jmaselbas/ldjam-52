#pragma once

enum asset_key {
	DEBUG_MESH_CROSS,
	DEBUG_MESH_CYLINDER,
	DEBUG_MESH_CUBE,
	DEBUG_MESH_SPHERE,
	MESH_QUAD,
	MESH_TEST,
	MESH_FLOOR,
	MESH_ROCK_PILAR,
	MESH_ROCK_SMALL,
	DEBUG_SHADER_TEXTURE,
	SHADER_SOLID,
	SHADER_TEST,
	SHADER_SKY,
	SHADER_GUI,
	TEXTURE_GUI_SHAPE,
	TEXTURE_TEXT,

	WAV_THEME,
	WAV_CLICK,
	ASSET_KEY_COUNT,
	/* internal assets id starts here, they are not handled as regular
	 * assets and should not be passed to game_get_*() */
	INTERNAL_TEXTURE,
};

enum asset_state {
	STATE_UNLOAD,
	STATE_LOADED,
};

struct game_asset {
	struct memory_zone *memzone;
	struct memory_zone  tmpzone;
	struct memory_zone *samples;
	struct res_data {
		enum asset_state state;
		time_t since;
		size_t size;
		void *base;
	} assets[ASSET_KEY_COUNT];
};

void game_asset_init(struct game_asset *game_asset, struct memory_zone *memzone, struct memory_zone *samples);
void game_asset_fini(struct game_asset *game_asset);
void game_asset_poll(struct game_asset *game_asset);

struct shader *game_get_shader(struct game_asset *game_asset, enum asset_key key);
struct mesh *game_get_mesh(struct game_asset *game_asset, enum asset_key key);
struct wav *game_get_wav(struct game_asset *game_asset, enum asset_key key);
struct smf * game_get_smf(struct game_asset *game_asset, enum asset_key key);
struct pre * game_get_pre(struct game_asset *game_asset, enum asset_key key);
struct texture *game_get_texture(struct game_asset *game_asset, enum asset_key key);
struct font_meta *game_get_font_meta(struct game_asset *game_asset, enum asset_key key);

