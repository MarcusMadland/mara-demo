#include <mengine/mengine.h>

#include <mrender/entry.h>
#include <mrender/input.h>

#include <mapp/os.h>

namespace 
{
	static bool s_isRunning;

	// Asset Compiling / Asset Packing
	void compileAssets()
	{
		struct Vertex
		{
			float x;
			float y;
		};

		Vertex quadVertices[] =
		{
			{-1.0f,  1.0f },
			{ 1.0f,  1.0f },
			{-1.0f, -1.0f },
			{ 1.0f, -1.0f }
		};

		const U16 quadTriList[] =
		{
			0, 1, 2,
			1, 3, 2
		};

		bgfx::VertexLayout layout;
		layout.begin()
			.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
			.end();

		mengine::GeometryAssetHandle geom = mengine::createGeometry(quadVertices, sizeof(Vertex) * 4, quadTriList, sizeof(U16) * 6, layout,
			"meshes/cube.bin");

		mengine::ShaderAssetHandle vert = mengine::createShader(mengine::compileShader(R"(
			attribute vec2 a_position;
			uniform mat4 u_modelViewProj;
			void main ()
			{
				vec4 tmpvar_1;
				tmpvar_1.w = 1.0;
				tmpvar_1.z = 0.0;
				tmpvar_1.xy = a_position;
				gl_Position = (u_modelViewProj * tmpvar_1);
			}
		)", mengine::ShaderType::Vertex), "shaders/vs_cube.bin");

		mengine::ShaderAssetHandle frag = mengine::createShader(mengine::compileShader(R"(
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
			, m_scale(_scale)
		{}

		bx::Vec3 m_position;
		bx::Vec3 m_scale;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_CAMERA)
	struct CameraComponent
	{
		CameraComponent()
			: m_position({0.0f, 0.0f, -5.0f})
			, m_lookAt({ 0.0f, 0.0f, 0.0f })
		{}
		
		bx::Vec3 m_position;
		bx::Vec3 m_lookAt;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_EVENT)
	struct EventComponent
	{
		EventComponent(U32 _width, U32 _height, U32 _debug, U32 _reset)
			: m_width(_width)
			, m_height(_height)
			, m_debug(_debug)
			, m_reset(_reset)
		{}

		mrender::WindowState m_windowState;
		mrender::MouseState m_mouseState;

		U32 m_width;
		U32 m_height;
		U32 m_debug;
		U32 m_reset;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_INPUT)
	struct InputComponent
	{
		InputComponent(U32 _controllerIndex)
			: m_controllerIndex(_controllerIndex)
		{}

		U32 m_controllerIndex;
	};

	MENGINE_DEFINE_COMPONENT(COMPONENT_PHYSICS)
	struct PhysicsComponent
	{
		PhysicsComponent(const bx::Vec3 _velocity)
			: m_velocity(_velocity)
		{}

		bx::Vec3 m_velocity;
	};

	// Game State
	struct GameState
	{
		U16 score_p1 = 0;
		U16 score_p2 = 0;
	};
	static GameState s_gameState;

	// Systems
	void renderSystem(float _dt)
	{
		mengine::EntityQuery qr = mengine::queryEntities(COMPONENT_MESH | COMPONENT_TRANSFORM);
		for (U32 i = 0; i < qr.m_count; i++)
		{
			MeshComponent* mesh = (MeshComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_MESH);
			TransformComponent* transform = (TransformComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_TRANSFORM);

			const U64 state = 0 | BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA;
			bgfx::setState(state);

			F32 translation[16];
			bx::mtxTranslate(translation, transform->m_position.x, transform->m_position.y, transform->m_position.z);

			F32 scale[16];
			bx::mtxScale(scale, transform->m_scale.x, transform->m_scale.y, transform->m_scale.z);
				
			F32 mtx[16];
			bx::mtxMul(mtx, scale, translation);

			bgfx::setTransform(mtx);
			bgfx::setGeometry(mesh->m_gah);

			bgfx::submit(0, mesh->m_ph);
		}
	}

	void cameraSystem(float _dt)
	{
		mengine::EntityQuery qr = mengine::queryEntities(COMPONENT_CAMERA | COMPONENT_EVENT);
		for (U32 i = 0; i < qr.m_count; i++)
		{
			CameraComponent* camera = (CameraComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_CAMERA);
			EventComponent* event = (EventComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_EVENT);

			F32 aspectRatio = float(event->m_width) / float(event->m_height);

			F32 view[16];
			bx::mtxLookAt(view, camera->m_position, camera->m_lookAt);

			F32 proj[16];
			bx::mtxOrtho(proj, -aspectRatio, aspectRatio, -1.0f, 1.0f, 0.1f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

			bgfx::setViewTransform(0, view, proj);
		};
	}

	void inputSystem(float _dt)
	{
		const float paddelSpeed = 0.001f * _dt;

		mengine::EntityQuery qr = mengine::queryEntities(COMPONENT_EVENT | COMPONENT_INPUT | COMPONENT_TRANSFORM);
		for (U32 i = 0; i < qr.m_count; i++)
		{
			EventComponent* event = (EventComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_EVENT);
			InputComponent* input = (InputComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_INPUT);
			TransformComponent* velocity = (TransformComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_TRANSFORM);

			bgfx::setDebug(event->m_debug);

			if (!mrender::processEvents(event->m_width, event->m_height, event->m_debug, event->m_reset, &event->m_mouseState))
			{
				bgfx::setViewRect(0, 0, 0, U16(event->m_width), U16(event->m_height));

				uint8_t modifiers;

				if (inputGetKeyState(mrender::Key::Esc, &modifiers))
				{
					s_isRunning = false;
				}

				mrender::Key::Enum upKey = input->m_controllerIndex == 0 ? mrender::Key::KeyW : mrender::Key::Up;
				mrender::Key::Enum downKey = input->m_controllerIndex == 0 ? mrender::Key::KeyS : mrender::Key::Down;
					
				if (inputGetKeyState(upKey, &modifiers))
				{
					velocity->m_position.y += paddelSpeed;
				}
				else if (inputGetKeyState(downKey, &modifiers))
				{
					velocity->m_position.y -= paddelSpeed;
				}
			}
			else
			{
				s_isRunning = false;
			}
		}
	}

	void physicsSystem(float _dt)
	{
		const float ballSpeed = 0.002f * _dt;

		mengine::EntityQuery qr = mengine::queryEntities(COMPONENT_EVENT | COMPONENT_TRANSFORM | COMPONENT_PHYSICS);
		for (U32 i = 0; i < qr.m_count; i++)
		{
			EventComponent* event = (EventComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_EVENT);
			TransformComponent* ballTransform = (TransformComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_TRANSFORM);
			PhysicsComponent* ballVelocity = (PhysicsComponent*)mengine::getComponentData(qr.m_entities[i], COMPONENT_PHYSICS);

			static float time = 0;
			if (time > 4000.0f && bx::length(ballVelocity->m_velocity) <= 0.0001f)
			{
				U64 dtAsInt = (U64)(_dt * 100000000); // @todo what even is this
				U64 flip = dtAsInt % 4;
				switch(flip)
				{
				case 0:
					ballVelocity->m_velocity = { 0.002f, 0.002f, 0.0f };
					break;

				case 1:
					ballVelocity->m_velocity = { 0.002f, -0.002f, 0.0f };
					break;

				case 2:
					ballVelocity->m_velocity = { -0.002f, -0.002f, 0.0f };
					break;

				case 3:
					ballVelocity->m_velocity = { -0.002f, 0.002f, 0.0f };
					break;
				}
				
			}
			time += _dt;

			bx::Vec3 calculatedBallPosition = ballTransform->m_position;
			calculatedBallPosition.x += ballVelocity->m_velocity.x;
			calculatedBallPosition.y += ballVelocity->m_velocity.y;
			calculatedBallPosition.z += ballVelocity->m_velocity.z;

			F32 aspectRatio = float(event->m_width) / float(event->m_height);

			if (calculatedBallPosition.x < -aspectRatio)
			{
				s_gameState.score_p1 += 1;
				ballTransform->m_position = { 0.0f, 0.0f, 0.0f };
				ballVelocity->m_velocity = { 0.0f, 0.0f, 0.0f };
				time = 0.0f;
				return;
			}
			if (calculatedBallPosition.x > aspectRatio)
			{
				s_gameState.score_p2 += 1;
				ballTransform->m_position = { 0.0f, 0.0f, 0.0f };
				ballVelocity->m_velocity = { 0.0f, 0.0f, 0.0f };
				time = 0.0f;
				return;
			}

			if (calculatedBallPosition.y < -1.0f || calculatedBallPosition.y > 1.0f)
			{
				ballVelocity->m_velocity.y *= -1.0f;
			}

			mengine::EntityQuery qre = mengine::queryEntities(COMPONENT_TRANSFORM);
			for (U32 j = 0; j < qre.m_count; j++)
			{
				if (qr.m_entities[i].idx == qre.m_entities[j].idx)
				{
					continue;
				}

				TransformComponent* playerTransform = (TransformComponent*)mengine::getComponentData(qre.m_entities[j], COMPONENT_TRANSFORM);

				if (calculatedBallPosition.x + playerTransform->m_scale.x > playerTransform->m_position.x - playerTransform->m_scale.x &&
					calculatedBallPosition.x - playerTransform->m_scale.x < playerTransform->m_position.x + playerTransform->m_scale.x &&
					calculatedBallPosition.y + playerTransform->m_scale.y > playerTransform->m_position.y - playerTransform->m_scale.y &&
					calculatedBallPosition.y - playerTransform->m_scale.y < playerTransform->m_position.y + playerTransform->m_scale.y)
				{
					F32 overlapX = bx::min(calculatedBallPosition.x + playerTransform->m_scale.x - playerTransform->m_position.x, playerTransform->m_position.x + playerTransform->m_scale.x - calculatedBallPosition.x);
					F32 overlapY = bx::min(calculatedBallPosition.y + playerTransform->m_scale.y - playerTransform->m_position.y, playerTransform->m_position.y + playerTransform->m_scale.y - calculatedBallPosition.y);

					if (overlapX < overlapY)
					{
						ballVelocity->m_velocity.x *= -1.0f;
					}
					else
					{
						ballVelocity->m_velocity.y *= -1.0f;
					}
					break;
				}
			}

			bx::Vec3 finalBallPosition = ballTransform->m_position;
			finalBallPosition.x += ballVelocity->m_velocity.x;
			finalBallPosition.y += ballVelocity->m_velocity.y;
			finalBallPosition.z += ballVelocity->m_velocity.z;
			ballTransform->m_position = finalBallPosition;
		}
	}

	// Game
	class Game : public mrender::AppI
	{
	public:
		Game(const char* _name, const char* _description)
			: mrender::AppI(_name, _description)
			, m_game(MENGINE_INVALID_HANDLE)
			, m_player1(MENGINE_INVALID_HANDLE)
			, m_player2(MENGINE_INVALID_HANDLE)
			, m_ball(MENGINE_INVALID_HANDLE)
		{
			mrender::setWindowTitle(mrender::kDefaultWindowHandle, "PONG (mengine demo) | Marcus Madland");
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			mengine::Init mengineInit;
			mengineInit.graphicsApi = bgfx::RendererType::OpenGL;
			mengineInit.resolution.width = _width;
			mengineInit.resolution.height = _height;
			s_isRunning = mengine::init(mengineInit);

#if 1  // COMPILE_ASSETS
			compileAssets();
#endif // COMPILE_ASSETS

			// Load asset pack
			mengine::loadAssetPack("data/assets.pak");

			// Entity Camera
			m_game = mengine::createEntity();

			mengine::addComponent(m_game, COMPONENT_CAMERA, mengine::createComponent(
				new CameraComponent(), sizeof(CameraComponent)));

			mengine::addComponent(m_game, COMPONENT_EVENT, mengine::createComponent(
				new EventComponent(_width, _height, BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC), sizeof(EventComponent)));

			// Entity Player 1
			m_player1 = mengine::createEntity();

			mengine::addComponent(m_player1, COMPONENT_MESH, mengine::createComponent(new MeshComponent(
				mengine::loadGeometry("meshes/cube.bin"), 
				mengine::loadShader("shaders/vs_cube.bin"),
				mengine::loadShader("shaders/fs_cube.bin")), sizeof(MeshComponent)));

			mengine::addComponent(m_player1, COMPONENT_TRANSFORM, mengine::createComponent(
				new TransformComponent({ -1.5f, 0.0f, 0.0f }, { 0.02f, 0.1f, 0.0f }), sizeof(TransformComponent)));

			mengine::addComponent(m_player1, COMPONENT_EVENT, mengine::createComponent(
				new EventComponent(_width, _height, BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC), sizeof(EventComponent)));

			mengine::addComponent(m_player1, COMPONENT_INPUT, mengine::createComponent(
				new InputComponent(0), sizeof(InputComponent)));

			// Entity Player 2
			m_player2 = mengine::createEntity();

			mengine::addComponent(m_player2, COMPONENT_MESH, mengine::createComponent(new MeshComponent(
				mengine::loadGeometry("meshes/cube.bin"),
				mengine::loadShader("shaders/vs_cube.bin"),
				mengine::loadShader("shaders/fs_cube.bin")), sizeof(MeshComponent)));

			mengine::addComponent(m_player2, COMPONENT_TRANSFORM, mengine::createComponent(
				new TransformComponent({ 1.5f, 0.0f, 0.0f }, { 0.02f, 0.1f, 0.0f }), sizeof(TransformComponent)));

			mengine::addComponent(m_player2, COMPONENT_EVENT, mengine::createComponent(
				new EventComponent(_width, _height, BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC), sizeof(EventComponent)));

			mengine::addComponent(m_player2, COMPONENT_INPUT, mengine::createComponent(
				new InputComponent(1), sizeof(InputComponent)));

			// Entity Ball
			m_ball = mengine::createEntity();

			mengine::addComponent(m_ball, COMPONENT_MESH, mengine::createComponent(new MeshComponent(
				mengine::loadGeometry("meshes/cube.bin"),
				mengine::loadShader("shaders/vs_cube.bin"),
				mengine::loadShader("shaders/fs_cube.bin")), sizeof(MeshComponent)));

			mengine::addComponent(m_ball, COMPONENT_TRANSFORM, mengine::createComponent(
				new TransformComponent({ 0.0f, 0.0f, 0.0f }, { 0.02f, 0.02f, 0.0f }), sizeof(TransformComponent)));

			mengine::addComponent(m_ball, COMPONENT_PHYSICS, mengine::createComponent(
				new PhysicsComponent({ 0.0f, 0.0f, 0.0f}), sizeof(PhysicsComponent)));

			mengine::addComponent(m_ball, COMPONENT_EVENT, mengine::createComponent(
				new EventComponent(_width, _height, BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC), sizeof(EventComponent)));
		}

		int shutdown() override
		{
			mengine::destroy(m_game);
			mengine::destroy(m_player1);
			mengine::destroy(m_player2);
			mengine::destroy(m_ball);

			mengine::shutdown();

			return 0;
		}

		bool update() override
		{
			if (!s_isRunning)
			{
				return false;
			}

			I64 now = bx::getHPCounter();
			static I64 last = now;
			const I64 frameTime = now - last;
			last = now;
			const F64 freq = F64(bx::getHPFrequency());
			const F64 toMs = 1000.0 / freq;

			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(5, 2, 0x0F, "P1: %u", s_gameState.score_p1);
			bgfx::dbgTextPrintf(5, 4, 0x0F, "P2: %u", s_gameState.score_p2);

			// Systems
			inputSystem(frameTime * toMs);
			cameraSystem(frameTime * toMs);
			renderSystem(frameTime * toMs);
			physicsSystem(frameTime * toMs);

			mengine::update();

			return true;
		}

		mengine::EntityHandle m_game;
		mengine::EntityHandle m_player1;
		mengine::EntityHandle m_player2;
		mengine::EntityHandle m_ball;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	::Game
	, "Game"
	, "An example of a game project"
);