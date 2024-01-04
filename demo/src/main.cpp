#include <mara/mara.h>

#include <graphics/entry.h>
#include <graphics/input.h>

#include <base/os.h>
#include <base/timer.h>

#include <imgui/imgui.h>

namespace 
{
	// Components
	MARA_DEFINE_COMPONENT(COMPONENT_PREFAB)
	struct PrefabComponent : mara::ComponentI
	{
		PrefabComponent()
			: m_ph(MARA_INVALID_HANDLE)
		{}

		virtual ~PrefabComponent() override
		{
			mara::destroy(m_ph);
		};

		mara::PrefabHandle m_ph;
	};

	MARA_DEFINE_COMPONENT(COMPONENT_TRANSFORM)
	struct TransformComponent : mara::ComponentI
	{
		TransformComponent()
			: m_position({0.0f, 0.0f, 0.0f})
			, m_rotation({ 0.0f, 0.0f, 0.0f })
			, m_scale({ 1.0f, 1.0f, 1.0f })
		{}

		virtual ~TransformComponent() override {};

		base::Vec3 m_position;
		base::Vec3 m_rotation;
		base::Vec3 m_scale;
	};

	MARA_DEFINE_COMPONENT(COMPONENT_CAMERA)
	struct CameraComponent : mara::ComponentI
	{
		CameraComponent()
			: m_position({ 0.0f, 2.0f, -5.0f })
			, m_direction({ 0.0f, 0.0f, 1.0f })
			, m_right({ 1.0f, 0.0f, 0.0f })
			, m_pitch(0.0f)
			, m_roll(0.0f)
			, m_yaw(0.0f)
			, m_speed(10.0f)
			, m_sensitivity(1.0f)
			, m_prevMouseX(0.0f)
			, m_prevMouseY(0.0f)
			, m_canRotate(false)
		{}

		virtual ~CameraComponent() override {};
		
		base::Vec3 m_position;
		base::Vec3 m_direction;
		base::Vec3 m_right;
		F32 m_pitch, m_roll, m_yaw;
		F32 m_speed, m_sensitivity;
		F32 m_prevMouseX, m_prevMouseY;
		bool m_canRotate;
	};

