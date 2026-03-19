#pragma once

#include <nlohmann/json.hpp>

namespace horizon::preferences
{
    /**
     * @class ConfigSection
     * @brief Interface for preference sections that can be serialized to/from JSON.
     */
    class ConfigSection
    {
    public:
        virtual ~ConfigSection() = default;

        /**
         * @brief Load section data from JSON.
         * @param j The JSON object representing this section.
         */
        virtual void from_json(const nlohmann::json& j) = 0;

        /**
         * @brief Serialize section data to JSON.
         * @return nlohmann::json The JSON representation of this section.
         */
        virtual nlohmann::json to_json() const = 0;
    };
} // namespace horizon::preferences
