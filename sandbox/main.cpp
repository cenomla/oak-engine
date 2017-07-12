#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <graphics/opengl/gl_texture.h>
#include <graphics/opengl/gl_api.h>
#include <graphics/opengl/gl_buffer_storage.h>
#include <graphics/opengl/gl_buffer.h>
#include <graphics/opengl/gl_shader.h>
#include <graphics/opengl/gl_framebuffer.h>

#include <graphics/render_system.h>
#include <graphics/model.h>
#include <log.h>
#include <system_manager.h>
#include <resource_manager.h>
#include <event_manager.h>
#include <input_manager.h>
#include <resource.h>
#include <component_storage.h>
#include <core_components.h>
#include <input_events.h>
#include <scene_events.h>
#include <update_events.h>
#include <input.h>
#include <prefab.h>
#include <scene_utils.h>

#include "component_ext.h"
#include "deferred_renderer.h"
#include "gui_renderer.h"

#include <glad/glad.h>

oak::graphics::Buffer matrix_ubo;
oak::graphics::Buffer proj_ubo; 
oak::graphics::Buffer ortho_ubo;
oak::graphics::Buffer light_ubo;

oak::CursorMode cmode;

struct TransformComp {
	glm::mat4 transform{ 1.0f };
};

struct CameraComp {
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f };
};

void pup(oak::Puper& puper, TransformComp& comp, const oak::ObjInfo& info) {

}

void pup(oak::Puper& puper, CameraComp& comp, const oak::ObjInfo& info) {

}

class CameraSystem : public oak::System {
public:
	CameraSystem(oak::Scene& scene) : scene_{ &scene } {}

	void init() override {
		cache_.requireComponent<oak::PrefabComponent>();
		cache_.requireComponent<CameraComp>();
		cache_.requirePrefab(std::hash<oak::string>{}("player"));
	}

	void run() override {
		cache_.update(*scene_);

		auto& cs = oak::getComponentStorage<CameraComp>(*scene_);

		auto& inputManager = oak::InputManager::inst();

		for (const auto& entity : cache_.entities()) {
			auto& cc = oak::getComponent<CameraComp>(cs, entity);

			//temp camera
			//look
			
			if (cmode == oak::CursorMode::DISABLED) {
				for (const auto& evt : oak::EventManager::inst().getQueue<oak::CursorEvent>()) {
					//translate to zero
					cc.rotation.y -= glm::radians(evt.x - px) / 6.0f;
					cc.rotation.x -= glm::radians(evt.y - py) / 6.0f;

					if (cc.rotation.y >= glm::pi<float>() * 2.0f) {
						cc.rotation.y = 0.0f;
					}
					if (cc.rotation.x >= glm::pi<float>() * 2.0f) {
						cc.rotation.x = 0.0f;
					}
					px = evt.x;
					py = evt.y;

				}
			} else {
				for (const auto& evt : oak::EventManager::inst().getQueue<oak::CursorEvent>()) {
					px = evt.x;
					py = evt.y;
				}
			}
			//move
			
			if (inputManager.getAction("move_forward")) {
				cc.position += glm::quat{ glm::vec3{ 0.0f, cc.rotation.y, 0.0f } } * glm::vec3{ 0.0f, 0.0f, -1.0f };
			}
			if (inputManager.getAction("move_backward")) {
				cc.position += glm::quat{ glm::vec3{ 0.0f, cc.rotation.y, 0.0f } } * glm::vec3{ 0.0f, 0.0f, 1.0f };
			}
			if (inputManager.getAction("strafe_left")) {
				cc.position += glm::quat{ glm::vec3{ 0.0f, cc.rotation.y, 0.0f } } * glm::vec3{ -1.0f, 0.0f, 0.0f };
			}
			if (inputManager.getAction("strafe_right")) {
				cc.position += glm::quat{ glm::vec3{ 0.0f, cc.rotation.y, 0.0f } } * glm::vec3{ 1.0f, 0.0f, 0.0f };
			}
			if (inputManager.getAction("move_up")) {
				cc.position.y += 0.4f;
			}
			if (inputManager.getAction("move_down")) {
				cc.position.y -= 0.4f;
			}
			
			
		}
	}

private:
	oak::EntityCache cache_;
	oak::Scene *scene_;
	float px = 0.0f, py = 0.0f;

};

struct stdoutstream : public oak::log::Stream {
	void write(const void *source, size_t size) override {
		fwrite(source, 1, size, stdout);
	}
};

struct logfilestream : public oak::log::Stream {
	void open() {
		if (file == nullptr) {
			file = fopen("log", "w");
		}
	}

	void close() {
		if (file != nullptr) {
			fclose(file);
			file = nullptr;
		}
	}

