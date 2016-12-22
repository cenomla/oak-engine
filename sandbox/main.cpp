#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

#include <graphics/opengl/gl_texture_atlas.h>
#include <graphics/opengl/gl_frame_renderer.h>
#include <graphics/opengl/gl_renderer.h>
#include <graphics/vertex.h>
#include <graphics/font.h>
#include <network/network_manager.h>
#include <log.h>
#include <events.h>
#include <engine.h>
#include <task.h>
#include <window.h>
#include <entity.h>
#include <prefab.h>
#include <lua_manager.h>
#include <script/lua_puper.h>
#include <binding.h>
#include <components.h>
#include <resource_manager.h>
#include <event_queue.h>

#include "binding.h"
#include "tile_system.h"
#include "systems.h"
#include "events.h"

std::chrono::duration<float> dt;

int main(int argc, char** argv) {
	//setup log
	std::ofstream file{ "log" };
	//std::ostream log{ std::cout.rdbuf(&oak::log::cout_stream) };
	oak::log::cout_stream.addStream(file);
	oak::log::cout_stream.addStream(std::cout);

	//create the engine
	oak::Engine engine;
	engine.init();

	//create all the systems
	oak::Window window{ engine, oak::Window::GL_CONTEXT };
	oak::LuaManager luam{ engine };
	oak::graphics::GLFrameRenderer frameRenderer{ engine };
	oak::graphics::GLRenderer entityRenderer{ engine };
	oak::ResourceManager resManager{ engine };
	oak::network::NetworkManager networkManager{ engine };
	oak::EntityManager entityManager{ engine };
	CollisionSystem collisionSystem{ engine };
	TileSystem tileSystem{ engine, 8, 16 };
	SpriteSystem spriteSystem{ engine };
	TextSystem textSystem{ engine };

	//add the systems to the engine and therefore initilize them
	engine.addSystem(&resManager);
	engine.addSystem(&networkManager);
	engine.addSystem(&entityManager);
	engine.addSystem(&window);
	engine.addSystem(&frameRenderer);
	engine.addSystem(&luam);
	engine.addSystem(&collisionSystem);
	engine.addSystem(&tileSystem);
	engine.addSystem(&spriteSystem);
	engine.addSystem(&textSystem);
	engine.addSystem(&entityRenderer);

	//set up event listeners that push input events to lua
	lua_State *L = luam.getState();
	engine.getEventManager().add<oak::KeyEvent>([L](const oak::KeyEvent &evt) {
		oak::luah::getGlobal(L, "oak.es.emit_event");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "key_press");
		oak::luah::pushValue(L, evt.key);
		oak::luah::pushValue(L, evt.scancode);
		oak::luah::pushValue(L, evt.action);
		oak::luah::pushValue(L, evt.mods);
		oak::luah::call(L, 6, 0);
	});
	engine.getEventManager().add<oak::ButtonEvent>([L](const oak::ButtonEvent &evt){
		oak::luah::getGlobal(L, "oak.es.emit_event");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "button_press");
		oak::luah::pushValue(L, evt.button);
		oak::luah::pushValue(L, evt.action);
		oak::luah::pushValue(L, evt.mods);
		oak::luah::call(L, 5, 0);
	});
	engine.getEventManager().add<oak::MouseMoveEvent>([L](const oak::MouseMoveEvent &evt) {
		oak::luah::getGlobal(L, "oak.es.emit_event");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "mouse_move");
		oak::luah::pushValue(L, evt.x);
		oak::luah::pushValue(L, evt.y);
		oak::luah::call(L, 4, 0);
	});
	engine.getEventManager().add<oak::EntityCollisionEvent>([L](const oak::EntityCollisionEvent &evt) {
		oak::luah::getGlobal(L, "oak.es.send_message");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "entity_collide");

		oak::luah::getGlobal(L, "oak.es.get_entity");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, evt.entity.index());
		oak::luah::call(L, 2, 1);

		oak::luah::getGlobal(L, "oak.es.get_entity");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, evt.entity.index());
		oak::luah::call(L, 2, 1);

		lua_newtable(L);
		oak::LuaPuper puper{ L, -2 };
		glm::vec2 n = evt.normal;
		oak::util::pup(puper, n, {});

		oak::luah::pushValue(L, evt.depth);

		oak::luah::call(L, 6, 0);
	});

	engine.getEventManager().add<TileCollisionEvent>([L](const TileCollisionEvent &evt) {
		oak::luah::getGlobal(L, "oak.es.send_message");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "tile_collide");

		oak::luah::getGlobal(L, "oak.es.get_entity");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, evt.entity.index());
		oak::luah::call(L, 2, 1);
		
		lua_newtable(L);
		oak::LuaPuper puper{ L, -2 };
		Tile t = evt.tile;
		pup(puper, t, {});

		lua_newtable(L);
		glm::vec2 n = evt.normal;
		oak::util::pup(puper, n, {});
		
		oak::luah::pushValue(L, evt.depth);
		
		oak::luah::call(L, 6, 0);
	});

	//register components with the entity manager
	entityManager.addComponentHandle<oak::TransformComponent>();
	entityManager.addComponentHandle<oak::SpriteComponent>();
	entityManager.addComponentHandle<oak::TextComponent>();
	entityManager.addComponentHandle<oak::AABB2dComponent>();
	entityManager.addComponentHandle<oak::PhysicsBody2dComponent>();

	//create resources
	//shaders
	auto &sdr_pass = resManager.add<oak::graphics::GLShader>("sdr_pass");
	sdr_pass.create("core/graphics/shaders/pass2d/opengl.vert", "core/graphics/shaders/pass2d/opengl.frag");

	auto &sdr_font = resManager.add<oak::graphics::GLShader>("sdr_font");
	sdr_font.create("core/graphics/shaders/font/opengl.vert", "core/graphics/shaders/font/opengl.frag");

	//textures
	auto &tex_entityAtlas = resManager.add<oak::graphics::GLTextureAtlas>("tex_entityAtlas", GLenum{ GL_TEXTURE_2D });
	tex_entityAtlas.addTexture("res/textures/character.png");
	tex_entityAtlas.addTexture("res/textures/block.png");
	tex_entityAtlas.bake(512, 512);

	auto &tex_guiAtlas = resManager.add<oak::graphics::GLTextureAtlas>("tex_guiAtlas", GLenum{ GL_TEXTURE_2D });
	tex_guiAtlas.addTexture("res/textures/button.png");
	tex_guiAtlas.bake(512, 512);

	auto &tex_tiles = resManager.add<oak::graphics::GLTexture>("tex_tiles", GLenum{ GL_TEXTURE_2D });
	tex_tiles.create("res/textures/tiles.png");

	auto &tex_font = resManager.add<oak::graphics::GLTexture>("tex_font", GLenum{ GL_TEXTURE_2D }, GLenum{ GL_LINEAR });
	tex_font.create("res/fonts/dejavu_sans/atlas.png");

	//materials
	auto &mat_entity = resManager.add<oak::graphics::GLMaterial>("mat_entity", std::hash<std::string>{}("mat_entity"), &sdr_pass, &tex_entityAtlas);
	auto &mat_gui = resManager.add<oak::graphics::GLMaterial>("mat_gui", std::hash<std::string>{}("mat_gui"), &sdr_pass, &tex_guiAtlas);
	auto &mat_tiles = resManager.add<oak::graphics::GLMaterial>("mat_tiles", std::hash<std::string>{}("mat_tiles"), &sdr_pass, &tex_tiles);
	auto &mat_font = resManager.add<oak::graphics::GLMaterial>("mat_font", std::hash<std::string>{}("mat_font"), &sdr_font, &tex_font, 
		[](const oak::graphics::GLMaterial& mat) {
			mat.shader->setUniform1f("text_width", 0.5f);
			mat.shader->setUniform1f("text_edge", 0.1f);
			mat.shader->setVector3f("text_color", glm::vec3{ 1.0f });
			mat.shader->setUniform1f("border_width", 0.6f);
			mat.shader->setUniform1f("border_edge", 0.2f);
			mat.shader->setVector3f("border_color", glm::vec3{ 0.0f });
		});
	
	//fonts
	auto &fnt_dejavu = resManager.add<oak::graphics::Font>("fnt_dejavu", mat_font.id);
	fnt_dejavu.create("res/fonts/dejavu_sans/glyphs.fnt");

	//sprites
	resManager.add<oak::graphics::Sprite>("spr_player", mat_entity.id, 8.0f, 8.0f, 16.0f, 16.0f, tex_entityAtlas.getTextureRegion("res/textures/character.png"));
	resManager.add<oak::graphics::Sprite>("spr_block", mat_entity.id, 64.0f, 16.0f, 128.0f, 32.0f, tex_entityAtlas.getTextureRegion("res/textures/block.png"));
	resManager.add<oak::graphics::Sprite>("spr_button", mat_gui.id, 0.0f, 0.0f, 48.0f, 48.0f, tex_guiAtlas.getTextureRegion("res/textures/button.png"), 2);

	//strings
	resManager.add<std::string>("txt_play", "Start Game");
	resManager.add<std::string>("txt_load", "Load Game");
	resManager.add<std::string>("txt_options", "Options");
	resManager.add<std::string>("txt_quit", "Quit Game");

	auto& prefab = resManager.add<oak::Prefab>("player", &entityManager);
	prefab.addComponent<oak::TransformComponent>(false, glm::vec3{ 216.0f, 128.0f, 0.0f }, 1.0f, glm::vec3{ 0.0f }, 0.0f);
	prefab.addComponent<oak::SpriteComponent>(true, std::hash<std::string>{}("spr_player"), 0, 0);
	prefab.addComponent<oak::AABB2dComponent>(true, glm::vec2{ 8.0f, 8.0f }, glm::vec2{ 0.0f, 0.0f });
	prefab.addComponent<oak::PhysicsBody2dComponent>(false, glm::vec2{ 0.0f }, glm::vec2{ 0.0f }, 0.025f * 0.2f, 0.4f, 0.2f, 0.1f);
	prefab.addComponent<oak::TextComponent>(true, std::hash<std::string>{}("fnt_dejavu"), std::hash<std::string>{}("txt_play"));

	auto& prefab0 = resManager.add<oak::Prefab>("block", &entityManager);
	prefab0.addComponent<oak::SpriteComponent>(true, std::hash<std::string>{}("spr_block"), 0, 0);
	prefab0.addComponent<oak::TransformComponent>(false, glm::vec3{ 256.0f, 512.0f, 0.0f }, 2.0f, glm::vec3{ 0.0f }, 0.0f);
	prefab0.addComponent<oak::AABB2dComponent>(true, glm::vec2{ 128.0f, 32.0f }, glm::vec2{ 0.0f, 0.0f });
	prefab0.addComponent<oak::PhysicsBody2dComponent>(false, glm::vec2{ 0.0f }, glm::vec2{ 0.0f }, 0.0f, 0.4f, 0.5f, 0.4f);

	auto& fab_button = resManager.add<oak::Prefab>("gui/button", &entityManager);
	fab_button.addComponent<oak::TransformComponent>(false, glm::vec3{ 0.0f }, 1.0f, glm::vec3{ 0.0f }, 0.0f);
	fab_button.addComponent<oak::SpriteComponent>(false, std::hash<std::string>{}("spr_button"), 0, 0);

	initBindings(L);
	//add the tile system to the oak global table
	lua_getglobal(L, "oak");
	oak::luah::pushValue(L, tileSystem);
	lua_setfield(L, -2, "ts");
	lua_pop(L, 1);

	//load main lua script
	oak::luah::loadScript(luam.getState(), "res/scripts/main.lua");
	
	//setup view matrix
	oak::graphics::UniformBufferObject uboData;
	uboData.proj = glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
	uboData.view = glm::mat4{ 1.0f };
	uboData.model = glm::mat4{ 1.0f };

	oak::graphics::GLBuffer ubo{ GL_UNIFORM_BUFFER };
	ubo.create();
	ubo.bindBufferBase(0);

	ubo.bind();
	ubo.bufferData(sizeof(oak::graphics::UniformBufferObject), &uboData, GL_STREAM_DRAW);
	ubo.unbind();

	//first frame time
	std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();
	//physics accum for frame independent physics
	float accum = 0.0f;
	//main game loop
	while (engine.isRunning()) {

		//emit update event in scripts (game logic)
		oak::luah::getGlobal(L, "oak.es.emit_event");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::pushValue(L, "update");
		oak::luah::pushValue(L, dt.count());
		oak::luah::call(L, 3, 0);

		oak::luah::getGlobal(L, "oak.es.process_events");
		oak::luah::getGlobal(L, "oak.es");
		oak::luah::call(L, 1, 0);

		//create / destroy / activate / deactivate entities 
		entityManager.update();

		collisionSystem.update();

		//update physics
		accum += dt.count();
		while (accum >= 1.0f/60.0f) {
			//physicsSystem.update(1.0f/60.0f);
			accum -= 1.0f/60.0f; 
		}

		//sumbit sprites and text to the entity renderer
		spriteSystem.update();
		textSystem.update();
		//render tiles
		tileSystem.render(mat_tiles);
		//render sprites
		entityRenderer.render();
		//swap buffers, clear buffer check for opengl errors
		frameRenderer.render();

		//poll input and check if the window should be closed
		window.update();

		//update the delta time
		std::chrono::high_resolution_clock::time_point currentFrame = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration_cast<std::chrono::duration<float>>(currentFrame - lastFrame);
		lastFrame = currentFrame;
	}

	//clean up
	engine.destroy();

	return 0;
}