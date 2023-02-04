// Tento zdrojovy kod jsem vypracoval zcela samostatne bez cizi pomoci
// Neokopiroval jsem ani neopsal jsem cizi zdrojove kody
// Nikdo mi pri vypracovani neradil
// Pokud nektery radek porusuje toto pravidlo je oznacen komentarem
// NENI MOJE TVORBA
// Poruseni techto pravidel se povazuje za podvod, ktery lze potrestat VYLOUCENIM ZE STUDIA
// Alexej Fedorenko, učo 37676 

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <variant>
#include <map>
#include <exception>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <stack>
#include <fstream>
#include <sstream>

namespace arx
{
    class Json;

    class JsonObjectContainer : public std::map<std::string, Json>
    {
        
    };

    template<typename T>
    concept JsonSerialiazable = requires(Json &json, const  std::decay_t<T> &value)
    {
        ToJson(json, value);
    };
    

    template<typename T>
    concept JsonDeserialiazable = requires(const Json &json, std::decay_t<T> &value)
    {
        FromJson(json, value);
    };

    struct NonDefaultType
    {
    };

    enum class JsonValueType
    {
        NULL_OBJ = 0,
        BOOL,
        INT,
        UINT,
        FLOATING,
        STRING,
        ARRAY,
        OBJECT_CONTAINER
    };

    const char *JsonValueTypeStr(JsonValueType type)
    {
        const char *strs[]{ "null", "bool", "int", "uint", "floating_point", "string", "array", "json_object_container"};
        return strs[(int)type];
    }

    class JsonExpectionTypeError : public std::runtime_error
    {
    public:
        JsonExpectionTypeError (JsonValueType expected, JsonValueType was) 
            : runtime_error(std::string("Type mismatch. Expected ") + JsonValueTypeStr(expected) 
            + " but was " + JsonValueTypeStr(was)) 
        {
        }
    };

    class JsonValue final
    {
        friend class Json;

    private:      
        union value
        {
            bool bool_value; 
            int64_t int_value;
            uint64_t uint_value;
            double floating_value;
            JsonObjectContainer *json_container;
            std::string *string_value; 
            std::vector<Json> *json_array;
        } m_value{ };

        JsonValueType m_type{ JsonValueType::NULL_OBJ };

    public:
        JsonValue() = default;

        JsonValue(JsonValue && value)
        {          
            using std::swap;
            swap(m_value, value.m_value);
        }

        JsonValue(const JsonValue &value) : JsonValue(value.Clone())
        {
        }

        ~JsonValue()
        {
            CleanUp();
        }

        JsonValue &operator=(JsonValue && value)
        {
            if (this == &value)
                return *this;

            using std::swap;
            swap(m_value, value.m_value);
            swap(m_type, value.m_type);

            return *this;
        }

        JsonValue &operator=(const JsonValue &value)
        {
            if (this == &value)
                return *this;

            *this = value.Clone();

            return *this;
            
        }

private:
        JsonValueType GetType()
        {
            return m_type;
        }

        template <typename T>
        T GetValue()
        {
            using type = std::remove_cvref_t<T>;

            if constexpr(std::is_same<type, std::nullptr_t>::value)
            {
                CheckType(JsonValueType::NULL_OBJ);
                return nullptr;
            }
            else if constexpr(std::is_same<type, bool>::value)
            {
                CheckType(JsonValueType::BOOL);
                return static_cast<T>(m_value.uint_value);
            }
            else if constexpr(std::is_unsigned<type>::value)
            {   
                CheckType(JsonValueType::UINT, std::array{ JsonValueType::INT, JsonValueType::FLOATING });
                return static_cast<T>(m_value.uint_value);
            }
            else if constexpr(std::is_signed<type>::value)
            {
                CheckType(JsonValueType::INT, std::array{ JsonValueType::UINT, JsonValueType::FLOATING });
                return static_cast<T>(m_value.int_value);
            }
            else if constexpr(std::is_floating_point<type>::value)
            {
                CheckType(JsonValueType::FLOATING, std::array{ JsonValueType::INT, JsonValueType::UINT });
                return static_cast<T>(m_value.floating_value);
            }
            else if constexpr(std::is_same<type, std::vector<Json>>::value)
            {
                CheckType(JsonValueType::ARRAY);
                return *(m_value.json_array);
            }
            else if constexpr(std::is_same<type, JsonObjectContainer>::value)
            {
                CheckType(JsonValueType::OBJECT_CONTAINER);
                return *(m_value.json_container);
            }
            else if constexpr(std::is_same<type, std::string>::value)
            {
                CheckType(JsonValueType::STRING);
                return *(m_value.string_value);
            }
            else
                return NonDefaultType();
        }
        /**
         * @brief Set the Value object
         * 
         * @tparam T 
         * @param value 
         * @return true if is default type
         * @return false if non default type
         */
        template <typename T>
        auto SetValue(T && value)
        {
            using type = std::remove_cvref_t<T>;

            if constexpr (std::is_same<type, std::nullptr_t>::value)
            {
                UpdateType(JsonValueType::NULL_OBJ);
                return std::true_type{};
            }
            else if constexpr(std::is_same<type, bool>::value)
            {
                UpdateType(JsonValueType::BOOL);
                m_value.bool_value = value;
                return std::true_type{};
            }
            else if constexpr(std::is_unsigned<type>::value)
            {
                UpdateType(JsonValueType::UINT);
                m_value.uint_value = value;
                return std::true_type{};
            }
            else if constexpr(std::is_signed<type>::value)
            {
                UpdateType(JsonValueType::INT);
                m_value.int_value = value;
                return std::true_type{};
            }
            else if constexpr(std::is_floating_point<type>::value)
            {
                UpdateType(JsonValueType::FLOATING);
                m_value.floating_value = value;
                return std::true_type{};
            }
            else if constexpr(std::is_same<type, JsonObjectContainer>::value)
            {
                UpdateType(JsonValueType::OBJECT_CONTAINER);
                *(m_value.json_container) = value;
                return std::true_type{};
            }
            else if constexpr(std::is_same<type, std::vector<Json>>::value)
            {
                UpdateType(JsonValueType::ARRAY);
                *(m_value.json_array) = value;
                return std::true_type{};
            }
            else if constexpr(std::is_same<type, std::string>::value || std::is_same<type, const char*>::value)
            {
                UpdateType(JsonValueType::STRING);
                *(m_value.string_value) = value;
                return std::true_type{};
            }
            else
                return std::false_type{ };
        }

