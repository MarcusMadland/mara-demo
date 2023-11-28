
#include <mengine/mengine.h>
#include <mrender/entry.h>
#include <imgui/imgui.h>

namespace game
{
	struct PosColorVertex
	{
		float x;
		float y;
		float z;
		uint32_t abgr;
	};

	static PosColorVertex s_cubeVertices[] =
	{
		{-1.0f,  1.0f,  1.0f, 0xff000000 },
		{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
		{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
		{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
		{-1.0f,  1.0f, -1.0f, 0xffff0000 },
		{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
		{-1.0f, -1.0f, -1.0f, 0xffffff00 },
		{ 1.0f, -1.0f, -1.0f, 0xffffffff },
	};

	static const U16 s_cubeTriList[] =
	{
		0, 1, 2, 1, 3, 2, 4, 6, 5, 
		5, 6, 7, 0, 2, 4, 4, 2, 6,
		1, 5, 3, 5, 7, 3, 0, 4, 1, 
		4, 5, 1, 2, 3, 6, 6, 3, 7,
	};

	static const char* s_vertShaderCodeGLSL = R"(attribute vec4 a_color0;
		attribute vec3 a_position;
		varying vec4 v_color0;
		uniform mat4 u_modelViewProj;
		void main ()
		{
			vec4 tmpvar_1;
			tmpvar_1.w = 1.0;
			tmpvar_1.xyz = a_position;
			gl_Position = (u_modelViewProj * tmpvar_1);
			v_color0 = a_color0;
		})";

	static const char* s_fragShaderCodeGLSL = R"(varying vec4 v_color0;
		void main ()
		{
		  gl_FragColor = v_color0;
		})";

	struct Position
	{
		Position(float _x, float _y)
			: x(_x), y(_y)
		{}

		float x, y;
	};
#define POSITION UINT32_C(0b00000001)

	struct Velocity
	{
		Velocity(float _x, float _y)
			: x(_x), y(_y)
		{}

		float x, y;
	};
#define VELOCITY UINT32_C(0b00000010)

	void moveSystem()
	{
		mengine::forEachComponent(POSITION | VELOCITY, [](mengine::EntityHandle _handle)
			{
				Position* pos = (Position*)mengine::getComponentData(_handle, POSITION);
				Velocity* vel = (Velocity*)mengine::getComponentData(_handle, VELOCITY);
			});
	}

	class Game : public mrender::AppI
	{
	public:
		Game(const char* _name, const char* _description)
			: mrender::AppI(_name, _description)
		{}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			m_width = _width;
			m_height = _height;
			m_debug = BGFX_DEBUG_TEXT;
			m_reset = BGFX_RESET_VSYNC;

			mengine::Init mengineInit;
			mengineInit.graphicsApi = bgfx::RendererType::OpenGL;
			mengineInit.resolution.width = _width;
			mengineInit.resolution.height = _height;
			mengine::init(mengineInit);

#if 0 
			bgfx::VertexLayout layout;
			layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
				.end();

			auto geom = mengine::createGeometry(s_cubeVertices, sizeof(PosColorVertex) * 8, s_cubeTriList, sizeof(U16) * 36, layout,
				"meshes/cube.bin");
			auto vert = mengine::createShader(mengine::compileShader(s_vertShaderCodeGLSL, mengine::ShaderType::Vertex),
				"shaders/vs_cube.bin");
			auto frag = mengine::createShader(mengine::compileShader(s_fragShaderCodeGLSL, mengine::ShaderType::Fragment),
				"shaders/fs_cube.bin");

			mengine::packAssets("data/assets.pak");

			mengine::destroy(geom);
			mengine::destroy(vert);
			mengine::destroy(frag);
#endif // COMPILE_ASSETS

			// Other
			m_timeOffset = bx::getHPCounter();

			// Renderer
			bgfx::setDebug(m_debug);
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030FF
				, 1.0f
				, 0
			);

			// Load scene
			mengine::loadAssetPack("data/assets.pak");

			m_gah = mengine::loadGeometry("meshes/cube.bin");
			m_vah = mengine::loadShader("shaders/vs_cube.bin");
			m_fah = mengine::loadShader("shaders/fs_cube.bin");

			// Program
			m_program = bgfx::createProgram(m_vah, m_fah);

			// Imgui
			mengine::imguiCreate();

			// ECS
			mengine::EntityHandle entity1 = mengine::createEntity();
			mengine::addComponent(entity1, POSITION, mengine::createComponent(new Position(0.111f, 0.111f), sizeof(Position)));
			mengine::addComponent(entity1, VELOCITY, mengine::createComponent(new Velocity(1.0f, 1.0f), sizeof(Velocity)));

			mengine::EntityHandle entity2 = mengine::createEntity();
			mengine::addComponent(entity2, POSITION, mengine::createComponent(new Position(0.222f, 0.222f), sizeof(Position)));
			//mengine::addComponent(entity2, VELOCITY, mengine::createComponent(new Velocity(2.0f, 2.0f), sizeof(Velocity)));

			// Shutdown
			//mengine::destroy(entity1);
			//mengine::destroy(entity2);
		}

		int shutdown() override
		{
			mengine::imguiDestroy();

			bgfx::destroy(m_program);

			mengine::destroy(m_gah); 
			mengine::destroy(m_vah);
			mengine::destroy(m_fah);

			mengine::shutdown();

			return 0;
		}

		bool update() override
		{
			if (!mrender::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
			{
				const mengine::Stats* stats = mengine::getStats();

				// ImGui
				mengine::imguiBeginFrame(m_mouseState.m_mx
					, m_mouseState.m_my
					, (m_mouseState.m_buttons[mrender::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
					| (m_mouseState.m_buttons[mrender::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
					| (m_mouseState.m_buttons[mrender::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
					, m_mouseState.m_mz
					, U16(m_width)
					, U16(m_height));

				ImGui::Begin("Stats");
				ImGui::Text("Num Entities: %u", stats->numEntities);
				ImGui::Text("Num Components: %u", stats->numComponents);
				ImGui::Separator();
				ImGui::Text("Num Geometry Assets: %u", stats->numGeometryAssets);
				ImGui::Text("Num Shader Assets: %u", stats->numShaderAssets);
				ImGui::End();

				mengine::imguiEndFrame();

				// Time
				F32 time = (F32)((bx::getHPCounter() - m_timeOffset) / F64(bx::getHPFrequency()));

				// Debug
				bgfx::dbgTextClear();
				bgfx::dbgTextPrintf(2, 1, 0x0f, "Testing font..");

				// Camera
				const bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
				const bx::Vec3 eye = { 0.0f, 2.0f, -5.0f };
				{
					float view[16];
					bx::mtxLookAt(view, eye, at);

					float proj[16];
					bx::mtxProj(proj, 60.0f, F32(m_width) / F32(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
					//bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
					bgfx::setViewTransform(0, view, proj);

					bgfx::setViewRect(0, 0, 0, U16(m_width), U16(m_height));
				}

				// Systems
				moveSystem();

				// Mesh
				F32 rot[16];
				bx::mtxRotateXY(rot
					, time * 0.37f
					, time * 0.37f
				);

				F32 scal[16];
				bx::mtxScale(scal, 1.f, 1.f, 1.f);

				F32 mtx[16];
				bx::mtxMul(mtx, rot, scal);
				bgfx::setTransform(mtx);
			
				bgfx::setGeometry(m_gah);

				U64 state = 0 | BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_MSAA;
				bgfx::setState(state);

				bgfx::submit(0, m_program);
				
				mengine::update();


				return true;
			}

			return false;
		}

		I64 m_timeOffset;

		mrender::MouseState m_mouseState;

		U32 m_width;
		U32 m_height;
		U32 m_debug;
		U32 m_reset;

		mengine::GeometryAssetHandle m_gah;
		mengine::ShaderAssetHandle m_vah;
		mengine::ShaderAssetHandle m_fah;

		bgfx::ProgramHandle m_program;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	game::Game
	, "Game"
	, "An example of a game project"
);