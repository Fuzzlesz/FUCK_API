#pragma once
#include "FUCK_API.h"

// ==================================================
// [ OPTIONAL ] CAMERA (API v7+)
// ==================================================
//
// Moves the game's camera with FUCK's shared controls, which always need Ctrl or RB held:
//   Ctrl + WASD/QE/RF          pan, orbit, pitch
//   Ctrl + Shift + WASD/QE/RF  zoom, FOV, roll, pitch
//   Middle-drag                pan (Ctrl: pitch, Ctrl+Shift: roll, +Right: orbit)
//   Ctrl (+Shift) + Scroll     zoom (FOV)
//   RB + sticks / triggers     pan, orbit, zoom, FOV (RB+LB: roll, pitch)
//   Right-drag, RB + X/Y       turn the subject
//
// While the controls are in use, FUCK keeps their input from the game so the two don't both act on it. It holds:
//   - the middle or right mouse button, or RB, while held. A quick tap of RB still reaches the game, so menus keep their bumper
//   - Ctrl, but only together with the camera's keys or the wheel
// Ctrl and Shift on their own still reach the game, so other mods' shortcuts keep working.
//
// One plugin owns the camera at a time, through a session over one of three bases:
//   kOffset       A camera the game holds still, e.g. RaceMenu's. FUCK's moves are offsets on it.
//   kThirdPerson  The player's third-person camera, e.g. in the inventory. Fixed where it is when the session
//                 begins, and kept there while time runs (Skyrim Souls). SmoothCam is paused for the session.
//   kFree         The game's free camera, entered for the session and left after.
//
// kOffset and kThirdPerson (the offset bases) are driven by the client each frame, from wherever the
// game moves its camera: RevertCamera() before, UpdateCamera() after. kFree drives itself.
//
// A newer session ends the current one, unless that one is kResumable: then it's paused, and resumes
// as it was left once the newer one ends.
//
// Get the camera ready before beginning (e.g. switching to third person), and undo your own changes
// after ending, as FUCK restores only what it changed: its offsets, the FOV, and the base.
//
// @code
//   class MyCamera : public FUCK::ICameraClient
//   {
//       void OnCameraUpdate(FUCK::CameraControlState& state, float) override
//       {
//           state.inputAllowed = !IsTypingName();
//       }
//   };
//
//   // Menu opens
//   FUCK::CameraSessionOptions opts;
//   opts.client = &myCamera;
//   opts.base   = FUCK::CameraBase::kThirdPerson;
//   FUCK::BeginCameraSession(opts);
//
//   // Each frame, around the game's camera update
//   FUCK::RevertCamera(&myCamera);
//   ...the game moves its camera...
//   FUCK::UpdateCamera(&myCamera, interval);
//
//   // Menu closes
//   FUCK::EndCameraSession(&myCamera);
// @endcode
//
// ==================================================

namespace FUCK
{
	enum class CameraBase
	{
		kOffset,      // A camera the game holds still, e.g. a menu's, with FUCK's moves as offsets on it
		kFree,        // The game's free camera, entered when the session begins and left when it ends
		kThirdPerson  // The player's third-person camera, fixed where it is when the session begins and kept there while time runs.
		              // SmoothCam is paused, and the subject turns by its heading, so restore the heading after ending
	};

	enum class CameraSessionFlags : int
	{
		kNone             = 0,
		kBlockControls    = 1 << 0,  // The game gets no input. FUCK's controls and hotkeys still work
		kDisableSaving    = 1 << 1,  // Saving is off for the session, so a save can't catch the camera mid-move. As Photo Mode does
		kFreezeTime       = 1 << 2,  // Soft pause, so the world holds still for the session
		kPivotAtCamera    = 1 << 3,  // Orbit turns the camera in place instead of circling the subject
		kZoomAcceleration = 1 << 4,  // Held zoom speeds up, for crossing long distances
		kPresetHotkeys    = 1 << 5,  // Ctrl+1-9 loads the preset bound to that number. Numbers are bound in the presets list
		kFaceCamera       = 1 << 6,  // The subject turns to face the camera as the session begins
		kRememberCamera   = 1 << 7,  // Offset bases: the camera is saved for the context as the session ends, and the next session in that context opens on it
		kResumable        = 1 << 8,  // A newer session pauses this one instead of ending it, and it resumes when that one ends
		kNoKeyboard       = 1 << 9,  // No Ctrl+WASD/QE/RF, for menus that use those keys themselves. The mouse and gamepad controls stay
		kRunPhysics       = 1 << 10  // FSMP's cloth and hair keep moving while the game is paused, e.g. by a menu. Does nothing without FSMP
	};

