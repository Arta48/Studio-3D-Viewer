/**
 * @file I18n.h
 * @brief Internationalization (i18n) subsystem supporting runtime English & Russian strings.
 */

#pragma once

#include <QString>
#include <QLocale>
#include <unordered_map>
#include <string>

namespace StudioViewer {

    class I18n {
    public:
        static void init();
        static QString tr(const std::string& key);
        static QString tr(const std::string& key, const std::unordered_map<std::string, QString>& args);

    private:
        static std::string s_locale;
        static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> s_strings;
    };

} // namespace StudioViewer
