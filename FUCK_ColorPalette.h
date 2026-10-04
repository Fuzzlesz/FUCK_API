#pragma once
#include "FUCK_API.h"

// ==================================================
// [ OPTIONAL ] COLOUR PALETTE (API v6+)
// ==================================================
//
// Swatch grid for recent and fixed colour palettes.
// For Save()/Load(), #define FUCK_API_ENABLE_SIMPLEINI first.
//
// ==================================================

namespace FUCK
{
	/// @brief A grid of colour swatches, for recent or fixed palettes.
	///
	/// kRecent: Newest first. Re-adding moves to the front, oldest drops off when full.
	/// kFixed:  Kept in order. Ignored once full.
	///
	/// Click to pick, right-click (or gamepad Y) to remove, drag a colour onto the grid to add.
	///
	/// @code
	///   static FUCK::ColorPalette recents(12);
	///
	///   FUCK::ColorEdit4("Tint", tint);
	///   if (FUCK::IsItemDeactivatedAfterEdit() && recents.Push(tint))
	///       SaveSettings();
	///
	///   if (recents.Draw("Recent", tint).changed)
	///       SaveSettings();
	/// @endcode
	class ColorPalette
	{
	public:
		enum class Mode
		{
			kRecent,
			kFixed
		};

		struct DrawResult
		{
			bool picked  = false;  // A swatch was clicked, colour written to ioColor
			bool changed = false;  // Palette contents changed
		};

		explicit ColorPalette(std::size_t capacity = 16, Mode mode = Mode::kRecent) :
			_capacity(capacity ? capacity : 1), _mode(mode) {}

		bool allowRemove       = true;   // Right-click / gamepad Y menu to remove swatches
		bool acceptDrops       = true;   // Accept colours dropped onto the grid
		bool highlightSelected = true;   // Outline the swatch matching ioColor
		bool showAddSlot       = false;  // Trailing "+" swatch to add ioColor
		int  columns           = 0;      // Swatches per row, 0 fits to width
		int  rows              = 0;      // Minimum rows, padded with empty slots

		/// @brief Adds a colour, returning true if the palette changed.
		/// False if already first (kRecent), or already present / full (kFixed).
		bool Push(const ImVec4& col)
		{
			const int existing = Find(col);
			if (_mode == Mode::kRecent) {
				if (existing == 0)
					return false;
				if (existing > 0)
					_colors.erase(_colors.begin() + existing);
				_colors.insert(_colors.begin(), col);
				if (_colors.size() > _capacity)
					_colors.resize(_capacity);
				return true;
			}
			if (existing >= 0 || Full())
				return false;
			_colors.push_back(col);
			return true;
		}

		template <std::size_t N>
		bool Push(const float (&col)[N])
		{
			return Push(ToVec4(col));
		}

		void Remove(std::size_t index)
		{
			if (index < _colors.size())
				_colors.erase(_colors.begin() + index);
		}

		void Clear() { _colors.clear(); }

		/// @brief Index of a matching colour, or -1.
		[[nodiscard]] int Find(const ImVec4& col) const
		{
			for (std::size_t i = 0; i < _colors.size(); ++i)
				if (Matches(_colors[i], col))
					return static_cast<int>(i);
			return -1;
		}

		[[nodiscard]] std::size_t                Size() const { return _colors.size(); }
		[[nodiscard]] bool                       Empty() const { return _colors.empty(); }
		[[nodiscard]] bool                       Full() const { return _colors.size() >= _capacity; }
		[[nodiscard]] std::size_t                Capacity() const { return _capacity; }
		[[nodiscard]] const ImVec4&              operator[](std::size_t i) const { return _colors[i]; }
		[[nodiscard]] const std::vector<ImVec4>& Colors() const { return _colors; }