	inline CameraSessionFlags  operator|(CameraSessionFlags a, CameraSessionFlags b) { return static_cast<CameraSessionFlags>(static_cast<int>(a) | static_cast<int>(b)); }
	inline bool                operator&(CameraSessionFlags a, CameraSessionFlags b) { return (static_cast<int>(a) & static_cast<int>(b)) != 0; }
	inline CameraSessionFlags& operator|=(CameraSessionFlags& a, CameraSessionFlags b) { return a = a | b; }

	/// @brief Where the camera is and where it's looking. Angles in radians, FOV in degrees.
	/// @note Pitch and yaw follow the free camera's own convention, and roll turns the view around where it looks.
	struct CameraPose
	{
		RE::NiPoint3 position{ 0.0f, 0.0f, 0.0f };
		float        pitch = 0.0f;
		float        yaw   = 0.0f;
		float        roll  = 0.0f;
		float        fov   = 0.0f;
	};

	/// @brief A screen area from -1 to 1 on each axis, X right and Y up.
	struct CameraScreenRect
	{
		float centreX    = 0.0f;
		float centreY    = 0.0f;
		float halfHeight = 1.0f;
	};

	/// @brief Filled by the client each frame, in OnCameraUpdate().
	struct CameraControlState
	{
		bool inputAllowed   = true;   // false drops queued input and waits for buttons to be released
		bool mouseLocked    = false;  // true when a mouse press began over your own UI, so the camera leaves that press alone
		bool gamepadBlocked = false;  // true while you use the gamepad for your own controls, so the camera ignores it
		bool uiShown        = true;   // false while the client's UI is hidden, so FUCK ignores ImGui hovering and the camera keeps the mouse
		bool cameraMoves    = true;   // false turns off the controls that move the camera, on every device. Turning the subject still works. E.g. for a client offering sliders instead
	};

	/// @brief Axes from -1 to 1, for clients that drive the camera with controls of their own.
	struct CameraMove
	{
		float panX  = 0.0f;
		float panZ  = 0.0f;
		float zoom  = 0.0f;
		float orbit = 0.0f;
	};

	/// @brief Offset bases: the camera's moves from its base (and body framing), in the base camera's own axes.
	/// Translate X is right, Y forward and Z up, in game units. Angles in radians, FOV in degrees (0 for unchanged).
	struct CameraOffsets
	{
		RE::NiPoint3 translate{ 0.0f, 0.0f, 0.0f };
		float        orbit = 0.0f;
		float        pitch = 0.0f;
		float        roll  = 0.0f;
		float        fov   = 0.0f;
	};

	/// @brief Implement this to own a camera session.
	/// @note Its methods never change, as FUCK can't call a method an older client was built without.
	class ICameraClient
	{
	public:
		virtual ~ICameraClient() = default;

		/// @brief Called each frame before the camera moves: by UpdateCamera() for the offset bases, by the free camera for kFree.
		/// Fill in the state, and call MoveCamera() for any controls of your own.
		virtual void OnCameraUpdate(CameraControlState&, float) {}

		/// @brief Asked only when a scroll is about to zoom, so a costly check is fine here.
		virtual bool IsCameraScrollBlocked() { return false; }

		/// @brief Another plugin began a session, so this one has ended and the camera was reset.
		virtual void OnCameraTakenOver() {}

		/// @brief kFree only: the position about to be rendered, after FUCK's own moves. Change it to move the camera there instead.
		/// @note Called whenever the game asks for the camera's position, which can be more than once a frame.
		virtual void OnCameraFinalPosition(RE::NiPoint3&) {}

		/// @brief kResumable: another plugin began a session, so this one is paused beneath it and the camera is put back where the game placed it.
		/// Driving calls do nothing until it resumes, but it can still be ended.
		virtual void OnCameraPaused() {}

		/// @brief The session that paused this one has ended and its camera is back, so this one carries on where it was left.
		virtual void OnCameraResumed() {}
	};

	struct CameraSessionOptions
	{
		std::uint32_t  size    = sizeof(CameraSessionOptions);  // Lets later versions add options, leave as is
		ICameraClient* client  = nullptr;
		const char*    context = nullptr;  // Names the offset-base presets and remembered camera this session uses. nullptr for this plugin's name. FUCK keeps its own copy
		CameraBase     base    = CameraBase::kOffset;
		RE::Actor*     subject = nullptr;  // Orbited, framed and turned. nullptr for the player

		CameraSessionFlags flags = CameraSessionFlags::kNone;

		// Offset bases: begin with the subject's whole body framed into this area at once, rather than swooping in.
		// With kRememberCamera, a remembered camera is used instead of this framing
		bool             startFramed   = false;
		CameraScreenRect framingRect;
		float            framingMargin = 0.0f;  // 0 for FUCK's default room around the body. See SetCameraBodyFraming()
	};

	enum class CameraSettingsFlags : int
	{
		kNone            = 0,
		kSpeeds          = 1 << 0,  // Per-device speed tabs, for an advanced view
		kSelectDeviceTab = 1 << 1   // Open on the tab for the device in use. Pass it on the frame your panel opens
	};

