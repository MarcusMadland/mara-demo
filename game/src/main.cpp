#include "imgui/imgui.h"
#include "mengine/mengine.h"

#include "mrender/entry.h"

namespace
{

class Game : public mrender::AppI
{
public:
	Game(const char* _name, const char* _description)
		: mrender::AppI(_name, _description)
	{}

	void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
	{
		mengine::Args args(_argc, _argv);

		m_width = _width;
		m_height = _height;
		m_debug = BGFX_DEBUG_TEXT;
		m_reset = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type = args.m_type;
		init.type = bgfx::RendererType::OpenGL; // @todo currently force opengl since we only have glsl compiler for now
		init.vendorId = args.m_pciId;
		init.resolution.width = _width;
		init.resolution.height = _height;
		init.platformData.nwh = mrender::getNativeWindowHandle(mrender::kDefaultWindowHandle);
		init.platformData.ndt = mrender::getNativeDisplayHandle();
		bgfx::init(init);

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

		// Program
		m_vertexShader = new mengine::ShaderAsset();
		m_fragmentShader = new mengine::ShaderAsset();
		if (m_vertexShader->deserialize("shaders/vs_mesh.bin") &&
			m_fragmentShader->deserialize("shaders/fs_mesh.bin"))
		{
			bgfx::ShaderHandle vsh = bgfx::createShader(m_vertexShader->m_memory);
			bgfx::ShaderHandle fsh = bgfx::createShader(m_fragmentShader->m_memory);
			m_program = bgfx::createProgram(vsh, fsh, true);
		}

		// Geometry
		m_geometry = new mengine::GeometryAsset();
		if (m_geometry->deserialize("meshes/katana.bin"))
		{
			bgfx::VertexLayout layout;
			layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.end();

			m_vbh = bgfx::createVertexBuffer(m_geometry->m_vertexMemory, layout);
			m_ibh = bgfx::createIndexBuffer(m_geometry->m_indexMemory);
		}

		// Texture
		m_texture = new mengine::TextureAsset();
		if (m_texture->deserialize("textures/katana_albedo.bin"))
		{
			m_th = bgfx::createTexture2D(4096, 4096, false, 1, bgfx::TextureFormat::RGB8, BGFX_TEXTURE_NONE, m_texture->m_memory);
		}

		// Shader uniforms
		s_texAlbedo = bgfx::createUniform("s_texAlbedo", bgfx::UniformType::Sampler);

		// Imgui
		mengine::imguiCreate();
	}

	int shutdown() override
	{
		mengine::imguiDestroy();

		bgfx::destroy(m_vbh);
		bgfx::destroy(m_ibh);
		bgfx::destroy(m_program);

		delete m_vertexShader;
		delete m_fragmentShader;
		delete m_geometry;
		delete m_texture;

		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!mrender::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
		{
			static bool imgui_showGeometry = false;

			mengine::imguiBeginFrame(m_mouseState.m_mx
				, m_mouseState.m_my
				, (m_mouseState.m_buttons[mrender::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
				| (m_mouseState.m_buttons[mrender::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
				| (m_mouseState.m_buttons[mrender::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				, m_mouseState.m_mz
				, U16(m_width)
				, U16(m_height)
			);
			{
				if (ImGui::Begin("Settings"))
				{
					ImGui::Checkbox("Show Geometry", &imgui_showGeometry);

				}
				ImGui::End();
			}
			mengine::imguiEndFrame();

			bgfx::setViewRect(0, 0, 0, U16(m_width), U16(m_height));

			bgfx::touch(0);

			// Time
			F32 time = (F32)((bx::getHPCounter() - m_timeOffset) / F64(bx::getHPFrequency()));

			// Debug
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(2, 1, 0x0f, "Hello world! Testing font..");

			// Camera
			const bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
			const bx::Vec3 eye = { 0.0f, 2.0f, -5.0f };
			{
				float view[16];
				bx::mtxLookAt(view, eye, at);

				float proj[16];
				bx::mtxProj(proj, 60.0f, F32(m_width) / F32(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
				bgfx::setViewTransform(0, view, proj);

				bgfx::setViewRect(0, 0, 0, U16(m_width), U16(m_height));
			}

			// Mesh
			F32 rot[16];
			bx::mtxRotateXY(rot
				, time * 0.37f
				, time * 0.37f
			);

			F32 scal[16];
			//bx::mtxScale(scal, 0.05f, 0.05f, 0.05f);
			//bx::mtxScale(scal, 1.0f, 1.0f, 1.0f);
			bx::mtxScale(scal, 4.f, 4.f, 4.f);

			F32 mtx[16];
			bx::mtxMul(mtx, rot, scal);
			bgfx::setTransform(mtx);
			
			bgfx::setVertexBuffer(0, m_vbh);
			bgfx::setIndexBuffer(m_ibh);

			bgfx::setTexture(0, s_texAlbedo, m_th); 

			U64 state = 0 | BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA;

			bgfx::setState(imgui_showGeometry ? state | BGFX_STATE_PT_LINESTRIP : state);

			bgfx::submit(0, m_program);

			// Swap buffers
			bgfx::frame();

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

	bgfx::ProgramHandle m_program;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	bgfx::UniformHandle s_texAlbedo;
	bgfx::TextureHandle m_th;


	mengine::ShaderAsset* m_vertexShader;
	mengine::ShaderAsset* m_fragmentShader;
	mengine::GeometryAsset* m_geometry;
	mengine::TextureAsset* m_texture;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	Game
	, "Game"
	, "An example of a game project"
);