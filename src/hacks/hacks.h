#pragma once
#include "../pch.h"
#include "../menu/keybinds.h"

namespace hacks
{
    void init();
    void draw();
    void update();

    // methods for saving and loading settings
    void save(nlohmann::json *data);
    void load(nlohmann::json *data);

    template <typename T>
    T *find_hack(std::string id)
    {
        for (auto hack : hacks)
        {
            if (hack->get_id() == id)
            {
                return (T *)hack;
            }
        }
        return nullptr;
    }

    template <typename T>
    T *find_component(std::string id)
    {
        for (auto window : windows)
        {
            for (auto component : window.get_components())
            {
                if (component->get_id() == id)
                {
                    return (T *)component;
                }
            }
        }
        return nullptr;
    }

    struct opcode_t
    {
        void *address;
        std::string library;
        std::vector<uint8_t> on_bytes;
        std::vector<uint8_t> off_bytes;
    };

    // methods for working with opcodes
    opcode_t read_opcode(nlohmann::json j);
    std::vector<opcode_t> read_pattern(nlohmann::json j);
    bool write_opcode(opcode_t opcode, bool enabled);

    class Hack
    {
    public:
        // called during hooks::init() (before loading hacks)
        virtual void init() = 0;
        // called during hooks::init() (after loading hacks)
        virtual void late_init() = 0;
        // called during menu rendering
        // (embedded = true if the hack is being rendered from "EmbeddedHackComponent")
        virtual void draw(bool embedded = false) = 0;
        // called every frame
        virtual void update() = 0;
        // used to create a keybind for this hack
        virtual bool load_keybind(struct keybinds::Keybind* keybind) { return false; }

        // methods for saving and loading settings for this hack
        virtual void load(nlohmann::json *data) = 0;
        virtual void save(nlohmann::json *data) = 0;

        // used to identify hacks
        virtual std::string get_id() = 0;
    };

    extern std::vector<Hack *> hacks;

    class Component
    {
    public:
        // called inside the window's draw method
        virtual void draw() = 0;

        // methods for saving and loading settings for this component
        virtual void load(nlohmann::json *data) = 0;
        virtual void save(nlohmann::json *data) = 0;

        // initialize the keybind
        virtual void create_keybind(keybinds::Keybind* keybind) {}

        virtual std::string get_id() { return ""; }
        virtual std::string get_sort_key() { return get_id(); }
    };

    // Controls a single toggleable hack
    class ToggleComponent : public Component
    {
    public:
        ToggleComponent(std::string &title, std::string id, std::function<void()> callback, std::vector<opcode_t> opcodes);
        virtual void draw() override;

        // save and load toggle setting
        virtual void load(nlohmann::json *data) override;
        virtual void save(nlohmann::json *data) override;

        virtual void create_keybind(keybinds::Keybind* keybind) override;

        virtual std::string get_id() override { return m_id; }
        virtual std::string get_sort_key() override { return m_title; }

        void set_description(std::string description) { m_description = description; }

        bool apply_patch();
        bool is_enabled();
        std::string get_title();

        std::vector<opcode_t> get_opcodes() { return m_opcodes; }

        void set_warnings(bool has_warnings) { m_has_warnings = has_warnings; }
        bool has_warnings() { return m_has_warnings; }
        void set_is_cheat(bool is_cheat) { m_is_cheat = is_cheat; }
        bool is_cheat() { return m_is_cheat; }

        void toggle();

    private:
        bool m_enabled;
        std::string m_title;
        std::string m_description;
        std::string m_id;
        std::function<void()> m_callback;
        std::vector<opcode_t> m_opcodes;
        bool m_has_warnings = false;
        bool m_is_cheat = false;
    };

    // A component that only displays text
    class TextComponent : public Component
    {
    public:
        TextComponent(std::string text);
        virtual void draw() override;

        // text components don't have any settings to save or load
        virtual void load(nlohmann::json *data) override {}
        virtual void save(nlohmann::json *data) override {}

    private:
        std::string m_text;
    };

    class EmbeddedHackComponent : public Component
    {
    public:
        EmbeddedHackComponent(Hack *hack, const char *id);
        virtual void draw() override;

        // saving is handled by the hack itself
        virtual void load(nlohmann::json *data) override {}
        virtual void save(nlohmann::json *data) override {}

        virtual void create_keybind(keybinds::Keybind* keybind) override;

        virtual std::string get_id() override { return m_id; }
        virtual std::string get_sort_key() override { return m_id; }

    private:
        Hack *m_hack;
        std::string m_id;
    };

    class Window
    {
    public:
        Window(std::string title);
        void draw();
        void add_component(Component *component);
        std::string get_title();
        std::vector<Component *> get_components() { return m_components; }

        // methods for saving and loading settings for this window
        void load(nlohmann::json *data);
        void save(nlohmann::json *data);

    private:
        std::string m_title;
        std::vector<Component *> m_components;

        friend void init();
    };

    extern std::vector<Window> windows;
    extern std::vector<ToggleComponent *> all_hacks;
}