	inline CameraSettingsFlags  operator|(CameraSettingsFlags a, CameraSettingsFlags b) { return static_cast<CameraSettingsFlags>(static_cast<int>(a) | static_cast<int>(b)); }
	inline bool                 operator&(CameraSettingsFlags a, CameraSettingsFlags b) { return (static_cast<int>(a) & static_cast<int>(b)) != 0; }
	inline CameraSettingsFlags& operator|=(CameraSettingsFlags& a, CameraSettingsFlags b) { return a = a | b; }

	// --------------------------------------------------
	// Sessions
	// --------------------------------------------------

	/// @brief Takes the camera. A session another plugin holds is ended first, or paused if it's kResumable, and its client told.
	/// Beginning again with the same client starts its session over.
	/// @return false if the options are invalid.
	inline bool BeginCameraSession(const CameraSessionOptions& options)
	{
		auto i = GetInterface();
		if (!i)
			return false;

		if (options.context)
			return i->BeginCameraSession(&options);

		auto named    = options;
		named.context = g_pluginName;
		return i->BeginCameraSession(&named);
	}

	/// @brief Ends the client's session, resetting the camera and restoring the FOV.
	inline void EndCameraSession(ICameraClient* client)
	{
		if (auto i = GetInterface())
			i->EndCameraSession(client);
	}

	/// @brief Whether the client holds the camera now. false while paused.
	inline bool IsCameraSessionActive(ICameraClient* client)
	{
		auto i = GetInterface();
		return i ? i->IsCameraSessionActive(client) : false;
	}

	/// @brief Whether the client's session is waiting beneath a newer one, see kResumable.
	inline bool IsCameraSessionPaused(ICameraClient* client)
	{
		auto i = GetInterface();
		return i ? i->IsCameraSessionPaused(client) : false;
	}

	/// @brief Puts the camera back where the game left it. Call before the game moves its camera.
	/// @note Offset bases only. A kFree session drives itself, so this does nothing there.
	/// Needed when the game moves its camera on from where it was last frame, e.g. while easing. UpdateCamera() also puts it back if this wasn't called.
	inline void RevertCamera(ICameraClient* client)
	{
		if (auto i = GetInterface())
			i->RevertCamera(client);
	}

	/// @brief Runs the controls and applies the camera. Call once a frame, after the game moves its camera.
	/// @note Offset bases only. A kFree session drives itself, so this does nothing there.
	/// For kThirdPerson, call it from your menu's update the same as kOffset, since the game's own camera update doesn't run while the game is paused.
	inline void UpdateCamera(ICameraClient* client, float interval)
	{
		if (auto i = GetInterface())
			i->UpdateCamera(client, interval);
	}

	/// @brief Moves the camera at the device's speed settings, scaled by distance and FOV like the shared controls.
	/// @note Only works inside OnCameraUpdate().
	inline void MoveCamera(const CameraMove& move, InputDevice device, float speedMult = 1.0f)
	{
		if (auto i = GetInterface())
			i->MoveCamera(&move, device, speedMult);
	}

	/// @brief Clears all offsets and body framing, and restores the FOV, keeping the session.
	/// kThirdPerson also turns the subject back to how it faced as the session began.
	inline void ResetCamera(ICameraClient* client)
	{
		if (auto i = GetInterface())
			i->ResetCamera(client);
	}

	/// @brief Whether the camera has been moved from where the game puts it.
	inline bool IsCameraModified(ICameraClient* client)
	{
		auto i = GetInterface();
		return i ? i->IsCameraModified(client) : false;
	}

	/// @brief Swoops the camera to frame the subject's whole body into a screen area, or back again.
	/// Already framed, a different area or margin re-frames in place without the swoop, keeping the camera's offsets.
	/// @param rect The part of the screen to frame the body in, e.g. above a menu's bottom bar.
	/// @param margin Optional. The room around the body, as a scale on its height: 1 fits it exactly, larger leaves more room. 0 uses FUCK's default,
	/// which suits most screens, as the area already keeps the body clear of a bottom bar. Pass one only for a deliberately different shot, e.g. a tight portrait.
	inline void SetCameraBodyFraming(ICameraClient* client, bool framed, const CameraScreenRect& rect, float margin = 0.0f)
	{
		if (auto i = GetInterface())
			i->SetCameraBodyFraming(client, framed, &rect, margin);
	}

	inline bool IsCameraBodyFramed(ICameraClient* client)
	{
		auto i = GetInterface();
		return i ? i->IsCameraBodyFramed(client) : false;
	}

	/// @brief Offset bases: where the camera is heading, which is where it settles once any easing ends.
	inline bool GetCameraOffsets(ICameraClient* client, CameraOffsets& offsets)
	{
		auto i = GetInterface();
		return i ? i->GetCameraOffsets(client, &offsets) : false;
	}

