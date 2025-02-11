#include "gui.h"
#include "../config.h"
#include "../hook.h"

int snapping_round(int x, int snapping)
{
    int a = (x / snapping) * snapping;
    int b = x + snapping;
    return (x - a > b - x) ? b : a;
}

namespace gui
{
    uint32_t compile_blur_shader()
    {
        // TODO!

        const char *vertex_shader = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main()
            {
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        const char *fragment_shader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            uniform sampler2D texture1;
            uniform float width;
            uniform float height;
            uniform float radius;
            uniform float sigma;
            uniform float intensity;
            void main()
            {
                vec2 tex_offset = 1.0 / vec2(width, height);
                vec3 result = texture(texture1, TexCoord).rgb * intensity;
                for(int x = -radius; x <= radius; x++)
                {
                    for(int y = -radius; y <= radius; y++)
                    {
                        float weight = exp(-float(x * x + y * y) / (2.0 * sigma * sigma));
                        result += texture(texture1, TexCoord + vec2(x, y) * tex_offset).rgb * weight * intensity;
                    }
                }
                FragColor = vec4(result, 1.0);
            }
        )";

        return 0;
    }

    void Begin(const char *name, bool *open)
    {
        const float size = globals::screen_size.x * config::menu_size;
        float windowSize = WINDOW_WIDTH * size;
        ImGui::SetNextWindowSizeConstraints({windowSize, 1}, {windowSize, 10000});

        ImGui::PushFont(globals::title_font);
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
        if (config::always_reposition)
            flags |= ImGuiWindowFlags_NoMove;
        ImGui::Begin(name, open, flags);
        ImGui::PushFont(globals::main_font);

        ImGui::SetWindowFontScale(size);

        if (!globals::window_positions.contains(name))
        {
            auto pos = ImGui::GetWindowPos();
            globals::window_positions[name]["x"] = pos.x;
            globals::window_positions[name]["y"] = pos.y;
        }

        float win_x = globals::window_positions[name]["x"] * globals::screen_size.x;
        float win_y = globals::window_positions[name]["y"] * globals::screen_size.x;

        if (!globals::animation_done)
        {
            uint32_t name_hash = std::hash<std::string>{}(name);
            uint8_t animation_type = (name_hash + globals::random_direction) % 8;
            int xoff = 0, yoff = 0;
            switch (animation_type)
            {
            case 0:
                xoff = 1400;
                yoff = 1400;
                break;
            case 1:
                xoff = 1400;
                yoff = -1400;
                break;
            case 2:
                xoff = -1400;
                yoff = 1400;
                break;
            case 3:
                xoff = -1400;
                yoff = -1400;
                break;
            case 4:
                xoff = 0;
                yoff = 1400;
                break;
            case 5:
                xoff = 0;
                yoff = -1400;
                break;
            case 6:
                xoff = 1400;
                yoff = 0;
                break;
            case 7:
                xoff = -1400;
                yoff = 0;
                break;
            }
            ImGui::SetWindowPos({win_x - (globals::animation * xoff),
                                 win_y - (globals::animation * yoff)});
        }
        else
        {
            if (globals::reset_windows && globals::window_positions.contains(name))
            {
                ImGui::SetWindowPos({win_x, win_y});
            }

            // Snapping
            if (config::window_snap > 1)
            {
                auto pos = ImGui::GetWindowPos();
                ImGui::SetWindowPos({(float)snapping_round((int)pos.x, config::window_snap),
                                     (float)snapping_round((int)pos.y, config::window_snap)});
            }
        }

        if (ImGui::IsMouseDragging(0) && ImGui::IsWindowHovered() && globals::animation_done && globals::animation <= 0)
        {
            auto pos = ImGui::GetWindowPos();
            globals::window_positions[name]["x"] = pos.x / globals::screen_size.x;
            globals::window_positions[name]["y"] = pos.y / globals::screen_size.x;
        }

        auto win_size = ImGui::GetWindowSize();
        globals::window_positions[name]["w"] = win_size.x / globals::screen_size.x;
        globals::window_positions[name]["h"] = win_size.y / globals::screen_size.x;
    }

    // same as Begin, but without animations and saving window position (+ always centered)
    void BeginPrompt(const char *name, bool *open)
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoMove;

        const float size = globals::screen_size.x * config::menu_size;
        ImGui::PushFont(globals::title_font);
        ImGui::Begin(name, open, flags);
        ImGui::PushFont(globals::main_font);
        ImGui::SetWindowFontScale(size);

