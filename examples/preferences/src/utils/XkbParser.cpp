#include <utils/XkbParser.hpp>
#include <fstream>
#include <string>
#include <vector>

namespace horizon::preferences
{
    static std::string extract_tag(const std::string& source, const std::string& parent_tag, const std::string& tag)
    {
        size_t parent_pos = source.find("<" + parent_tag + ">");
        if (parent_pos == std::string::npos) return "";
        size_t parent_end = source.find("</" + parent_tag + ">", parent_pos);
        if (parent_end == std::string::npos) return "";

        std::string content = source.substr(parent_pos, parent_end - parent_pos);
        size_t start = content.find("<" + tag + ">");
        if (start == std::string::npos) return "";
        size_t end = content.find("</" + tag + ">");
        if (end == std::string::npos) return "";

        size_t tag_len = tag.length() + 2;
        return content.substr(start + tag_len, end - start - tag_len);
    }

    std::vector<KeyboardModel> XkbParser::get_models()
    {
        std::vector<KeyboardModel> models;
        std::ifstream file("/usr/share/X11/xkb/rules/evdev.xml");
        if (!file.is_open()) return models;

        std::string line, content;
        bool in_model_list = false;

        while (std::getline(file, line))
        {
            if (line.find("<modelList>") != std::string::npos) in_model_list = true;
            if (line.find("</modelList>") != std::string::npos) break;

            if (in_model_list)
            {
                if (line.find("<model>") != std::string::npos)
                {
                    content = "";
                    while (std::getline(file, line) && line.find("</model>") == std::string::npos)
                    {
                        content += line;
                    }

                    KeyboardModel model;
                    model.id = extract_tag(content, "configItem", "name");
                    model.description = extract_tag(content, "configItem", "description");
                    model.vendor = extract_tag(content, "configItem", "vendor");
                    if (!model.id.empty()) models.push_back(model);
                }
            }
        }
        return models;
    }

    std::vector<KeyboardLayout> XkbParser::get_layouts()
    {
        std::vector<KeyboardLayout> layouts;
        std::ifstream file("/usr/share/X11/xkb/rules/evdev.xml");
        if (!file.is_open()) return layouts;

        std::string line, content;
        bool in_layout_list = false;

        while (std::getline(file, line))
        {
            if (line.find("<layoutList>") != std::string::npos) in_layout_list = true;
            if (line.find("</layoutList>") != std::string::npos) break;

            if (in_layout_list)
            {
                if (line.find("<layout>") != std::string::npos)
                {
                    content = "";
                    while (std::getline(file, line) && line.find("</layout>") == std::string::npos)
                    {
                        content += line;
                    }

                    KeyboardLayout layout;
                    layout.id = extract_tag(content, "configItem", "name");
                    layout.description = extract_tag(content, "configItem", "description");
                    if (!layout.id.empty()) layouts.push_back(layout);
                }
            }
        }
        return layouts;
    }
} // namespace horizon::preferences
