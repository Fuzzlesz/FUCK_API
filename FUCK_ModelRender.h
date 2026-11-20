#pragma once
#include "FUCK_API.h"

// ==================================================
// [ OPTIONAL ] MODEL RENDER (API v9+)
// ==================================================
//
// Off-screen pictures of 3D, lit as a studio would light them, on FUCK's own backdrop or a clear one:
//   RenderModel()      A loaded model, e.g. an actor's head that isn't on screen, saved to a .png
//   DrawMeshes()       Meshes you've built yourself, drawn into your own buffer, e.g. item cards
//   ReadMeshMaterial() A shape's look as FUCK reads it, to fill a MeshDraw the way FUCK draws its own

namespace FUCK
{
	/// @brief One layer of a face's tint, for ModelRenderRequest.
	struct ModelRenderTintLayer
	{
		const char* mask = nullptr;  // A greyscale .dds from the game's data, under textures\ or not. Copied
		float       colour[4]{};     // RGB laid over where the mask is white, at A's strength. 0 to 1
	};

	/// @brief Options for RenderModel().
	struct ModelRenderRequest
	{
		std::uint32_t   size        = sizeof(ModelRenderRequest);  // Lets later versions add options, leave as is
		RE::NiAVObject* model       = nullptr;  // A loaded model or part of one, e.g. an actor's face node. Held until it's drawn
		const char*     path        = nullptr;  // The .png to write. Its folders are made. Copied
		std::uint32_t   pixels      = 256;      // Square
		float           yaw         = 0.0f;     // Degrees. 0 is front on, as an actor faces
		float           pitch       = 0.0f;     // Degrees, looking down on it
		float           margin      = 1.05f;    // Room around the model, as a scale on its size
		bool            transparent = false;    // A clear background, else FUCK's studio backdrop
		void*           userdata    = nullptr;
		void (*callback)(const char* path, bool success, void* userdata) = nullptr;  // Optional. Called on a background thread once written

		// The face's tint built from these, in order, in place of its own. For a face whose tint isn't its own to draw into,
		// e.g. a copy made at runtime, which shares the game's
		const ModelRenderTintLayer* faceTint      = nullptr;
		std::uint32_t               faceTintCount = 0;

		bool frameFace = false;  // Framed on the face alone, so every face is one size whatever its hair or beard
	};

	/// @brief One mesh for DrawMeshes(), already on the GPU as the caller made it.
	struct MeshDraw
	{
		void*         vertexBuffer     = nullptr;  // An ID3D11Buffer of position xyz, UV, and normal xyz as floats, 32 bytes a vertex
		bool          vertexColours    = false;    // Each vertex is followed by its colour, RGBA as floats, 48 bytes a vertex
		bool          vertexAlpha      = false;    // The vertex colour's alpha fades it too, as the game's Vertex_Alpha shader flag asks
		void*         indexBuffer      = nullptr;  // An ID3D11Buffer of 32-bit indices
		std::uint32_t indexCount       = 0;
		void*         diffuse          = nullptr;  // An ID3D11ShaderResourceView, or none for flat grey
		bool          useAlpha         = false;    // The diffuse's alpha cuts it
		bool          blendAlpha       = false;    // Drawn over the solid meshes, blended
		float         alpha            = 1.0f;
		float         cutoff           = 0.0f;     // Alpha at or under this isn't drawn
		float         specularStrength = 0.10f;
		float         specularPower    = 42.0f;
		float         emissive[3]{};
		float         emissiveStrength = 0.0f;
		bool          useTint          = false;    // Hair's tint, as the game's hair shader takes it from the material
		float         tint[3]{ 1.0f, 1.0f, 1.0f };
		bool          twoSided         = true;     // Drawn from behind too. The game culls a shape's back unless its shader says two-sided
		void*         glow             = nullptr;  // An ID3D11ShaderResourceView of a glow map, where the emissive colour shows, as the game's glow shader takes it
		void*         envMap           = nullptr;  // An ID3D11ShaderResourceView of a cube map it reflects, as the game's environment shader takes it
		void*         envMask          = nullptr;  // An ID3D11ShaderResourceView of where it reflects (red), or none for everywhere
		bool          envMaskInAlpha   = false;    // Where it reflects is envMask's alpha instead, e.g. a normal map's gloss, as the game takes it for a material with no mask
		float         envScale         = 0.0f;     // How strongly it reflects, the material's envMapScale
		// Drawn as the game's effect shader draws a glowing shell, e.g. a Ghostblade's: unlit, its colour the diffuse times effectColour
		// and effectScale, over the solid meshes. ReadMeshMaterial fills these from a BSEffectShaderProperty
		bool          effect           = false;
		float         effectColour[4]{ 1.0f, 1.0f, 1.0f, 1.0f };  // The material's base colour and alpha
		float         effectScale      = 1.0f;     // The material's base colour scale, how bright it glows
		void*         palette          = nullptr;  // An ID3D11ShaderResourceView the diffuse's green picks a colour from along a row the base colour's red picks, as the game's greyscale to palette does
		bool          paletteAlpha     = false;    // The palette's alpha is the alpha too, picked by the diffuse's alpha along the row the base colour's alpha picks
		bool          falloff          = false;    // Faded by how square on it is to the camera, as the game's falloff does
		float         falloffAngles[2]{ 1.0f, 0.0f };     // The cosines it fades between, square on first
		float         falloffOpacity[2]{ 1.0f, 1.0f };    // The opacity at each
		bool          additive         = false;    // Added to what's behind it rather than laid over it
	};