	// Systems
	void render(float _dt)
	{
		graphics::setViewClear(0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);

		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_PREFAB | COMPONENT_TRANSFORM);
		{
			// Render all loaded prefabs
			for (U32 i = 0; i < qr->m_count; i++)
			{
				PrefabComponent* prefab = (PrefabComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_PREFAB);
				TransformComponent* transform = (TransformComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_TRANSFORM);

				for (U16 i = 0; i < mara::getNumMeshes(prefab->m_ph); i++)
				{
					mara::MeshHandle* meshes = mara::getMeshes(prefab->m_ph);

					const U64 state = 0 | GRAPHICS_STATE_WRITE_RGB
						| GRAPHICS_STATE_WRITE_A
						| GRAPHICS_STATE_WRITE_Z
						| GRAPHICS_STATE_DEPTH_TEST_LESS
						| GRAPHICS_STATE_MSAA;
					graphics::setState(state);

					F32 entityMtx[16];
					base::mtxSRT(entityMtx
						, transform->m_scale.x, transform->m_scale.y, transform->m_scale.z
						, transform->m_rotation.x, transform->m_rotation.y, transform->m_rotation.z
						, transform->m_position.x, transform->m_position.y, transform->m_position.z);

					F32 meshMtx[16];
					mara::getMeshTransform(meshMtx, meshes[i]);

					F32 mtx[16];
					base::mtxMul(mtx, entityMtx, meshMtx);

					graphics::setTransform(mtx);

					graphics::submit(0, meshes[i]);
				}
			}

			// Make sure we still clear screen if nothing is loaded
			if (qr->m_count <= 0)
			{
				graphics::touch(0);
			}
		}
		base::free(entry::getAllocator(), qr);
	}

	void camera(float _dt)
	{
		static bool s_reset = true;

		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_CAMERA);
		for (U32 i = 0; i < qr->m_count; i++)
		{
			CameraComponent* camera = (CameraComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_CAMERA);
			const entry::MouseState* mouseState = mara::getMouseState();

			camera->m_canRotate = !!mouseState->m_buttons[entry::MouseButton::Left];
			if (camera->m_canRotate && !ImGui::GetIO().WantCaptureMouse)
			{
				if (s_reset)
				{
					camera->m_prevMouseX = (F32)mouseState->m_mx;
					camera->m_prevMouseY = (F32)mouseState->m_my;

					s_reset = false;
				}

				const F32 mouseDeltaX = (F32)(mouseState->m_mx - camera->m_prevMouseX) * 
					(camera->m_sensitivity * _dt);
				const F32 mouseDeltaY = -(F32)(mouseState->m_my - camera->m_prevMouseY) * 
					(camera->m_sensitivity * _dt);
				camera->m_prevMouseX = mouseState->m_mx;
				camera->m_prevMouseY = mouseState->m_my;

				camera->m_right = base::cross(camera->m_direction, { 0.0f, 1.0f, 0.0f });
				camera->m_right = base::normalize(camera->m_right);
				camera->m_yaw += mouseDeltaX;
				camera->m_pitch += mouseDeltaY;
				camera->m_pitch = base::clamp(camera->m_pitch, -base::kPi * 0.495f, base::kPi * 0.495f);

				const F32 cosPitch = base::cos(camera->m_pitch);
				const F32 sinPitch = base::sin(camera->m_pitch);
				const F32 cosYaw = base::cos(camera->m_yaw);
				const F32 sinYaw = base::sin(camera->m_yaw);

				camera->m_direction.x = sinYaw * cosPitch;
				camera->m_direction.y = sinPitch;
				camera->m_direction.z = cosYaw * cosPitch;

				camera->m_direction = base::normalize(camera->m_direction);
			}
			else
			{
				s_reset = true;
			}

			if (inputGetKeyState(entry::Key::KeyA, (U8*)0))
			{
				camera->m_position = base::add(camera->m_position, 
					base::mul(camera->m_right, camera->m_speed * _dt));
			}
			if (inputGetKeyState(entry::Key::KeyD, (U8*)0))
			{
				camera->m_position = base::sub(camera->m_position, 
					base::mul(camera->m_right, camera->m_speed * _dt));
			}
			if (inputGetKeyState(entry::Key::KeyW, (U8*)0))
			{
				camera->m_position = base::add(camera->m_position, 
					base::mul(camera->m_direction, camera->m_speed * _dt));
			}
			if (inputGetKeyState(entry::Key::KeyS, (U8*)0))
			{
				camera->m_position = base::sub(camera->m_position, 
					base::mul(camera->m_direction, camera->m_speed * _dt));
			}
			if (inputGetKeyState(entry::Key::KeyE, (U8*)0))
			{
				camera->m_position.y += camera->m_speed * _dt;
			}
			if (inputGetKeyState(entry::Key::KeyQ, (U8*)0))
			{
				camera->m_position.y -= camera->m_speed * _dt;
			}

			const graphics::Stats* renderStats = graphics::getStats();
			F32 aspectRatio = float(renderStats->width) / float(renderStats->height);

			F32 view[16];
			base::mtxLookAt(view, camera->m_position, base::add(camera->m_position, camera->m_direction));

			F32 proj[16];
			base::mtxProj(proj, 60.0f, aspectRatio, 0.01f, 10000.0f, graphics::getCaps()->homogeneousDepth);

			graphics::setViewTransform(0, view, proj);
		};
		base::free(entry::getAllocator(), qr);
	}

	void debugRender(float _dt)
	{
		constexpr U32 x = 40;
		constexpr U32 offset = 14;
		constexpr U8 color = 0xA;
		constexpr U32 plotLength = 500;

		constexpr F64 cpuLimit = 40.0;
		constexpr F64 gpuLimit = 25.0;
		constexpr F64 fpsLimit = 60.0;
		constexpr F64 texLimit = 1464.0f;

		const mara::Stats* stats = mara::getStats();
		const graphics::Stats* graphicsStats = graphics::getStats();

		static F64 cpu = 0.0f;
		static F64 cpuHighest = 0.0f;
		static F64 gpu = 0.0f;
		static F64 gpuHighest = 0.0f;
		static F64 framerate = 0.0f;
		static F64 texture = 0.0f;

		if (base::getHPCounter() % 10 == 0)
		{
			const F64 cpuToMs = 1000.0 / graphicsStats->cpuTimerFreq;
			cpu = F64(graphicsStats->cpuTimeFrame) * cpuToMs;
			if (cpu > cpuHighest) cpuHighest = cpu;

			const F64 gpuToMs = 1000.0 / graphicsStats->gpuTimerFreq;
			gpu = F64(graphicsStats->gpuTimeEnd - graphicsStats->gpuTimeBegin) * gpuToMs;
			if (gpu > gpuHighest) gpuHighest = gpu;

			framerate = 1 / _dt, _dt;
			texture = F64(graphicsStats->textureMemoryUsed) / (1024.0 * 1024.0);
		}

		ImGui::SetNextWindowPos({ (F32)graphicsStats->width - (230), 110 });
		ImGui::SetNextWindowSize({ 0, 20 });
		ImGui::Begin("Framerate Plot", (bool*)0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		{
			static float framerateData[plotLength] = { 0 }; // Assuming a fixed-size buffer
			for (int i = 0; i < plotLength-1; ++i) {
				framerateData[i] = framerateData[i + 1];
			}
			framerateData[plotLength-1] = framerate; // Add the new framerate value
			ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::PlotLines("##framerate", framerateData, IM_ARRAYSIZE(framerateData), 0, "", 0.0f, 120.0f, ImVec2(150, 20));
			ImGui::PopStyleColor();
		}
		ImGui::End();

		graphics::dbgTextClear();
		graphics::dbgTextPrintf(graphicsStats->textWidth - x, 2, cpuLimit < cpu ? 0x4 : color,			"cpu(game):");
		graphics::dbgTextPrintf(graphicsStats->textWidth - x, 3, color,									"cpu(render):");
		graphics::dbgTextPrintf(graphicsStats->textWidth - x, 4, gpuLimit < gpu ? 0x4 : color,			"gpu:");
		graphics::dbgTextPrintf(graphicsStats->textWidth - x, 5, framerate < fpsLimit ? 0x4 : color,	"framerate:");
		graphics::dbgTextPrintf(graphicsStats->textWidth - x, 6, texLimit < texture ? 0x4 : color,		"textures:");

		graphics::dbgTextPrintf(graphicsStats->textWidth - (x - offset), 2, cpuLimit < cpu ? 0x4 : color,		"%.2f ms [%.2f ms]", cpu, cpuHighest);
		graphics::dbgTextPrintf(graphicsStats->textWidth - (x - offset), 3, color,								"00.00 ms [0.00 ms]");
		graphics::dbgTextPrintf(graphicsStats->textWidth - (x - offset), 4, gpuLimit < gpu ? 0x4 : color,		"%.2f ms [%.2f ms]", gpu, gpuHighest);
		graphics::dbgTextPrintf(graphicsStats->textWidth - (x - offset), 5, framerate < fpsLimit ? 0x4 : color, "%.2f fps", framerate);
		graphics::dbgTextPrintf(graphicsStats->textWidth - (x - offset), 6, texLimit < texture ? 0x4 : color,	"%.2f / %.0f MiB", texture, texLimit);

		ImGui::Begin("Development");

		if (ImGui::CollapsingHeader("Resources", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Num Paks: %u", stats->numPaks);
			ImGui::Text("Num Pak Entries: %u", stats->numPakEntries);

			ImGui::Separator();

			ImGui::Text("Num Entity Instances: %u", stats->numEntities);
			ImGui::Text("Num Component Instances: %u", stats->numComponents);
			ImGui::Text("Num Resources: %u", stats->numResources);
			ImGui::Text("Num Geometries: %u", stats->numGeometries);
			ImGui::Text("Num Shaders: %u", stats->numShaders);
			ImGui::Text("Num Textures: %u", stats->numTextures);
			ImGui::Text("Num Materials: %u", stats->numMaterials);
			ImGui::Text("Num Meshes: %u", stats->numMeshes);
			ImGui::Text("Num Prefabs: %u", stats->numPrefabs);
		}

		if (ImGui::CollapsingHeader("Scene Hierarchy"))
		{
			
		}
		
		ImGui::End();
	}

	// Game
	class Game : public entry::AppI
	{
	public:
		Game(const char* _name, const char* _description)
			: entry::AppI(_name, _description)
			, m_time(0)
			, m_camera(MARA_INVALID_HANDLE)
		{
			entry::setWindowTitle(entry::kDefaultWindowHandle, "Mara | Demo");
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			// Engine
			mara::Init maraInit;
			maraInit.graphicsApi = graphics::RendererType::Direct3D11;
			maraInit.resolution.width = _width;
			maraInit.resolution.height = _height;
			mara::init(maraInit);

			// ImGui
			mara::imguiCreate();

			// Load PAK
			mara::loadPak("C:/Users/marcu/Dev/mara-demo/demo/build/bin/data/assets.pak");

			// Camera
			m_camera = mara::createEntity();
			{
				CameraComponent* cameraComp = new CameraComponent();
				cameraComp->m_speed = 10.0f;
				mara::addComponent(m_camera, COMPONENT_CAMERA, mara::createComponent(cameraComp));
			}

			// Scene
			m_scene = mara::createEntity();
			{
				PrefabComponent* prefabComp = new PrefabComponent();
				prefabComp->m_ph = mara::createPrefab(mara::loadPrefab("scenes/scene.bin"));

				TransformComponent* transComp = new TransformComponent();
				transComp->m_position = { 0.0f, 0.0f, 0.0f };
				transComp->m_rotation = { 0.0f, 0.0f, 0.0f };
				transComp->m_scale = { 1.0f, 1.0f, 1.0f };

				mara::addComponent(m_scene, COMPONENT_PREFAB, mara::createComponent(prefabComp));
				mara::addComponent(m_scene, COMPONENT_TRANSFORM, mara::createComponent(transComp));
			}

			// Character
			m_character = mara::createEntity();
			{
				PrefabComponent* prefabComp = new PrefabComponent();
				prefabComp->m_ph = mara::createPrefab(mara::loadPrefab("characters/character.bin"));

				TransformComponent* transComp = new TransformComponent();
				transComp->m_position = { 10.0f, .5f, 0.0f };
				transComp->m_rotation = { 0.0f, 45.0f, 0.0f };
				transComp->m_scale = { 0.01f, 0.01f, 0.01f };

				
				mara::addComponent(m_character, COMPONENT_PREFAB, mara::createComponent(prefabComp));
				mara::addComponent(m_character, COMPONENT_TRANSFORM, mara::createComponent(transComp));
			}
		}

		int shutdown() override
		{
			// Camera
			mara::destroy(m_camera);

			// Scene
			mara::destroy(m_scene);

			// Character
			mara::destroy(m_character);

			// Unload PAK
			mara::unloadPak("C:/Users/marcu/Dev/mara-demo/demo/build/bin/data/assets.pak");

			// ImGui
			mara::imguiDestroy();

			// Engine
			mara::shutdown();

			return 0;
		}

		bool update() override
		{
			// Time
			const int64_t frameTime = base::getHPCounter() - m_time;
			m_time = base::getHPCounter();
			const F64 freq = F64(base::getHPFrequency());
			F32 deltaTime = F32(frameTime / freq);

			// Update
			if (mara::update(GRAPHICS_DEBUG_TEXT, GRAPHICS_RESET_VSYNC))
			{
				// Systems
				camera(deltaTime);
				render(deltaTime);

				mara::imguiBeginFrame();
				debugRender(deltaTime);
				mara::imguiEndFrame();

				// Swap buffers
				graphics::frame();

				return true;
			}
			
			return false;
		}

		I64 m_time;

		mara::EntityHandle m_camera;
		mara::EntityHandle m_scene;
		mara::EntityHandle m_character;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	::Game
	, "Game"
	, "An example of a game project"
)