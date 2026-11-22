#pragma once
#include "FUCK_API.h"

// ==================================================
// [ OPTIONAL ] MODEL RENDER (API v9+)
// ==================================================
//
// Off-screen renders under a fixed studio light, on FUCK's backdrop or a clear one.
//   RenderModel()      Queue a loaded model to be saved as a .png
//   DrawMeshes()       Draw your own buffers into your RGBA buffer, now (render thread only)
//   ReadMeshMaterial() Fill a MeshDraw's look from a game shape
// Each struct's size field lets later versions add fields; leave it at its default.

namespace FUCK
{
	/// @brief One face tint layer, for ModelRenderRequest::faceTint.
	struct ModelRenderTintLayer
	{
		const char* mask = nullptr;  // Tint mask .dds, "textures\" optional. Copied
		float       colour[4]{};     // RGBA 0-1: RGB laid where the mask is white, at A's strength
	};

	/// @brief Options for RenderModel().
	struct ModelRenderRequest
	{
		std::uint32_t   size        = sizeof(ModelRenderRequest);
		RE::NiAVObject* model       = nullptr;  // e.g. an actor's face node. Held until drawn
		const char*     path        = nullptr;  // Output .png; folders are created. Copied
		std::uint32_t   pixels      = 256;      // Square
		float           yaw         = 0.0f;     // Degrees; 0 faces the model's front (+Y)
		float           pitch       = 0.0f;     // Degrees; positive looks down
		float           margin      = 1.05f;    // Space round the model; 1 is a tight fit
		bool            transparent = false;    // Clear background instead of the backdrop
		void*           userdata    = nullptr;
		// Optional. Called after writing, on FUCK's writer thread (render thread if drawing failed)
		void (*callback)(const char* path, bool success, void* userdata) = nullptr;

		// Optional. Replaces a FaceGen face's tint with these layers, in order. For runtime copies, which share the game's. Copied
		const ModelRenderTintLayer* faceTint      = nullptr;
		std::uint32_t               faceTintCount = 0;

		bool          frameFace   = false;  // Frame the face only, so hair and beards don't change its size
		std::uint32_t supersample = 2;      // Render this many times larger and scale down. 1-4, max 4096 px
	};

	/// @brief One mesh for DrawMeshes(), in GPU buffers you own. ReadMeshMaterial() can fill its look.
	struct MeshDraw
	{
		std::uint32_t size                = sizeof(MeshDraw);  // Same on every mesh in a request
		void*         vertexBuffer        = nullptr;           // ID3D11Buffer: position xyz, UV, normal xyz as floats (32 bytes)
		bool          vertexColours       = false;             // Plus RGBA floats per vertex (48 bytes). Multiplies the colour; hair tints by green
		bool          vertexAlpha         = false;             // Vertex alpha fades it too. Needs vertexColours
		void*         indexBuffer         = nullptr;           // ID3D11Buffer of 32-bit triangle list indices
		std::uint32_t indexCount          = 0;

		void*         diffuse             = nullptr;           // SRV; none draws grey
		bool          useAlpha            = false;             // Diffuse alpha cuts or fades it
		bool          blendAlpha          = false;             // Blend over solid meshes instead of cutting at cutoff
		float         alpha               = 1.0f;
		float         cutoff              = 0.0f;              // Alpha at or below isn't drawn
		float         specularStrength    = 0.10f;
		float         specularPower       = 42.0f;
		float         emissive[3]{};
		float         emissiveStrength    = 0.0f;
		bool          useTint             = false;             // Hair: tinted and softly lit, as the game's hair shader
		float         tint[3]{ 1.0f, 1.0f, 1.0f };
		bool          twoSided            = true;              // Game shapes are one-sided unless their shader says otherwise
		void*         glow                = nullptr;           // SRV; where emissive shows

		void*         envMap              = nullptr;           // Cube map SRV; needs envScale > 0
		void*         envMask             = nullptr;           // SRV, red is where it reflects; none is everywhere
		bool          envMaskInAlpha      = false;             // Mask is envMask's alpha, e.g. a normal map's
		float         envScale            = 0.0f;

