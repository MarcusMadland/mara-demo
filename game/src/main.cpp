#include <mengine/mengine.h>

#include <mrender/entry.h>
#include <mrender/input.h>

#include <mapp/os.h>

#include <imgui/imgui.h>

namespace 
{
	// Asset Compiling / Asset Packing
	void compileAssets()
	{
		struct Vertex
		{
			float x;
			float y;
			float z;
		};

		Vertex quadVertices[] =
		{
			{-1.0f,  1.0f,  1.0f, },
			{ 1.0f,  1.0f,  1.0f, },
			{-1.0f, -1.0f,  1.0f, },
			{ 1.0f, -1.0f,  1.0f, },
			{-1.0f,  1.0f, -1.0f, },
			{ 1.0f,  1.0f, -1.0f, },
			{-1.0f, -1.0f, -1.0f, },
			{ 1.0f, -1.0f, -1.0f, },
		};

		const U16 quadTriList[] =
		{
			0, 1, 2, 1, 3, 2, 4, 6, 5,
			5, 6, 7, 0, 2, 4, 4, 2, 6,
			1, 5, 3, 5, 7, 3, 0, 4, 1,
			4, 5, 1, 2, 3, 6, 6, 3, 7,
		};

		bgfx::VertexLayout layout;
		layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();

		auto geom = mengine::createGeometry(quadVertices, sizeof(Vertex) * 8, quadTriList, sizeof(U16) * 36, layout,
			"meshes/cube.bin");

		auto vert = mengine::createShader(mengine::compileShader(R"(
			attribute vec3 a_position;
			uniform mat4 u_modelViewProj;
			void main ()
			{
				vec4 tmpvar_1;
				tmpvar_1.w = 1.0;
				tmpvar_1.xyz = a_position;
				gl_Position = (u_modelViewProj * tmpvar_1);
			}
		)", mengine::ShaderType::Vertex), "shaders/vs_cube.bin");

		auto frag = mengine::createShader(mengine::compileShader(R"(
			void main ()
			{
				gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
			}
		)", mengine::ShaderType::Fragment), "shaders/fs_cube.bin");

		mengine::packAssets("data/assets.pak");

		mengine::destroy(geom);
		mengine::destroy(vert);
		mengine::destroy(frag);
	}

	// Components
	MENGINE_DEFINE_COMPONENT(COMPONENT_MESH)
	struct MeshComponent
	{
		MeshComponent(mengine::GeometryAssetHandle _handle,
			mengine::ShaderAssetHandle _vsah, mengine::ShaderAssetHandle _fsah)
			: m_gah(_handle)
		{
			m_ph = bgfx::createProgram(_vsah, _fsah);
		}

		mengine::GeometryAssetHandle m_gah;
		bgfx::ProgramHandle m_ph;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_TRANSFORM)
	struct TransformComponent
	{
		TransformComponent(const bx::Vec3& _pos, const bx::Vec3& _scale)
			: m_position(_pos)
			, m_rotation(0.0f, 0.0f, 0.0f)
			, m_scale(_scale)
		{}

		bx::Vec3 m_position;
		bx::Vec3 m_rotation;
		bx::Vec3 m_scale;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_CAMERA)
	struct CameraComponent
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
		
		bx::Vec3 m_position;
		bx::Vec3 m_direction;
		bx::Vec3 m_right;
		F32 m_pitch, m_roll, m_yaw;
		F32 m_speed, m_sensitivity;
		F32 m_prevMouseX, m_prevMouseY;
		bool m_canRotate;
	};

	// Systems
	void render(float _dt)
	{
		mengine::EntityQuery* qr = mengine::queryEntities(COMPONENT_MESH | COMPONENT_TRANSFORM);
		for (U32 i = 0; i < qr->m_count; i++)
		{
			MeshComponent* mesh = (MeshComponent*)mengine::getComponentData(qr->m_entities[i], COMPONENT_MESH);
			TransformComponent* transform = (TransformComponent*)mengine::getComponentData(qr->m_entities[i], COMPONENT_TRANSFORM);

			const U64 state = 0 | BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA;
			bgfx::setState(state);

			F32 translation[16];
			bx::mtxTranslate(translation, transform->m_position.x, transform->m_position.y, transform->m_position.z);

			F32 rotation[16];
			bx::mtxRotateXYZ(rotation, transform->m_rotation.x, transform->m_rotation.y, transform->m_rotation.z);

			F32 scale[16];
			bx::mtxScale(scale, transform->m_scale.x, transform->m_scale.y, transform->m_scale.z);

			F32 temp[16];
			F32 mtx[16];
			bx::mtxMul(temp, scale, translation);
			bx::mtxMul(mtx, temp, rotation);

			bgfx::setTransform(mtx);
			bgfx::setGeometry(mesh->m_gah);

			bgfx::submit(0, mesh->m_ph);
		}
		bx::free(mengine::getAllocator(), qr);
	}

	void camera(float _dt)
	{
		static bool s_reset = true;

		mengine::EntityQuery* qr = mengine::queryEntities(COMPONENT_CAMERA);
		for (U32 i = 0; i < qr->m_count; i++)
		{
			CameraComponent* camera = (CameraComponent*)mengine::getComponentData(qr->m_entities[i], COMPONENT_CAMERA);
			const mrender::MouseState* mouseState = mengine::getMouseState();

			camera->m_canRotate = !!mouseState->m_buttons[mrender::MouseButton::Left];
			if (camera->m_canRotate)
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

				camera->m_right = bx::cross(camera->m_direction, { 0.0f, 1.0f, 0.0f });
				camera->m_right = bx::normalize(camera->m_right);
				camera->m_yaw += mouseDeltaX;
				camera->m_pitch += mouseDeltaY;
				camera->m_pitch = bx::clamp(camera->m_pitch, -bx::kPi * 0.495f, bx::kPi * 0.495f);

				const F32 cosPitch = bx::cos(camera->m_pitch);
				const F32 sinPitch = bx::sin(camera->m_pitch);
				const F32 cosYaw = bx::cos(camera->m_yaw);
				const F32 sinYaw = bx::sin(camera->m_yaw);

				camera->m_direction.x = sinYaw * cosPitch;
				camera->m_direction.y = sinPitch;
				camera->m_direction.z = cosYaw * cosPitch;

				camera->m_direction = bx::normalize(camera->m_direction);
			}
			else
			{
				s_reset = true;
			}

			if (inputGetKeyState(mrender::Key::KeyA, (U8*)0))
			{
				camera->m_position = bx::add(camera->m_position, 
					bx::mul(camera->m_right, camera->m_speed * _dt));
			}
			if (inputGetKeyState(mrender::Key::KeyD, (U8*)0))
			{
				camera->m_position = bx::sub(camera->m_position, 
					bx::mul(camera->m_right, camera->m_speed * _dt));
			}
			if (inputGetKeyState(mrender::Key::KeyW, (U8*)0))
			{
				camera->m_position = bx::add(camera->m_position, 
					bx::mul(camera->m_direction, camera->m_speed * _dt));
			}
			if (inputGetKeyState(mrender::Key::KeyS, (U8*)0))
			{
				camera->m_position = bx::sub(camera->m_position, 
					bx::mul(camera->m_direction, camera->m_speed * _dt));
			}
			if (inputGetKeyState(mrender::Key::KeyE, (U8*)0))
			{
				camera->m_position.y += camera->m_speed * _dt;
			}
			if (inputGetKeyState(mrender::Key::KeyQ, (U8*)0))
			{
				camera->m_position.y -= camera->m_speed * _dt;
			}

			const bgfx::Stats* renderStats = bgfx::getStats();
			F32 aspectRatio = float(renderStats->width) / float(renderStats->height);

			F32 view[16];
			bx::mtxLookAt(view, camera->m_position, bx::add(camera->m_position, camera->m_direction));

			F32 proj[16];
			bx::mtxProj(proj, 60.0f, aspectRatio, 0.01f, 100.0f, bgfx::getCaps()->homogeneousDepth);

			bgfx::setViewTransform(0, view, proj);
		};
		bx::free(mengine::getAllocator(), qr);
	}