	/// @brief Offset bases: moves the camera there, easing when cinematic as the controls do. For your own sliders: get the offsets, change one value, then set them.
	inline bool SetCameraOffsets(ICameraClient* client, const CameraOffsets& offsets)
	{
		auto i = GetInterface();
		return i ? i->SetCameraOffsets(client, &offsets) : false;
	}

	/// @brief kFree only: where the camera is now, including any moves still easing in.
	inline bool GetCameraPose(ICameraClient* client, CameraPose& pose)
	{
		auto i = GetInterface();
		return i ? i->GetCameraPose(client, &pose) : false;
	}

	/// @brief kFree only: puts the camera there at once, clearing offsets and body framing.
	/// @note A pose with fov 0 keeps the current FOV.
	inline bool SetCameraPose(ICameraClient* client, const CameraPose& pose)
	{
		auto i = GetInterface();
		return i ? i->SetCameraPose(client, &pose) : false;
	}

	/// @brief The world camera's current field of view, in degrees.
	inline bool GetCameraFov(float& horizontal, float& vertical)
	{
		auto i = GetInterface();
		return i ? i->GetCameraFov(&horizontal, &vertical) : false;
	}

	// --------------------------------------------------
	// Presets
	// --------------------------------------------------
	//
	// Kept in FUCK's camera folder.
	// Offset-base presets are relative to the session's camera, so each context keeps its own in one file (camera\still\<context>.json),
	// the context being the plugin's name unless the session names another. They hold the offsets and the subject's facing.
	// kFree presets are world poses, so every free-camera session shares them, one file each (camera\free\). They hold the pose,
	// and optionally where the subject stands and faces, which is only restored while that cell is loaded.

	/// @brief Saves the camera as it stands, replacing a preset of the same name.
	/// @param name nullptr or empty for the next free "Preset N".
	/// @param withSubject kFree only: also keep where the subject stands.
	inline bool SaveCameraPreset(ICameraClient* client, const char* name, bool withSubject = false)
	{
		auto i = GetInterface();
		return i ? i->SaveCameraPreset(client, name, withSubject) : false;
	}

	/// @brief Eases to an offset-base preset when cinematic, or jumps to a kFree one.
	/// @return false if there's no preset of that name, or name is nullptr.
	inline bool LoadCameraPreset(ICameraClient* client, const char* name)
	{
		auto i = GetInterface();
		return i ? i->LoadCameraPreset(client, name) : false;
	}

	/// @return false if there's no preset of that name, or name is nullptr.
	inline bool DeleteCameraPreset(ICameraClient* client, const char* name)
	{
		auto i = GetInterface();
		return i ? i->DeleteCameraPreset(client, name) : false;
	}

	inline int GetCameraPresetCount(ICameraClient* client)
	{
		auto i = GetInterface();
		return i ? i->GetCameraPresetCount(client) : 0;
	}

	/// @return nullptr past the end. Valid until the presets next change.
	inline const char* GetCameraPresetName(ICameraClient* client, int index)
	{
		auto i = GetInterface();
		return i ? i->GetCameraPresetName(client, index) : nullptr;
	}

	// --------------------------------------------------
	// Applets
	// --------------------------------------------------

	/// @brief The presets list with Delete, a name field with Save, and for kFree a checkbox to keep the subject's position.
	/// Picking a preset loads it. With kPresetHotkeys, Ctrl+1-9 over an open list's entry gives it that shortcut, and Ctrl+0 clears it.
	/// @param width The rows' width, or 0 for the space available.
	/// @return true if a preset was loaded.
	inline bool DrawCameraPresets(ICameraClient* client, float width = 0.0f)
	{
		auto i = GetInterface();
		return i ? i->DrawCameraPresets(client, width) : false;
	}

	/// @brief The camera's settings, saved by FUCK as they change.
	/// @return true if anything changed.
	inline bool DrawCameraSettings(CameraSettingsFlags flags = CameraSettingsFlags::kNone)
	{
		auto i = GetInterface();
		return i ? i->DrawCameraSettings(static_cast<int>(flags)) : false;
	}

	/// @brief A help marker listing the controls for the device in use.
	/// @param flags The flags your session begins with, so the help matches it even while no session runs. With kNoKeyboard the keyboard's controls are left out.
	/// @param extraKBM, extraGP Text added after the camera controls, for shortcuts of your own.
	inline void DrawCameraHelpMarker(CameraSessionFlags flags = CameraSessionFlags::kNone, const char* extraKBM = nullptr, const char* extraGP = nullptr)
	{
		if (auto i = GetInterface())
			i->DrawCameraHelpMarker(static_cast<int>(flags), extraKBM, extraGP);
	}
}
