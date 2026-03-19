#include "BalanceConfig.h"

#include "../Engine/Engine.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace balance
{
    namespace
    {
        Settings gSettings{};
        bool gLoaded = false;

        std::string Trim(const std::string& s)
        {
            const char* ws = " \t\r\n";
            const std::size_t begin = s.find_first_not_of(ws);
            if (begin == std::string::npos)
                return {};

            const std::size_t end = s.find_last_not_of(ws);
            return s.substr(begin, end - begin + 1);
        }

        bool TryParseInt(const std::string& text, int& out)
        {
            errno = 0;
            char* end = nullptr;
            const long parsed = std::strtol(text.c_str(), &end, 10);
            if (end == text.c_str() || errno == ERANGE)
                return false;

            while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)))
                ++end;

            if (*end != '\0')
                return false;

            out = static_cast<int>(parsed);
            return true;
        }

        bool TryParseDouble(const std::string& text, double& out)
        {
            errno = 0;
            char* end = nullptr;
            const double parsed = std::strtod(text.c_str(), &end);
            if (end == text.c_str() || errno == ERANGE)
                return false;

            while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)))
                ++end;

            if (*end != '\0')
                return false;

            out = parsed;
            return true;
        }

        bool TryParseFloat(const std::string& text, float& out)
        {
            double parsed = 0.0;
            if (!TryParseDouble(text, parsed))
                return false;

            out = static_cast<float>(parsed);
            return true;
        }

        void SkipWs(const std::string& text, std::size_t& i)
        {
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i])))
                ++i;
        }

        bool ParseJsonString(const std::string& text, std::size_t& i, std::string& out, std::string& err)
        {
            if (i >= text.size() || text[i] != '"')
            {
                err = "expected '\"'";
                return false;
            }

            ++i;
            out.clear();
            while (i < text.size())
            {
                const char c = text[i++];
                if (c == '"')
                    return true;

                if (c == '\\')
                {
                    if (i >= text.size())
                    {
                        err = "unterminated escape sequence";
                        return false;
                    }

                    const char esc = text[i++];
                    switch (esc)
                    {
                    case '"':  out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/':  out.push_back('/'); break;
                    case 'b':  out.push_back('\b'); break;
                    case 'f':  out.push_back('\f'); break;
                    case 'n':  out.push_back('\n'); break;
                    case 'r':  out.push_back('\r'); break;
                    case 't':  out.push_back('\t'); break;
                    default:
                        err = "unsupported escape sequence";
                        return false;
                    }
                }
                else
                {
                    out.push_back(c);
                }
            }

            err = "unterminated string";
            return false;
        }

        bool ParseJsonNumberToken(const std::string& text, std::size_t& i, std::string& out)
        {
            const std::size_t begin = i;

            if (i < text.size() && (text[i] == '-' || text[i] == '+'))
                ++i;

            bool hasDigit = false;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
            {
                ++i;
                hasDigit = true;
            }

            if (i < text.size() && text[i] == '.')
            {
                ++i;
                while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
                {
                    ++i;
                    hasDigit = true;
                }
            }

            if (!hasDigit)
                return false;

            if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
            {
                ++i;
                if (i < text.size() && (text[i] == '-' || text[i] == '+'))
                    ++i;

                bool expDigit = false;
                while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
                {
                    ++i;
                    expDigit = true;
                }

                if (!expDigit)
                    return false;
            }

            out = text.substr(begin, i - begin);
            return true;
        }

        bool TryParseFlatJsonObject(const std::string& text, std::vector<std::pair<std::string, std::string>>& outEntries, std::string& outError)
        {
            outEntries.clear();
            std::size_t i = 0;

            // Allow UTF-8 BOM at file start.
            if (text.size() >= 3 &&
                static_cast<unsigned char>(text[0]) == 0xEF &&
                static_cast<unsigned char>(text[1]) == 0xBB &&
                static_cast<unsigned char>(text[2]) == 0xBF)
            {
                i = 3;
            }

            SkipWs(text, i);

            if (i >= text.size() || text[i] != '{')
            {
                outError = "root must be an object";
                return false;
            }
            ++i;

            for (;;)
            {
                SkipWs(text, i);
                if (i >= text.size())
                {
                    outError = "unexpected end of file";
                    return false;
                }

                if (text[i] == '}')
                {
                    ++i;
                    break;
                }

                std::string key;
                if (!ParseJsonString(text, i, key, outError))
                    return false;

                SkipWs(text, i);
                if (i >= text.size() || text[i] != ':')
                {
                    outError = "expected ':' after key";
                    return false;
                }
                ++i;

                SkipWs(text, i);
                if (i >= text.size())
                {
                    outError = "missing value";
                    return false;
                }

                std::string value;
                if (text[i] == '"')
                {
                    if (!ParseJsonString(text, i, value, outError))
                        return false;
                }
                else if (!text.compare(i, 4, "true"))
                {
                    value = "1";
                    i += 4;
                }
                else if (!text.compare(i, 5, "false"))
                {
                    value = "0";
                    i += 5;
                }
                else if (!text.compare(i, 4, "null"))
                {
                    value = "0";
                    i += 4;
                }
                else
                {
                    if (!ParseJsonNumberToken(text, i, value))
                    {
                        outError = "value must be number/bool/string";
                        return false;
                    }
                }

                outEntries.emplace_back(std::move(key), std::move(value));

                SkipWs(text, i);
                if (i >= text.size())
                {
                    outError = "unexpected end after value";
                    return false;
                }

                if (text[i] == ',')
                {
                    ++i;
                    continue;
                }
                if (text[i] == '}')
                {
                    ++i;
                    break;
                }

                outError = "expected ',' or '}'";
                return false;
            }

            SkipWs(text, i);
            if (i != text.size())
            {
                outError = "trailing content after root object";
                return false;
            }

            return true;
        }

        void WarnInvalidValue(const std::string& key, const std::string& value)
        {
            Engine::GetLogger().LogWarning("[Balance] Invalid value for '" + key + "': '" + value + "'");
        }

        void WarnUnknownKey(const std::string& key)
        {
            Engine::GetLogger().LogWarning("[Balance] Unknown key ignored: '" + key + "'");
        }

        void ApplySetting(Settings& settings, const std::string& key, const std::string& value)
        {
            if (key == "spawn.total_cores")
            {
                if (!TryParseInt(value, settings.spawn.totalCoreCount)) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.interval_tier0")
            {
                if (!TryParseDouble(value, settings.spawn.intervalByTierSec[0])) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.interval_tier1")
            {
                if (!TryParseDouble(value, settings.spawn.intervalByTierSec[1])) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.interval_tier2")
            {
                if (!TryParseDouble(value, settings.spawn.intervalByTierSec[2])) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.max_enemies_tier0")
            {
                if (!TryParseInt(value, settings.spawn.maxEnemiesByTier[0])) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.max_enemies_tier1")
            {
                if (!TryParseInt(value, settings.spawn.maxEnemiesByTier[1])) WarnInvalidValue(key, value);
            }
            else if (key == "spawn.max_enemies_tier2")
            {
                if (!TryParseInt(value, settings.spawn.maxEnemiesByTier[2])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.early_end_sec")
            {
                if (!TryParseDouble(value, settings.phase.earlyEndSec)) WarnInvalidValue(key, value);
            }
            else if (key == "phase.mid_end_sec")
            {
                if (!TryParseDouble(value, settings.phase.midEndSec)) WarnInvalidValue(key, value);
            }
            else if (key == "phase.spawn_interval_mul_early")
            {
                if (!TryParseDouble(value, settings.phase.spawnIntervalMul[0])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.spawn_interval_mul_mid")
            {
                if (!TryParseDouble(value, settings.phase.spawnIntervalMul[1])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.spawn_interval_mul_late")
            {
                if (!TryParseDouble(value, settings.phase.spawnIntervalMul[2])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.max_enemies_mul_early")
            {
                if (!TryParseDouble(value, settings.phase.maxEnemiesMul[0])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.max_enemies_mul_mid")
            {
                if (!TryParseDouble(value, settings.phase.maxEnemiesMul[1])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.max_enemies_mul_late")
            {
                if (!TryParseDouble(value, settings.phase.maxEnemiesMul[2])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.fire_cooldown_mul_early")
            {
                if (!TryParseDouble(value, settings.phase.fireCooldownMul[0])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.fire_cooldown_mul_mid")
            {
                if (!TryParseDouble(value, settings.phase.fireCooldownMul[1])) WarnInvalidValue(key, value);
            }
            else if (key == "phase.fire_cooldown_mul_late")
            {
                if (!TryParseDouble(value, settings.phase.fireCooldownMul[2])) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.interval")
            {
                if (!TryParseDouble(value, settings.weapon.machineGunFireIntervalSec)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.muzzle_offset")
            {
                if (!TryParseFloat(value, settings.weapon.machineMuzzleOffset)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.bullet_speed")
            {
                if (!TryParseFloat(value, settings.weapon.machineBulletSpeed)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.bullet_life")
            {
                if (!TryParseDouble(value, settings.weapon.machineBulletLifeSec)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.bullet_damage")
            {
                if (!TryParseInt(value, settings.weapon.machineBulletDamage)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.machine.hit_radius")
            {
                if (!TryParseFloat(value, settings.weapon.machineBulletHitRadius)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.interval")
            {
                if (!TryParseDouble(value, settings.weapon.shotgunFireIntervalSec)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.muzzle_offset")
            {
                if (!TryParseFloat(value, settings.weapon.shotgunMuzzleOffset)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.pellet_count")
            {
                if (!TryParseInt(value, settings.weapon.shotgunPelletCount)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.spread_deg")
            {
                if (!TryParseFloat(value, settings.weapon.shotgunSpreadDeg)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.bullet_speed")
            {
                if (!TryParseFloat(value, settings.weapon.shotgunBulletSpeed)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.bullet_life")
            {
                if (!TryParseDouble(value, settings.weapon.shotgunBulletLifeSec)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.bullet_damage")
            {
                if (!TryParseInt(value, settings.weapon.shotgunBulletDamage)) WarnInvalidValue(key, value);
            }
            else if (key == "weapon.shotgun.hit_radius")
            {
                if (!TryParseFloat(value, settings.weapon.shotgunBulletHitRadius)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.attack_cooldown")
            {
                if (!TryParseDouble(value, settings.enemy.attackCooldownSec)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.contact_damage")
            {
                if (!TryParseInt(value, settings.enemy.contactDamage)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.overlap_min_distance")
            {
                if (!TryParseFloat(value, settings.enemy.overlapMinDistance)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.overlap_world_padding")
            {
                if (!TryParseFloat(value, settings.enemy.overlapWorldPadding)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.spawn_edge_inset")
            {
                if (!TryParseFloat(value, settings.enemy.spawnEdgeInset)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.spawn_min_gap")
            {
                if (!TryParseFloat(value, settings.enemy.spawnMinGap)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.normal.move_speed")
            {
                if (!TryParseFloat(value, settings.enemy.normal.moveSpeed)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.normal.health")
            {
                if (!TryParseInt(value, settings.enemy.normal.health)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.normal.attack_radius")
            {
                if (!TryParseFloat(value, settings.enemy.normal.attackRadius)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.normal.knockback_force")
            {
                if (!TryParseFloat(value, settings.enemy.normal.knockbackForce)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.normal.scale")
            {
                if (!TryParseFloat(value, settings.enemy.normal.scale)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.fast.move_speed")
            {
                if (!TryParseFloat(value, settings.enemy.fast.moveSpeed)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.fast.health")
            {
                if (!TryParseInt(value, settings.enemy.fast.health)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.fast.attack_radius")
            {
                if (!TryParseFloat(value, settings.enemy.fast.attackRadius)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.fast.knockback_force")
            {
                if (!TryParseFloat(value, settings.enemy.fast.knockbackForce)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.fast.scale")
            {
                if (!TryParseFloat(value, settings.enemy.fast.scale)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.heavy.move_speed")
            {
                if (!TryParseFloat(value, settings.enemy.heavy.moveSpeed)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.heavy.health")
            {
                if (!TryParseInt(value, settings.enemy.heavy.health)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.heavy.attack_radius")
            {
                if (!TryParseFloat(value, settings.enemy.heavy.attackRadius)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.heavy.knockback_force")
            {
                if (!TryParseFloat(value, settings.enemy.heavy.knockbackForce)) WarnInvalidValue(key, value);
            }
            else if (key == "enemy.heavy.scale")
            {
                if (!TryParseFloat(value, settings.enemy.heavy.scale)) WarnInvalidValue(key, value);
            }
            else
            {
                WarnUnknownKey(key);
            }
        }

        void ClampToSafeRanges(Settings& settings)
        {
            settings.spawn.totalCoreCount = (std::max)(1, settings.spawn.totalCoreCount);

            for (double& interval : settings.spawn.intervalByTierSec)
                interval = (std::max)(0.05, interval);
            for (int& maxEnemies : settings.spawn.maxEnemiesByTier)
                maxEnemies = (std::max)(1, maxEnemies);

            settings.phase.earlyEndSec = (std::max)(5.0, settings.phase.earlyEndSec);
            settings.phase.midEndSec = (std::max)(settings.phase.earlyEndSec + 5.0, settings.phase.midEndSec);
            for (double& mul : settings.phase.spawnIntervalMul)
                mul = (std::max)(0.10, mul);
            for (double& mul : settings.phase.maxEnemiesMul)
                mul = (std::max)(0.10, mul);
            for (double& mul : settings.phase.fireCooldownMul)
                mul = (std::max)(0.10, mul);

            settings.weapon.machineGunFireIntervalSec = (std::max)(0.01, settings.weapon.machineGunFireIntervalSec);
            settings.weapon.shotgunFireIntervalSec = (std::max)(0.01, settings.weapon.shotgunFireIntervalSec);
            settings.weapon.machineMuzzleOffset = (std::max)(0.0f, settings.weapon.machineMuzzleOffset);
            settings.weapon.shotgunMuzzleOffset = (std::max)(0.0f, settings.weapon.shotgunMuzzleOffset);

            settings.weapon.machineBulletSpeed = (std::max)(1.0f, settings.weapon.machineBulletSpeed);
            settings.weapon.shotgunBulletSpeed = (std::max)(1.0f, settings.weapon.shotgunBulletSpeed);
            settings.weapon.machineBulletLifeSec = (std::max)(0.05, settings.weapon.machineBulletLifeSec);
            settings.weapon.shotgunBulletLifeSec = (std::max)(0.05, settings.weapon.shotgunBulletLifeSec);
            settings.weapon.machineBulletDamage = (std::max)(0, settings.weapon.machineBulletDamage);
            settings.weapon.shotgunBulletDamage = (std::max)(0, settings.weapon.shotgunBulletDamage);
            settings.weapon.machineBulletHitRadius = (std::max)(0.0f, settings.weapon.machineBulletHitRadius);
            settings.weapon.shotgunBulletHitRadius = (std::max)(0.0f, settings.weapon.shotgunBulletHitRadius);
            settings.weapon.shotgunPelletCount = (std::max)(1, settings.weapon.shotgunPelletCount);
            settings.weapon.shotgunSpreadDeg = (std::max)(0.0f, settings.weapon.shotgunSpreadDeg);

            settings.enemy.attackCooldownSec = (std::max)(0.05, settings.enemy.attackCooldownSec);
            settings.enemy.contactDamage = (std::max)(0, settings.enemy.contactDamage);
            settings.enemy.overlapMinDistance = (std::max)(0.0f, settings.enemy.overlapMinDistance);
            settings.enemy.overlapWorldPadding = (std::max)(0.0f, settings.enemy.overlapWorldPadding);
            settings.enemy.spawnEdgeInset = (std::max)(0.0f, settings.enemy.spawnEdgeInset);
            settings.enemy.spawnMinGap = (std::max)(0.0f, settings.enemy.spawnMinGap);

            auto clampVariant = [](EnemyVariantSettings& variant)
            {
                variant.moveSpeed = (std::max)(1.0f, variant.moveSpeed);
                variant.health = (std::max)(1, variant.health);
                variant.attackRadius = (std::max)(1.0f, variant.attackRadius);
                variant.knockbackForce = (std::max)(0.0f, variant.knockbackForce);
                variant.scale = (std::max)(0.1f, variant.scale);
            };

            clampVariant(settings.enemy.normal);
            clampVariant(settings.enemy.fast);
            clampVariant(settings.enemy.heavy);
        }

        void LoadFromFile()
        {
            gSettings = Settings{};

            constexpr const char* kJsonPath = "assets/config/gameplay_balance.json";
            {
                std::ifstream jsonFile(kJsonPath);
                if (jsonFile.is_open())
                {
                    std::string jsonText((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
                    std::vector<std::pair<std::string, std::string>> entries;
                    std::string parseError;
                    if (TryParseFlatJsonObject(jsonText, entries, parseError))
                    {
                        for (const auto& entry : entries)
                        {
                            ApplySetting(gSettings, entry.first, entry.second);
                        }

                        ClampToSafeRanges(gSettings);
                        Engine::GetLogger().LogEvent("[Balance] Loaded assets/config/gameplay_balance.json");
                        return;
                    }

                    Engine::GetLogger().LogWarning(
                        std::string("[Balance] Failed to parse assets/config/gameplay_balance.json: ") + parseError +
                        " (falling back to .cfg)");
                }
            }

            constexpr const char* kBalancePath = "assets/config/gameplay_balance.cfg";
            std::ifstream file(kBalancePath);
            if (!file.is_open())
            {
                Engine::GetLogger().LogWarning("[Balance] Config not found: gameplay_balance.json/.cfg (using defaults)");
                ClampToSafeRanges(gSettings);
                return;
            }

            std::string line;
            int lineNo = 0;
            while (std::getline(file, line))
            {
                ++lineNo;

                const std::size_t commentPos = line.find('#');
                if (commentPos != std::string::npos)
                    line.erase(commentPos);

                const std::string trimmed = Trim(line);
                if (trimmed.empty())
                    continue;

                const std::size_t eqPos = trimmed.find('=');
                if (eqPos == std::string::npos)
                {
                    Engine::GetLogger().LogWarning("[Balance] Invalid line " + std::to_string(lineNo) + " (missing '=')");
                    continue;
                }

                const std::string key = Trim(trimmed.substr(0, eqPos));
                const std::string value = Trim(trimmed.substr(eqPos + 1));
                if (key.empty() || value.empty())
                {
                    Engine::GetLogger().LogWarning("[Balance] Invalid line " + std::to_string(lineNo) + " (empty key/value)");
                    continue;
                }

                ApplySetting(gSettings, key, value);
            }

            ClampToSafeRanges(gSettings);
            Engine::GetLogger().LogEvent("[Balance] Loaded assets/config/gameplay_balance.cfg");
        }
    }

    const Settings& Get()
    {
        if (!gLoaded)
            Reload();
        return gSettings;
    }

    void Reload()
    {
        LoadFromFile();
        gLoaded = true;
    }
}