	/// @brief Options for DrawMeshes().
	struct MeshDrawRequest
	{
		std::uint32_t   size      = sizeof(MeshDrawRequest);  // Lets later versions add options, leave as is
		const MeshDraw* meshes    = nullptr;
		std::uint32_t   meshCount = 0;
		float           viewProj[16]{};                     // The view times the projection, as DirectX::XMStoreFloat4x4 stores it
		float           eye[3]{};                           // The camera, in the meshes' space
		float           lightDir[3]{ 0.0f, 0.0f, 1.0f };    // Towards the key light
		std::uint32_t   pixels       = 256;                 // Square
		float           backdropLift = 0.0f;                // Brightens the studio backdrop, e.g. behind dark hair
		bool            transparent  = false;               // A clear background, else the studio backdrop
		std::uint8_t*   rgba         = nullptr;             // pixels x pixels x 4 bytes, filled with the picture
	};

	/// @brief Draws a loaded model off screen with its own textures, lit as a studio would, and saves it to a .png, e.g. a portrait
	/// of an actor's head that isn't on screen. Taken at the start of FUCK's next draw.
	/// @note Skinned parts are drawn as the model's skeleton poses them, and a head's face as FaceGen has morphed it.
	/// @return false if the request was refused, e.g. no model or path.
	inline bool RenderModel(const ModelRenderRequest& request)
	{
		auto i = GetInterface();
		return i ? i->RenderModel(&request) : false;
	}

	/// @brief Draws meshes you've made off screen at once, lit as RenderModel() lights a model, into request.rgba, e.g. item cards.
	/// @note On the render thread only, e.g. from a present hook. The meshes are only read during the call.
	/// @return false if nothing could be drawn, e.g. no meshes or FUCK's pipeline failed.
	inline bool DrawMeshes(const MeshDrawRequest& request)
	{
		auto i = GetInterface();
		return i ? i->DrawMeshes(&request) : false;
	}

	/// @brief Fills a MeshDraw's look from a shape's shader and alpha properties as FUCK reads them for its own renders: its
	/// diffuse, alpha and how it fades, specular, emission and glow, hair tint, sidedness, and environment map, or an effect
	/// shader's shell: its colour, palette, falloff and blend. Its buffers are left as they are.
	/// @param shaderProperty The shape's RE::BSShaderProperty, e.g. its runtime data's shaderProperty.
	/// @param alphaProperty The shape's RE::NiAlphaProperty, or nullptr for none.
	/// @note The texture views are the game's own, valid while the shape's model is loaded.
	/// @return false if it has neither a lighting nor an effect shader, leaving out as it was.
	inline bool ReadMeshMaterial(void* shaderProperty, void* alphaProperty, MeshDraw& out)
	{
		auto i = GetInterface();
		return i ? i->ReadMeshMaterial(shaderProperty, alphaProperty, &out) : false;
	}
}
