#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    json j = json::parse("{\"height\": 128}");
    
    try {
        float h = j["height"].get<float>();
        std::cout << "get<float> works: " << h << std::endl;
    } catch (const std::exception& e) {
        std::cout << "get<float> failed: " << e.what() << std::endl;
    }
    
    try {
        float h = j["height"];
        std::cout << "implicit works: " << h << std::endl;
    } catch (const std::exception& e) {
        std::cout << "implicit failed: " << e.what() << std::endl;
    }
    
    try {
        float h = j.value("height", 0.0f);
        std::cout << "value(float) works: " << h << std::endl;
    } catch (const std::exception& e) {
        std::cout << "value(float) failed: " << e.what() << std::endl;
    }

    return 0;
}
