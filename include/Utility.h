#pragma once

#include "Maps.h"

class Utility : public Singleton<Utility>
{
    static auto IsEditorID(const std::string_view identifier) { return !identifier.contains('~'); }

    static FormIDAndPluginName GetFormIDAndPluginName(const std::string& identifier)
    {
        if (const auto tilde_pos{ identifier.find('~') }; tilde_pos != std::string_view::npos) {
            const auto form_id{ Maps::ToFormID(identifier.substr(0, tilde_pos)) };
            const auto plugin_name{ identifier.substr(tilde_pos + 1) };
            return { form_id, plugin_name };
        }
        logger::error("ERROR: Failed to get FormID and plugin name for {}", identifier);

        return { 0, "" };
    }

    static RE::TESBoundObject* GetBoundObject(const std::string& identifier)
    {
        if (IsEditorID(identifier)) {
            if (const auto bound_obj{ RE::TESForm::LookupByEditorID<RE::TESBoundObject>(identifier) }) {
                return bound_obj;
            }
        }
        else {
            const auto handler{ RE::TESDataHandler::GetSingleton() };
            const auto [form_id, plugin_name]{ GetFormIDAndPluginName(identifier) };
            if (const auto form{ handler->LookupForm(form_id, plugin_name) }) {
                if (const auto bound_obj{ form->As<RE::TESBoundObject>() }) {
                    return bound_obj;
                }
            }
        }
        logger::warn("WARNING: Failed to find bound object for {}", identifier);

        return nullptr;
    }

    static RE::TESLevItem* GetLevItem(const std::string& identifier)
    {
        if (IsEditorID(identifier)) {
            if (const auto lev_item{ RE::TESForm::LookupByEditorID<RE::TESLevItem>(identifier) }) {
                return lev_item;
            }
        }
        else {
            const auto handler{ RE::TESDataHandler::GetSingleton() };
            const auto [form_id, plugin_name]{ GetFormIDAndPluginName(identifier) };
            if (const auto form{ handler->LookupForm(form_id, plugin_name) }) {
                if (const auto lev_item{ form->As<RE::TESLevItem>() }) {
                    return lev_item;
                }
            }
        }
        logger::warn("WARNING: Failed to find leveled list for {}", identifier);

        return nullptr;
    }

    static Container GetContainer(const std::string& to_identifier)
    {
        if (IsEditorID(to_identifier)) {
            if (const auto form{ RE::TESForm::LookupByEditorID(to_identifier) }) {
                if (const auto cont{ form->As<RE::TESContainer>() }) {
                    return { cont, form->GetFormID(), form->GetFormType(), form->GetName() };
                }
            }
        }
        else {
            const auto handler{ RE::TESDataHandler::GetSingleton() };
            const auto [form_id, plugin_name]{ GetFormIDAndPluginName(to_identifier) };
            if (const auto form{ handler->LookupForm(form_id, plugin_name) }) {
                if (const auto cont{ form->As<RE::TESContainer>() }) {
                    return { cont, form->GetFormID(), form->GetFormType(), form->GetName() };
                }
            }
        }
        logger::warn("WARNING: Failed to find container for {}", to_identifier);

        return { nullptr, 0, RE::FormType::Container, "" };
    }

    inline static u16 player_level{};

public:
    static auto CachePlayerLevel() { player_level = RE::PlayerCharacter::GetSingleton()->GetLevel(); }

    static auto GetRandomChance()
    {
        static std::random_device                 rd;
        static std::mt19937                       rng(rd());
        static std::uniform_int_distribution<u32> distr(1, 100);

        return distr(rng);
    }

    static auto GetChance(const std::string& s)
    {
        const auto quest_pos{ s.find('?') };

        return quest_pos == std::string_view::npos ? 100 : Maps::ToUnsignedInt(s.substr(quest_pos + 1));
    }

    static auto ResolveLeveledList(RE::TESLevItem* list)
    {
        RE::BSScrapArray<RE::CALCED_OBJECT> calced_objects{};
        list->CalculateCurrentFormList(player_level, 1, calced_objects, 0, true);

        std::vector<std::pair<RE::TESBoundObject*, int>> obj_and_counts{};
        for (const auto& [form, count, pad0A, pad0C, containerItem] : calced_objects) {
            if (const auto bound_obj{ form->As<RE::TESBoundObject>() }) {
                obj_and_counts.emplace_back(bound_obj, count);
            }
        }

        return obj_and_counts;
    }

    static DistrObject BuildDistrObject(const DistrToken& distr_token) noexcept
    {
        if (const auto leveled_list{ GetLevItem(distr_token.identifier) }) {
            if (const auto cont{ GetContainer(distr_token.to_identifier) }; cont.container) {
                if (distr_token.type == DistrType::Replace || distr_token.type == DistrType::ReplaceAll) {
                    if (const auto replace_with_list{ GetLevItem(distr_token.rhs.value()) }) {
                        return { distr_token.type, nullptr, leveled_list, distr_token.filename, nullptr, replace_with_list, distr_token.count, distr_token.rhs_count, cont };
                    }
                    if (const auto replace_with_obj{ GetBoundObject(distr_token.rhs.value()) }) {
                        return { distr_token.type, nullptr, leveled_list, distr_token.filename, replace_with_obj, nullptr, distr_token.count, distr_token.rhs_count, cont };
                    }
                }
                return { distr_token.type, nullptr, leveled_list, distr_token.filename, nullptr, nullptr, distr_token.count, std::nullopt, cont, std::nullopt };
            }
        }
        else if (const auto bound_obj{ GetBoundObject(distr_token.identifier) }) {
            if (const auto cont{ GetContainer(distr_token.to_identifier) }; cont.container) {
                if (distr_token.type == DistrType::Replace || distr_token.type == DistrType::ReplaceAll) {
                    if (const auto replace_with_list{ GetLevItem(distr_token.rhs.value()) }) {
                        return { distr_token.type,      bound_obj, nullptr,           distr_token.filename, nullptr, replace_with_list, distr_token.count,
                                 distr_token.rhs_count, cont,      distr_token.chance };
                    }
                    if (const auto replace_with_obj{ GetBoundObject(distr_token.rhs.value()) }) {
                        return { distr_token.type,      bound_obj, nullptr,           distr_token.filename, replace_with_obj, nullptr, distr_token.count,
                                 distr_token.rhs_count, cont,      distr_token.chance };
                    }
                }
                return { distr_token.type, bound_obj, nullptr, distr_token.filename, nullptr, nullptr, distr_token.count, std::nullopt, cont, distr_token.chance };
            }
        }
        logger::error("ERROR: Failed to build DistrObject for {}", distr_token);

        return { DistrType::Error, nullptr, nullptr, distr_token.filename, nullptr, nullptr, std::nullopt, std::nullopt, std::nullopt };
    }
};