        void UpdateType(JsonValueType newType)
        {
            if (m_type != newType)
            {
                CleanUp();
                m_type = newType;
                switch(m_type)
                {
                    case JsonValueType::OBJECT_CONTAINER:
                        m_value.json_container = new JsonObjectContainer();
                        break;
                    case JsonValueType::ARRAY:
                        m_value.json_array = new std::vector<Json>{};
                        break;
                    case JsonValueType::STRING:
                        m_value.string_value = new std::string{};
                        break;
                    default:
                        break;
                }
            }
        }

        JsonValue Clone() const
        {
            JsonValue retVal;
            retVal.m_type = m_type;

            switch(m_type)
            {
                case JsonValueType::OBJECT_CONTAINER:
                    retVal.m_value.json_container = new JsonObjectContainer(*m_value.json_container);
                    break;
                case JsonValueType::ARRAY:
                    retVal.m_value.json_array = new std::vector<Json>(*m_value.json_array);
                    break;
                case JsonValueType::STRING:
                    retVal.m_value.string_value = new std::string(*m_value.string_value);
                    break;
                default:
                    retVal.m_value = m_value;
                    break;
            }

            return retVal;
        }

        void CheckType(JsonValueType expected)
        {
            if (m_type != expected)
                throw JsonExpectionTypeError(expected, m_type);
        }

        template<size_t N>
        void CheckType(JsonValueType expected, const std::array<JsonValueType, N> &conversions)
        {
            if (m_type != expected && std::find(conversions.begin(), conversions.end(), expected) == conversions.end())
                throw JsonExpectionTypeError(expected, m_type);
        }

        void CleanUp()
        {
            switch(m_type)
            {
                case JsonValueType::OBJECT_CONTAINER:
                    delete m_value.json_container;
                    break;
                case JsonValueType::ARRAY:
                    delete m_value.json_array;
                    break;
                case JsonValueType::STRING:
                    delete m_value.string_value;
                    break;
                default:
                    break;
            }
        }
    };

    template<typename T, typename T2>
    concept not_same = !std::is_same<T, T2>::value;

    template<typename T>
    concept JsonDefaultType = requires(JsonValue &js, T && value)
    {
        { js.GetValue<std::remove_cvref_t<T>>() } -> not_same<NonDefaultType>;
        { js.SetValue<std::remove_cvref_t<T>>(std::forward<T>(value)) } -> std::same_as<std::true_type>;
    };


    template<typename...Ts>
    struct type_sequence
    {
        template<template<typename...> class T > using set = T<Ts...>;
        template<typename...Ts2> using concat = type_sequence<Ts..., Ts2...>;
        template<template<typename...> class T> using create_multiple = type_sequence<T<Ts>...>;
        template<typename T> using merge = typename T::concat<Ts...>;
    };

    class IJsonLexer;
    class IJsonParser;
    class IJsonConverter;

    class Json final
    {                      
    public:
        Json() = default;

        /*move constructor*/
        Json(Json && value)
        {
            using std::swap;
            swap(m_json_obj, value.m_json_obj);     
        }

        /*copy constructor*/
        Json(const Json &value)
        {
            m_json_obj = value.m_json_obj;
        }

        /**
         * @brief Constructs Json object from known types
         * 
         * @tparam T that matches concept JsonDefaultType or is JsonSerialiazable (has FromJson function)
         * @param value 
         */
        template<typename T>
            requires JsonDefaultType<std::remove_cvref_t<T>> || JsonSerialiazable<std::remove_cvref_t<T>>
        Json(T && value)
        {
            Set<T>(std::forward<T>(value));
        }

        /**
         * @brief Special version of constructor that converts const char* to std::string
         * 
         * @param value text
         */
        Json(const char *value)
        {
            m_json_obj.SetValue(std::string(value));
        }

        /**
         * @brief Makes possible to construct Json in jsonish/python style
         * 
         * @param js 
         */
        Json(std::initializer_list<std::pair<std::string, Json>> js)
        {
            m_json_obj.SetValue<JsonObjectContainer>(JsonObjectContainer());

            for (auto &[key, val] : js)
                operator[](key) = val;
        }
        
        /**
         * @brief Special version of assignment that converts const char* to std::string
         * 
         * @param value text
         * @return Json& 
         */
        Json &operator=(const char *value)
        {
            m_json_obj.SetValue(value);
            return *this;
        }

