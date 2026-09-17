#include "core/I18n.h"

namespace StudioViewer {

    std::string I18n::s_locale = "en";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> I18n::s_strings;

    void I18n::init() {
        QString sysLang = QLocale::system().name().toLower();
        s_locale = sysLang.startsWith("ru") ? "ru" : "en";

        s_strings["en"] = {
            {"window_title", "Studio 3D Viewer"},
            {"toolbar_main", "Main Toolbar"},
            {"action_open", "Open"},
            {"action_focus", "Focus"},
            {"action_fly", "Fly Mode"},
            {"action_smooth", "Smooth Shading"},
            {"action_tex_filter", "Texture Filtering"},
            {"action_textures", "Textures"},
            {"action_normals", "Normals"},
            {"action_wireframe", "Wireframe"},
            {"action_floor", "Floor Grid"},
            {"action_about", "About"},
            {"about_title", "About"},
            {"about_text", "Version 1.0\nDepartment of Computer Science\nCopyright © 2026"},
            {"status_ready", "Ready"},
            {"status_loading", "Loading {filename}..."},
            {"status_reading_geo", "Reading geometry ({ext})..."},
            {"status_decompress_tex", "Decompressing textures ({count} items)..."},
            {"status_loaded", "Model loaded successfully • {tri_count} polygons"},
            {"status_error", "Failed to open"},
            {"dialog_open_title", "Select 3D Model"},
            {"dialog_filter", "3D Models (*.obj *.glb *.gltf);;Wavefront OBJ (*.obj);;glTF / GLB (*.glb *.gltf);;All Files (*)"},
            {"error_load_title", "Load Error"},
            {"error_load_msg", "Failed to read model:\n{error}"},
            {"hud_standby", "Standby..."},
            {"hud_stats_placeholder", "Polygons: —"},
            {"hud_materials_placeholder", "Materials: —"},
            {"hud_time_placeholder", "Load time: —"},
            {"hud_stats", "Polygons: {tri_count}  •  Vertices: {vert_count}"},
            {"hud_materials", "Objects: {batches}  •  Textures: {textures}  •  Normals: {normals}"},
            {"hud_time", "Loaded in {load_time}s ({workers} CPU cores)"},
            {"drop_title", "Drag & drop a 3D model into the window"},
            {"drop_subtitle", "Supported formats: Wavefront OBJ (+ MTL) and GLB / GLTF"},
            {"drop_button", "Choose File"},
            {"error_unsupported_format", "Only OBJ, GLB, and GLTF formats are supported"}
        };

        s_strings["ru"] = {
            {"window_title", "Studio 3D Viewer"},
            {"toolbar_main", "Основная панель"},
            {"action_open", "Открыть"},
            {"action_focus", "В фокус"},
            {"action_fly", "Режим полёта"},
            {"action_smooth", "Сглаживание"},
            {"action_tex_filter", "Фильтрация текстур"},
            {"action_textures", "Текстуры"},
            {"action_normals", "Нормали"},
            {"action_wireframe", "Сетка"},
            {"action_floor", "Пол"},
            {"action_about", "О программе"},
            {"about_title", "О программе"},
            {"about_text", "Версия 1.0\nФакультет компьютерных наук\nCopyright © 2026"},
            {"status_ready", "Готов к работе"},
            {"status_loading", "Загрузка {filename}..."},
            {"status_reading_geo", "Чтение геометрии ({ext})..."},
            {"status_decompress_tex", "Декомпрессия текстур ({count} шт.)..."},
            {"status_loaded", "Модель успешно загружена • {tri_count} полигонов"},
            {"status_error", "Ошибка при открытии"},
            {"dialog_open_title", "Выберите 3D Модель"},
            {"dialog_filter", "3D Модели (*.obj *.glb *.gltf);;Wavefront OBJ (*.obj);;glTF / GLB (*.glb *.gltf);;Все файлы (*)"},
            {"error_load_title", "Ошибка загрузки"},
            {"error_load_msg", "Не удалось прочесть модель:\n{error}"},
            {"hud_standby", "Ожидание..."},
            {"hud_stats_placeholder", "Полигоны: —"},
            {"hud_materials_placeholder", "Материалы: —"},
            {"hud_time_placeholder", "Загрузка: —"},
            {"hud_stats", "Полигонов: {tri_count}  •  Вершин: {vert_count}"},
            {"hud_materials", "Объектов: {batches}  •  Текстур: {textures}  •  Нормалей: {normals}"},
            {"hud_time", "Загружено за {load_time}с ({workers} CPU)"},
            {"drop_title", "Перетащите 3D модель в окно"},
            {"drop_subtitle", "Поддерживаются форматы Wavefront OBJ (+ MTL) и GLB / GLTF"},
            {"drop_button", "Выбрать файл"},
            {"error_unsupported_format", "Поддерживаются только форматы OBJ, GLB и GLTF"}
        };
    }

    QString I18n::tr(const std::string& key) {
        auto& langMap = s_strings[s_locale];
        auto it = langMap.find(key);
        if (it != langMap.end()) return QString::fromUtf8(it->second.c_str());
        auto& enMap = s_strings["en"];
        auto itEn = enMap.find(key);
        if (itEn != enMap.end()) return QString::fromUtf8(itEn->second.c_str());
        return QString::fromStdString(key);
    }

    QString I18n::tr(const std::string& key, const std::unordered_map<std::string, QString>& args) {
        QString res = tr(key);
        for (const auto& [k, v] : args) {
            res.replace(QString("{%1}").arg(QString::fromStdString(k)), v);
        }
        return res;
    }

} // namespace StudioViewer