	void debugRender(float _dt)
	{
		mengine::imguiBeginFrame();

		constexpr U32 x = 40;
		constexpr U32 offset = 14;
		constexpr U8 color = 0xA;
		constexpr U32 plotLength = 500;

		constexpr F64 cpuLimit = 40.0;
		constexpr F64 gpuLimit = 25.0;
		constexpr F64 fpsLimit = 60.0;
		constexpr F64 texLimit = 1464.0f;

		const mengine::Stats* stats = mengine::getStats();
		const bgfx::Stats* bgfxStats = bgfx::getStats();

		static F64 cpu = 0.0f;
		static F64 cpuHighest = 0.0f;
		static F64 gpu = 0.0f;
		static F64 gpuHighest = 0.0f;
		static F64 framerate = 0.0f;
		static F64 texture = 0.0f;

		if (bx::getHPCounter() % 10 == 0)
		{
			const F64 cpuToMs = 1000.0 / bgfxStats->cpuTimerFreq;
			cpu = F64(bgfxStats->cpuTimeFrame) * cpuToMs;
			if (cpu > cpuHighest) cpuHighest = cpu;

			const F64 gpuToMs = 1000.0 / bgfxStats->gpuTimerFreq;
			gpu = F64(bgfxStats->gpuTimeEnd - bgfxStats->gpuTimeBegin) * gpuToMs;
			if (gpu > gpuHighest) gpuHighest = gpu;

			framerate = 1 / _dt, _dt;
			texture = F64(bgfxStats->textureMemoryUsed) / (1024.0 * 1024.0);
		}

		ImGui::SetNextWindowPos({ (F32)bgfxStats->width - (230), 110 });
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

		bgfx::dbgTextClear();
		bgfx::dbgTextPrintf(bgfxStats->textWidth - x, 2, cpuLimit < cpu ? 0x4 : color,			"cpu(game):");
		bgfx::dbgTextPrintf(bgfxStats->textWidth - x, 3, color,									"cpu(render):");
		bgfx::dbgTextPrintf(bgfxStats->textWidth - x, 4, gpuLimit < gpu ? 0x4 : color,			"gpu:");
		bgfx::dbgTextPrintf(bgfxStats->textWidth - x, 5, framerate < fpsLimit ? 0x4 : color,	"framerate:");
		bgfx::dbgTextPrintf(bgfxStats->textWidth - x, 6, texLimit < texture ? 0x4 : color,		"textures:");

		bgfx::dbgTextPrintf(bgfxStats->textWidth - (x - offset), 2, cpuLimit < cpu ? 0x4 : color,		"%.2f ms [%.2f ms]", cpu, cpuHighest);
		bgfx::dbgTextPrintf(bgfxStats->textWidth - (x - offset), 3, color,								"00.00 ms [0.00 ms]");
		bgfx::dbgTextPrintf(bgfxStats->textWidth - (x - offset), 4, gpuLimit < gpu ? 0x4 : color,		"%.2f ms [%.2f ms]", gpu, gpuHighest);
		bgfx::dbgTextPrintf(bgfxStats->textWidth - (x - offset), 5, framerate < fpsLimit ? 0x4 : color, "%.2f fps", framerate);
		bgfx::dbgTextPrintf(bgfxStats->textWidth - (x - offset), 6, texLimit < texture ? 0x4 : color,	"%.2f / %.0f MiB", texture, texLimit);

		ImGui::Begin("Development");
		if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Num Entity Instances: %u", stats->numEntities);
			ImGui::Text("Num Component Instances: %u", stats->numComponents);
			ImGui::Text("Num Geometry Assets: %u", stats->numGeometryAssets);
			ImGui::Text("Num Shader Assets: %u", stats->numShaderAssets);
		}
		ImGui::End();