        /*move assignment*/
        Json &operator=(Json && j)
        {
            using std::swap;
            swap(m_json_obj, j.m_json_obj);

            return *this;
        }

        /*copy assignment*/
        Json &operator=(const Json &j)
        {
            if (this == &j)
                return *this;
            
            m_json_obj = j.m_json_obj;

            return *this;
        }

        template<typename T>
            requires JsonDefaultType<std::remove_cvref_t<T>> || JsonSerialiazable<std::remove_cvref_t<T>>
        Json &operator=(T && value)
        {
            Set<T>(std::forward<T>(value));
            return *this;
        }

        /**
         * @brief Makes possible to assign object to Json in jsonish/python style
         * 
         * @param js 
         */
        Json &operator=(std::initializer_list<std::pair<std::string, Json>> js)
        {
            Set<JsonObjectContainer>(JsonObjectContainer());

            for (auto &[key, val] : js)
                operator[](key) = val;

            return *this;
        }
        
        /**
         * @brief 
         * 
         * @throw JsonExpectionTypeError - if Json object is not a JsonObjectContainer
         * @param key 
         * @return const Json& 
         */
        const Json &operator[](const std::string &key) const
        {
            return GetValueByKey(key);
        }

        /**
         * @brief 
         * 
         * @throw JsonExpectionTypeError - if Json object is not a JsonObjectContainer
         * @param key 
         * @return const Json& 
         */
        Json &operator[](const std::string &key)
        {
            return GetValueByKey(key);
        }

        /**
         * @brief 
         * 
         * @throw JsonExpectionTypeError - if Json object is not an array
         * @param key 
         * @return const Json& 
         */
        const Json &operator[](size_t index) const
        {
            return GetValueByIndex(index);
        }

        /**
         * @brief 
         * 
         * @throw JsonExpectionTypeError - if Json object is not an array
         * @param key 
         * @return const Json& 
         */
        Json &operator[](size_t index)
        {
            return const_cast<Json&>(GetValueByIndex(index));
        }

        /**
         * @brief Get value from Json object
         * 
         * @tparam T object that mets requirements of JsonDefaultType
         * @return copy of value that is stored in Json
         */
        template<typename T>
            requires JsonDefaultType<std::remove_cvref_t<T>>
        auto Get() const
        {
            return m_json_obj.GetValue<std::remove_cvref_t<T>>();
        }

        /**
         * @brief Get value from Json object
         * 
         * @tparam T object that mets requirements of JsonDeserialiazable
         * @return copy of value that is stored in Json
         */
        template<typename T>
            requires JsonDeserialiazable<std::remove_cvref_t<T>>
        auto Get() const
        {
            std::remove_cvref_t<T> nonStandartValue;
            FromJson(*this, nonStandartValue);
            return nonStandartValue;
        }

        /**
         * @brief Get object stored in Json by const reference
         * 
         * @tparam T object that mets requirements of sonDefaultType
         * @return const reference to value that is stored in Json
         */
        template<typename T>
            requires JsonDefaultType<std::remove_cvref_t<T>>
        std::add_lvalue_reference_t<std::add_const_t<T>> GetConstRef() const
        {
            return m_json_obj.GetValue<std::add_lvalue_reference_t<std::add_const_t<T>>>();
        }
        
        /**
         * @brief Special version of Set that converts const char* to std::string
         * 
         * @param value text
         * @return Json& 
         */
        void Set(const char *value)
        {
            m_json_obj.SetValue(std::string(value));
        }
        
        /**
         * @brief Set value to Json object
         * 
         * @tparam T object that mets requirements of JsonDefaultType
         * @param value value that we want to store
         */
        template<typename T>
            requires JsonDefaultType<std::remove_cvref_t<T>> 
        void Set(T && value)
        {
            m_json_obj.SetValue(std::forward<T>(value));
        }

        /**
         * @brief Set value to Json object
         * 
         * @tparam T object that mets requirements of JsonSerialiazable
         * @param value value that we want to store
         */
        template<typename T>
            requires JsonSerialiazable<std::remove_cvref_t<T>>
        void Set(T && value)
        {
            ToJson(*this, std::forward<T>(value));
        }

        /**
         * @brief Adds object in the end of array
         * @throw JsonExpectionTypeError if object is not array
         * @param obj 
         */
        void PushBack(const Json &obj)
        {
            std::vector<Json> &array = m_json_obj.GetValue<std::vector<Json>&>();
            array.push_back(obj);
        }

        /**
         * @brief Adds object in the end of array
         * @throw JsonExpectionTypeError if object is not array
         * @param obj
         */
        void PushBack(Json &&obj)
        {
            std::vector<Json> &array = m_json_obj.GetValue<std::vector<Json>&>();
            array.push_back(std::move(obj));
        }

        /**
         * @brief Gets amount of objects that are contained in array or objectContainer
         * @throw std::runtime_error if object is not an array/objectContainer
         * @return size_t 
         */
        size_t Count() const
        {
            if (GetType() == JsonValueType::ARRAY)
                return m_json_obj.GetValue<const std::vector<Json>&>().size();
            else if (GetType() == JsonValueType::OBJECT_CONTAINER)
                return m_json_obj.GetValue<const JsonObjectContainer&>().size();
            else
                throw std::runtime_error("not an array/object container");

            return 0;

        }