		/// @brief Draws the grid. Text before "##" is drawn as a left label.
		/// A swatchSize of zero uses frame height, or fills the width when columns is set.
		DrawResult Draw(const char* id, ImVec4& ioColor, float swatchSize = 0.0f, ColorEditFlags swatchFlags = ColorEditFlags::kNone)
		{
			const bool hasLabel = id && id[0] && !(id[0] == '#' && id[1] == '#');

			float width = 0.0f;
			if (hasLabel) {
				LeftLabel(id);
				width = CalcItemWidth();
			} else {
				width = GetContentRegionAvail().x;
			}

			// Whole pixels throughout, as ColorButton snaps to them
			const float spacing = GetStyleVarVec(ImGuiStyleVar_ItemInnerSpacing).x;
			float       size    = 0.0f;
			int         cols    = 0;
			if (columns > 0) {
				cols = columns;
				size = swatchSize > 0.0f ? std::ceil(swatchSize) : std::floor((width - spacing * (cols - 1)) / cols);
			} else {
				size = std::ceil(swatchSize > 0.0f ? swatchSize : GetFrameHeight());
				cols = std::max(1, static_cast<int>((width + spacing) / (size + spacing)));
			}
			size = std::max(1.0f, size);

			// Only show the add slot if it can add something
			const bool showAdd = showAddSlot && (_mode == Mode::kRecent || !Full());

			// Pad with empty slots to fill the requested rows, or show one so drops have a target
			const int items = static_cast<int>(_colors.size()) + (showAdd ? 1 : 0);
			const int cells = std::max(items, rows > 0 ? rows * cols : (items == 0 ? 1 : 0));

			DrawResult result;
			int        removeIdx = -1;

			PushID(id);
			BeginGroup();

			for (int cell = 0; cell < cells; ++cell) {
				if (cell % cols != 0)
					SameLine(0.0f, spacing);

				const std::size_t i = static_cast<std::size_t>(cell);

				if (i < _colors.size()) {
					PushID(cell);
					const bool selected = highlightSelected && Matches(_colors[i], ioColor);
					if (ColorButton("##swatch", _colors[i], swatchFlags, ImVec2(size, size), selected)) {
						ioColor       = _colors[i];
						result.picked = true;
					}
					if (allowRemove && DrawRemoveMenu())
						removeIdx = cell;
					PopID();
				} else if (showAdd && i == _colors.size()) {
					// Previews the colour to add, greyed out if already present
					const bool present = Find(ioColor) >= 0;
					BeginDisabled(present);
					if (ColorButton("##add", ioColor, swatchFlags, ImVec2(size, size)))
						result.changed |= Push(ioColor);
					DrawPlusIcon(GetItemRectMin(), size, present ? GetStyleVar(ImGuiStyleVar_DisabledAlpha) : 1.0f);
					EndDisabled();
				} else {
					const ImVec2 p(std::floor(GetCursorScreenPos().x), std::floor(GetCursorScreenPos().y));
					SetCursorScreenPos(p);
					Dummy(ImVec2(size, size));
					DrawRect(p, ImVec2(p.x + size, p.y + size), GetStyleColorVec4(ImGuiCol_TextDisabled), GetStyleVar(ImGuiStyleVar_FrameRounding), 1.0f);
				}
			}

			EndGroup();

			if (acceptDrops && BeginDragDropTarget()) {
				if (const ImGuiPayload* p4 = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F)) {
					const float* f = static_cast<const float*>(p4->Data);
					result.changed |= Push(ImVec4(f[0], f[1], f[2], f[3]));
				} else if (const ImGuiPayload* p3 = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F)) {
					const float* f = static_cast<const float*>(p3->Data);
					result.changed |= Push(ImVec4(f[0], f[1], f[2], 1.0f));
				}
				EndDragDropTarget();
			}

			PopID();

			if (removeIdx >= 0) {
				Remove(static_cast<std::size_t>(removeIdx));
				result.changed = true;
			}

			return result;
		}

		/// @brief Draw() for float[3] or float[4] colours.
		template <std::size_t N>
		DrawResult Draw(const char* id, float (&ioColor)[N], float swatchSize = 0.0f, ColorEditFlags swatchFlags = ColorEditFlags::kNone)
		{
			ImVec4     col    = ToVec4(ioColor);
			DrawResult result = Draw(id, col, swatchSize, swatchFlags);
			if (result.picked) {
				ioColor[0] = col.x;
				ioColor[1] = col.y;
				ioColor[2] = col.z;
				if constexpr (N == 4)
					ioColor[3] = col.w;
			}
			return result;
		}

