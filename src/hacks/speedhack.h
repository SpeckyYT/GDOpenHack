#pragma once
#include "../pch.h"
#include "hacks.h"

namespace hacks
{
    class Speedhack : public Hack
    {
    public:
        Speedhack();
        virtual void init() override;
        virtual void late_init() override;
        virtual void draw(bool embedded = false) override;
        virtual void update() override;
        virtual void load(nlohmann::json *data) override;
        virtual void save(nlohmann::json *data) override;
        virtual std::string get_id() override { return "speedhack"; }
        virtual bool load_keybind(keybinds::Keybind *keybind) override;

        bool is_cheating() { return m_enabled && m_speed != 1.0f; }

    private:
        bool m_enabled = false;
        bool m_audio = false;
        float m_speed = 1.0f;

        friend void __fastcall CCScheduler_update_hook(cocos2d::CCScheduler *self, float dt);
    };
}