        /**
         * @brief Get the Type of object
         * 
         * @return JsonValueType 
         */
        JsonValueType GetType() const { return m_json_obj.GetType(); }

        /**
         * @brief Parses text and returns Json object.
         * This version accept custom Json parser that matches interface IJsonParser
         * 
         * @param text 
         * @param parser 
         * @return Json 
         */
        static Json Parse(const std::string &text, IJsonParser &parser);

        /**
         * @brief Parses text and returns Json object.
         * This version accept custom Json lexer that matches interface IJsonLexer
         * 
         * @param text 
         * @param lexer class that matches interface
         * @return Json 
         */
        static Json Parse(const std::string &text, IJsonLexer &lexer);

        /**
         * @brief Parses text and returns Json object.
         * 
         * @param text 
         * @return Json 
         */
        static Json Parse(const std::string &text);

        /**
         * @brief Loads json file, parses it and returns Json object
         * 
         * @param filename 
         * @return Json 
         */
        static Json FromFile(const std::string &filename);

        /**
         * @brief Converts json object to json string
         * This version accepts custom Converter that matches interface
         * 
         * @param j Json object
         * @return std::string 
         */ 
        static std::string ToJsonStr(const Json &j, const IJsonConverter &convertor);

        /**
         * @brief Converts json object to json string
         * 
         * @param j Json object
         * @return std::string 
         */ 
        static std::string ToJsonStr(const Json &j);

        /**
         * @brief Get value by string key
         * @throw JsonExpectionTypeError - if Json object is not an JsonObjectContainer
         * @param key string key
         * @return const Json& 
         */ 
        const Json &GetValueByKey(const std::string &key) const
        {
            const JsonObjectContainer &container = m_json_obj.GetValue<const JsonObjectContainer&>();
            return container.at(key);
        }

        /**
         * @brief Get value by string key
         * @throw JsonExpectionTypeError - if Json object is not an JsonObjectContainer
         * @param key string key
         * @return Json& 
         */ 
        Json &GetValueByKey(const std::string &key)
        {
            if (m_json_obj.GetType() == JsonValueType::NULL_OBJ)
                m_json_obj.SetValue(JsonObjectContainer{ });

            JsonObjectContainer &container = m_json_obj.GetValue<JsonObjectContainer&>();
            if (!container.contains(key))
                container[key] = Json{ };

            return container[key];
        }

        /**
         * @brief Get value by index
         * @throw JsonExpectionTypeError - if json object is not an std::vector<Json>
         * @param index unsigned integer
         * @return const Json& 
         */
        const Json &GetValueByIndex(size_t index) const
        {
            const std::vector<Json> &array = m_json_obj.GetValue<const std::vector<Json>&>();
            return array.at(index);
        }

        /**
         * @brief Get value by index
         * @throw JsonExpectionTypeError - if Json object is not an std::vector<Json>
         * @param index unsigned integer
         * @return Json& 
         */
        Json &GetValueByIndex(size_t index)
        {
            std::vector<Json> &array = m_json_obj.GetValue<std::vector<Json>&>();
            return array.at(index);
        }

    private:
        mutable JsonValue m_json_obj{}; 
    };

    template<typename RetT, typename InstanceT, typename...Ts>
    class StateMachine
    {
    public:

        StateMachine(uint beginState, std::initializer_list<typename std::vector<std::function<RetT(StateMachine<RetT, InstanceT, Ts...> &, Ts...)>>::value_type> transitions)
            : m_transitions(std::move(transitions)), m_state(beginState), m_functor(m_transitions[m_state])
        {
        }

        void Process(Ts...value)
        {
            m_functor(*this, value...);
        }

        InstanceT *GetInstance()
        {
            return m_instance;
        }

        void SetInstance(InstanceT *instance)
        {
            m_instance = instance;
        }

        void SetState(uint state) { m_state = state; m_functor = m_transitions.at(state); }
        uint GetState() { return m_state; }

    private:
        uint m_state;
        InstanceT *m_instance;
        
        std::vector<std::function<RetT(StateMachine<RetT, InstanceT, Ts...> &, Ts...)>> m_transitions;
        std::function<RetT(StateMachine<RetT, InstanceT, Ts...> &, Ts...)> m_functor;
    };

    template<typename RetT, typename...Ts>
    class StateMachine<RetT, void, Ts...>
    {
    public:

        StateMachine(uint beginState, std::initializer_list<typename std::vector<std::function<RetT(StateMachine<RetT, Ts...> &, Ts...)>>::value_type> transitions)
            : m_transitions(transitions), m_state(beginState), m_functor(m_transitions[m_state])
        {
        }

        void Process(Ts...value)
        {
            m_functor(*this, value...);
        }

        void SetState(uint state) { m_state = state; m_functor = m_transitions.at(state); }
        uint GetState() { return m_state; }

    private:
        uint m_state;
        
        std::vector<std::function<RetT(StateMachine<RetT, Ts...> &, Ts...)>> m_transitions;
        std::function<RetT(StateMachine<RetT, Ts...> &, Ts...)> m_functor;
    };

    template<typename T, typename RetT, typename...Tsfn>
    class obj_fn_wrapper
    {
    public:
        obj_fn_wrapper(T *instance, RetT (T::*fn)(Tsfn...))
            : m_instance(instance), m_function(fn)
        {
        }

        RetT operator()(Tsfn...parameters)
        {
           return ((*m_instance).*(m_function))(parameters...);
        }

