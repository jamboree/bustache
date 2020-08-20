namespace test
{
    struct context : std::unordered_map<std::string, bustache::format>
    {
        using unordered_map::unordered_map;

        bustache::format const* operator()(std::string const& key) const
        {
            auto it = find(key);
            return it == end() ? nullptr : &it->second;
        }
    };

    struct value;

    struct object : std::vector<std::pair<std::string, value>>
    {
        using vector::vector;
        using mapped_type = value;

        const_iterator find(std::string const& key) const;
    };

    using array = std::vector<value>;
    using lazy_value = std::function<value(bustache::ast::content_list const*)>;
    using lazy_format = std::function<bustache::format(bustache::ast::content_list const*)>;

    struct value : std::variant<bool, int, double, std::string, object, array, lazy_value, lazy_format>
    {
        using variant::variant;

        value(char const* str) : variant(std::string(str)) {}
    };

    object::const_iterator object::find(std::string const& key) const
    {
        return std::find_if(begin(), end(), [&key](value_type const& pair)
        {
            return pair.first == key;
        });
    }
}

template<>
struct bustache::impl_compatible<test::value>
{
    static value_ptr get_value_ptr(test::value const& self)
    {
        return std::visit([](auto const& val) { return value_ptr(&val); }, self);
    }
};