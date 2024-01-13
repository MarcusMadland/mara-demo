#include <mara/mara.h>

#include <base/spring.h>

#include <graphics/entry.h>
#include <graphics/input.h>

#include <imgui/imgui.h>
#include <imgui/imgui_debug.h>
#include <stdio.h>

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
			, m_rotation({ 0.0f, 0.0f, 0.0f, 1.0f })
			, m_scale({ 1.0f, 1.0f, 1.0f })
		{}

		virtual ~TransformComponent() override {};

		base::Vec3 m_position;
		base::Quaternion m_rotation;
		base::Vec3 m_scale;
	};

	MARA_DEFINE_COMPONENT(COMPONENT_CAMERA)
	struct CameraComponent : mara::ComponentI
	{
		CameraComponent()
			: m_position({ 0.0f, 0.0f, 0.0f })
			, m_lookAt({ 0.0f, 0.0f, 0.0f })
			, m_forward({ 0.0f, 0.0f, 1.0f })
			, m_right({ 1.0f, 0.0f, 0.0f })
			, m_offset({ 0.0f, 0.0f, 0.0f })
			, m_pitch(0.0f)
			, m_roll(0.0f)
			, m_yaw(0.0f)
			, m_speed(1.0f)
			, m_sensitivity(0.0025f)
			, m_armLength(5.0f)
			, m_fov(60.0f)
			, m_far(5000.0f)
			, m_near(0.01f)
			, m_isFree(false)
			, m_isActive(false)
		{}

		virtual ~CameraComponent() override {};
		
		base::Vec3 m_position;
		base::Vec3 m_lookAt;
		base::Vec3 m_forward;
		base::Vec3 m_right;
		base::Vec3 m_offset;
		F32 m_pitch, m_roll, m_yaw;
		F32 m_speed, m_sensitivity;
		F32 m_armLength;
		F32 m_fov;
		F32 m_far, m_near;
		bool m_isFree;
		bool m_isActive;
	};

	MARA_DEFINE_COMPONENT(COMPONENT_MOVEMENT)
	struct MovementComponent : mara::ComponentI
	{
		MovementComponent()
			: velocity({ 0.0f, 0.0f, 0.0f })
			, acceleration({ 0.0f, 0.0f, 0.0f })
			, desiredVelocity({ 0.0f, 0.0f, 0.0f })
			, positionHalflife(0.8f)
			, speed(5.0f)
		{}

		virtual ~MovementComponent() override {};

		base::Vec3 velocity;
		base::Vec3 acceleration;
		base::Vec3 desiredVelocity;
		F32 positionHalflife;
		F32 speed;
	};

	MARA_DEFINE_COMPONENT(COMPONENT_TRAJECTORY)
	struct TrajectoryComponent : mara::ComponentI
	{
		TrajectoryComponent()
			: trajMax(1600)
			, trajSub(200)
			, predMax(4)
			, predSub(250)
		{
			trajxPrev = new F32[trajMax];
			trajyPrev = new F32[trajMax];
			predx = new F32[predMax];
			predy = new F32[predMax];
			predxv = new F32[predMax];
			predyv = new F32[predMax];
			predxa = new F32[predMax];
			predya = new F32[predMax];
		}

		virtual ~TrajectoryComponent() override 
		{
			delete[] trajxPrev;
			delete[] trajyPrev;
			delete[] predx;
			delete[] predy;
			delete[] predxv;
			delete[] predyv;
			delete[] predxa;
			delete[] predya;
		};

		U32 trajMax;
		U32 trajSub;
		U32 predMax;
		U32 predSub;

		F32* trajxPrev;
		F32* trajyPrev;

		F32* predx;
		F32* predy;
		F32* predxv; 
		F32* predyv;
		F32* predxa; 
		F32* predya;
	};

	// Systems
	void render(F32 _dt)
	{
		// Clear screen
		graphics::setViewClear(0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);

		// Forward renderer.
		// 
		// This system requires these components:
		// - Prefab Component: Prefab that contains all meshes that should be rendered
		// - Transform Component: Transform of entity (optional)
		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_PREFAB); 
		{
			// Forward render all loaded prefabs
			for (U32 i = 0; i < qr->m_count; i++)
			{
				// Get components
				PrefabComponent* prefab = (PrefabComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_PREFAB);
				TransformComponent* transform = (TransformComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_TRANSFORM); 

				// Go over all meshes in prefab
				const mara::MeshHandle* meshes = mara::getMeshes(prefab->m_ph);
				for (U16 i = 0; i < mara::getNumMeshes(prefab->m_ph); i++)
				{
					const mara::MeshHandle mesh = meshes[i];

					// Set render state for mesh
					const U64 state = 0 | GRAPHICS_STATE_WRITE_RGB
						| GRAPHICS_STATE_WRITE_A
						| GRAPHICS_STATE_WRITE_Z
						| GRAPHICS_STATE_DEPTH_TEST_LESS
						| GRAPHICS_STATE_MSAA;
					graphics::setState(state);

					// Create entity matrix
					F32 entityMtx[16];
					base::mtxIdentity(entityMtx);

					// Transform component is optional
					if (transform)
					{
						F32 scale[16];
						base::mtxIdentity(scale);
						base::mtxScale(scale, transform->m_scale.x, transform->m_scale.y, transform->m_scale.z);

						F32 rotation[16];
						base::mtxIdentity(rotation);
						base::mtxFromQuaternion(rotation, transform->m_rotation);

						F32 translation[16];
						base::mtxIdentity(translation);
						base::mtxTranslate(translation, transform->m_position.x, transform->m_position.y, -transform->m_position.z);

						base::mtxMul(entityMtx, scale, rotation);
						base::mtxMul(entityMtx, entityMtx, translation);
					}
					
					// Create mesh matrix
					F32 meshMtx[16];
					mara::getMeshTransform(meshMtx, mesh);

					// Calculate final matrix
					F32 mtx[16];
					base::mtxMul(mtx, entityMtx, meshMtx);

					// Set final transformation matrix
					graphics::setTransform(mtx);

					// Submit mesh for rendering
					graphics::submit(0, mesh);
				}
			}

			// Make sure we still clear screen if nothing is loaded.
			// We have to call touch, since we have nothing to submit.
			if (qr->m_count <= 0)
			{
				graphics::touch(0);
			}
		}
		base::free(entry::getAllocator(), qr);
	}

	void input(F32 _dt, bool _enableInput)
	{
		// Modifies entity's transform and camera component from input.
		// 
		// This system requires these components:
		// - Camera Component: Camera settings
		// - Transform Component: Entity transform. 
		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_CAMERA | COMPONENT_TRANSFORM | COMPONENT_MOVEMENT);
		{
			// Calculate camera for each entity
			for (U32 i = 0; i < qr->m_count; i++)
			{
				// Get components
				CameraComponent* cameraComponent = (CameraComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_CAMERA);
				TransformComponent* transformComponent = (TransformComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_TRANSFORM);
				MovementComponent* movementComponent = (MovementComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_MOVEMENT);

				// Get right joystick input
				F32 gamepadRightX = 0.0f;
				F32 gamepadRightY = 0.0f;
				if (_enableInput)
				{
					gamepadRightX = base::clamp((F32)inputGetGamepadAxis({ 0 }, entry::GamepadAxis::RightX), -1.0f, 1.0f);  
					gamepadRightY = base::clamp((F32)inputGetGamepadAxis({ 0 }, entry::GamepadAxis::RightY), -1.0f, 1.0f);
				}
				
				// Get left joystick input
				F32 gamepadLeftX = 0.0f;
				F32 gamepadLeftY = 0.0f;
				if (_enableInput)
				{
					gamepadLeftX = base::clamp((F32)inputGetGamepadAxis({ 0 }, entry::GamepadAxis::LeftX), -1.0f, 1.0f);
					gamepadLeftY = base::clamp((F32)inputGetGamepadAxis({ 0 }, entry::GamepadAxis::LeftY), -1.0f, 1.0f);
				}

				// Get shoulder input
				bool gamepadShoulderRight = false;
				bool gamepadShoulderLeft = false;
				if (_enableInput)
				{
					gamepadShoulderRight = inputGetKeyState(entry::Key::GamepadShoulderR, (U8*)0);
					gamepadShoulderLeft = inputGetKeyState(entry::Key::GamepadShoulderL, (U8*)0);
				}

				// Look camera around based on gamepad input
				cameraComponent->m_yaw += gamepadRightX * cameraComponent->m_sensitivity;
				cameraComponent->m_pitch -= gamepadRightY * cameraComponent->m_sensitivity;

				// Remove vertical from camera forward vector
				base::Vec3 horizontalForward = cameraComponent->m_forward;
				horizontalForward.y = 0; 
				horizontalForward = base::normalize(horizontalForward);

				// Create input vector in world space relative to camera
				const base::Vec3 inputDirection = base::normalize(base::add(
					base::mul(cameraComponent->m_right, -gamepadLeftX),
					base::mul(horizontalForward, -gamepadLeftY))
				);

				// Check movement mode
				if (cameraComponent->m_isFree)
				{
					// Add to camera position based on input direction relative to camera
					if (gamepadLeftX != 0.0f || gamepadLeftY != 0.0f)
					{
						cameraComponent->m_position = base::add(cameraComponent->m_position, base::mul(inputDirection, cameraComponent->m_speed * _dt));
					}

					// Add up/down to camera position based on shoulder input
					if (gamepadShoulderRight)
					{
						cameraComponent->m_position.y += cameraComponent->m_speed * _dt;
					}
					if (gamepadShoulderLeft)
					{
						cameraComponent->m_position.y -= cameraComponent->m_speed * _dt;
					}

					// Set final look at position
					cameraComponent->m_lookAt = base::add(cameraComponent->m_position, cameraComponent->m_forward);
				}
				else
				{
					// Set desired velocity based on input direction relative to camera
					if (gamepadLeftX != 0.0f || gamepadLeftY != 0.0f)
					{
						const base::Vec3 inputDirectionXZ = { inputDirection.x, 0.0f, inputDirection.z };
						movementComponent->desiredVelocity = base::mul(inputDirectionXZ, movementComponent->speed);
					}
					else
					{
						movementComponent->desiredVelocity = { 0.0f, 0.0f, 0.0f };
					}

					// Get orbit character position in local space
					base::Vec3 localCharacterPosition = base::add(transformComponent->m_position, base::mul(cameraComponent->m_right, cameraComponent->m_offset.x));
					localCharacterPosition = base::add(localCharacterPosition, base::mul({ 0.0f, 1.0f, 0.0f }, cameraComponent->m_offset.y));
					localCharacterPosition = base::add(localCharacterPosition, base::mul(cameraComponent->m_forward, cameraComponent->m_offset.z));

					// Get the camera position from that so the camera stays behind the character
					const base::Vec3 localCameraPosition = base::mul(base::neg(cameraComponent->m_forward), cameraComponent->m_armLength);
					cameraComponent->m_position = base::add(localCharacterPosition, localCameraPosition);

					// Set final look at position
					cameraComponent->m_lookAt = localCharacterPosition;
				}
			};
		}
		base::free(entry::getAllocator(), qr);
	}

	void camera(F32 _dt)
	{
		// Calculate camera camera and sends data to GPU.
		// 
		// This system requires these components:
		// - Camera Component: Camera settings
		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_CAMERA);
		{
			// Calculate camera for each entity
			for (U32 i = 0; i < qr->m_count; i++)
			{
				// Get components
				CameraComponent* cameraComponent = (CameraComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_CAMERA);

				// Clamp pitch so we can't look further than bottom and the top
				constexpr F32 threshold = base::kPi * 0.495f;
				cameraComponent->m_pitch = base::clamp(cameraComponent->m_pitch, -threshold, threshold);

				// Calculate sine and cosine based on pitch and yaw
				const F32 sinPitch = base::sin(cameraComponent->m_pitch);
				const F32 cosPitch = base::cos(cameraComponent->m_pitch);
				const F32 sinYaw = base::sin(cameraComponent->m_yaw);
				const F32 cosYaw = base::cos(cameraComponent->m_yaw);

				// Calculate forward vector based on sine and cosine
				cameraComponent->m_forward.x = sinYaw * cosPitch;
				cameraComponent->m_forward.y = sinPitch;
				cameraComponent->m_forward.z = cosYaw * cosPitch;
				cameraComponent->m_forward = base::normalize(cameraComponent->m_forward);

				// Calculate right vector 
				cameraComponent->m_right = base::normalize(base::cross(cameraComponent->m_forward, { 0.0f, 1.0f, 0.0f }));

				// Calculate aspect ratio
				const graphics::Stats* renderStats = graphics::getStats();
				F32 aspectRatio = F32(renderStats->width) / F32(renderStats->height);

				// Calculate view and projection matrix
				F32 view[16];
				F32 proj[16];
				base::mtxLookAt(view, cameraComponent->m_position, cameraComponent->m_lookAt);
				base::mtxProj(proj, cameraComponent->m_fov, aspectRatio, cameraComponent->m_near, cameraComponent->m_far, graphics::getCaps()->homogeneousDepth);

				// Send view projection matrix to the graphics pipeline
				graphics::setViewTransform(0, view, proj);
			};
		}
		base::free(entry::getAllocator(), qr);
	}

	void movement(F32 _dt)
	{
		mara::EntityQuery* qr = mara::queryEntities(COMPONENT_MOVEMENT | COMPONENT_TRANSFORM);
		{
			for (U32 i = 0; i < qr->m_count; i++)
			{
				TransformComponent* transformComponent = (TransformComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_TRANSFORM);
				MovementComponent* movementComponent = (MovementComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_MOVEMENT);
				TrajectoryComponent* trajectoryComponent = (TrajectoryComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_TRAJECTORY);
				
				// Restructure trajectory
				for (I32 i = trajectoryComponent->trajMax - 1; i > 0; i--)
				{
					trajectoryComponent->trajxPrev[i] = trajectoryComponent->trajxPrev[i - 1];
					trajectoryComponent->trajyPrev[i] = trajectoryComponent->trajyPrev[i - 1];
				}

				// Simulate spring damper for movement
				base::criticalSpringDamper(
					transformComponent->m_position.x,
					movementComponent->velocity.x,
					movementComponent->acceleration.x,
					movementComponent->desiredVelocity.x,
					movementComponent->positionHalflife,
					_dt
				);
				base::criticalSpringDamper(
					transformComponent->m_position.z,
					movementComponent->velocity.z,
					movementComponent->acceleration.z,
					movementComponent->desiredVelocity.z,
					movementComponent->positionHalflife,
					_dt
				);

				// Predict simulate spring damper for movement and store data in trajectory component
				base::criticalSpringDamperPredict(
					trajectoryComponent->predx, 
					trajectoryComponent->predxv, 
					trajectoryComponent->predxa, 
					trajectoryComponent->predMax,
					transformComponent->m_position.x, 
					movementComponent->velocity.x, 
					movementComponent->acceleration.x, 
					movementComponent->desiredVelocity.x, 
					movementComponent->positionHalflife, 
					_dt * trajectoryComponent->predSub);

				base::criticalSpringDamperPredict(
					trajectoryComponent->predy, 
					trajectoryComponent->predyv, 
					trajectoryComponent->predya,
					trajectoryComponent->predMax,
					transformComponent->m_position.z, 
					movementComponent->velocity.z, 
					movementComponent->acceleration.z, 
					movementComponent->desiredVelocity.z, 
					movementComponent->positionHalflife,
					_dt * trajectoryComponent->predSub);

				// Set previous position to new position
				trajectoryComponent->trajxPrev[0] = transformComponent->m_position.x;
				trajectoryComponent->trajyPrev[0] = transformComponent->m_position.z;

				// Create rotation based upon calculated spring velocity
				base::Vec3 normalizedVel = base::normalize(movementComponent->velocity);
				if (base::length(normalizedVel) > 0.0f)
				{
					transformComponent->m_rotation = base::fromAxisAngle({ 0.0f, 1.0f, 0.0f }, base::atan2(normalizedVel.x, normalizedVel.z) + base::toRad(180.0f));
				}
			}
		}
		base::free(entry::getAllocator(), qr);
	}

	// Game
	class Game : public entry::AppI
	{
	public:
		Game(const char* _name, const char* _description)
			: entry::AppI(_name, _description)
			, m_scene(MARA_INVALID_HANDLE)
			, m_character(MARA_INVALID_HANDLE)
		{
			// Set window title
			entry::setWindowTitle(entry::kDefaultWindowHandle, _name);

			// Debug default values
#ifdef BASE_CONFIG_DEBUG
			m_debug.showDbgText = true;
			m_debug.showDbgDraw = true;
			m_debug.showBuild = true;

#else
			m_debug.showDbgText = false;
			m_debug.showDbgDraw = false;
			m_debug.showBuild = false;
#endif
			m_debug.menu = false;
			m_debug.menuType = Debug::Default;
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			// Init Engine
			mara::Init maraInit;
			maraInit.graphicsApi = graphics::RendererType::Direct3D11;
			maraInit.resolution.width = _width;
			maraInit.resolution.height = _height;
			mara::init(maraInit);

			// Create ImGui
			mara::imguiCreate();

			// Load PAK
			mara::loadPak("C:/Users/marcu/Dev/mara-demo/demo/build/bin/data/assets.pak");

			// Create Scene
			m_scene = mara::createEntity();
			{
				PrefabComponent* prefabComp = new PrefabComponent();
				prefabComp->m_ph = mara::createPrefab(mara::loadPrefab("scenes/scene.bin"));

				mara::addComponent(m_scene, COMPONENT_PREFAB, mara::createComponent(prefabComp));
			}

			// Create Character
			m_character = mara::createEntity();
			{
				CameraComponent* cameraComp = new CameraComponent();
				cameraComp->m_isActive = true;
				cameraComp->m_speed = 3.0f;
				cameraComp->m_armLength = 4.0f;
				cameraComp->m_offset = { -0.5f, 1.0f, 0.0f };

				PrefabComponent* prefabComp = new PrefabComponent();
				prefabComp->m_ph = mara::createPrefab(mara::loadPrefab("characters/character.bin"));

				TransformComponent* transComp = new TransformComponent();
				transComp->m_position = { 0.0f, 0.5f, 0.0f };
				transComp->m_scale = { 0.01f, 0.01f, 0.01f };

				MovementComponent* moveComp = new MovementComponent();
				moveComp->speed = 1.0f;

				TrajectoryComponent* trajComp = new TrajectoryComponent();

				mara::addComponent(m_character, COMPONENT_CAMERA, mara::createComponent(cameraComp));
				mara::addComponent(m_character, COMPONENT_PREFAB, mara::createComponent(prefabComp));
				mara::addComponent(m_character, COMPONENT_TRANSFORM, mara::createComponent(transComp));
				mara::addComponent(m_character, COMPONENT_MOVEMENT, mara::createComponent(moveComp));
				mara::addComponent(m_character, COMPONENT_TRAJECTORY, mara::createComponent(trajComp));
			}
		}

		I32 shutdown() override
		{
			// Destroy Scene
			mara::destroy(m_scene);

			// Destroy Character
			mara::destroy(m_character);

			// Unload PAK
			mara::unloadPak("C:/Users/marcu/Dev/mara-demo/demo/build/bin/data/assets.pak");

			// Destroy ImGui
			mara::imguiDestroy();

			// Shutdown Engine
			mara::shutdown();

			return 0;
		}

		bool update() override
		{
			// Update
			if (mara::update(GRAPHICS_DEBUG_TEXT, GRAPHICS_RESET_VSYNC))
			{
				// Debug
				mara::imguiBeginFrame();
				debug(mara::getDeltaTime());
				mara::imguiEndFrame();

				// Systems
				camera(mara::getDeltaTime());
				input(mara::getDeltaTime(), !m_debug.menu);
				movement(mara::getDeltaTime());
				render(mara::getDeltaTime());

				// Swap buffers
				graphics::frame();

				return true;
			}
			
			return false;
		}

		void debug(F32 _dt)
		{
			// Toggle debug menu
			if (inputGetKeyState(entry::Key::GamepadThumbL) && inputGetKeyState(entry::Key::GamepadThumbR))
			{
				m_debug.menu = !m_debug.menu;
				inputSetKeyState(entry::Key::GamepadThumbL, (U8)0, false);
				inputSetKeyState(entry::Key::GamepadThumbR, (U8)0, false);
			}

			// Handle dev menu
			if (m_debug.menu)
			{
				// Draw menus
				switch (m_debug.menuType)
				{
					case Debug::Default:
					{
						ImGui::BeginDeveloperMenu("Dev Menu");
						if (ImGui::DeveloperMenuButton("Display"))	 m_debug.menuType = Debug::Display;
						if (ImGui::DeveloperMenuButton("Rendering")) m_debug.menuType = Debug::Rendering;
						if (ImGui::DeveloperMenuButton("Engine"))	 m_debug.menuType = Debug::Engine;
						if (ImGui::DeveloperMenuButton("Levels"))	 m_debug.menuType = Debug::Levels;
						if (ImGui::DeveloperMenuButton("Camera"))	 m_debug.menuType = Debug::Camera;
						if (ImGui::DeveloperMenuButton("Bin Files")) m_debug.menuType = Debug::BinFiles;
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::Display:
					{
						ImGui::BeginDeveloperMenu("Display");
						ImGui::DeveloperMenuCheckbox("Display Stats", &m_debug.showDbgText);
						ImGui::DeveloperMenuCheckbox("Display Debug Draw", &m_debug.showDbgDraw);
						ImGui::DeveloperMenuCheckbox("Display Build Info", &m_debug.showBuild);
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::Rendering:
					{
						ImGui::BeginDeveloperMenu("Rendering");
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::Engine:
					{
						ImGui::BeginDeveloperMenu("Engine");
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::Levels:
					{
						ImGui::BeginDeveloperMenu("Levels");
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::Camera:
					{
						ImGui::BeginDeveloperMenu("Camera");
						if (ImGui::DeveloperMenuCheckbox("Free Camera", &m_debug.freeCamera))
						{
							mara::EntityQuery* qr = mara::queryEntities(COMPONENT_CAMERA | COMPONENT_TRANSFORM);
							for (U32 i = 0; i < qr->m_count; i++)
							{
								CameraComponent* cameraComponent = (CameraComponent*)mara::getComponentData(qr->m_entities[i], COMPONENT_CAMERA);
								cameraComponent->m_isFree = m_debug.freeCamera;
							}
							base::free(entry::getAllocator(), qr);
						}
						ImGui::EndDeveloperMenu();
						break;
					}

					case Debug::BinFiles:
					{
						ImGui::BeginDeveloperMenu("Bin Files");
						mara::ResourceInfo info[100];
						U32 num = mara::getResourceInfo(info, true);
						for (U32 i = 0; i < num; i++)
						{
							char formattedString[256];
							sprintf(formattedString, "%s [%d]", info[i].vfp.getCPtr(), info[i].refCount);
							ImGui::DeveloperMenuText(formattedString);
						}
						ImGui::EndDeveloperMenu();
						break;
					}
				}

				// Go back to default menu or exit dev menu when presing back key
				if (inputGetKeyState(entry::Key::GamepadB))
				{
					if (m_debug.menuType != Debug::Default)
					{
						m_debug.menuType = Debug::Default;
					}
					else
					{
						m_debug.menu = false;
					}
					inputSetKeyState(entry::Key::GamepadB, 0, false);

				}

				// If no item is focused we auto focus to the first item in the list
				inputSetKeyState(entry::Key::GamepadDown, 0, true);
				if (ImGui::IsAnyItemFocused())
				{
					inputSetKeyState(entry::Key::GamepadDown, 0, false);
				}
			}

			if (m_debug.showDbgText)
			{
				ImGui::ShowDebugStats();
			}

			if (m_debug.showDbgDraw)
			{
				// Grid
				graphics::dbgDrawGrid({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 100.0f, 10.0f, 0xFF707070);

				// Player
				const TransformComponent* characterTransform = (TransformComponent*)mara::getComponentData(m_character, COMPONENT_TRANSFORM);
				const MovementComponent* movementComponent = (MovementComponent*)mara::getComponentData(m_character, COMPONENT_MOVEMENT);
				const TrajectoryComponent* trajectoryComponent = (TrajectoryComponent*)mara::getComponentData(m_character, COMPONENT_TRAJECTORY);

				// Character Position and input
				{
					const base::Vec3 start = { characterTransform->m_position.x, 1.0f, characterTransform->m_position.z };
					const base::Vec3 stopVel = base::add(start, base::mul(base::normalize(movementComponent->velocity), 0.5f));
					const base::Vec3 stopDesiredVel = base::add(start, base::mul(base::normalize(movementComponent->desiredVelocity), 0.5f));

					graphics::dbgDrawCircle({ 0.0f, 1.0f, 0.0f }, start, 0.5f, 0.0f, 0xFF00FFFF);
					graphics::dbgDrawLine(start, stopVel, 0xFF00FFFF);
					graphics::dbgDrawLine(start, stopDesiredVel, 0xFF0000FF);
				}

				// Character Movement Trajectory
				for (int i = 0; i < trajectoryComponent->trajMax - trajectoryComponent->trajSub; i += trajectoryComponent->trajSub)
				{
					const base::Vec3 start = { trajectoryComponent->trajxPrev[i + 0], 0.501f, trajectoryComponent->trajyPrev[i + 0] };
					const base::Vec3 stop = { trajectoryComponent->trajxPrev[i + trajectoryComponent->trajSub], 0.501f, trajectoryComponent->trajyPrev[i + trajectoryComponent->trajSub] };

					graphics::dbgDrawCircle({ 0.0f, 1.0f, 0.0f }, start, 0.05f, 0.0f, 0xFFFF0000);
					graphics::dbgDrawLine(start, stop, 0xFFFF0000);
				}

				// Character Movement Prediction
				for (int i = 1; i < trajectoryComponent->predMax; i++)
				{
					const base::Vec3 start = { trajectoryComponent->predx[i + 0], 0.501f, trajectoryComponent->predy[i + 0] };
					const base::Vec3 stop = { trajectoryComponent->predx[i - 1], 0.501f, trajectoryComponent->predy[i - 1] };

					graphics::dbgDrawCircle({ 0.0f, 1.0f, 0.0f }, start, 0.05f, 0.0f, 0xFF0000FF);
					graphics::dbgDrawLine(start, stop, 0xFF0000FF);
				}
			}

			if (m_debug.showBuild)
			{
				ImGui::ShowDebugBuild();
			}
		}

		mara::EntityHandle m_scene;
		mara::EntityHandle m_character;

		struct Debug
		{
			bool menu;
			enum MenuType
			{
				Default,
				Display,
				Rendering,
				Engine,
				Levels,
				Camera,
				BinFiles,
			} menuType;

			bool showDbgText;
			bool showDbgDraw;
			bool showBuild;

			bool freeCamera;

		} m_debug;
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	::Game
	, "Game"
	, "An example of a game project"
)