		bool          effect              = false;             // Effect shader shell (e.g. a Ghostblade): unlit, diffuse * effectColour * effectScale, over solid meshes

		float         effectColour[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float         effectScale         = 1.0f;
		void*         palette             = nullptr;            // Greyscale-to-palette SRV: diffuse green picks the column, effectColour red the row
		bool          paletteAlpha        = false;              // Alpha from the palette too (diffuse alpha, effectColour alpha)
		bool          falloff             = false;              // Fade by view angle
		float         falloffAngles[2]{ 1.0f, 0.0f };           // Cosines, face on then edge on
		float         falloffOpacity[2]{ 1.0f, 1.0f };          // Opacity at each
		bool          additive            = false;              // Added onto what's behind it rather than blended over; needs blendAlpha. Effect or not

		void*         normalMap           = nullptr;           // SRV. Tangent space (DirectX), no tangents needed; alpha is where it shines
		bool          normalMapModelSpace = false;             // Model space instead, rotated by normalTurn
		float         normalTurn[9]{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };  // Rows: shape space to vertex space
		float         uvOffset[2]{};                           // UV * uvScale + uvOffset, for every texture
		float         uvScale[2]{ 1.0f, 1.0f };
		bool          envUntinted         = false;             // Reflect like glass (e.g. a cornea), not tinted like metal
		bool          skinTint            = false;             // Skin (body, hands, feet): skinTintColour soft-lit over the diffuse, as the game's skin shader does
		float         skinTintColour[3]{ 1.0f, 1.0f, 1.0f };   // RGB 0-1, the actor's skin tone, or a dye colour where a mod dyes by this material
	};

	/// @brief Options for DrawMeshes().
	struct MeshDrawRequest
	{
		std::uint32_t   size         = sizeof(MeshDrawRequest);
		const MeshDraw* meshes       = nullptr;
		std::uint32_t   meshCount    = 0;
		float           viewProj[16]{};                   // XMStoreFloat4x4 layout. Right-handed, or it's mirrored
		float           eye[3]{};                         // Camera position
		float           lightDir[3]{ 0.0f, 0.0f, 1.0f };  // Towards the key light
		std::uint32_t   pixels       = 256;               // Square
		float           backdropLift = 0.0f;              // Brightens the backdrop, e.g. behind dark hair
		bool            transparent  = false;
		std::uint8_t*   rgba         = nullptr;           // pixels * pixels * 4, straight alpha
		std::uint32_t   supersample  = 2;
		float           exposure     = 1.0f;              // Brightness before highlights roll off
		float           saturation   = 1.0f;              // 0 grey, 1 unchanged
		float           contrast     = 1.0f;              // 1 unchanged
	};

	/// @brief Queues a loaded model to be drawn with its own textures and saved as a .png.
	/// @note Drawn as currently posed and morphed; culled shapes and see-through ones without a loaded texture are skipped.
	/// @return false if refused (no model or path); no callback then.
	inline bool RenderModel(const ModelRenderRequest& request)
	{
		auto i = GetInterface();
		return i ? i->RenderModel(&request) : false;
	}

	/// @brief Draws meshes into request.rgba now, lit as RenderModel() is.
	/// @note Render thread only. Waits on the GPU. Blended meshes go on in the order given, so pass them farthest first.
	/// @return false if nothing was drawn, e.g. off the render thread or an unknown MeshDraw size.
	inline bool DrawMeshes(const MeshDrawRequest& request)
	{
		auto i = GetInterface();
		return i ? i->DrawMeshes(&request) : false;
	}

	/// @brief Fills out's look from a shape's BSShaderProperty and NiAlphaProperty (or nullptr).
	/// @note Keeps size, buffers, indexCount, vertexColours and normalTurn. Textures are the game's, valid while the model is loaded.
	/// @return false, out unchanged, if it has neither a lighting nor an effect shader.
	inline bool ReadMeshMaterial(void* shaderProperty, void* alphaProperty, MeshDraw& out)
	{
		auto i = GetInterface();
		return i ? i->ReadMeshMaterial(shaderProperty, alphaProperty, &out) : false;
	}
}