    private:
        T *m_instance;
        RetT (T::*m_function)(Tsfn...);
    };

    template<typename T, typename...Tsfn>
    class obj_fn_wrapper<T, void, Tsfn...>
    {
    public:
        obj_fn_wrapper(T *instance, void (T::*fn)(Tsfn...))
            : m_instance(instance), m_function(fn)
        {
        }

        void operator()(Tsfn...parameters)
        {
           ((*m_instance).*(m_function))(parameters...);
        }

    private:
        T *m_instance;
        void (T::*m_function)(Tsfn...);
    };

    /* lexer part */

    enum LexerValueType
    {
        OPEN_CURLY_BRACKET = 0, //'{'
        CLOSE_CURLY_BRACKET, //'}'
        OPEN_SQUARE_BRACKET, //'['
        CLOSE_SQUARE_BRACKET, //']'
        SEPARATOR, //',', not used 
        OPERATOR, //':', not used
        NUMBER, //digits
        FLOAT_NUMBER, //digits with .
        STRING, //string
        IDENTIFIER, //string with alpha symbols, numbers or spaces
        BOOL, //true or false
        NULLOBJ //null
    };

    const char *LexerValueTypeStr(LexerValueType type)
    {
        const char *valueTypeStr[]{
            "OPEN_CURLY_BRACKET", "CLOSE_CURLY_BRACKET",
            "OPEN_SQUARE_BRACKET", "CLOSE_SQUARE_BRACKET",
            "SEPARATOR", 
            "OPERATOR",
            "NUMBER",
            "FLOAT_NUMBER",
            "STRING",
            "IDENTIFIER",
            "BOOL",
            "NULL"
        };

        return valueTypeStr[type];
    }

    enum LexerState
    {
        GET_INITIAL_BRACKET, // { or [
        GET_IDENTIFIER_INIT, // first occurence of "
        GET_IDENTIFIER, //accepts spaces and alpha symbols until finds another "
        GET_OPERATOR, // :
        GET_OBJ, //determines type of object by first letter
        READ_STRING, //read string
        READ_ESCAPE_CHARACTER, //read escape characters. only one character escape sequences are supported
        EXPANSION, //expands array/object or finishes object/array (] or })
        FINISH, //desired state
        READ_INT, //read int
        READ_DOUBLE, //read double
        READ_NULL, //read null, case sensetive
        READ_TRUE, //read true, case sensetive
        READ_FALSE //read false, case sensetive
    };

    class JsonLexerExpection : public std::runtime_error
    {
    public:
        JsonLexerExpection (const std::string &reason) 
            : runtime_error(reason)
        {          
        }
    };

    class IJsonLexer    
    {
    public:
        virtual std::vector<std::pair<LexerValueType, std::string>> GetTokens(const std::string &text) = 0;
    };
 
    class JsonLexer : public IJsonLexer
    {
    public:
        struct LexerArgs
        {
            std::vector<std::pair<LexerValueType, std::string>> &tokens;
            std::string &token;
            std::stack<LexerValueType> &valueHolders;
            char readLetter;
        };

        std::vector<std::pair<LexerValueType, std::string>> GetTokens(const std::string &text) override
        {
            std::vector<std::pair<LexerValueType, std::string>> tokens;
            std::string token;
            std::stack<LexerValueType> valueHolders;

            StateMachine<void, LexerArgs> stateMachine { 
                GET_INITIAL_BRACKET,
                {
                    obj_fn_wrapper(this, &JsonLexer::GetInitialBracketHandler),
                    obj_fn_wrapper(this, &JsonLexer::GetIdentifierInitHandler),
                    obj_fn_wrapper(this, &JsonLexer::GetIdentifierHandler),
                    obj_fn_wrapper(this, &JsonLexer::GetOperatorHandler),
                    obj_fn_wrapper(this, &JsonLexer::GetObjHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadStringHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadEscapeCharacterHandler),
                    obj_fn_wrapper(this, &JsonLexer::ExpansionHandler),
                    obj_fn_wrapper(this, &JsonLexer::FinishHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadIntHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadDoubleHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadNullHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadTrueHandler),
                    obj_fn_wrapper(this, &JsonLexer::ReadFalseHandler)
                }
            };

            LexerArgs args(tokens, token, valueHolders);
            stateMachine.SetInstance(&args);

            for (size_t i = 0; i < text.size(); i++)
            {
#ifdef DEBUG_ARX_JSON_LEXER
                std::cout << "symbol " << text[i] << ", current state " << stateMachine.GetState() << std::endl;
#endif
                args.readLetter = text[i];
                stateMachine.Process();
            }

            if (stateMachine.GetState() != FINISH)
                throw JsonLexerExpection("Unexpected end of string");    

            return tokens;
        }
        
        private:
            bool isSkippable(char c) { return isblank(c) || c == '\n'; }

            void GetInitialBracketHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();
                instance->token += instance->readLetter;

                if (instance->readLetter == '{')
                {
                    instance->valueHolders.push(OPEN_CURLY_BRACKET);
                    Store(stateMachine, OPEN_CURLY_BRACKET, GET_IDENTIFIER_INIT);
                }
                else if (instance->readLetter == '[')
                {
                    instance->valueHolders.push(OPEN_SQUARE_BRACKET);
                    Store(stateMachine, OPEN_SQUARE_BRACKET, GET_OBJ);
                }
                else if (isSkippable(instance->readLetter))
                    return;
                else
                    throw JsonLexerExpection(std::string("Expected '{' or '[' but got ") + instance->readLetter);
            }

