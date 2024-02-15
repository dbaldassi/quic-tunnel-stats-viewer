#ifndef CSV_READER_H
#define CSV_READER_H

#include <tuple>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

template<const char DELIMITER, typename... Ts>
class CsvReader
{
    fs::path _path;

public:

    CsvReader(fs::path p) : _path(std::move(p)) {}

    class iterator
    {
        friend CsvReader;

        std::tuple<Ts...> _line;
        std::ifstream _ifs;
        std::istringstream _iss;
        bool _finish = false;

        template<size_t I=0>
        void load_line() {
            constexpr auto size = std::tuple_size<value_type>();

            if(_ifs.eof()) _finish = true;

            if constexpr(I == 0) {
                std::string line_str;
                std::getline(_ifs, line_str);

                _iss = std::istringstream(line_str);
            }

            std::string token;
            std::getline(_iss, token, DELIMITER);
            std::istringstream tmpss{token};
            tmpss >> std::get<I>(_line);

            if constexpr((I+1) < size) {
                load_line<I+1>();
            }
        }

        explicit iterator(bool f) : _finish(f) {}

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::tuple<Ts...>;
        using difference_type = value_type;
        using pointer = value_type*;
        using reference = value_type&;

        explicit iterator(const fs::path& p) {
            _ifs.open(p.c_str());

            if(!_ifs.is_open()) {
                throw std::runtime_error("Could not open csv file with provided path");
            }

            load_line();
        }

        iterator& operator++() {
            load_line();
            return *this;
        }

        iterator operator++(int) {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        bool operator==(const iterator& other) const { return (_finish == other._finish) || (_line == other._line); }
        bool operator!=(const iterator& other) const { return !(*this == other); }

        const value_type& operator*() const { return _line; }
    };

    iterator begin() { return iterator(_path); }
    iterator end() { return iterator(true); }
};

namespace impl
{

template<const char DELIMITER, size_t NUM, typename... Ts>
struct TypeDuplicator;

template<const char DELIMITER, size_t NUM, typename T, typename... Ts>
struct TypeDuplicator<DELIMITER,NUM, T, Ts...>
{
    using Type = TypeDuplicator<DELIMITER, NUM-1, T, T, Ts...>::Type;
};

template<const char DELIMITER, size_t NUM, typename T>
struct TypeDuplicator<DELIMITER,NUM, T>
{
    using Type = TypeDuplicator<DELIMITER,NUM-1, T, T>::Type;
};

template<const char DELIMITER, typename T, typename... Ts>
struct TypeDuplicator<DELIMITER, 0,T, Ts...>
{
    using Type = CsvReader<',', Ts...>;
};

}

template<const char DELIMITER, typename T, size_t NUM>
using CsvReaderTypeRepeat = impl::TypeDuplicator<DELIMITER, NUM, T>::Type;

#endif // CSV_READER_H