        ImGui::SetWindowPos(ImVec2(
            (1920 - ImGui::GetWindowWidth()) / 2,
            (1080 - ImGui::GetWindowHeight()) / 2));
    }

    void End()
    {
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }

    void ImText(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }

    // TODO: use these wrappers to create search ability to controls

    bool ImCheckbox(const char *label, bool *v)
    {
        bool res = ImGui::Checkbox(label, v);

        return res;
    }

    bool ImButton(const char *label, float width)
    {
        ImGui::PushItemWidth(-1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.07f, 0.07f, 0.07f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));

        ImVec2 cr = width != -1.0f ? ImVec2(width * globals::screen_size.x * config::menu_size, 0)
                                   : ImGui::GetWindowContentRegionMax();
        bool res = ImGui::Button(label, ImVec2(cr.x - 4, 0));

        // draw lines on the left and right side of the button to make it look like a clickable object
        // and not just a text
        auto color = ImGui::ColorConvertFloat4ToU32(globals::current_color);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetItemRectMin().x + 1, ImGui::GetItemRectMin().y + 1),
            ImVec2(ImGui::GetItemRectMin().x + 1, ImGui::GetItemRectMax().y - 1),
            color, 2.0f);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetItemRectMax().x - 2, ImGui::GetItemRectMin().y + 1),
            ImVec2(ImGui::GetItemRectMax().x - 2, ImGui::GetItemRectMax().y - 1),
            color, 2.0f);

        ImGui::PopStyleColor(3);
        ImGui::PopItemWidth();

        return res;
    }

    bool ImInputFloat(const char *label, float *v, const char *format)
    {
        bool res = ImGui::InputFloat(label, v, 0.0f, 0.0f, format);

        return res;
    }

    bool ImInputInt(const char *label, int *v, int step)
    {
        bool res = ImGui::InputInt(label, v, step);

        return res;
    }

    bool ImInputInt2(const char *label, int *v)
    {
        bool res = ImGui::InputInt2(label, v);

        return res;
    }

    bool ImInputText(const char *label, char *buf, size_t buf_size)
    {
        bool res = ImGui::InputText(label, buf, buf_size);

        return res;
    }

    bool ImCombo(const char *label, int *current_item, const char *const items, int items_count)
    {
        bool res = ImGui::Combo(label, current_item, items, items_count);

        return res;
    }

    bool ImColorEdit3(const char *label, float *col, ImGuiColorEditFlags flags)
    {
        bool res = ImGui::ColorEdit3(label, col, flags);

        return res;
    }

    bool ImColorEdit4(const char *label, float *col, ImGuiColorEditFlags flags)
    {
        bool res = ImGui::ColorEdit4(label, col, flags);

        return res;
    }

    void ImProgressBar(float fraction, float width)
    {
        auto progress = fmt::format("{:.0f}%", fraction * 100);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, globals::current_color);
        if (width == -1.f)
        {
            float max_width = ImGui::GetWindowContentRegionMax().x - 4;
            ImGui::ProgressBar(fraction, ImVec2(max_width, 0), progress.c_str());
        }
        else
        {
            ImGui::ProgressBar(fraction, ImVec2(width * globals::screen_size.x * config::menu_size, 0), progress.c_str());
        }
        ImGui::PopStyleColor();
    }

    bool ImToggleButton(const char *label, bool *v, std::function<void()> settings, float width)
    {
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.07f, 0.07f, 0.07f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, *v ? globals::current_color : config::disabled_color.to_imvec4());
        ImVec2 cr = width != -1.0f ? ImVec2(width * globals::screen_size.x * config::menu_size, 0)
                                   : ImGui::GetWindowContentRegionMax();
        bool toggled = false;

        // check for search
        if (globals::search[0] != '\0')
        {
            ImGui::PopStyleColor(1);

            std::string label_lower = label;
            std::transform(label_lower.begin(), label_lower.end(), label_lower.begin(), ::tolower);
            std::string search_lower = globals::search;
            std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

            if (label_lower.find(search_lower) != std::string::npos)
            {
                auto color = config::text_color.to_imvec4();
                color.w = 1.f;
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            }
            else
            {
                auto color = config::disabled_color.to_imvec4();
                color.w = 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            }
        }

        float ratio = globals::screen_size.x * config::menu_size;
        if (settings)
        {
            toggled = ImGui::Button(label, ImVec2(cr.x - 20 * ratio, 0));
            ImGui::SameLine();

            bool settings_toggled = ImGui::Button(" ", ImVec2(16 * ratio, 0));

            ImGui::GetWindowDrawList()->AddTriangleFilled(
                ImVec2(ImGui::GetItemRectMax().x - 2 * ratio, ImGui::GetItemRectMin().y + 4 * ratio),
                ImVec2(ImGui::GetItemRectMax().x - 2 * ratio, ImGui::GetItemRectMax().y - 4 * ratio),
                ImVec2(ImGui::GetItemRectMax().x - 14 * ratio, ImGui::GetItemRectMax().y - 4 * ratio),
                config::disabled_color.to_uint32());

            std::string settings_label = "##" + std::string(label);
            if (settings_toggled)
            {
                ImGui::OpenPopup(settings_label.c_str());
            }

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            if (ImGui::BeginPopup(settings_label.c_str()))
            {
                settings();
                ImGui::EndPopup();
            }
        }
        else
        {
            toggled = ImGui::Button(label, ImVec2(cr.x - 4 * ratio, 0));
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(ImGui::GetItemRectMax().x - 5 * ratio, ImGui::GetItemRectMin().y + 1 * ratio),
                ImVec2(ImGui::GetItemRectMax().x - 2 * ratio, ImGui::GetItemRectMax().y - 1 * ratio),
                ImGui::ColorConvertFloat4ToU32(*v ? globals::current_color : config::disabled_color.to_imvec4()));
        }

        if (toggled)
            *v = !*v;

        ImGui::PopID();
        ImGui::PopItemWidth();

        return toggled;
    }

    void ImPopupSetting(const char *label, std::function<void()> settings, float width)
    {
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, config::text_color.to_imvec4());
        ImVec2 cr = width != -1.0f ? ImVec2(width * globals::screen_size.x * config::menu_size, 0)
                                   : ImGui::GetWindowContentRegionMax();

        float ratio = globals::screen_size.x * config::menu_size;
        ImGui::Button(label, ImVec2(cr.x - 20 * ratio, 0));
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.07f, 0.07f, 0.07f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));
        bool settings_toggled = ImGui::Button(" ", ImVec2(16 * ratio, 0));
        ImGui::PopStyleColor(3);

        ImGui::GetWindowDrawList()->AddTriangleFilled(
            ImVec2(ImGui::GetItemRectMax().x - 2 * ratio, ImGui::GetItemRectMin().y + 4 * ratio),
            ImVec2(ImGui::GetItemRectMax().x - 2 * ratio, ImGui::GetItemRectMax().y - 4 * ratio),
            ImVec2(ImGui::GetItemRectMax().x - 14 * ratio, ImGui::GetItemRectMax().y - 4 * ratio),
            config::disabled_color.to_uint32());

        std::string settings_label = "##" + std::string(label);
        if (settings_toggled)
        {
            ImGui::OpenPopup(settings_label.c_str());
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);

        if (ImGui::BeginPopup(settings_label.c_str()))
        {
            settings();
            ImGui::EndPopup();
        }

        ImGui::PopID();
        ImGui::PopItemWidth();
    }

    bool ImKeybind(const char *label, uint32_t *key, float width, size_t max_chars, bool show_delete)
    {
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, config::text_color.to_imvec4());
        ImVec2 cr = width != -1.0f ? ImVec2(width * globals::screen_size.x * config::menu_size, 0)
                                   : ImGui::GetWindowContentRegionMax();

        float ratio = globals::screen_size.x * config::menu_size;

        // cut label if it's too long
        std::string label_ = label;
        bool cut = false;
        if (label_.length() > max_chars)
        {
            label_ = label_.substr(0, max_chars);
            label_ += "...";
            cut = true;
        }

        ImGui::Button(label_.c_str(), ImVec2(cr.x * 0.58f - (show_delete ? 20.f * ratio : 0.f), 0));

        // if cut, show tooltip
        if (cut && ImGui::IsItemHovered())
        {
            ImVec2 pos = ImGui::GetMousePos();
            pos.x += 10;
            pos.y += 10;
            ImGui::SetNextWindowPos(pos);
            ImGui::SetTooltip(label);
        }

        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.07f, 0.07f, 0.07f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));
        auto key_name = utils::key_name(*key);
        bool keybind_toggled = ImGui::Button(key_name.c_str(), ImVec2(cr.x * 0.4f, 0));
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        auto popup_name = fmt::format("##{}", label);
        if (keybind_toggled)
        {
            ImGui::OpenPopup(popup_name.c_str());
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);

        if (ImGui::BeginPopup(popup_name.c_str()))
        {
            hook::lock_inputs = true;
            ImGui::Text("Press any key to bind...");
            ImGui::Separator();
            ImGui::Text("Press ESC to cancel.");

            if (utils::is_key_pressed(VK_ESCAPE))
            {
                ImGui::CloseCurrentPopup();
            }
            else
            {
                for (int i = 0; i < 256; i++)
                {
                    if (utils::is_key_pressed(i))
                    {
                        *key = i;
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
            }

            ImGui::EndPopup();
        }

        if (show_delete)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(cr.x - 20.f * ratio);
            auto delete_name = fmt::format("X##{}", label);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.07f, 0.07f, 0.07f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04f, 0.04f, 0.04f, 0.5f));
            bool deleted = ImGui::Button(delete_name.c_str(), ImVec2(20.f * ratio, 0));
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            ImGui::PopID();
            ImGui::PopItemWidth();
            return deleted;
        }

        ImGui::PopID();
        ImGui::PopItemWidth();

        return false;
    }

    void PushWidth(float width)
    {
        ImGui::PushItemWidth(width * globals::screen_size.x * config::menu_size);
    }

    void PopWidth()
    {
        ImGui::PopItemWidth();
    }
}