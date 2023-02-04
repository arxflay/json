// Tento zdrojovy kod jsem vypracoval zcela samostatne bez cizi pomoci
// Neokopiroval jsem ani neopsal jsem cizi zdrojove kody
// Nikdo mi pri vypracovani neradil
// Pokud nektery radek porusuje toto pravidlo je oznacen komentarem
// NENI MOJE TVORBA
// Poruseni techto pravidel se povazuje za podvod, ktery lze potrestat VYLOUCENIM ZE STUDIA
// Alexej Fedorenko, učo 37676 

//#define DEBUG_ARX_JSON_PARSER
//#define DEBUG_ARX_JSON_LEXER

#include "json_parser.hxx"
#include <cassert>
#include <type_traits>
#include <iostream>

struct person
{
    std::string name;
    int age;
};

namespace arx
{
    void ToJson(arx::Json &json, const person &p)
    {
        json["name"] = p.name;
        json["age"] = p.age;
    }
    
    void FromJson(const arx::Json &json, person &p)
    {
        p.age = json["age"].Get<int>();
        p.name = json["name"].Get<std::string>();
    }
};

void parseTest()
{
    arx::Json personJson(arx::Json::Parse(R"(
        { 
            "name": "\"Alexej", 
            "age" : 22,
            "isProgrammer" : true,
            "devices" : ["steamdeck",
                    {
                        "cpu" : "Ryzen 5",
                        "ram_gb": 16
                    }
                ]
        })"));

    person p = personJson.Get<person>();
    assert(p.name == "\"Alexej");
    assert(p.age == 22);
    assert(personJson["isProgrammer"].Get<bool>() == true);
    assert(personJson["devices"][0].Get<std::string>() == "steamdeck");
    assert(personJson["devices"][1]["cpu"].Get<std::string>() == "Ryzen 5");
    assert(personJson["devices"][1]["ram_gb"].Get<int>() == 16);

    std::cout << arx::Json::ToJsonStr(personJson) << '\n';
}

void parseTestFromFile()
{
    arx::Json personJson(arx::Json::FromFile("test.json"));

    person p = personJson.Get<person>();
    assert(p.name == "\"Alexej");
    assert(p.age == 22);
    assert(personJson["isProgrammer"].Get<bool>() == true);
    assert(personJson["devices"][0].Get<std::string>() == "steamdeck");
    assert(personJson["devices"][1]["cpu"].Get<std::string>() == "Ryzen 5");
    assert(personJson["devices"][1]["ram_gb"].Get<int>() == 16);
}

void jsonishSyntaxTest()
{
    arx::Json j = {
        {"name", "\"Alexej"},
        {"age", 22 },
        {"isProgrammer", true},
        {"devices", std::vector<arx::Json>
            {
                "steamdeck", 
                { 
                    {"cpu", "Ryzen 5"}, 
                    {"ram_gb", 16 }
                }
            }
        }
    };

    person p = j.Get<person>();
    assert(p.name == "\"Alexej");
    assert(p.age == 22);
    assert(j["isProgrammer"].Get<bool>() == true);
    assert(j["devices"][0].Get<std::string>() == "steamdeck");
    assert(j["devices"][1]["cpu"].Get<std::string>() == "Ryzen 5");
    assert(j["devices"][1]["ram_gb"].Get<int>() == 16);
}

int main()
{
    /*arx::Json j = {
        {"name", "\"Alexej"},
        {"age", 22 },
        {"isProgrammer", true},
        {"test", arx::JsonObjectContainer{} }
    };

    std::cout << arx::Json::ToJsonStr(j) << '\n';*/

    parseTest();
    jsonishSyntaxTest();
    parseTestFromFile();
    return 0;
}