		mengine::imguiEndFrame();
	}

	// Game
	class Game : public mrender::AppI
	{
	public:
		Game(const char* _name, const char* _description)
			: mrender::AppI(_name, _description)
			, m_time(0)
			, m_camera(MENGINE_INVALID_HANDLE)
			, m_cube(MENGINE_INVALID_HANDLE)
		{
			mrender::setWindowTitle(mrender::kDefaultWindowHandle, "3D (mengine demo) | Marcus Madland");
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			mengine::Init mengineInit;
			mengineInit.graphicsApi = bgfx::RendererType::OpenGL;
			mengineInit.resolution.width = _width;
			mengineInit.resolution.height = _height;
			mengine::init(mengineInit);
			mengine::imguiCreate();

#if 0  // COMPILE_ASSETS
			compileAssets();
#endif // COMPILE_ASSETS

			// Load asset pack
			mengine::loadAssetPack("data/assets.pak");

			{	// Entity Camera
				m_camera = mengine::createEntity();

				mengine::addComponent(m_camera, COMPONENT_CAMERA, mengine::createComponent(
					new CameraComponent(), sizeof(CameraComponent)));
			}
			
			{	// Entity Cube
				m_cube = mengine::createEntity();

				mengine::addComponent(m_cube, COMPONENT_MESH, mengine::createComponent(new MeshComponent(
					mengine::loadGeometry("meshes/cube.bin"),
					mengine::loadShader("shaders/vs_cube.bin"),
					mengine::loadShader("shaders/fs_cube.bin")), sizeof(MeshComponent)));

				mengine::addComponent(m_cube, COMPONENT_TRANSFORM, mengine::createComponent(
					new TransformComponent({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }), sizeof(TransformComponent)));
			}

			for (I32 x = -5; x < 5; x++)
			{
				for (I32 y = -5; y < 5; y++) 
				{
					for (I32 z = -5; z < 5; z++) 
					{
						const mengine::EntityHandle cube = mengine::createEntity();

						mengine::addComponent(cube, COMPONENT_MESH, mengine::createComponent(new MeshComponent(
							mengine::loadGeometry("meshes/cube.bin"),
							mengine::loadShader("shaders/vs_cube.bin"),
							mengine::loadShader("shaders/fs_cube.bin")), sizeof(MeshComponent)));

						mengine::addComponent(cube, COMPONENT_TRANSFORM, mengine::createComponent(
							new TransformComponent({ (F32)x, (F32)y , (F32)z }, { 0.3f, 0.3f, 0.3f }), sizeof(TransformComponent)));
					}
				}
			}
		}

		int shutdown() override
		{
			mengine::destroy(m_camera);
			mengine::destroy(m_cube);

			mengine::imguiDestroy();
			mengine::shutdown();

			return 0;
		}

		bool update() override
		{
			const int64_t frameTime = bx::getHPCounter() - m_time;
			m_time = bx::getHPCounter();
			const F64 freq = F64(bx::getHPFrequency());
			F32 deltaTime = F32(frameTime / freq);

			if (mengine::update(BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC))
			{
				camera(deltaTime);
				render(deltaTime);
				debugRender(deltaTime);

				bgfx::frame();

				return true;
			}
			
			return false;
		}

		I64 m_time;

		mengine::EntityHandle m_camera;
		mengine::EntityHandle m_cube;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	::Game
	, "Game"
	, "An example of a game project"
);