#ifdef FUCK_API_ENABLE_SIMPLEINI
		/// @brief Saves to an INI key. HDR values are clamped.
		void Save(CSimpleIniA& ini, const char* sec, const char* key) const
		{
			if (_colors.empty()) {
				ini.Delete(sec, key, true);
				return;
			}
			std::string out;
			out.reserve(_colors.size() * 9);
			for (const auto& c : _colors) {
				char buf[16];
				snprintf(buf, sizeof(buf), "%s%02X%02X%02X%02X", out.empty() ? "" : ",", ToByte(c.x), ToByte(c.y), ToByte(c.z), ToByte(c.w));
				out += buf;
			}
			ini.SetValue(sec, key, out.c_str());
		}

		/// @brief Loads what Save() wrote, replacing the contents.
		void Load(const CSimpleIniA& ini, const char* sec, const char* key)
		{
			_colors.clear();
			const char* val = ini.GetValue(sec, key, "");
			while (val && *val && !Full()) {
				unsigned int rgba = 0;
				if (sscanf_s(val, "%8x", &rgba) == 1) {
					_colors.emplace_back(
						((rgba >> 24) & 0xFF) / 255.0f,
						((rgba >> 16) & 0xFF) / 255.0f,
						((rgba >> 8) & 0xFF) / 255.0f,
						(rgba & 0xFF) / 255.0f);
				}
				val = strchr(val, ',');
				if (val)
					++val;
			}
		}
#endif

	private:
		static bool Matches(const ImVec4& a, const ImVec4& b)
		{
			return ToByte(a.x) == ToByte(b.x) && ToByte(a.y) == ToByte(b.y) && ToByte(a.z) == ToByte(b.z) && ToByte(a.w) == ToByte(b.w);
		}

		template <std::size_t N>
		static ImVec4 ToVec4(const float (&c)[N])
		{
			static_assert(N == 3 || N == 4, "FUCK::ColorPalette expects float[3] or float[4].");
			if constexpr (N == 4)
				return ImVec4(c[0], c[1], c[2], c[3]);
			else
				return ImVec4(c[0], c[1], c[2], 1.0f);
		}

		static unsigned int ToByte(float v)
		{
			v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
			return static_cast<unsigned int>(v * 255.0f + 0.5f);
		}

		static const char* RemoveLabel()
		{
			return Translate("$FUCK_Palette_Remove");
		}

		// Opens on right-click or gamepad Y, padded to match SetTooltip. Returns true if Remove was chosen.
		static bool DrawRemoveMenu()
		{
			if ((IsItemHovered() && IsMouseReleased(1)) || ((IsItemHovered() || IsItemFocused()) && IsKeyPressed(ImGuiKey_GamepadFaceUp, false)))
				OpenPopup("##swatchmenu");

			PushStyleVar(ImGuiStyleVar_WindowPadding, UIScale(12.0f, 12.0f));
			const bool open = BeginPopup("##swatchmenu");
			PopStyleVar();

			bool removed = false;
			if (open) {
				removed = Selectable(RemoveLabel());
				EndPopup();
			}
			return removed;
		}

		// Bars sized to whole pixels with the swatch's parity so the "+" sits dead centre, outlined to read on any colour
		static void DrawPlusIcon(const ImVec2& min, float size, float alpha)
		{
			const int s = static_cast<int>(size);
			int       t = std::max(1, static_cast<int>(std::round(s * 0.1f)));
			int       l = std::max(t, static_cast<int>(std::round(s * 0.5f)));
			if ((s - t) % 2)
				++t;
			if ((s - l) % 2)
				++l;

			const float  o = std::max(1.0f, std::round(UIScale(1.0f)));
			const ImVec2 h0(min.x + (s - l) / 2, min.y + (s - t) / 2), h1(h0.x + l, h0.y + t);
			const ImVec2 v0(min.x + (s - t) / 2, min.y + (s - l) / 2), v1(v0.x + t, v0.y + l);

			const ImVec4 outline(0.0f, 0.0f, 0.0f, alpha);
			DrawRectFilled(ImVec2(h0.x - o, h0.y - o), ImVec2(h1.x + o, h1.y + o), outline);
			DrawRectFilled(ImVec2(v0.x - o, v0.y - o), ImVec2(v1.x + o, v1.y + o), outline);

			ImVec4 fill = GetStyleColorVec4(ImGuiCol_Text);
			fill.w *= alpha;
			DrawRectFilled(h0, h1, fill);
			DrawRectFilled(v0, v1, fill);
		}

		std::vector<ImVec4> _colors;
		std::size_t         _capacity;
		Mode                _mode;
	};
}