	void write(const void *source, size_t size) override {
		if (file != nullptr) {
			fwrite(source, 1, size, file);
			fflush(file);
		}
	}

	FILE *file = nullptr;
};

bool isRunning = false;

int main(int argc, char** argv) {
	//setup log
	stdoutstream sos;
	logfilestream lfs;
	lfs.open();
	oak::log::cout.addStream(&sos);
	oak::log::cout.addStream(&lfs);
	oak::log::cwarn.addStream(&sos);
	oak::log::cwarn.addStream(&lfs);
	oak::log::cerr.addStream(&sos);
	oak::log::cerr.addStream(&lfs);

	//init engine managers
	oak::EventManager evtManager;
	oak::InputManager inputManager;
	oak::SystemManager sysManager;
	oak::ComponentTypeManager chs;
	oak::ResourceManager resManager;

	inputManager.bind("move_forward", oak::key::w, true);
	inputManager.bind("move_backward", oak::key::s, true);
	inputManager.bind("strafe_left", oak::key::a, true);
	inputManager.bind("strafe_right", oak::key::d, true);
	inputManager.bind("move_up", oak::key::space, true);
	inputManager.bind("move_down", oak::key::lshift, true);

	//add all events
	evtManager.addQueue<oak::EntityCreateEvent>();
	evtManager.addQueue<oak::EntityDestroyEvent>();
	evtManager.addQueue<oak::EntityActivateEvent>();
	evtManager.addQueue<oak::EntityDeactivateEvent>();
	evtManager.addQueue<oak::WindowCreateEvent>();
	evtManager.addQueue<oak::WindowCloseEvent>();
	evtManager.addQueue<oak::WindowResizeEvent>();
	evtManager.addQueue<oak::FrameSizeEvent>();
	evtManager.addQueue<oak::KeyEvent>();
	evtManager.addQueue<oak::ButtonEvent>();
	evtManager.addQueue<oak::CursorEvent>();
	evtManager.addQueue<oak::CursorModeEvent>();
	evtManager.addQueue<oak::TextEvent>();
	evtManager.addQueue<oak::TickEvent>();
	evtManager.addQueue<oak::SimulateEvent>();

	//create the scene
	oak::Scene scene;

	//create all the systems
	oak::graphics::GLApi gl_api;

	//create the rendering system
	oak::graphics::RenderSystem renderSystem{ scene, gl_api };

	//basic test renderer
	DeferredRenderer sceneRenderer;
	GuiRenderer guiRenderer;
	renderSystem.pushLayerBack(sceneRenderer);
	renderSystem.pushLayerBack(guiRenderer);

	//define vertex attribute layouts that will be used in the scene
	oak::graphics::AttributeLayout _3dlayout{ oak::vector<oak::graphics::AttributeType>{ 
		oak::graphics::AttributeType::POSITION,
		oak::graphics::AttributeType::NORMAL,
		oak::graphics::AttributeType::UV
	} };

	oak::graphics::AttributeLayout _2dlayout{ oak::vector<oak::graphics::AttributeType>{ 
		oak::graphics::AttributeType::POSITION2D,
		oak::graphics::AttributeType::UV
	} };

	oak::graphics::GLBufferStorage _3dstorage;
	oak::graphics::GLBufferStorage _2dstorage;

	renderSystem.batcher_.addBufferStorage(&_3dlayout, &_3dstorage);
	renderSystem.batcher_.addBufferStorage(&_2dlayout, &_2dstorage);
	

	CameraSystem cameraSystem{ scene };
	//add them to the system manager
	sysManager.addSystem(renderSystem, "render_system");
	sysManager.addSystem(cameraSystem, "camera_system");

	//create component type handles
	chs.addHandle<oak::EventComponent>("event");
	chs.addHandle<oak::PrefabComponent>("prefab");
	chs.addHandle<TransformComp>("transform");
	chs.addHandle<CameraComp>("camera");

	//create component storage
	oak::ComponentStorage eventStorage{ "event" };
	oak::ComponentStorage prefabStorage{ "prefab" };
	oak::ComponentStorage transformStorage{ "transform" };
	oak::ComponentStorage cameraStorage{ "camera" };

	//add component storage to scene
	scene.addComponentStorage(eventStorage);
	scene.addComponentStorage(prefabStorage);
	scene.addComponentStorage(transformStorage);
	scene.addComponentStorage(cameraStorage);

	//init the test renderer
	sceneRenderer.init();
	guiRenderer.init();

	//initialize the buffer storage
	_3dstorage.create(&_3dlayout);
	_2dstorage.create(&_2dlayout);

	//setup uniforms
	struct {
		glm::mat4 proj;
		glm::mat4 view;
	} matrix;
	matrix.proj = glm::perspective(70.0f, 1280.0f / 720.0f, 0.5f, 500.0f);

	struct {
		glm::mat4 invProj;
		glm::mat4 proj;
	} pmatrix;
	pmatrix.invProj = glm::inverse(matrix.proj);
	pmatrix.proj = matrix.proj;

	struct {
		glm::mat4 proj;
		glm::mat4 view;
	} omatrix;
	omatrix.view = glm::mat4{ 1.0f };
	omatrix.proj = glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);

	oak::graphics::BufferInfo bufferInfo;
	//create matrix buffer
	bufferInfo.type = oak::graphics::BufferType::UNIFORM;
	bufferInfo.hint = oak::graphics::BufferHint::STREAM;
	bufferInfo.base = 0;
	matrix_ubo = oak::graphics::GLBuffer::create(bufferInfo);
	//create proj invProj buffer
	bufferInfo.hint = oak::graphics::BufferHint::STATIC;
	bufferInfo.base = 2;
	proj_ubo = oak::graphics::GLBuffer::create(bufferInfo);
	oak::graphics::GLBuffer::bind(proj_ubo);
	oak::graphics::GLBuffer::data(proj_ubo, sizeof(pmatrix), &pmatrix);
	//create ortho matrix buffer
	bufferInfo.base = 1;
	ortho_ubo = oak::graphics::GLBuffer::create(bufferInfo);
	oak::graphics::GLBuffer::bind(ortho_ubo);
	oak::graphics::GLBuffer::data(ortho_ubo, sizeof(omatrix), &omatrix);

	//lights
	struct {
		glm::vec3 pos;
		float radius;
		glm::vec3 color;
		float padding = 0.0f;
	} lights[8];

	glm::vec3 lpos[8] = {
		{ 16.0f, 5.0f, 34.0f },
		{ 16.0f, 11.0f, 34.0f },
		{ 16.0f, 5.0f, 40.0f },
		{ 16.0f, 11.0f, 40.0f },
		{ 32.0f, 8.0f, 52.0f },
		{ 35.0f, 32.0f, 37.0f },
		{ 48.0f, 8.0f, 37.0f },
		{ 1.0f, 1.0f, 1.0f }
	};
	
	for (int i = 0; i < 8; i++) {
		lights[i].radius = 128.0f;
		lights[i].color = { 1.0f, 1.0f, 1.0f };
	}

	bufferInfo.hint = oak::graphics::BufferHint::STREAM;
	bufferInfo.base = 4;
	light_ubo = oak::graphics::GLBuffer::create(bufferInfo);
	
	//shader setup
	oak::graphics::ShaderInfo shaderInfo;
	shaderInfo.vertex = "core/graphics/shaders/deferred/geometry/vert.glsl";
	shaderInfo.fragment = "core/graphics/shaders/deferred/geometry/frag.glsl";
	auto& sh_geometry = resManager.add<oak::graphics::Shader>("sh_geometry", oak::graphics::GLShader::create(shaderInfo));
	shaderInfo.vertex = "core/graphics/shaders/forward/pass2d/vert.glsl";
	shaderInfo.fragment = "core/graphics/shaders/forward/pass2d/frag.glsl";
	auto& sh_pass2d = resManager.add<oak::graphics::Shader>("sh_pass2d", oak::graphics::GLShader::create(shaderInfo));
	//textures
	oak::graphics::TextureInfo textureInfo;
	textureInfo.minFilter = oak::graphics::TextureFilter::NEAREST;
	auto& tex_character = resManager.add<oak::graphics::Texture>("tex_character", oak::graphics::GLTexture::create("sandbox/res/textures/character.png", textureInfo));
	textureInfo.minFilter = oak::graphics::TextureFilter::LINEAR_MIP_NEAREST;
	textureInfo.magFilter = oak::graphics::TextureFilter::NEAREST;
	textureInfo.width = 4096;
	textureInfo.height = 4096;
	auto& colorAtlas = resManager.add<oak::graphics::TextureAtlas>("colorAtlas", 
		oak::graphics::GLTexture::createAtlas({ 
			"sandbox/res/textures/pbr_rust/color.png",
			"sandbox/res/textures/pbr_grass/color.png",
			"sandbox/res/textures/pbr_rock/color.png",
		}, textureInfo));
	auto& normalAtlas = resManager.add<oak::graphics::TextureAtlas>("normalAtlas",
		oak::graphics::GLTexture::createAtlas({
			"sandbox/res/textures/pbr_rust/normal.png",
			"sandbox/res/textures/pbr_grass/normal.png"
		},	textureInfo));
	textureInfo.format = oak::graphics::TextureFormat::BYTE_R;
	auto& metalAtlas = resManager.add<oak::graphics::TextureAtlas>("metalAtlas",
		oak::graphics::GLTexture::createAtlas({
			"sandbox/res/textures/pbr_rust/metalness.png",
			"sandbox/res/textures/pbr_grass/metalness.png",
			"sandbox/res/textures/pbr_rock/metalness.png"
		}, textureInfo));
	auto& roughAtlas = resManager.add<oak::graphics::TextureAtlas>("roughAtlas",
		oak::graphics::GLTexture::createAtlas({
			"sandbox/res/textures/pbr_rust/roughness.png",
			"sandbox/res/textures/pbr_grass/roughness.png",
			"sandbox/res/textures/pbr_rock/roughness.png"
		}, textureInfo));

	//materials
	auto& mat_box = resManager.add<oak::graphics::Material>("mat_box", &_3dlayout, &sh_geometry, &colorAtlas.texture, &roughAtlas.texture, &metalAtlas.texture);
	auto& mat_overlay = resManager.add<oak::graphics::Material>("mat_overlay", &_2dlayout, &sh_pass2d, &tex_character);

	//meshes
	auto& mesh_box = resManager.add<oak::graphics::Model>("model_box");
	mesh_box.load("sandbox/res/models/box.obj");
	mesh_box.setTextureRegion(colorAtlas.regions[2].second);

	auto& mesh_floor = resManager.add<oak::graphics::Mesh>("mesh_floor");

	oak::vector<oak::graphics::Mesh::Vertex> vertices = {
		{ glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 0.0f } },
		{ glm::vec3{ 0.0f, 0.0f, 64.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f } },
		{ glm::vec3{ 64.0f, 0.0f, 64.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 1.0f } },
		{ glm::vec3{ 64.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f } }
	};

	oak::vector<uint32_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	mesh_floor.setData(vertices, indices);
	mesh_floor.setTextureRegion(colorAtlas.regions[1].second);

	renderSystem.batcher_.addMesh(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 32.0f, 2.0f, 32.0f }), &mesh_box.getMeshes()[0], &mat_box, 0);
	renderSystem.batcher_.addMesh(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }), &mesh_floor, &mat_box, 0);

	//create entities
	oak::EntityId player = scene.createEntity();
	oak::addComponent<oak::PrefabComponent>(scene, player, std::hash<oak::string>{}("player"));
	oak::addComponent<TransformComp>(scene, player);
	oak::addComponent<CameraComp>(scene, player, glm::vec3{ 16.0f, 8.0f, 16.0f }, glm::vec3{ 0.0f, glm::pi<float>(), 0.0f });
	scene.activateEntity(player);

	//first frame time
	std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> dt;


	cmode = oak::CursorMode::NORMAL;
	oak::EventManager::inst().getQueue<oak::CursorModeEvent>().emit({ cmode });

	//main game loop
	isRunning = true;
	while (isRunning) {
		if (inputManager.getKey(oak::key::esc)) {
			if (cmode == oak::CursorMode::NORMAL) {
				cmode = oak::CursorMode::DISABLED;
			} else {
				cmode = oak::CursorMode::NORMAL;
			}

			evtManager.getQueue<oak::CursorModeEvent>().emit({ cmode });
			inputManager.setKey(oak::key::esc, 0);
		}
		inputManager.update();
		//create / destroy / activate / deactivate entities
		scene.update();
		//update view
		auto& cc = oak::getComponent<CameraComp>(scene, player);
		matrix.view = glm::mat4{ glm::quat{ cc.rotation } };
		matrix.view[3] = glm::vec4{ cc.position, 1.0f };
		matrix.view = glm::inverse(matrix.view); //move view instead of camera
		//update lights
		for (int i = 0; i < 8; i++) {
			lights[i].pos = glm::vec3{ matrix.view * glm::vec4{ lpos[i], 1.0f} };
		}

		//upload buffers
		oak::graphics::GLBuffer::bind(matrix_ubo);
		oak::graphics::GLBuffer::data(matrix_ubo, sizeof(matrix), &matrix);
		oak::graphics::GLBuffer::bind(light_ubo);
		oak::graphics::GLBuffer::data(light_ubo, sizeof(lights), &lights);
	

		cameraSystem.run();
		renderSystem.run();

		//check for exit
		if (!evtManager.getQueue<oak::WindowCloseEvent>().empty()) {
			isRunning = false; 
		}

		//update the delta time
		std::chrono::high_resolution_clock::time_point currentFrame = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration_cast<std::chrono::duration<float>>(currentFrame - lastFrame);
		lastFrame = currentFrame;

		printf("dt: %f\n", dt.count());

		//do engine things
		evtManager.clear();
		oak::oalloc_frame.clear();
	}

	//clean up
	scene.reset();

	lfs.close();

	return 0;
}