            void GetIdentifierInitHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == '"')
                    stateMachine.SetState(GET_IDENTIFIER);
                else if (instance->readLetter == '}')
                {
                    instance->tokens.push_back(std::make_pair(CLOSE_CURLY_BRACKET, std::string(instance->readLetter, '}')));
                    stateMachine.SetState(FINISH);
                }
                else if (!isSkippable(instance->readLetter))
                    throw JsonLexerExpection(std::string("Expected '\"' but got ") + instance->readLetter);
            }

            void GetIdentifierHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == '"')
                    Store(stateMachine, IDENTIFIER, GET_OPERATOR);
                else if (isalnum(instance->readLetter) || instance->readLetter == '_')
                    instance->token += instance->readLetter ;
                else
                    throw JsonLexerExpection(std::string("Expected character/number but got ") + instance->readLetter);
            }

            void GetOperatorHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == ':')
                    stateMachine.SetState(GET_OBJ);
                else if (!isSkippable(instance->readLetter))
                    throw JsonLexerExpection(std::string("expected ':' but got ") + instance->readLetter);
            }

            void GetObjHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();
                
                if (instance->readLetter == '{')
                {
                    instance->token += instance->readLetter;
                    instance->valueHolders.push(OPEN_CURLY_BRACKET);
                    Store(stateMachine, OPEN_CURLY_BRACKET, GET_IDENTIFIER_INIT);
                }               
                else if (instance->readLetter == '[')
                {
                    instance->token += instance->readLetter;
                    instance->valueHolders.push(OPEN_SQUARE_BRACKET);
                    Store(stateMachine, OPEN_SQUARE_BRACKET, GET_OBJ);
                }
                else if (instance->readLetter == ']')
                {
                    instance->token += instance->readLetter;
                    ExpansionHandler(stateMachine);
                }
                else if (instance->readLetter == '"')
                    stateMachine.SetState(READ_STRING);
                else if (instance->readLetter == 't')
                {
                    instance->token += instance->readLetter;
                    stateMachine.SetState(READ_TRUE);
                }
                else if (instance->readLetter == 'f')
                {
                    instance->token += instance->readLetter;
                    stateMachine.SetState(READ_FALSE);
                }
                else if (instance->readLetter == 'n')
                {
                    instance->token += instance->readLetter;
                    stateMachine.SetState(READ_NULL);
                }
                else if (isdigit(instance->readLetter) || instance->readLetter == '-')
                {
                    instance->token += instance->readLetter;
                    stateMachine.SetState(READ_INT);
                }
                else if (!isSkippable(instance->readLetter))
                    throw JsonLexerExpection(std::string("Unexpected character ") + instance->readLetter);
            }

            void ReadStringHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == '"')
                {
                    Store(stateMachine, STRING, EXPANSION);
                    return;
                }
                else if (instance->readLetter == '\\')
                {
                    stateMachine.SetState(READ_ESCAPE_CHARACTER);
                    return;
                }

                instance->token += instance->readLetter;
            }

            void ReadEscapeCharacterHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();
                const char characters[12]{ 'n', 't', 'b', 'r', 'a', '\'', '"', '?', '\\', 'f', 'v', '\0'};
                const char escapeCharacters[12]{ '\n', '\t', '\b', '\r', '\a', '\'', '\"', '\?', '\\', '\f', '\v', '\0'};

                for (size_t i = 0; i < (sizeof(characters) / sizeof(char)); i++)
                {
                    if (instance->readLetter == characters[i])
                    {
                        instance->token += escapeCharacters[i];
                        stateMachine.SetState(READ_STRING);
                        return;
                    }
                }

                /*if not found*/
                throw JsonLexerExpection("Invalid escape character");
            }

            void ExpansionHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == ',')
                {
                    instance->token.clear();
                    if (instance->valueHolders.top() == OPEN_CURLY_BRACKET)
                        stateMachine.SetState(GET_IDENTIFIER_INIT);
                    else
                        stateMachine.SetState(GET_OBJ);
                }
                else if (instance->readLetter == '}')
                {
                    instance->token += instance->readLetter;
                    if (instance->valueHolders.top() != OPEN_CURLY_BRACKET)
                       throw JsonLexerExpection("Brackets mismatch, expected ']'");

                    instance->valueHolders.pop();
                    if (instance->valueHolders.size() == 0)
                        Store(stateMachine, CLOSE_CURLY_BRACKET, FINISH);
                    else if (instance->valueHolders.top() == OPEN_CURLY_BRACKET)
                        Store(stateMachine, CLOSE_CURLY_BRACKET, GET_IDENTIFIER_INIT);
                    else
                        Store(stateMachine, CLOSE_CURLY_BRACKET, GET_OBJ);
                                            
                }
                else if (instance->readLetter == ']')
                {
                    instance->token += instance->readLetter;
                    if (instance->valueHolders.top() != OPEN_SQUARE_BRACKET)
                        throw JsonLexerExpection("Brackets mismatch, expected '}'");

                    instance->valueHolders.pop();
                    if (instance->valueHolders.size() == 0)
                        Store(stateMachine, CLOSE_SQUARE_BRACKET, FINISH);
                    else if (instance->valueHolders.top() == OPEN_CURLY_BRACKET)
                        Store(stateMachine, CLOSE_SQUARE_BRACKET, GET_IDENTIFIER_INIT);
                    else
                        Store(stateMachine, CLOSE_SQUARE_BRACKET, GET_OBJ);
                }
                else if (!isSkippable(instance->readLetter))
                    throw JsonLexerExpection(std::string("Unexpected character ") + instance->readLetter);
            }

            void FinishHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (!isSkippable(instance->readLetter))
                    throw JsonLexerExpection("Unexpected symbols after main object/array end");
            }

            void ReadIntHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (instance->readLetter == '.' && instance->token != "-") 
                {
                    instance->token += '.';
                    stateMachine.SetState(READ_DOUBLE);
                }
                else if (isSkippable(instance->readLetter) && instance->token != "-")
                    Store(stateMachine, NUMBER, EXPANSION);
                else if ((instance->readLetter == ']' || instance->readLetter == '}' || instance->readLetter == ',') && instance->token != "-")
                {
                    Store(stateMachine, NUMBER, EXPANSION);
                    ExpansionHandler(stateMachine);
                }
                else if (isdigit(instance->readLetter))
                    instance->token += instance->readLetter;
                else if (instance->token == "-")
                    throw JsonLexerExpection("Expected digit after minus sign");
                else
                    throw JsonLexerExpection(std::string("Expected digit but got ") + instance->readLetter);
            }

            void ReadDoubleHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();

                if (isSkippable(instance->readLetter))
                    Store(stateMachine, FLOAT_NUMBER, EXPANSION);
                else if (instance->readLetter == ']' || instance->readLetter == '}' || instance->readLetter == ',')
                {
                    Store(stateMachine, FLOAT_NUMBER, EXPANSION);
                    ExpansionHandler(stateMachine);
                }
                else if (!isdigit(instance->readLetter))
                    throw JsonLexerExpection(std::string("Expected digit but got ") + instance->readLetter);

                instance->token += instance->readLetter;
            }

            void ReadNullHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                auto *instance = stateMachine.GetInstance();
                const char *nullStr = "null";
                instance->token += instance->readLetter;
                size_t pos = instance->token.length() - 1;

                if (instance->token[pos] != nullStr[pos])
                    throw JsonLexerExpection(std::string("(NULLOBJ) Expected ") + nullStr[pos] + " but got " + instance->token[pos]);
                else if (instance->token.length() == 4)
                     Store(stateMachine, NULLOBJ, EXPANSION);
            }

            void ReadTrueHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                const char *trueStr = "true";
                auto *instance = stateMachine.GetInstance();
                instance->token += instance->readLetter;
                size_t pos = instance->token.length() - 1;

                if (instance->token[pos] != trueStr[pos])
                    throw JsonLexerExpection("(BOOL) Expected true");
                else if (instance->token.length() == 4)
                    Store(stateMachine, BOOL, EXPANSION);
            }

            void ReadFalseHandler(StateMachine<void, LexerArgs> &stateMachine)
            {
                const char *falseStr = "false";
                auto *instance = stateMachine.GetInstance();
                instance->token += instance->readLetter;
                size_t pos = instance->token.length() - 1;

                if (instance->token[pos] != falseStr[pos])
                    throw JsonLexerExpection("(BOOL) Expected false");
                else if (instance->token.length() == 5)
                    Store(stateMachine, BOOL, EXPANSION);
            }

            /**
             * @brief Stores value stored in StateMachine instance, clears value and sets nextState
             * 
             * @param stateMachine State machine with LexerArgs
             * @param type type of value that is stored
             * @param nextState next StateMachine state
             */
            void Store(StateMachine<void, LexerArgs> &stateMachine, LexerValueType type, LexerState nextState)
            {
                auto *instance = stateMachine.GetInstance();
                instance->tokens.push_back(std::make_pair(type, instance->token));
                instance->token.clear();
                stateMachine.SetState(nextState);
            }
    };

    class IJsonParser
    {
    public:
        virtual Json Parse(const std::string &text) = 0;
    };
    
    class JsonParser : public IJsonParser
    {
    public:
        JsonParser(IJsonLexer &lexer)
            : m_lexer(lexer)
        {
        }

        Json Parse(const std::string &text) override
        {
            auto tokens(m_lexer.GetTokens(text));
            Json retVal;
            std::stack<Json*> topLevelObjects;
            Json *currentObj = &retVal;

            for (auto &[valType, val] : tokens)
            {
#ifdef DEBUG_ARX_JSON_PARSER
                std::cout << LexerValueTypeStr(valType) << std::endl;
#endif
                switch(valType)
                {
                    case OPEN_CURLY_BRACKET:
                        if (currentObj->GetType() == JsonValueType::ARRAY) 
                        {
                            currentObj->PushBack(Json{ });
                            currentObj = &currentObj->operator[](currentObj->Count() - 1);
                        }
                        currentObj->Set(JsonObjectContainer{ });
                        topLevelObjects.push(currentObj);
                        break;
                    case CLOSE_CURLY_BRACKET:
                        topLevelObjects.pop();
                        if (!topLevelObjects.empty())                  
                            currentObj = topLevelObjects.top();
                        break;
                    case OPEN_SQUARE_BRACKET:
                        if (currentObj->GetType() == JsonValueType::ARRAY) 
                        {
                            currentObj->PushBack(Json{ });
                            currentObj = &currentObj->operator[](currentObj->Count() - 1);
                        }
                        currentObj->Set(std::vector<Json>{});
                        topLevelObjects.push(currentObj);
                        break;
                    case CLOSE_SQUARE_BRACKET:
                        topLevelObjects.pop();
                        if (!topLevelObjects.empty())
                            currentObj = topLevelObjects.top();
                        break;
                    case NUMBER:
                        if (topLevelObjects.top()->GetType() == JsonValueType::ARRAY)
                            currentObj->PushBack(Json(std::stol(val)));
                        else
                        {
                            currentObj->Set(std::stoi(val));
                            currentObj = topLevelObjects.top();
                        }
                        break;
                    case STRING:
                        if (topLevelObjects.top()->GetType() == JsonValueType::ARRAY)
                            currentObj->PushBack(Json(std::move(val)));
                        else
                        {
                            currentObj->Set(val);
                            currentObj = topLevelObjects.top();
                        }
                        break;
                    case FLOAT_NUMBER:
                        if (topLevelObjects.top()->GetType() == JsonValueType::ARRAY)
                            currentObj->PushBack(Json(std::stod(val)));
                        else
                        {
                            currentObj->Set(std::stod(val));
                            currentObj = topLevelObjects.top();
                        }
                        break;
                    case BOOL:
                        if (topLevelObjects.top()->GetType() == JsonValueType::ARRAY)
                            currentObj->PushBack(Json((val == "true") ? true : false));
                        else
                        {
                            currentObj->Set((val == "true") ? true : false);
                            currentObj = topLevelObjects.top();
                        }
                        break;
                    case NULLOBJ:
                        if (topLevelObjects.top()->GetType() == JsonValueType::ARRAY)
                            currentObj->PushBack(Json(nullptr));
                        else
                        {
                            currentObj->Set(nullptr);
                            currentObj = topLevelObjects.top();
                        }
                        break;
                    case IDENTIFIER:
                        currentObj = &currentObj->operator[](val);
                        break;
                    default:
                        throw std::runtime_error("JsonParser unexpected error");
                        break;
                }
            }

            return retVal;       
        }
    private:
        IJsonLexer &m_lexer;
    };

    /* static */ Json Json::Parse(const std::string &text, IJsonParser &parser)
    {
        return parser.Parse(text);
    }

    /* static */ Json Json::Parse(const std::string &text, IJsonLexer &lexer)
    {
        return JsonParser(lexer).Parse(text);
    }

    /* static */ Json Json::Parse(const std::string &text)
    {
        JsonLexer defaultLexer;
        return JsonParser(defaultLexer).Parse(text);
    }

    /* static */ Json Json::FromFile(const std::string &filename)
    {
        std::ifstream file;
        file.exceptions(std::fstream::failbit | std::fstream::badbit);
        file.open(filename);
        std::stringstream strStream;
        strStream << file.rdbuf();

        return Parse(strStream.str());
    }

    class IJsonConverter
    {
    public:
        virtual std::string ToJsonStr(const Json &j) const = 0;
    };

    class JsonConvertor : public IJsonConverter
    {
    public:

        std::string ToJsonStr(const Json &j) const
        {
            return ToJsonStrRec(j, 0);
        }

    private:
        std::string ToJsonStrRec(const Json &j, uint depth) const
        {
            std::string retVal;
            switch(j.GetType())
            {
                case JsonValueType::NULL_OBJ:
                    retVal += "null";
                    break;
                case JsonValueType::BOOL:
                    retVal += j.Get<bool>() ? "true" : "false";
                    break;
                case JsonValueType::UINT:
                    retVal +=  std::to_string(j.Get<uint>());
                    break;
                case JsonValueType::INT:
                    retVal +=  std::to_string(j.Get<int>());
                    break;
                case JsonValueType::FLOATING:
                    retVal +=  std::to_string(j.Get<double>());
                    break;
                case JsonValueType::OBJECT_CONTAINER:
                    {
                        retVal += "{";
                        retVal += '\n';
                        uint counter = 0;
                        for (const auto &[key, storedObject] : j.GetConstRef<JsonObjectContainer>())
                        {
                            retVal += std::string(depth + 1, '\t');
                            retVal += '"' + key + '"' + ": ";
                            retVal += ToJsonStrRec(storedObject, depth + 1);

                            if (counter + 1 != j.Count())
                                retVal += ',';

                            retVal += '\n';

                            counter++;

                        }
                        retVal += std::string(depth, '\t') + "}" + "\n";
                    }
                    break;
                case JsonValueType::ARRAY:
                    {
                        retVal += "[";
                        uint counter = 0;
                        for (const auto &storedObject : j.GetConstRef<std::vector<Json>>())
                        {
                            retVal += ToJsonStrRec(storedObject, depth + 1);
                            if (counter + 1 != j.Count())
                                retVal += ", ";
                            
                            counter++;
                        }
                        retVal += std::string(depth, '\t') + "]";
                    }
                    break;
                case JsonValueType::STRING:
                    retVal +=  std::string("\"") + j.Get<std::string>() + "\"";
                    break;
                default:
                    break;
            }

            return retVal;
        }
    };

    /* static */ std::string Json::ToJsonStr(const Json &j, const IJsonConverter &convertor)
    {
        return convertor.ToJsonStr(j);
    }

    /* static */ std::string Json::ToJsonStr(const Json &j)
    {
        JsonConvertor defaultConvertor;
        return defaultConvertor.ToJsonStr(j);
    }
}