// Copyright 2013-2024 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_BASIC_JSON_HPP
#define JSONCONS_BASIC_JSON_HPP

#include <limits> // std::numeric_limits
#include <string>
#include <vector>
#include <exception>
#include <cstring>
#include <ostream> 
#include <memory> // std::allocator
#include <typeinfo>
#include <cstring> // std::memcpy
#include <algorithm> // std::swap
#include <initializer_list> // std::initializer_list
#include <utility> // std::move
#include <type_traits> // std::enable_if
#include <istream> // std::basic_istream
#include <jsoncons/json_fwd.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/config/version.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/pretty_print.hpp>
#include <jsoncons/json_object.hpp>
#include <jsoncons/json_array.hpp>
#include <jsoncons/bigint.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/json_encoder.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_reader.hpp>
#include <jsoncons/json_type_traits.hpp>
#include <jsoncons/byte_string.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/allocator_set.hpp>
#include <jsoncons/detail/heap_string.hpp>
#if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
#include <memory_resource> // std::poymorphic_allocator
#endif

namespace jsoncons { 

    namespace extension_traits {

        template <typename Container>
        using 
        container_array_iterator_type_t = decltype(Container::array_iterator_type);
        template <typename Container>
        using 
        container_const_array_iterator_type_t = decltype(Container::const_array_iterator_type);
        template <typename Container>
        using 
        container_object_iterator_type_t = decltype(Container::object_iterator_type);
        template <typename Container>
        using 
        container_const_object_iterator_type_t = decltype(Container::const_object_iterator_type);

        namespace detail {

            template <typename T>
            using
            basic_json_t = basic_json<typename T::char_type,typename T::policy_type,typename T::allocator_type>;

        } // namespace detail

        template <typename T,typename Enable = void>
        struct is_basic_json : std::false_type {};

        template <typename T>
        struct is_basic_json<T,
            typename std::enable_if<extension_traits::is_detected<detail::basic_json_t,typename std::decay<T>::type>::value>::type
        > : std::true_type {};

    } // namespace extension_traits

    namespace detail {

        template <typename Iterator,typename Enable = void>
        class random_access_iterator_wrapper
        {
        };

        template <typename Iterator>
        class random_access_iterator_wrapper<Iterator,
                 typename std::enable_if<std::is_same<typename std::iterator_traits<Iterator>::iterator_category, 
                                                      std::random_access_iterator_tag>::value>::type> 
        { 
            Iterator it_; 
            bool has_value_;

            template <typename Iter,typename Enable> 
            friend class random_access_iterator_wrapper;
        public:
            using iterator_category = std::random_access_iterator_tag;

            using value_type = typename std::iterator_traits<Iterator>::value_type;
            using difference_type = typename std::iterator_traits<Iterator>::difference_type;
            using pointer = typename std::iterator_traits<Iterator>::pointer;
            using reference = typename std::iterator_traits<Iterator>::reference;
        
            random_access_iterator_wrapper() : it_(), has_value_(false) 
            { 
            }

            explicit random_access_iterator_wrapper(Iterator ptr) : it_(ptr), has_value_(true)  
            {
            }

            random_access_iterator_wrapper(const random_access_iterator_wrapper&) = default;
            random_access_iterator_wrapper(random_access_iterator_wrapper&&) = default;
            random_access_iterator_wrapper& operator=(const random_access_iterator_wrapper&) = default;
            random_access_iterator_wrapper& operator=(random_access_iterator_wrapper&&) = default;

            template <typename Iter,
                      class=typename std::enable_if<!std::is_same<Iter,Iterator>::value && std::is_convertible<Iter,Iterator>::value>::type>
            random_access_iterator_wrapper(const random_access_iterator_wrapper<Iter>& other)
                : it_(other.it_), has_value_(other.has_value_)
            {
            }

            operator Iterator() const
            { 
                return it_; 
            }

            reference operator*() const 
            {
                return *it_;
            }

            pointer operator->() const 
            {
                return &(*it_);
            }

            random_access_iterator_wrapper& operator++() 
            {
                ++it_;
                return *this;
            }

            random_access_iterator_wrapper operator++(int) 
            {
                random_access_iterator_wrapper temp = *this;
                ++*this;
                return temp;
            }

            random_access_iterator_wrapper& operator--() 
            {
                --it_;
                return *this;
            }

            random_access_iterator_wrapper operator--(int) 
            {
                random_access_iterator_wrapper temp = *this;
                --*this;
                return temp;
            }

            random_access_iterator_wrapper& operator+=(const difference_type offset) 
            {
                it_ += offset;
                return *this;
            }

            random_access_iterator_wrapper operator+(const difference_type offset) const 
            {
                random_access_iterator_wrapper temp = *this;
                return temp += offset;
            }

            random_access_iterator_wrapper& operator-=(const difference_type offset) 
            {
                return *this += -offset;
            }

            random_access_iterator_wrapper operator-(const difference_type offset) const 
            {
                random_access_iterator_wrapper temp = *this;
                return temp -= offset;
            }

            difference_type operator-(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return it_ - rhs.it_;
            }

            reference operator[](const difference_type offset) const noexcept
            {
                return *(*this + offset);
            }

            bool operator==(const random_access_iterator_wrapper& rhs) const noexcept
            {
                if (!has_value_ || !rhs.has_value_)
                {
                    return has_value_ == rhs.has_value_ ? true : false;
                }
                else
                {
                    return it_ == rhs.it_;
                }
            }

            bool operator!=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            bool operator<(const random_access_iterator_wrapper& rhs) const noexcept
            {
                if (!has_value_ || !rhs.has_value_)
                {
                    return has_value_ == rhs.has_value_ ? false :(has_value_ ? false : true);
                }
                else
                {
                    return it_ < rhs.it_;
                }
            }

            bool operator>(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return rhs < *this;
            }

            bool operator<=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(rhs < *this);
            }

            bool operator>=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(*this < rhs);
            }

            inline 
            friend random_access_iterator_wrapper<Iterator> operator+(
                difference_type offset, random_access_iterator_wrapper<Iterator> next) 
            {
                return next += offset;
            }

            bool has_value() const
            {
                return has_value_;
            }
        };
    } // namespace detail

    struct sorted_policy 
    {
        template <typename KeyT,typename Json>
        using object = sorted_json_object<KeyT,Json,std::vector>;

        template <typename Json>
        using array = json_array<Json,std::vector>;
        
        template <typename CharT,typename CharTraits,typename Allocator>
        using member_key = std::basic_string<CharT, CharTraits, Allocator>;
    };

    struct order_preserving_policy
    {
        template <typename KeyT,typename Json>
        using object = order_preserving_json_object<KeyT,Json,std::vector>;

        template <typename Json>
        using array = json_array<Json,std::vector>;
        
        template <typename CharT,typename CharTraits,typename Allocator>
        using member_key = std::basic_string<CharT, CharTraits, Allocator>;
    };

    template <typename Policy,typename KeyT,typename Json,typename Enable=void>
    struct object_iterator_typedefs
    {
    };

    template <typename Policy,typename KeyT,typename Json>
    struct object_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        !extension_traits::is_detected<extension_traits::container_object_iterator_type_t, Policy>::value ||
        !extension_traits::is_detected<extension_traits::container_const_object_iterator_type_t, Policy>::value>::type>
    {
        using object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object<KeyT,Json>::iterator>;                    
        using const_object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object<KeyT,Json>::const_iterator>;
    };

    template <typename Policy,typename KeyT,typename Json>
    struct object_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        extension_traits::is_detected<extension_traits::container_object_iterator_type_t, Policy>::value &&
        extension_traits::is_detected<extension_traits::container_const_object_iterator_type_t, Policy>::value>::type>
    {
        using object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object_iterator<KeyT,Json>>;
        using const_object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template const_object_iterator<KeyT,Json>>;
    };

    template <typename Policy,typename KeyT,typename Json,typename Enable=void>
    struct array_iterator_typedefs
    {
    };

    template <typename Policy,typename KeyT,typename Json>
    struct array_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        !extension_traits::is_detected<extension_traits::container_array_iterator_type_t, Policy>::value ||
        !extension_traits::is_detected<extension_traits::container_const_array_iterator_type_t, Policy>::value>::type>
    {
        using array_iterator_type = typename Policy::template array<Json>::iterator;
        using const_array_iterator_type = typename Policy::template array<Json>::const_iterator;
    };

    template <typename Policy,typename KeyT,typename Json>
    struct array_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        extension_traits::is_detected<extension_traits::container_array_iterator_type_t, Policy>::value &&
        extension_traits::is_detected<extension_traits::container_const_array_iterator_type_t, Policy>::value>::type>
    {
        using array_iterator_type = typename Policy::template array_iterator_type<Json>;
        using const_array_iterator_type = typename Policy::template const_array_iterator_type<Json>;
    };

    template <typename IteratorT,typename ConstIteratorT>
    class range 
    {
    public:
        using iterator = IteratorT;
        using const_iterator = ConstIteratorT;
        using reverse_iterator = std::reverse_iterator<IteratorT>;
        using const_reverse_iterator = std::reverse_iterator<ConstIteratorT>;
    private:
        iterator first_;
        iterator last_;
    public:
        range(const IteratorT& first, const IteratorT& last)
            : first_(first), last_(last)
        {
        }

        iterator begin() const noexcept
        {
            return first_;
        }
        iterator end() const noexcept
        {
            return last_;
        }
        const_iterator cbegin() const noexcept
        {
            return first_;
        }
        const_iterator cend() const noexcept
        {
            return last_;
        }
        reverse_iterator rbegin() const noexcept
        {
            return reverse_iterator(last_);
        }
        reverse_iterator rend() const noexcept
        {
            return reverse_iterator(first_);
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return reverse_iterator(last_);
        }
        const_reverse_iterator crend() const noexcept
        {
            return reverse_iterator(first_);
        }
    };

    // is_proxy_of

    template <typename T,typename Json,typename Enable = void>
    struct is_proxy_of : std::false_type {};

    template <typename Proxy,typename Json>
    struct is_proxy_of<Proxy,Json,
        typename std::enable_if<std::is_same<typename Proxy::proxied_type,Json>::value>::type
    > : std::true_type {};


    // is_proxy

    template <typename T,typename Enable = void>
    struct is_proxy : std::false_type {};

    template <typename T>
    struct is_proxy<T,typename std::enable_if<is_proxy_of<T,typename T::proxied_type>::value>::type
    > : std::true_type {};

    template <typename CharT,typename Policy,typename Allocator>
    class basic_json
    {
    public:
        static_assert(extension_traits::is_stateless<Allocator>::value || extension_traits::is_propagating_allocator<Allocator>::value,
                      "Regular stateful allocators must be wrapped with std::scoped_allocator_adaptor");

        using allocator_type = Allocator; 

        using policy_type = Policy;

        using char_type = CharT;
        using char_traits_type = std::char_traits<char_type>;
        using string_view_type = jsoncons::basic_string_view<char_type,char_traits_type>;

        using char_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<char_type>;

        using string_type = std::basic_string<char_type,char_traits_type,char_allocator_type>;

        using key_type = typename policy_type::template member_key<char_type,char_traits_type,char_allocator_type>;


        using reference = basic_json&;
        using const_reference = const basic_json&;
        using pointer = basic_json*;
        using const_pointer = const basic_json*;

        using key_value_type = key_value<key_type,basic_json>;

        using array = typename policy_type::template array<basic_json>;

        using key_value_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<key_value_type>;                       

        using object = typename policy_type::template object<key_type,basic_json>;

        using object_iterator = typename object_iterator_typedefs<policy_type,key_type,basic_json>::object_iterator_type;                    
        using const_object_iterator = typename object_iterator_typedefs<policy_type,key_type,basic_json>::const_object_iterator_type;                    
        using array_iterator = typename array_iterator_typedefs<policy_type,key_type,basic_json>::array_iterator_type;                    
        using const_array_iterator = typename array_iterator_typedefs<policy_type,key_type,basic_json>::const_array_iterator_type;

        using object_range_type = range<object_iterator, const_object_iterator>;
        using const_object_range_type = range<const_object_iterator, const_object_iterator>;
        using array_range_type = range<array_iterator, const_array_iterator>;
        using const_array_range_type = range<const_array_iterator, const_array_iterator>;

    private:

        static constexpr uint8_t major_type_shift = 0x04;
        static constexpr uint8_t additional_information_mask = (1U << 4) - 1;

    public:
        class common_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        };

        class null_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;

            null_storage(semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::null)), short_str_length_(0), tag_(tag)
            {
            }
        };

        class empty_object_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;

            empty_object_storage(semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::empty_object)), short_str_length_(0), tag_(tag)
            {
            }
        };  

        class bool_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            bool val_;
        public:
            bool_storage(bool val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::boolean)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            bool value() const
            {
                return val_;
            }

        };

        class int64_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            int64_t val_;
        public:
            int64_storage(int64_t val, 
                       semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::int64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            int64_t value() const
            {
                return val_;
            }
        };

        class uint64_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            uint64_t val_;
        public:
            uint64_storage(uint64_t val, 
                        semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::uint64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            uint64_t value() const
            {
                return val_;
            }
        };

        class half_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            uint16_t val_;
        public:
            half_storage(uint16_t val, semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::half_float)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            uint16_t value() const
            {
                return val_;
            }
        };

        class double_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            double val_;
        public:
            double_storage(double val, 
                           semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::float64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            double value() const
            {
                return val_;
            }
        };

        class short_string_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            static constexpr size_t capacity = (2*sizeof(uint64_t) - 2*sizeof(uint8_t))/sizeof(char_type);
            char_type data_[capacity];
        public:
            static constexpr size_t max_length = capacity - 1;

            short_string_storage(semantic_tag tag, const char_type* p, uint8_t length)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::short_str)), short_str_length_(length), tag_(tag)
            {
                JSONCONS_ASSERT(length <= max_length);
                std::memcpy(data_,p,length*sizeof(char_type));
                data_[length] = 0;
            }

            short_string_storage(const short_string_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(other.short_str_length_), tag_(other.tag_)
            {
                std::memcpy(data_,other.data_,other.short_str_length_*sizeof(char_type));
                data_[short_str_length_] = 0;
            }
           
            short_string_storage& operator=(const short_string_storage& other) = delete;

            uint8_t length() const
            {
                return short_str_length_;
            }

            const char_type* data() const
            {
                return data_;
            }

            const char_type* c_str() const
            {
                return data_;
            }
        };

        // long_string_storage
        class long_string_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            using heap_string_factory_type = jsoncons::detail::heap_string_factory<char_type,null_type,Allocator>;
            using pointer = typename heap_string_factory_type::pointer;

            pointer ptr_;
        public:

            long_string_storage(semantic_tag tag, const char_type* data, std::size_t length, const Allocator& alloc)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(tag)
            {
                ptr_ = heap_string_factory_type::create(data, length, null_type(), alloc);
            }
            long_string_storage(const long_string_storage& other)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), other.get_allocator());
            }

            long_string_storage(const long_string_storage& other, const Allocator& alloc)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), alloc);
            }

            long_string_storage(long_string_storage&& other) noexcept
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
                other.tag_ = semantic_tag::none;
                other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
            }

            long_string_storage(long_string_storage&& other, const Allocator& alloc)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(other.tag_)
            {
                if (other.get_allocator() == alloc)
                {
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    other.tag_ = semantic_tag::none;
                    other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                }
                else
                {
                    ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), alloc);
                }
            }

            void assign(const long_string_storage& other)
            {
                assign(std::integral_constant<bool,std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value>(), other);
            }

            void assign(std::true_type, const long_string_storage& other)
            {
                tag_ = other.tag_;
                heap_string_factory_type::destroy(ptr_);
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), other.get_allocator());
            }

            void assign(std::false_type, const long_string_storage& other)
            {
                auto alloc = get_allocator();
                tag_ = other.tag_;
                heap_string_factory_type::destroy(ptr_);
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), alloc);
            }

            void assign(long_string_storage&& other)
            {
                swap(other);
            }

            ~long_string_storage() noexcept
            {
                heap_string_factory_type::destroy(ptr_);
            }

            void swap(long_string_storage& other)
            {
                using std::swap;
                swap(ptr_, other.ptr_);
                swap(tag_, other.tag_);
            }

            semantic_tag tag() const
            {
                return tag_;
            }

            const char_type* data() const
            {
                return ptr_->data();
            }

            const char_type* c_str() const
            {
                return ptr_->c_str();
            }

            std::size_t length() const
            {
                return ptr_->length();
            }
        
            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };

        // byte_string_storage
        class byte_string_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            using heap_string_factory_type = jsoncons::detail::heap_string_factory<uint8_t,uint64_t,Allocator>;
            using pointer = typename heap_string_factory_type::pointer;

            pointer ptr_;
        public:

            byte_string_storage(semantic_tag tag, const uint8_t* data, std::size_t length, uint64_t ext_tag, const Allocator& alloc)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::byte_str)), short_str_length_(0), tag_(tag)
            {
                ptr_ = heap_string_factory_type::create(data, length, ext_tag, alloc);
            }

            byte_string_storage(const byte_string_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), other.ext_tag(), other.get_allocator());
            }

            byte_string_storage(const byte_string_storage& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), other.ext_tag(), alloc);
            }

            byte_string_storage(byte_string_storage&& other) noexcept
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_)
            {
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
                other.tag_ = semantic_tag::none;
                other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
            }

            byte_string_storage(byte_string_storage&& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_)
            {
                if (other.get_allocator() == alloc)
                {
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    other.tag_ = semantic_tag::none;
                    other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                }
                else
                {
                    ptr_ = heap_string_factory_type::create(other.data(), other.length(), other.ext_tag(), alloc);
                }
            }

            void assign(const byte_string_storage& other)
            {
                assign(std::integral_constant<bool,std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value>(), other);
            }

            void assign(std::true_type, const byte_string_storage& other)
            {
                tag_ = other.tag_;
                heap_string_factory_type::destroy(ptr_);
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), null_type(), other.get_allocator());
            }

            void assign(std::false_type, const byte_string_storage& other)
            {
                auto alloc = get_allocator();
                tag_ = other.tag_;
                heap_string_factory_type::destroy(ptr_);
                ptr_ = heap_string_factory_type::create(other.data(), other.length(), other.ext_tag(), alloc);
            }

            void assign(byte_string_storage&& other)
            {
                swap(other);
            }

            ~byte_string_storage() noexcept
            {
                heap_string_factory_type::destroy(ptr_);
            }

            void swap(byte_string_storage& other)
            {
                using std::swap;
                swap(ptr_, other.ptr_);
                swap(tag_, other.tag_);
            }

            semantic_tag tag() const
            {
                return tag_;
            }

            const uint8_t* data() const
            {
                return ptr_->data();
            }

            std::size_t length() const
            {
                return ptr_->length();
            }

            uint64_t ext_tag() const
            {
                return ptr_->extra();
            }

            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

        // array_storage
        class array_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            using array_allocator = typename std::allocator_traits<Allocator>:: template rebind_alloc<array>;
            using pointer = typename std::allocator_traits<array_allocator>::pointer;

            pointer ptr_;

            template <typename... Args>
            void create(array_allocator alloc, Args&& ... args)
            {
                ptr_ = std::allocator_traits<array_allocator>::allocate(alloc, 1);
                JSONCONS_TRY
                {
                    std::allocator_traits<array_allocator>::construct(alloc, extension_traits::to_plain_pointer(ptr_), 
                        std::forward<Args>(args)...);
                }
                JSONCONS_CATCH(...)
                {
                    std::allocator_traits<array_allocator>::deallocate(alloc, ptr_,1);
                    JSONCONS_RETHROW;
                }
            }

            void destroy() noexcept
            {
                array_allocator alloc(ptr_->get_allocator());
                std::allocator_traits<array_allocator>::destroy(alloc, extension_traits::to_plain_pointer(ptr_));
                std::allocator_traits<array_allocator>::deallocate(alloc, ptr_,1);
            }
        public:
            array_storage(const array& val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::array)), short_str_length_(0), tag_(tag), ptr_(nullptr)
            {
                create(val.get_allocator(), val);
            }

            array_storage(array&& val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::array)), short_str_length_(0), tag_(tag), ptr_(nullptr)
            {
                create(val.get_allocator(), std::move(val));
            }

            array_storage(const array_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                create(other.ptr_->get_allocator(), *(other.ptr_));
            }

            array_storage(array_storage&& other) noexcept
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_),
                  ptr_(nullptr)
            {
                using std::swap;
                swap(other.ptr_, ptr_);

                other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                other.short_str_length_ = 0;
                other.tag_ = semantic_tag::none;
            }

            array_storage(const array_storage& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                create(array_allocator(alloc), *(other.ptr_));
            }

            array_storage(array_storage&& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                if (other.get_allocator() == alloc)
                {
                    // Transfer resources 
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;

                    other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                    other.tag_ = semantic_tag::none;
                }
                else
                {
                    create(array_allocator(alloc), *(other.ptr_));
                }
            }

            ~array_storage() noexcept
            {
                if (ptr_ != nullptr)
                {
                    destroy();
                }
            }

            void assign(const array_storage& other)
            {
                assign(std::integral_constant<bool,std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value>(), other);
            }

            void assign(std::true_type, const array_storage& other)
            {
                tag_ = other.tag_;
                destroy();
                create(array_allocator(other.get_allocator()), *(other.ptr_));
            }

            void assign(std::false_type, const array_storage& other)
            {
                auto alloc = get_allocator();
                tag_ = other.tag_;
                destroy();
                create(array_allocator(alloc), *(other.ptr_));
            }

            void assign(array_storage&& other)
            {
                swap(other);
            }

            void swap(array_storage& other)
            {
                using std::swap;
                swap(ptr_, other.ptr_);
                swap(tag_, other.tag_);
            }

            semantic_tag tag() const
            {
                return tag_;
            }

            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }

            array& value()
            {
                return *ptr_;
            }

            const array& value() const
            {
                return *ptr_;
            }
        };

        // object_storage
        class object_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            using object_allocator = typename std::allocator_traits<Allocator>:: template rebind_alloc<object>;
            using pointer = typename std::allocator_traits<object_allocator>::pointer;

            pointer ptr_;

            template <typename... Args>
            void create(object_allocator alloc, Args&& ... args)
            {
                ptr_ = std::allocator_traits<object_allocator>::allocate(alloc, 1);
                JSONCONS_TRY
                {
                    std::allocator_traits<object_allocator>::construct(alloc, extension_traits::to_plain_pointer(ptr_), std::forward<Args>(args)...);
                }
                JSONCONS_CATCH(...)
                {
                    std::allocator_traits<object_allocator>::deallocate(alloc, ptr_,1);
                    JSONCONS_RETHROW;
                }
            }
        public:
            object_storage(const object& val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::object)), short_str_length_(0), tag_(tag), ptr_(nullptr)
            {
                create(val.get_allocator(), val);
            }

            object_storage(object&& val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::object)), short_str_length_(0), tag_(tag), ptr_(nullptr)
            {
                create(val.get_allocator(), std::move(val));
            }

            explicit object_storage(const object_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                create(other.ptr_->get_allocator(), *(other.ptr_));
            }

            object_storage(const object_storage& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                create(object_allocator(alloc), *(other.ptr_));
            }

            explicit object_storage(object_storage&& other) noexcept
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_),
                  ptr_(nullptr)
            {
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
                other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                other.short_str_length_ = 0;
                other.tag_ = semantic_tag::none;
            }

            object_storage(object_storage&& other, const Allocator& alloc)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(nullptr)
            {
                if (other.get_allocator() == alloc)
                {
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    other.storage_kind_ = static_cast<uint8_t>(json_storage_kind::null);
                    other.short_str_length_ = 0;
                    other.tag_ = semantic_tag::none;
                }
                else
                {
                    create(object_allocator(alloc), *(other.ptr_));
                }
            }

            ~object_storage() noexcept
            {
                if (ptr_ != nullptr)
                {
                    destroy();
                }
            }

            void assign(const object_storage& other)
            {
                assign(std::integral_constant<bool,std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value>(), other);
            }

            void assign(std::true_type, const object_storage& other)
            {
                tag_ = other.tag_;
                destroy();
                ptr_ = nullptr;
                create(object_allocator(other.get_allocator()), *(other.ptr_));
            }

            void assign(std::false_type, const object_storage& other)
            {
                auto alloc = get_allocator();
                tag_ = other.tag_;
                destroy();
                ptr_ = nullptr;
                create(object_allocator(alloc), *(other.ptr_));
            }

            void assign(object_storage&& other)
            {
                swap(other);
            }

            void swap(object_storage& other)
            {
                using std::swap;
                swap(ptr_, other.ptr_);
                swap(tag_, other.tag_);
            }

            semantic_tag tag() const
            {
                return tag_;
            }

            object& value()
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return *ptr_;
            }

            const object& value() const
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return *ptr_;
            }

            allocator_type get_allocator() const
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return ptr_->get_allocator();
            }
        private:

            void destroy() noexcept
            {
                object_allocator alloc(ptr_->get_allocator());
                std::allocator_traits<object_allocator>::destroy(alloc, extension_traits::to_plain_pointer(ptr_));
                std::allocator_traits<object_allocator>::deallocate(alloc, ptr_,1);
            }
        };
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic pop
#endif

    private:
        class json_const_pointer_storage final
        {
        public:
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        private:
            const basic_json* p_;
        public:
            json_const_pointer_storage(const basic_json* p)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::const_json_pointer)), short_str_length_(0), tag_(p->tag()),
                  p_(p)
            {
            }

            const basic_json* value() const
            {
                return p_;
            }
        };

        template <typename ParentType>
        class proxy 
        {
            friend class basic_json<char_type,policy_type,allocator_type>;

            ParentType& parent_;
            string_view_type key_;

            proxy() = delete;

            proxy(const proxy& other) = default;
            proxy(proxy&& other) = default;
            proxy& operator = (const proxy& other) = delete; 
            proxy& operator = (proxy&& other) = delete; 

            proxy(ParentType& parent, const string_view_type& key)
                : parent_(parent), key_(key)
            {
            }

            basic_json& evaluate_with_default()
            {
                basic_json& val = parent_.evaluate_with_default();
                auto it = val.find(key_);
                if (it == val.object_range().end())
                {
                    auto r = val.try_emplace(key_, json_object_arg, semantic_tag::none);
                    return r.first->value();
                }
                else
                {
                    return it->value();
                }
            }

            basic_json& evaluate(std::size_t index)
            {
                return evaluate().at(index);
            }

            const basic_json& evaluate(std::size_t index) const
            {
                return evaluate().at(index);
            }

            basic_json& evaluate(const string_view_type& index)
            {
                return evaluate().at(index);
            }

            const basic_json& evaluate(const string_view_type& index) const
            {
                return evaluate().at(index);
            }
        public:
            using proxied_type = basic_json;
            using proxy_type = proxy<typename ParentType::proxy_type>;

            basic_json& evaluate() 
            {
                return parent_.evaluate(key_);
            }

            const basic_json& evaluate() const
            {
                return parent_.evaluate(key_);
            }

            operator basic_json&()
            {
                return evaluate();
            }

            operator const basic_json&() const
            {
                return evaluate();
            }

            object_range_type object_range()
            {
                return evaluate().object_range();
            }

            const_object_range_type object_range() const
            {
                return evaluate().object_range();
            }

            array_range_type array_range()
            {
                return evaluate().array_range();
            }

            const_array_range_type array_range() const
            {
                return evaluate().array_range();
            }

            std::size_t size() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return 0;
                }
                return evaluate().size();
            }

            json_storage_kind storage_kind() const
            {
                return evaluate().storage_kind();
            }

            semantic_tag tag() const
            {
                return evaluate().tag();
            }

            json_type type() const
            {
                return evaluate().type();
            }

            std::size_t count(const string_view_type& name) const
            {
                return evaluate().count(name);
            }

            allocator_type get_allocator() const
            {
                return evaluate().get_allocator();
            }

            uint64_t ext_tag() const
            {
                return evaluate().ext_tag();
            }

            bool contains(const string_view_type& key) const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }

                return evaluate().contains(key);
            }

            bool is_null() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_null();
            }

            bool empty() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return true;
                }
                return evaluate().empty();
            }

            std::size_t capacity() const
            {
                return evaluate().capacity();
            }

            void reserve(std::size_t n)
            {
                evaluate().reserve(n);
            }

            void resize(std::size_t n)
            {
                evaluate().resize(n);
            }

            template <typename T>
            void resize(std::size_t n, T val)
            {
                evaluate().resize(n,val);
            }

            template <typename T,typename... Args>
            bool is(Args&&... args) const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().template is<T>(std::forward<Args>(args)...);
            }

            bool is_string() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_string();
            }

            bool is_string_view() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_string_view();
            }

            bool is_byte_string() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_byte_string();
            }

            bool is_byte_string_view() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_byte_string_view();
            }

            bool is_bignum() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_bignum();
            }

            bool is_number() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_number();
            }
            bool is_bool() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_bool();
            }

            bool is_object() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_object();
            }

            bool is_array() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_array();
            }

            bool is_int64() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_int64();
            }

            bool is_uint64() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_uint64();
            }

            bool is_half() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_half();
            }

            bool is_double() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().is_double();
            }

            string_view_type as_string_view() const 
            {
                return evaluate().as_string_view();
            }

            byte_string_view as_byte_string_view() const 
            {
                return evaluate().as_byte_string_view();
            }

            template <typename SAllocator=std::allocator<char_type>>
            std::basic_string<char_type,char_traits_type,SAllocator> as_string() const 
            {
                return evaluate().as_string();
            }

            template <typename SAllocator=std::allocator<char_type>>
            std::basic_string<char_type,char_traits_type,SAllocator> as_string(const SAllocator& alloc) const 
            {
                return evaluate().as_string(alloc);
            }

            template <typename BAllocator=std::allocator<uint8_t>>
            basic_byte_string<BAllocator> as_byte_string() const
            {
                return evaluate().template as_byte_string<BAllocator>();
            }

            template <typename T>
            typename std::enable_if<is_json_type_traits_specialized<basic_json,T>::value,T>::type
            as() const
            {
                return evaluate().template as<T>();
            }

            template <typename T>
            typename std::enable_if<std::is_convertible<uint8_t,typename T::value_type>::value,T>::type
            as(byte_string_arg_t, semantic_tag hint) const
            {
                return evaluate().template as<T>(byte_string_arg, hint);
            }

            bool as_bool() const
            {
                return evaluate().as_bool();
            }

            double as_double() const
            {
                return evaluate().as_double();
            }

            template <typename T>
            T as_integer() const
            {
                return evaluate().template as_integer<T>();
            }

            template <typename T>
            proxy& operator=(T&& val) 
            {
                parent_.evaluate_with_default().insert_or_assign(key_, std::forward<T>(val));
                return *this;
            }

            basic_json& operator[](std::size_t i)
            {
                return evaluate_with_default().at(i);
            }

            const basic_json& operator[](std::size_t i) const
            {
                return evaluate().at(i);
            }

            proxy_type operator[](const string_view_type& key)
            {
                return proxy_type(*this,key);
            }

            const basic_json& operator[](const string_view_type& name) const
            {
                return at(name);
            }

            basic_json& at(const string_view_type& name)
            {
                return evaluate().at(name);
            }

            const basic_json& at(const string_view_type& name) const
            {
                return evaluate().at(name);
            }

            const basic_json& at_or_null(const string_view_type& name) const
            {
                return evaluate().at_or_null(name);
            }

            const basic_json& at(std::size_t index)
            {
                return evaluate().at(index);
            }

            const basic_json& at(std::size_t index) const
            {
                return evaluate().at(index);
            }

            object_iterator find(const string_view_type& name)
            {
                return evaluate().find(name);
            }

            const_object_iterator find(const string_view_type& name) const
            {
                return evaluate().find(name);
            }

            template <typename T,typename U>
            T get_value_or(const string_view_type& name, U&& default_value) const
            {
                static_assert(std::is_copy_constructible<T>::value,
                              "get_value_or: T must be copy constructible");
                static_assert(std::is_convertible<U&&, T>::value,
                              "get_value_or: U must be convertible to T");
                return evaluate().template get_value_or<T,U>(name,std::forward<U>(default_value));
            }

            void shrink_to_fit()
            {
                evaluate_with_default().shrink_to_fit();
            }

            void clear()
            {
                evaluate().clear();
            }
            // Remove all elements from an array or object

            object_iterator erase(const_object_iterator pos)
            {
                return evaluate().erase(pos);
            }
            // Remove a range of elements from an object 

            object_iterator erase(const_object_iterator first, const_object_iterator last)
            {
                return evaluate().erase(first, last);
            }
            // Remove a range of elements from an object 

            void erase(const string_view_type& name)
            {
                evaluate().erase(name);
            }

            array_iterator erase(const_array_iterator pos)
            {
                return evaluate().erase(pos);
            }
            // Removes the element at pos 

            array_iterator erase(const_array_iterator first, const_array_iterator last)
            {
                return evaluate().erase(first, last);
            }
            // Remove a range of elements from an array 

            // merge

            void merge(const basic_json& source)
            {
                return evaluate().merge(source);
            }

            void merge(basic_json&& source)
            {
                return evaluate().merge(std::move(source));
            }

            void merge(object_iterator hint, const basic_json& source)
            {
                return evaluate().merge(hint, source);
            }

            void merge(object_iterator hint, basic_json&& source)
            {
                return evaluate().merge(hint, std::move(source));
            }

            // merge_or_update

            void merge_or_update(const basic_json& source)
            {
                return evaluate().merge_or_update(source);
            }

            void merge_or_update(basic_json&& source)
            {
                return evaluate().merge_or_update(std::move(source));
            }

            void merge_or_update(object_iterator hint, const basic_json& source)
            {
                return evaluate().merge_or_update(hint, source);
            }

            void merge_or_update(object_iterator hint, basic_json&& source)
            {
                return evaluate().merge_or_update(hint, std::move(source));
            }

            template <typename T>
            std::pair<object_iterator,bool> insert_or_assign(const string_view_type& name, T&& val)
            {
                return evaluate().insert_or_assign(name,std::forward<T>(val));
            }

           // emplace

            template <typename ... Args>
            std::pair<object_iterator,bool> try_emplace(const string_view_type& name, Args&&... args)
            {
                return evaluate().try_emplace(name,std::forward<Args>(args)...);
            }

            template <typename T>
            object_iterator insert_or_assign(object_iterator hint, const string_view_type& name, T&& val)
            {
                return evaluate().insert_or_assign(hint, name, std::forward<T>(val));
            }

            template <typename ... Args>
            object_iterator try_emplace(object_iterator hint, const string_view_type& name, Args&&... args)
            {
                return evaluate().try_emplace(hint, name, std::forward<Args>(args)...);
            }

            template <typename... Args> 
            array_iterator emplace(const_array_iterator pos, Args&&... args)
            {
                return evaluate_with_default().emplace(pos, std::forward<Args>(args)...);
            }

            template <typename... Args> 
            basic_json& emplace_back(Args&&... args)
            {
                return evaluate_with_default().emplace_back(std::forward<Args>(args)...);
            }

            template <typename T>
            void push_back(T&& val)
            {
                evaluate_with_default().push_back(std::forward<T>(val));
            }

            template <typename T>
            array_iterator insert(const_array_iterator pos, T&& val)
            {
                return evaluate_with_default().insert(pos, std::forward<T>(val));
            }

            template <typename InputIt>
            array_iterator insert(const_array_iterator pos, InputIt first, InputIt last)
            {
                return evaluate_with_default().insert(pos, first, last);
            }

            template <typename InputIt>
            void insert(InputIt first, InputIt last)
            {
                evaluate_with_default().insert(first, last);
            }

            template <typename InputIt>
            void insert(sorted_unique_range_tag tag, InputIt first, InputIt last)
            {
                evaluate_with_default().insert(tag, first, last);
            }

            template <typename... Args>
            void dump(Args&& ... args) const
            {
                evaluate().dump(std::forward<Args>(args)...);
            }

            template <typename... Args>
            void dump_pretty(Args&& ... args) const
            {
                evaluate().dump_pretty(std::forward<Args>(args)...);
            }

            void swap(basic_json& other) 
            {
                evaluate_with_default().swap(other);
            }

            friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const proxy& o)
            {
                o.dump(os);
                return os;
            }

            std::basic_string<char_type> to_string() const 
            {
                return evaluate().to_string();
            }

            template <typename IntegerType>
            bool is_integer() const noexcept
            {
                if (!parent_.contains(key_))
                {
                    return false;
                }
                return evaluate().template is_integer<IntegerType>();
            }

        };

        using proxy_type = proxy<basic_json>;

        union 
        {
            common_storage common_stor_;
            null_storage null_stor_;
            bool_storage bool_stor_;
            int64_storage int64_stor_;
            uint64_storage uint64_stor_;
            half_storage half_stor_;
            double_storage double_stor_;
            short_string_storage short_string_stor_;
            long_string_storage long_string_stor_;
            byte_string_storage byte_string_stor_;
            array_storage array_stor_;
            object_storage object_stor_;
            empty_object_storage empty_object_stor_;
            json_const_pointer_storage json_const_pointer_stor_;
        };

        void destroy()
        {
            switch (storage_kind())
            {
                case json_storage_kind::long_str:
                    destroy_var<long_string_storage>();
                    break;
                case json_storage_kind::byte_str:
                    destroy_var<byte_string_storage>();
                    break;
                case json_storage_kind::array:
                    destroy_var<array_storage>();
                    break;
                case json_storage_kind::object:
                    destroy_var<object_storage>();
                    break;
                default:
                    break;
            }
        }

        template <typename VariantType,typename... Args>
        void construct(Args&&... args)
        {
            ::new (&cast<VariantType>()) VariantType(std::forward<Args>(args)...);
        }

        template <typename T>
        void destroy_var()
        {
            cast<T>().~T();
        }

        template <typename T>
        struct identity { using type = T*; };
    public:
        template <typename T> 
        T& cast()
        {
            return cast(identity<T>());
        }

        template <typename T> 
        const T& cast() const
        {
            return cast(identity<T>());
        }
    private:
        null_storage& cast(identity<null_storage>) 
        {
            return null_stor_;
        }

        const null_storage& cast(identity<null_storage>) const
        {
            return null_stor_;
        }

        empty_object_storage& cast(identity<empty_object_storage>) 
        {
            return empty_object_stor_;
        }

        const empty_object_storage& cast(identity<empty_object_storage>) const
        {
            return empty_object_stor_;
        }

        bool_storage& cast(identity<bool_storage>) 
        {
            return bool_stor_;
        }

        const bool_storage& cast(identity<bool_storage>) const
        {
            return bool_stor_;
        }

        int64_storage& cast(identity<int64_storage>) 
        {
            return int64_stor_;
        }

        const int64_storage& cast(identity<int64_storage>) const
        {
            return int64_stor_;
        }

        uint64_storage& cast(identity<uint64_storage>) 
        {
            return uint64_stor_;
        }

        const uint64_storage& cast(identity<uint64_storage>) const
        {
            return uint64_stor_;
        }

        half_storage& cast(identity<half_storage>)
        {
            return half_stor_;
        }

        const half_storage& cast(identity<half_storage>) const
        {
            return half_stor_;
        }

        double_storage& cast(identity<double_storage>) 
        {
            return double_stor_;
        }

        const double_storage& cast(identity<double_storage>) const
        {
            return double_stor_;
        }

        short_string_storage& cast(identity<short_string_storage>)
        {
            return short_string_stor_;
        }

        const short_string_storage& cast(identity<short_string_storage>) const
        {
            return short_string_stor_;
        }

        long_string_storage& cast(identity<long_string_storage>)
        {
            return long_string_stor_;
        }

        const long_string_storage& cast(identity<long_string_storage>) const
        {
            return long_string_stor_;
        }

        byte_string_storage& cast(identity<byte_string_storage>)
        {
            return byte_string_stor_;
        }

        const byte_string_storage& cast(identity<byte_string_storage>) const
        {
            return byte_string_stor_;
        }

        object_storage& cast(identity<object_storage>)
        {
            return object_stor_;
        }

        const object_storage& cast(identity<object_storage>) const
        {
            return object_stor_;
        }

        array_storage& cast(identity<array_storage>)
        {
            return array_stor_;
        }

        const array_storage& cast(identity<array_storage>) const
        {
            return array_stor_;
        }

        json_const_pointer_storage& cast(identity<json_const_pointer_storage>) 
        {
            return json_const_pointer_stor_;
        }

        const json_const_pointer_storage& cast(identity<json_const_pointer_storage>) const
        {
            return json_const_pointer_stor_;
        }

        template <typename TypeL,typename TypeR>
        void swap_l_r(basic_json& other)
        {
            swap_l_r(identity<TypeL>(), identity<TypeR>(), other);
        }

        template <typename TypeL,typename TypeR>
        void swap_l_r(identity<TypeL>,identity<TypeR>,basic_json& other)
        {
            TypeR tmpR(std::move(other.cast<TypeR>())); 

            other.destroy();
            other.construct<TypeL>(std::move(cast<TypeL>()));

            destroy();
            construct<TypeR>(std::move(tmpR));
        }

        void swap_l_r(identity<long_string_storage>,identity<long_string_storage>,basic_json& other)
        {
            cast<long_string_storage>().swap(other.cast<long_string_storage>());
        }

        void swap_l_r(identity<byte_string_storage>,identity<byte_string_storage>,basic_json& other)
        {
            cast<byte_string_storage>().swap(other.cast<byte_string_storage>());
        }

        void swap_l_r(identity<array_storage>,identity<array_storage>,basic_json& other)
        {
            cast<array_storage>().swap(other.cast<array_storage>());
        }

        void swap_l_r(identity<object_storage>,identity<object_storage>,basic_json& other)
        {
            cast<object_storage>().swap(other.cast<object_storage>());
        }

        template <typename TypeL>
        void swap_l(basic_json& other)
        {
            switch (other.storage_kind())
            {
                case json_storage_kind::null         : swap_l_r<TypeL, null_storage>(other); break;
                case json_storage_kind::empty_object : swap_l_r<TypeL, empty_object_storage>(other); break;
                case json_storage_kind::boolean         : swap_l_r<TypeL, bool_storage>(other); break;
                case json_storage_kind::int64        : swap_l_r<TypeL, int64_storage>(other); break;
                case json_storage_kind::uint64       : swap_l_r<TypeL, uint64_storage>(other); break;
                case json_storage_kind::half_float         : swap_l_r<TypeL, half_storage>(other); break;
                case json_storage_kind::float64       : swap_l_r<TypeL, double_storage>(other); break;
                case json_storage_kind::short_str : swap_l_r<TypeL, short_string_storage>(other); break;
                case json_storage_kind::long_str  : swap_l_r<TypeL, long_string_storage>(other); break;
                case json_storage_kind::byte_str  : swap_l_r<TypeL, byte_string_storage>(other); break;
                case json_storage_kind::array        : swap_l_r<TypeL, array_storage>(other); break;
                case json_storage_kind::object       : swap_l_r<TypeL, object_storage>(other); break;
                case json_storage_kind::const_json_pointer : swap_l_r<TypeL, json_const_pointer_storage>(other); break;
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        void uninitialized_copy(const basic_json& other)
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        construct<long_string_storage>(other.cast<long_string_storage>());
                        break;
                    case json_storage_kind::byte_str:
                        construct<byte_string_storage>(other.cast<byte_string_storage>());
                        break;
                    case json_storage_kind::object:
                        construct<object_storage>(other.cast<object_storage>());
                        break;
                    case json_storage_kind::array:
                        construct<array_storage>(other.cast<array_storage>());
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_copy_a(const basic_json& other, const Allocator& alloc)
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        construct<long_string_storage>(other.cast<long_string_storage>(),alloc);
                        break;
                    case json_storage_kind::byte_str:
                        construct<byte_string_storage>(other.cast<byte_string_storage>(),alloc);
                        break;
                    case json_storage_kind::array:
                        construct<array_storage>(other.cast<array_storage>(),alloc);
                        break;
                    case json_storage_kind::object:
                        construct<object_storage>(other.cast<object_storage>(),alloc);
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_move(basic_json&& other) noexcept
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        construct<long_string_storage>(std::move(other.cast<long_string_storage>()));
                        break;
                    case json_storage_kind::byte_str:
                        construct<byte_string_storage>(std::move(other.cast<byte_string_storage>()));
                        break;
                    case json_storage_kind::array:
                        construct<array_storage>(std::move(other.cast<array_storage>()));
                        break;
                    case json_storage_kind::object:
                        construct<object_storage>(std::move(other.cast<object_storage>()));
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_move_a(std::true_type /* stateless allocator */, 
            basic_json&& other, const Allocator&) noexcept
        {
            uninitialized_move(std::move(other));
        }

        void uninitialized_move_a(std::false_type /* stateful allocator */, 
            basic_json&& other, const Allocator& alloc) noexcept
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        construct<long_string_storage>(std::move(other.cast<long_string_storage>()), alloc);
                        break;
                    case json_storage_kind::byte_str:
                        construct<byte_string_storage>(std::move(other.cast<byte_string_storage>()), alloc);
                        break;
                    case json_storage_kind::array:
                        construct<array_storage>(std::move(other.cast<array_storage>()), alloc);
                        break;
                    case json_storage_kind::object:
                        construct<object_storage>(std::move(other.cast<object_storage>()), alloc);
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void copy_assignment(const basic_json& other)
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                destroy();
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        if (storage_kind() == json_storage_kind::long_str)
                        {
                            cast<long_string_storage>().assign(other.cast<long_string_storage>());
                        }
                        else
                        {
                            destroy();
                            uninitialized_copy(other);
                        }
                        break;
                    case json_storage_kind::byte_str:
                        if (storage_kind() == json_storage_kind::byte_str)
                        {
                            cast<byte_string_storage>().assign(other.cast<byte_string_storage>());
                        }
                        else
                        {
                            destroy();
                            uninitialized_copy(other);
                        }
                        break;
                    case json_storage_kind::array:
                        if (storage_kind() == json_storage_kind::array)
                        {
                            cast<array_storage>().assign(other.cast<array_storage>());
                        }
                        else
                        {
                            destroy();
                            uninitialized_copy(other);
                        }
                        break;
                    case json_storage_kind::object:
                        if (storage_kind() == json_storage_kind::object)
                        {
                            cast<object_storage>().assign(other.cast<object_storage>());
                        }
                        else
                        {
                            destroy();
                            uninitialized_copy(other);
                        }
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void move_assignment(basic_json&& other)
        {
            if (is_scalar_storage(other.storage_kind()))
            {
                destroy();
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        if (storage_kind() == json_storage_kind::long_str)
                        {
                            cast<long_string_storage>().assign(std::move(other.cast<long_string_storage>()));
                        }
                        else
                        {
                            destroy();
                            uninitialized_move(std::move(other));
                        }
                        break;
                    case json_storage_kind::byte_str:
                        if (storage_kind() == json_storage_kind::byte_str)
                        {
                            cast<byte_string_storage>().assign(std::move(other.cast<byte_string_storage>()));
                        }
                        else
                        {
                            destroy();
                            uninitialized_move(std::move(other));
                        }
                        break;
                    case json_storage_kind::array:
                        if (storage_kind() == json_storage_kind::array)
                        {
                            cast<array_storage>().assign(std::move(other.cast<array_storage>()));
                        }
                        else
                        {
                            destroy();
                            uninitialized_move(std::move(other));
                        }
                        break;
                    case json_storage_kind::object:
                        if (storage_kind() == json_storage_kind::object)
                        {
                            cast<object_storage>().assign(std::move(other.cast<object_storage>()));
                        }
                        else
                        {
                            destroy();
                            uninitialized_move(std::move(other));
                        }
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        basic_json& evaluate_with_default() 
        {
            return *this;
        }

        basic_json& evaluate(const string_view_type& name) 
        {
            return at(name);
        }

        const basic_json& evaluate(const string_view_type& name) const
        {
            return at(name);
        }

    public:

        basic_json& evaluate() 
        {
            return *this;
        }

        const basic_json& evaluate() const
        {
            return *this;
        }

        basic_json& operator=(const basic_json& other)
        {
            if (this != &other)
            {
                copy_assignment(other);
            }
            return *this;
        }

        basic_json& operator=(basic_json&& other) noexcept
        {
            if (this != &other)
            {
                move_assignment(std::move(other));
            }
            return *this;
        }

        json_storage_kind storage_kind() const
        {
            // It is legal to access 'common_stor_.storage_kind_' even though 
            // common_stor_ is not the active member of the union because 'storage_kind_' 
            // is a part of the common initial sequence of all union members
            // as defined in 11.4-25 of the Standard.
            return static_cast<json_storage_kind>(common_stor_.storage_kind_);
        }

        json_type type() const
        {
            switch(storage_kind())
            {
                case json_storage_kind::null:
                    return json_type::null_value;
                case json_storage_kind::boolean:
                    return json_type::bool_value;
                case json_storage_kind::int64:
                    return json_type::int64_value;
                case json_storage_kind::uint64:
                    return json_type::uint64_value;
                case json_storage_kind::half_float:
                    return json_type::half_value;
                case json_storage_kind::float64:
                    return json_type::double_value;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return json_type::string_value;
                case json_storage_kind::byte_str:
                    return json_type::byte_string_value;
                case json_storage_kind::array:
                    return json_type::array_value;
                case json_storage_kind::empty_object:
                case json_storage_kind::object:
                    return json_type::object_value;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->type();
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        semantic_tag tag() const
        {
            // It is legal to access 'common_stor_.tag_' even though 
            // common_stor_ is not the active member of the union because 'tag_' 
            // is a part of the common initial sequence of all union members
            // as defined in 11.4-25 of the Standard.
            switch(storage_kind())
            {
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->tag();
                default:
                    return common_stor_.tag_;
            }
        }

        std::size_t size() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().size();
                case json_storage_kind::empty_object:
                    return 0;
                case json_storage_kind::object:
                    return cast<object_storage>().value().size();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->size();
                default:
                    return 0;
            }
        }

        string_view_type as_string_view() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                    return string_view_type(cast<short_string_storage>().data(),cast<short_string_storage>().length());
                case json_storage_kind::long_str:
                    return string_view_type(cast<long_string_storage>().data(),cast<long_string_storage>().length());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_string_view();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a string"));
            }
        }

        template <typename BAllocator=std::allocator<uint8_t>>
        basic_byte_string<BAllocator> as_byte_string() const
        {
            using byte_string_type = basic_byte_string<BAllocator>;
            std::error_code ec;

            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    value_converter<jsoncons::string_view, byte_string_type> converter;
                    byte_string_type v = converter.convert(as_string_view(),tag(), ec);
                    if (ec)
                    {
                        JSONCONS_THROW(ser_error(ec));
                    }
                    return v;
                }
                case json_storage_kind::byte_str:
                    return basic_byte_string<BAllocator>(cast<byte_string_storage>().data(),cast<byte_string_storage>().length());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_byte_string();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a byte string"));
            }
        }

        byte_string_view as_byte_string_view() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return byte_string_view(cast<byte_string_storage>().data(),cast<byte_string_storage>().length());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_byte_string_view();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a byte string"));
            }
        }

        int compare(const basic_json& rhs) const noexcept
        {
            if (this == &rhs)
            {
                return 0;
            }
            switch (storage_kind())
            {
                case json_storage_kind::const_json_pointer:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::const_json_pointer:
                            return (cast<json_const_pointer_storage>().value())->compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return (cast<json_const_pointer_storage>().value())->compare(rhs);
                    }
                    break;
                case json_storage_kind::null:
                    return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                case json_storage_kind::empty_object:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            return 0;
                        case json_storage_kind::object:
                            return rhs.empty() ? 0 : -1;
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::boolean:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::boolean:
                            return static_cast<int>(cast<bool_storage>().value()) - static_cast<int>(rhs.cast<bool_storage>().value());
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::int64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                        {
                            if (cast<int64_storage>().value() == rhs.cast<int64_storage>().value())
                                return 0;
                            return cast<int64_storage>().value() < rhs.cast<int64_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::uint64:
                            if (cast<int64_storage>().value() < 0) 
                                return -1;
                            else if (static_cast<uint64_t>(cast<int64_storage>().value()) == rhs.cast<uint64_storage>().value()) 
                                return 0;
                            else
                                return static_cast<uint64_t>(cast<int64_storage>().value()) < rhs.cast<uint64_storage>().value() ? -1 : 1;
                        case json_storage_kind::float64:
                        {
                            double r = static_cast<double>(cast<int64_storage>().value()) - rhs.cast<double_storage>().value();
                            return r == 0.0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::uint64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                            if (rhs.cast<int64_storage>().value() < 0) 
                                return 1;
                            else if (cast<uint64_storage>().value() == static_cast<uint64_t>(rhs.cast<int64_storage>().value()))
                                return 0;
                            else
                                return cast<uint64_storage>().value() < static_cast<uint64_t>(rhs.cast<int64_storage>().value()) ? -1 : 1;
                        case json_storage_kind::uint64:
                            if (cast<uint64_storage>().value() == static_cast<uint64_t>(rhs.cast<int64_storage>().value()))
                                return 0;
                            else
                                return cast<uint64_storage>().value() < static_cast<uint64_t>(rhs.cast<int64_storage>().value()) ? -1 : 1;
                        case json_storage_kind::float64:
                        {
                            auto r = static_cast<double>(cast<uint64_storage>().value()) - rhs.cast<double_storage>().value();
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::float64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                        {
                            auto r = cast<double_storage>().value() - static_cast<double>(rhs.cast<int64_storage>().value());
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::uint64:
                        {
                            auto r = cast<double_storage>().value() - static_cast<double>(rhs.cast<uint64_storage>().value());
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::float64:
                        {
                            auto r = cast<double_storage>().value() - rhs.cast<double_storage>().value();
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            if (is_string_storage(rhs.storage_kind()))
                            {
                                double val1 = as_double();
                                double val2 = rhs.as_double();
                                auto r = val1 - val2;
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            else
                            {
                                return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                            }
                    }
                    break;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    if (is_number_tag(tag()))
                    {
                        double val1 = as_double(); 
                        switch (rhs.storage_kind())
                        {
                            case json_storage_kind::int64:
                            {
                                auto r = val1 - static_cast<double>(rhs.cast<int64_storage>().value());
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::uint64:
                            {
                                auto r = val1 - static_cast<double>(rhs.cast<uint64_storage>().value());
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::float64:
                            {
                                auto r = val1 - rhs.cast<double_storage>().value();
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::const_json_pointer:
                                return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                            default:
                                if (is_string_storage(rhs.storage_kind()))
                                {
                                    double val2 = rhs.as_double();
                                    auto r = val1 - val2; 
                                    return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                                }
                                else
                                {
                                    return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                                }
                        }
                    }
                    else
                    {
                        // compare regular text
                        switch (rhs.storage_kind())
                        {
                            case json_storage_kind::short_str:
                                return as_string_view().compare(rhs.as_string_view());
                            case json_storage_kind::long_str:
                                return as_string_view().compare(rhs.as_string_view());
                            case json_storage_kind::const_json_pointer:
                                return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                            default:
                                return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                        }
                    }
                    break;
                case json_storage_kind::byte_str:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::byte_str:
                        {
                            return as_byte_string_view().compare(rhs.as_byte_string_view());
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::array:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::array:
                        {
                            if (cast<array_storage>().value() == rhs.cast<array_storage>().value())
                                return 0; 
                            else 
                                return cast<array_storage>().value() < rhs.cast<array_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::object:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            return empty() ? 0 : 1;
                        case json_storage_kind::object:
                        {
                            if (cast<object_storage>().value() == rhs.cast<object_storage>().value())
                                return 0; 
                            else 
                                return cast<object_storage>().value() < rhs.cast<object_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::const_json_pointer:
                            return compare(*(rhs.cast<json_const_pointer_storage>().value()));
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>((int)rhs.storage_kind());
                    }
                    break;
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        void swap(basic_json& other) noexcept
        {
            if (this == &other)
            {
                return;
            }

            switch (storage_kind())
            {
                case json_storage_kind::null: swap_l<null_storage>(other); break;
                case json_storage_kind::empty_object : swap_l<empty_object_storage>(other); break;
                case json_storage_kind::boolean: swap_l<bool_storage>(other); break;
                case json_storage_kind::int64: swap_l<int64_storage>(other); break;
                case json_storage_kind::uint64: swap_l<uint64_storage>(other); break;
                case json_storage_kind::half_float: swap_l<half_storage>(other); break;
                case json_storage_kind::float64: swap_l<double_storage>(other); break;
                case json_storage_kind::short_str: swap_l<short_string_storage>(other); break;
                case json_storage_kind::long_str: swap_l<long_string_storage>(other); break;
                case json_storage_kind::byte_str: swap_l<byte_string_storage>(other); break;
                case json_storage_kind::array: swap_l<array_storage>(other); break;
                case json_storage_kind::object: swap_l<object_storage>(other); break;
                case json_storage_kind::const_json_pointer: swap_l<json_const_pointer_storage>(other); break;
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }
        // from string

        template <typename Source>
        static
         typename std::enable_if<extension_traits::is_sequence_of<Source,char_type>::value,basic_json>::type
            parse(const Source& source, 
              const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            json_decoder<basic_json> decoder;
            basic_json_parser<char_type> parser(options);

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        template <typename Source,typename TempAllocator >
        static
         typename std::enable_if<extension_traits::is_sequence_of<Source,char_type>::value,basic_json>::type
            parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, const Source& source, 
              const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            json_decoder<basic_json> decoder(alloc_set.get_allocator(), alloc_set.get_temp_allocator());
            basic_json_parser<char_type,TempAllocator> parser(options, alloc_set.get_temp_allocator());

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        static basic_json parse(const char_type* str, std::size_t length, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(jsoncons::string_view(str,length), options);
        }

        static basic_json parse(const char_type* source, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(jsoncons::basic_string_view<char_type>(source), options);
        }

        template <typename TempAllocator >
        static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, const char_type* source, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(alloc_set, jsoncons::basic_string_view<char_type>(source), options);
        }

        template <typename TempAllocator >
        static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, 
            const char_type* str, std::size_t length,
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(alloc_set, jsoncons::basic_string_view<char_type>(str, length), options);
        }

        // from stream

        static basic_json parse(std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,stream_source<char_type>,Allocator> reader(is, decoder, options);
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        template <typename TempAllocator >
        static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder(alloc_set.get_allocator(), alloc_set.get_temp_allocator());
            basic_json_reader<char_type,stream_source<char_type>,Allocator> reader(is, decoder, options, alloc_set.get_temp_allocator());
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        // from iterator

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,iterator_source<InputIt>,Allocator> reader(iterator_source<InputIt>(std::forward<InputIt>(first),
                std::forward<InputIt>(last)), decoder, options);
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

        template <typename InputIt,typename TempAllocator >
        static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder(alloc_set.get_allocator(), alloc_set.get_temp_allocator());
            basic_json_reader<char_type,iterator_source<InputIt>,Allocator> reader(iterator_source<InputIt>(std::forward<InputIt>(first),
                std::forward<InputIt>(last)), 
                decoder, options, alloc_set.get_temp_allocator());
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

#if !defined(JSONCONS_NO_DEPRECATED)

        static basic_json parse(const char_type* s, 
            const basic_json_decode_options<char_type>& options, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(jsoncons::basic_string_view<char_type>(s), options, err_handler);
        }

        static basic_json parse(const char_type* s, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(jsoncons::basic_string_view<char_type>(s), basic_json_decode_options<char_type>(), err_handler);
        }

        static basic_json parse(std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,stream_source<char_type>> reader(is, decoder, options, err_handler);
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        static basic_json parse(std::basic_istream<char_type>& is, std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(is, basic_json_decode_options<CharT>(), err_handler);
        }

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,iterator_source<InputIt>> reader(iterator_source<InputIt>(std::forward<InputIt>(first),std::forward<InputIt>(last)), decoder, options, err_handler);
            reader.read_next();
            reader.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

        template <typename Source>
        static
        typename std::enable_if<extension_traits::is_sequence_of<Source,char_type>::value,basic_json>::type
        parse(const Source& source, 
              const basic_json_decode_options<char_type>& options, 
              std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_parser<char_type> parser(options,err_handler);

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (!decoder.is_valid())
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        template <typename Source>
        static
        typename std::enable_if<extension_traits::is_sequence_of<Source,char_type>::value,basic_json>::type
        parse(const Source& source, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(source, basic_json_decode_options<CharT>(), err_handler);
        }

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(first, last, basic_json_decode_options<CharT>(), err_handler);
        }
#endif
        static basic_json make_array()
        {
            return basic_json(array());
        }

        static basic_json make_array(const array& alloc)
        {
            return basic_json(alloc);
        }

        static basic_json make_array(const array& a, allocator_type alloc)
        {
            return basic_json(a, semantic_tag::none, alloc);
        }

        static basic_json make_array(std::initializer_list<basic_json> init, const Allocator& alloc = Allocator())
        {
            return array(std::move(init),alloc);
        }

        static basic_json make_array(std::size_t n, const Allocator& alloc = Allocator())
        {
            return array(n,alloc);
        }

        template <typename T>
        static basic_json make_array(std::size_t n, const T& val, const Allocator& alloc = Allocator())
        {
            return basic_json::array(n, val,alloc);
        }

        template <std::size_t dim>
        static typename std::enable_if<dim==1,basic_json>::type make_array(std::size_t n)
        {
            return array(n);
        }

        template <std::size_t dim,typename T>
        static typename std::enable_if<dim==1,basic_json>::type make_array(std::size_t n, const T& val, const Allocator& alloc = Allocator())
        {
            return array(n,val,alloc);
        }

        template <std::size_t dim,typename... Args>
        static typename std::enable_if<(dim>1),basic_json>::type make_array(std::size_t n, Args... args)
        {
            const size_t dim1 = dim - 1;

            basic_json val = make_array<dim1>(std::forward<Args>(args)...);
            val.resize(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                val[i] = make_array<dim1>(std::forward<Args>(args)...);
            }
            return val;
        }

        static const basic_json& null()
        {
            static const basic_json a_null = basic_json(null_type(), semantic_tag::none);
            return a_null;
        }

        basic_json() 
        {
            construct<empty_object_storage>(semantic_tag::none);
        }

        explicit basic_json(const Allocator&) 
        {
            construct<empty_object_storage>(semantic_tag::none);
        }

        basic_json(semantic_tag tag) 
        {
            construct<empty_object_storage>(tag);
        }

        basic_json(const basic_json& other)
        {
            uninitialized_copy(other);
        }

        basic_json(const basic_json& other, const Allocator& alloc)
        {
            uninitialized_copy_a(other,alloc);
        }

        basic_json(basic_json&& other) noexcept
        {
            uninitialized_move(std::move(other));
        }

        template <typename U = Allocator>
        basic_json(basic_json&& other, const Allocator& alloc) noexcept
        {
            uninitialized_move_a(extension_traits::is_stateless<U>{}, std::move(other), alloc);
        }

        explicit basic_json(json_object_arg_t, 
                            semantic_tag tag,
                            const Allocator& alloc = Allocator()) 
        {
            construct<object_storage>(object(alloc), tag);
        }

        explicit basic_json(json_object_arg_t, const Allocator& alloc = Allocator()) 
        {
            construct<object_storage>(object(alloc), semantic_tag::none);
        }

        template <typename InputIt>
        basic_json(json_object_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag = semantic_tag::none,
                   const Allocator& alloc = Allocator()) 
        {
            construct<object_storage>(object(first,last,alloc), tag);
        }

        basic_json(json_object_arg_t, 
                   std::initializer_list<std::pair<std::basic_string<char_type>,basic_json>> init, 
                   semantic_tag tag = semantic_tag::none, 
                   const Allocator& alloc = Allocator()) 
        {
            construct<object_storage>(object(init,alloc), tag);
        }

        explicit basic_json(json_array_arg_t, const Allocator& alloc = Allocator()) 
        {
            construct<array_storage>(array(alloc), semantic_tag::none);
        }

        explicit basic_json(json_array_arg_t, 
                            semantic_tag tag, 
                            const Allocator& alloc = Allocator()) 
        {
            construct<array_storage>(array(alloc), tag);
        }

        template <typename InputIt>
        basic_json(json_array_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag = semantic_tag::none, 
                   const Allocator& alloc = Allocator()) 
        {
            construct<array_storage>(array(first,last,alloc), tag);
        }

        basic_json(json_array_arg_t, 
                   std::initializer_list<basic_json> init, 
                   semantic_tag tag = semantic_tag::none, 
                   const Allocator& alloc = Allocator()) 
        {
            construct<array_storage>(array(init,alloc), tag);
        }

        basic_json(json_const_pointer_arg_t, const basic_json* p) noexcept 
        {
            if (p == nullptr)
            {
                construct<null_storage>(semantic_tag::none);
            }
            else
            {
                construct<json_const_pointer_storage>(p);
            }
        }

        basic_json(const array& val, semantic_tag tag = semantic_tag::none)
        {
            construct<array_storage>(val, tag);
        }

        basic_json(array&& val, semantic_tag tag = semantic_tag::none)
        {
            construct<array_storage>(std::move(val), tag);
        }

        basic_json(const object& val, semantic_tag tag = semantic_tag::none)
        {
            construct<object_storage>(val, tag);
        }

        basic_json(object&& val, semantic_tag tag = semantic_tag::none)
        {
            construct<object_storage>(std::move(val), tag);
        }

        template <typename T,
                  class = typename std::enable_if<!is_proxy_of<T,basic_json>::value && !extension_traits::is_basic_json<T>::value>::type>
        basic_json(const T& val)
            : basic_json(json_type_traits<basic_json,T>::to_json(val))
        {
        }

        template <typename T,
                  class = typename std::enable_if<!is_proxy_of<T,basic_json>::value && !extension_traits::is_basic_json<T>::value>::type>
        basic_json(const T& val, const Allocator& alloc)
            : basic_json(json_type_traits<basic_json,T>::to_json(val,alloc))
        {
        }

        basic_json(const string_type& s)
            : basic_json(s.data(), s.size(), semantic_tag::none, s.get_allocator())
        {
        }

        basic_json(const string_view_type& s, const allocator_type& alloc = allocator_type())
            : basic_json(s.data(), s.size(), semantic_tag::none, alloc)
        {
        }

        basic_json(const string_type& s, semantic_tag tag)
            : basic_json(s.data(), s.size(), tag, s.get_allocator())
        {
        }

        basic_json(const string_type& s, semantic_tag tag, const allocator_type& alloc)
            : basic_json(s.data(), s.size(), tag, alloc)
        {
        }

        basic_json(const char_type* s, semantic_tag tag = semantic_tag::none)
            : basic_json(s, char_traits_type::length(s), tag)
        {
        }

        basic_json(const char_type* s, semantic_tag tag, const allocator_type& alloc)
            : basic_json(s, char_traits_type::length(s), tag, alloc)
        {
        }

        basic_json(const char_type* s, const Allocator& alloc)
            : basic_json(s, char_traits_type::length(s), semantic_tag::none, alloc)
        {
        }

        basic_json(const char_type* s, std::size_t length, semantic_tag tag = semantic_tag::none)
        {
            if (length <= short_string_storage::max_length)
            {
                construct<short_string_storage>(tag, s, static_cast<uint8_t>(length));
            }
            else
            {
                construct<long_string_storage>(tag, s, length, char_allocator_type());
            }
        }

        basic_json(const char_type* s, std::size_t length, semantic_tag tag, const Allocator& alloc)
        {
            if (length <= short_string_storage::max_length)
            {
                construct<short_string_storage>(tag, s, static_cast<uint8_t>(length));
            }
            else
            {
                construct<long_string_storage>(tag, s, length, alloc);
            }
        }

        basic_json(half_arg_t, uint16_t val, semantic_tag tag = semantic_tag::none)
        {
            construct<half_storage>(val, tag);
        }

        basic_json(half_arg_t, uint16_t val, semantic_tag tag, const allocator_type&)
        {
            construct<half_storage>(val, tag);
        }

        basic_json(double val, semantic_tag tag)
        {
            construct<double_storage>(val, tag);
        }

        basic_json(double val, semantic_tag tag, const allocator_type&)
        {
            construct<double_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, 
                   typename std::enable_if<extension_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(uint64_t), int>::type = 0)
        {
            construct<uint64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, Allocator, 
                   typename std::enable_if<extension_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(uint64_t), int>::type = 0)
        {
            construct<uint64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag, Allocator alloc = Allocator(),
                   typename std::enable_if<extension_traits::is_unsigned_integer<IntegerType>::value && sizeof(uint64_t) < sizeof(IntegerType), int>::type = 0)
        {
            std::basic_string<CharT> s;
            jsoncons::detail::from_integer(val, s);
            if (s.length() <= short_string_storage::max_length)
            {
                construct<short_string_storage>(semantic_tag::bigint, s.data(), static_cast<uint8_t>(s.length()));
            }
            else
            {
                construct<long_string_storage>(semantic_tag::bigint, s.data(), s.length(), alloc);
            }
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag,
                   typename std::enable_if<extension_traits::is_signed_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),int>::type = 0)
        {
            construct<int64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, Allocator,
                   typename std::enable_if<extension_traits::is_signed_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),int>::type = 0)
        {
            construct<int64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag, Allocator alloc = Allocator(),
                   typename std::enable_if<extension_traits::is_signed_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),int>::type = 0)
        {
            std::basic_string<CharT> s;
            jsoncons::detail::from_integer(val, s);
            if (s.length() <= short_string_storage::max_length)
            {
                construct<short_string_storage>(semantic_tag::bigint, s.data(), static_cast<uint8_t>(s.length()));
            }
            else
            {
                construct<long_string_storage>(semantic_tag::bigint, s.data(), s.length(), alloc);
            }
        }

        basic_json(null_type, semantic_tag tag)
        {
            construct<null_storage>(tag);
        }

        basic_json(null_type, semantic_tag tag, const Allocator&)
        {
            construct<null_storage>(tag);
        }

        basic_json(bool val, semantic_tag tag)
        {
            construct<bool_storage>(val,tag);
        }

        basic_json(bool val, semantic_tag tag, const Allocator&)
        {
            construct<bool_storage>(val,tag);
        }

        basic_json(const string_view_type& sv, semantic_tag tag, const allocator_type& alloc = allocator_type())
            : basic_json(sv.data(), sv.length(), tag, alloc)
        {
        }

        template <typename Source>
        basic_json(byte_string_arg_t, const Source& source, 
                   semantic_tag tag = semantic_tag::none,
                   const Allocator& alloc = Allocator(),
                   typename std::enable_if<extension_traits::is_byte_sequence<Source>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());
            construct<byte_string_storage>(tag, bytes.data(), bytes.size(), 0, alloc);
        }

        template <typename Source>
        basic_json(byte_string_arg_t, const Source& source, 
                   uint64_t ext_tag,
                   const Allocator& alloc = Allocator(),
                   typename std::enable_if<extension_traits::is_byte_sequence<Source>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());
            construct<byte_string_storage>(semantic_tag::ext, bytes.data(), bytes.size(), ext_tag, alloc);
        }

        ~basic_json() noexcept
        {
             destroy();
        }

        template <typename T>
        basic_json& operator=(const T& val)
        {
            *this = json_type_traits<basic_json,T>::to_json(val);
            return *this;
        }

        basic_json& operator=(const char_type* s)
        {
            *this = basic_json(s, char_traits_type::length(s), semantic_tag::none);
            return *this;
        }

        basic_json& operator[](std::size_t i)
        {
            return at(i);
        }

        const basic_json& operator[](std::size_t i) const
        {
            return at(i);
        }

        proxy_type operator[](const string_view_type& name)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object: 
                create_object_implicitly();
                JSONCONS_FALLTHROUGH;
            case json_storage_kind::object:
                return proxy_type(*this, name);
                break;
            default:
                JSONCONS_THROW(not_an_object(name.data(),name.length()));
                break;
            }
        }

        const basic_json& operator[](const string_view_type& name) const
        {
            return at(name);
        }


        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
             const basic_json_encode_options<char_type>& options = basic_json_options<CharT>(),
             indenting indent = indenting::no_indent) const
        {
            std::error_code ec;
            dump(cont, options, indent, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont,
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump_pretty(cont, options, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>(),
            indenting indent = indenting::no_indent) const
        {
            std::error_code ec;
            dump(os, options, indent, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont,
            const basic_json_encode_options<char_type>& options, 
            std::error_code& ec) const
        {
            basic_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options);
            dump(encoder, ec);
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont, 
            std::error_code& ec) const
        {
            dump_pretty(cont, basic_json_encode_options<char_type>(), ec);
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump_pretty(os, options, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options, 
            std::error_code& ec) const
        {
            basic_json_encoder<char_type> encoder(os, options);
            dump(encoder, ec);
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            std::error_code& ec) const
        {
            dump_pretty(os, basic_json_encode_options<char_type>(), ec);
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, indenting indent) const
        {
            std::error_code ec;

            dump(cont, indent, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  indenting indent) const
        {
            std::error_code ec;

            dump(os, indent, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(basic_json_visitor<char_type>& visitor) const
        {
            std::error_code ec;
            dump(visitor, ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        // dump
        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
                  const basic_json_encode_options<char_type>& options, 
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options);
            dump(encoder, ec);
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, 
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont);
            dump(encoder, ec);
        }

        void dump(std::basic_ostream<char_type>& os, 
                  const basic_json_encode_options<char_type>& options,
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type> encoder(os, options);
            dump(encoder, ec);
        }

        void dump(std::basic_ostream<char_type>& os, 
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type> encoder(os);
            dump(encoder, ec);
        }

        // legacy
        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
                  const basic_json_encode_options<char_type>& options, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(cont, options, ec);
            }
            else
            {
                dump(cont, options, ec);
            }
        }

        template <typename CharContainer>
        typename std::enable_if<extension_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(cont, ec);
            }
            else
            {
                dump(cont, ec);
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  const basic_json_encode_options<char_type>& options, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(os, options, ec);
            }
            else
            {
                dump(os, options, ec);
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(os, ec);
            }
            else
            {
                dump(os, ec);
            }
        }
    // end legacy

        void dump(basic_json_visitor<char_type>& visitor, 
                  std::error_code& ec) const
        {
            dump_noflush(visitor, ec);
            if (ec)
            {
                return;
            }
            visitor.flush();
        }

        bool is_null() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::null:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_null();
                default:
                    return false;
            }
        }

        allocator_type get_allocator() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::long_str:
                {
                    return cast<long_string_storage>().get_allocator();
                }
                case json_storage_kind::byte_str:
                {
                    return cast<byte_string_storage>().get_allocator();
                }
                case json_storage_kind::array:
                {
                    return cast<array_storage>().get_allocator();
                }
                case json_storage_kind::object:
                {
                    return cast<object_storage>().get_allocator();
                }
                default:
                    return allocator_type();
            }
        }

        uint64_t ext_tag() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                {
                    return cast<byte_string_storage>().ext_tag();
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->ext_tag();
                default:
                    return 0;
            }
        }

        bool contains(const string_view_type& key) const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    return it != cast<object_storage>().value().end();
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->contains(key);
                default:
                    return false;
            }
        }

        std::size_t count(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        return 0;
                    }
                    std::size_t count = 0;
                    while (it != cast<object_storage>().value().end()&& it->key() == key)
                    {
                        ++count;
                        ++it;
                    }
                    return count;
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->count(key);
                default:
                    return 0;
            }
        }

        template <typename T,typename... Args>
        bool is(Args&&... args) const noexcept
        {
            return json_type_traits<basic_json,T>::is(*this,std::forward<Args>(args)...);
        }

        bool is_string() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_string();
                default:
                    return false;
            }
        }

        bool is_string_view() const noexcept
        {
            return is_string();
        }

        bool is_byte_string() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_byte_string();
                default:
                    return false;
            }
        }

        bool is_byte_string_view() const noexcept
        {
            return is_byte_string();
        }

        bool is_bignum() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return jsoncons::detail::is_base10(as_string_view().data(), as_string_view().length());
                case json_storage_kind::int64:
                case json_storage_kind::uint64:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_bignum();
                default:
                    return false;
            }
        }

        bool is_bool() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::boolean:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_bool();
                default:
                    return false;
            }
        }

        bool is_object() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                case json_storage_kind::object:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_object();
                default:
                    return false;
            }
        }

        bool is_array() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_array();
                default:
                    return false;
            }
        }

        bool is_int64() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return true;
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_int64();
                default:
                    return false;
            }
        }

        bool is_uint64() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::uint64:
                    return true;
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_uint64();
                default:
                    return false;
            }
        }

        bool is_half() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::half_float:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_half();
                default:
                    return false;
            }
        }

        bool is_double() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::float64:
                    return true;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_double();
                default:
                    return false;
            }
        }

        bool is_number() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                case json_storage_kind::uint64:
                case json_storage_kind::half_float:
                case json_storage_kind::float64:
                    return true;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return tag() == semantic_tag::bigint ||
                           tag() == semantic_tag::bigdec ||
                           tag() == semantic_tag::bigfloat;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->is_number();
                default:
                    return false;
            }
        }

        bool empty() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return cast<byte_string_storage>().length() == 0;
                    break;
                case json_storage_kind::short_str:
                    return cast<short_string_storage>().length() == 0;
                case json_storage_kind::long_str:
                    return cast<long_string_storage>().length() == 0;
                case json_storage_kind::array:
                    return cast<array_storage>().value().empty();
                case json_storage_kind::empty_object:
                    return true;
                case json_storage_kind::object:
                    return cast<object_storage>().value().empty();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->empty();
                default:
                    return false;
            }
        }

        std::size_t capacity() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().capacity();
                case json_storage_kind::object:
                    return cast<object_storage>().value().capacity();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->capacity();
                default:
                    return 0;
            }
        }

        template <typename U=Allocator>
        void create_object_implicitly()
        {
            create_object_implicitly(extension_traits::is_stateless<U>());
        }

        void create_object_implicitly(std::false_type)
        {
            JSONCONS_THROW(json_runtime_error<std::domain_error>("Cannot create object implicitly - allocator is stateful."));
        }

        void create_object_implicitly(std::true_type)
        {
            *this = basic_json(json_object_arg, tag());
        }

        void reserve(std::size_t n)
        {
            if (n > 0)
            {
                switch (storage_kind())
                {
                    case json_storage_kind::array:
                        cast<array_storage>().value().reserve(n);
                        break;
                    case json_storage_kind::empty_object:
                    {
                        create_object_implicitly();
                        cast<object_storage>().value().reserve(n);
                    }
                    break;
                    case json_storage_kind::object:
                    {
                        cast<object_storage>().value().reserve(n);
                    }
                        break;
                    default:
                        break;
                }
            }
        }

        void resize(std::size_t n)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().resize(n);
                    break;
                default:
                    break;
            }
        }

        template <typename T>
        void resize(std::size_t n, T val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().resize(n, val);
                    break;
                default:
                    break;
            }
        }

        template <typename T>
        typename std::enable_if<is_json_type_traits_specialized<basic_json,T>::value,T>::type
        as() const
        {
            T val = json_type_traits<basic_json,T>::as(*this);
            return val;
        }

        template <typename T>
        typename std::enable_if<(!extension_traits::is_string<T>::value && 
                                 extension_traits::is_back_insertable_byte_container<T>::value) ||
                                 extension_traits::is_basic_byte_string<T>::value,T>::type
        as(byte_string_arg_t, semantic_tag hint) const
        {
            std::error_code ec;
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    switch (tag())
                    {
                        case semantic_tag::base16:
                        case semantic_tag::base64:
                        case semantic_tag::base64url:
                        {
                            value_converter<jsoncons::basic_string_view<char_type>,T> converter;
                            T v = converter.convert(as_string_view(),tag(), ec);
                            if (ec)
                            {
                                JSONCONS_THROW(ser_error(ec));
                            }
                            return v;
                        }
                        default:
                        {
                            value_converter<jsoncons::basic_string_view<char_type>, T> converter;
                            T v = converter.convert(as_string_view(), hint, ec);
                            if (ec)
                            {
                                JSONCONS_THROW(ser_error(ec));
                            }
                            return T(v.begin(),v.end());
                        }
                        break;
                    }
                    break;
                }
                case json_storage_kind::byte_str:
                    return T(as_byte_string_view().begin(), as_byte_string_view().end());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template as<T>(byte_string_arg, hint);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a byte string"));
            }
        }

        bool as_bool() const 
        {
            switch (storage_kind())
            {
                case json_storage_kind::boolean:
                    return cast<bool_storage>().value();
                case json_storage_kind::int64:
                    return cast<int64_storage>().value() != 0;
                case json_storage_kind::uint64:
                    return cast<uint64_storage>().value() != 0;
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_bool();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a bool"));
            }
        }

        template <typename IntegerType>
        IntegerType as_integer() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    IntegerType val;
                    auto result = jsoncons::detail::to_integer(as_string_view().data(), as_string_view().length(), val);
                    if (!result)
                    {
                        JSONCONS_THROW(json_runtime_error<std::runtime_error>(result.error_code().message()));
                    }
                    return val;
                }
                case json_storage_kind::half_float:
                    return static_cast<IntegerType>(cast<half_storage>().value());
                case json_storage_kind::float64:
                    return static_cast<IntegerType>(cast<double_storage>().value());
                case json_storage_kind::int64:
                    return static_cast<IntegerType>(cast<int64_storage>().value());
                case json_storage_kind::uint64:
                    return static_cast<IntegerType>(cast<uint64_storage>().value());
                case json_storage_kind::boolean:
                    return static_cast<IntegerType>(cast<bool_storage>().value() ? 1 : 0);
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template as_integer<IntegerType>();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an integer"));
            }
        }

        template <typename IntegerType>
        typename std::enable_if<extension_traits::is_signed_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return (as_integer<int64_t>() >= (extension_traits::integer_limits<IntegerType>::lowest)()) && (as_integer<int64_t>() <= (extension_traits::integer_limits<IntegerType>::max)());
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((extension_traits::integer_limits<IntegerType>::max)());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        template <typename IntegerType>
        typename std::enable_if<extension_traits::is_signed_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    IntegerType val;
                    auto result = jsoncons::detail::to_integer(as_string_view().data(), as_string_view().length(), val);
                    return result ? true : false;
                }
                case json_storage_kind::int64:
                    return (as_integer<int64_t>() >= (extension_traits::integer_limits<IntegerType>::lowest)()) && (as_integer<int64_t>() <= (extension_traits::integer_limits<IntegerType>::max)());
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((extension_traits::integer_limits<IntegerType>::max)());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        template <typename IntegerType>
        typename std::enable_if<extension_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0 && static_cast<uint64_t>(as_integer<int64_t>()) <= (extension_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= (extension_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        template <typename IntegerType>
        typename std::enable_if<extension_traits::is_unsigned_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    IntegerType val;
                    auto result = jsoncons::detail::to_integer(as_string_view().data(), as_string_view().length(), val);
                    return result ? true : false;
                }
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0 && static_cast<uint64_t>(as_integer<int64_t>()) <= (extension_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= (extension_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        double as_double() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    jsoncons::detail::chars_to to_double;
                    // to_double() throws std::invalid_argument if conversion fails
                    return to_double(as_cstring(), as_string_view().length());
                }
                case json_storage_kind::half_float:
                    return binary::decode_half(cast<half_storage>().value());
                case json_storage_kind::float64:
                    return cast<double_storage>().value();
                case json_storage_kind::int64:
                    return static_cast<double>(cast<int64_storage>().value());
                case json_storage_kind::uint64:
                    return static_cast<double>(cast<uint64_storage>().value());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_double();
                default:
                    JSONCONS_THROW(json_runtime_error<std::invalid_argument>("Not a double"));
            }
        }

        template <typename SAllocator=std::allocator<char_type>>
        std::basic_string<char_type,char_traits_type,SAllocator> as_string() const 
        {
            return as_string(SAllocator());
        }

        template <typename SAllocator=std::allocator<char_type>>
        std::basic_string<char_type,char_traits_type,SAllocator> as_string(const SAllocator& alloc) const 
        {
            using string_type2 = std::basic_string<char_type,char_traits_type,SAllocator>;

            std::error_code ec;
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    return string_type2(as_string_view().data(),as_string_view().length(),alloc);
                }
                case json_storage_kind::byte_str:
                {
                    value_converter<byte_string_view,string_type2> converter;
                    auto s = converter.convert(as_byte_string_view(), tag(), ec);
                    if (ec)
                    {
                        JSONCONS_THROW(ser_error(ec));
                    }
                    return s;
                }
                case json_storage_kind::array:
                {
                    string_type2 s(alloc);
                    {
                        basic_compact_json_encoder<char_type,jsoncons::string_sink<string_type2>> encoder(s);
                        dump(encoder);
                    }
                    return s;
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_string(alloc);
                default:
                {
                    string_type2 s(alloc);
                    basic_compact_json_encoder<char_type,jsoncons::string_sink<string_type2>> encoder(s);
                    dump(encoder);
                    return s;
                }
            }
        }

        const char_type* as_cstring() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                    return cast<short_string_storage>().c_str();
                case json_storage_kind::long_str:
                    return cast<long_string_storage>().c_str();
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->as_cstring();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a cstring"));
            }
        }

        basic_json& at(const string_view_type& name)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    JSONCONS_THROW(key_not_found(name.data(),name.length()));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(name);
                    if (it == cast<object_storage>().value().end())
                    {
                        JSONCONS_THROW(key_not_found(name.data(),name.length()));
                    }
                    return it->value();
                }
                default:
                {
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
                }
            }
        }

        const basic_json& at(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    JSONCONS_THROW(key_not_found(key.data(),key.length()));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        JSONCONS_THROW(key_not_found(key.data(),key.length()));
                    }
                    return it->value();
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->at(key);
                default:
                {
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
                }
            }
        }

        basic_json& at(std::size_t i)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    if (i >= cast<array_storage>().value().size())
                    {
                        JSONCONS_THROW(json_runtime_error<std::out_of_range>("Invalid array subscript"));
                    }
                    return cast<array_storage>().value().operator[](i);
                case json_storage_kind::object:
                    return cast<object_storage>().value().at(i);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Index on non-array value not supported"));
            }
        }

        const basic_json& at(std::size_t i) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    if (i >= cast<array_storage>().value().size())
                    {
                        JSONCONS_THROW(json_runtime_error<std::out_of_range>("Invalid array subscript"));
                    }
                    return cast<array_storage>().value().operator[](i);
                case json_storage_kind::object:
                    return cast<object_storage>().value().at(i);
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->at(i);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Index on non-array value not supported"));
            }
        }

        object_iterator find(const string_view_type& name)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().find(name));
                default:
                {
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
                }
            }
        }

        const_object_iterator find(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return const_object_iterator(cast<object_storage>().value().find(key));
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->find(key);
                default:
                {
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
                }
            }
        }

        const basic_json& at_or_null(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::null:
                case json_storage_kind::empty_object:
                {
                    return null();
                }
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it != cast<object_storage>().value().end())
                    {
                        return it->value();
                    }
                    else
                    {
                        return null();
                    }
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->at_or_null(key);
                default:
                {
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
                }
            }
        }

        template <typename T,typename U>
        T get_value_or(const string_view_type& key, U&& default_value) const
        {
            static_assert(std::is_copy_constructible<T>::value,
                          "get_value_or: T must be copy constructible");
            static_assert(std::is_convertible<U&&, T>::value,
                          "get_value_or: U must be convertible to T");
            switch (storage_kind())
            {
                case json_storage_kind::null:
                case json_storage_kind::empty_object:
                    return static_cast<T>(std::forward<U>(default_value));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it != cast<object_storage>().value().end())
                    {
                        return it->value().template as<T>();
                    }
                    else
                    {
                        return static_cast<T>(std::forward<U>(default_value));
                    }
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->template get_value_or<T,U>(key,std::forward<U>(default_value));
                default:
                {
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
                }
            }
        }

        // Modifiers

        void shrink_to_fit()
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                cast<array_storage>().value().shrink_to_fit();
                break;
            case json_storage_kind::object:
                cast<object_storage>().value().shrink_to_fit();
                break;
            default:
                break;
            }
        }

        void clear()
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                cast<array_storage>().value().clear();
                break;
            case json_storage_kind::object:
                cast<object_storage>().value().clear();
                break;
            default:
                break;
            }
        }

        object_iterator erase(const_object_iterator pos)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                return object_range().end();
            case json_storage_kind::object:
                return object_iterator(cast<object_storage>().value().erase(pos));
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
                break;
            }
        }

        object_iterator erase(const_object_iterator first, const_object_iterator last)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                return object_range().end();
            case json_storage_kind::object:
                return object_iterator(cast<object_storage>().value().erase(first, last));
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
                break;
            }
        }

        array_iterator erase(const_array_iterator pos)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().erase(pos);
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

        array_iterator erase(const_array_iterator first, const_array_iterator last)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().erase(first, last);
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
                break;
            }
        }

        // Removes all elements from an array value whose index is between from_index, inclusive, and to_index, exclusive.

        void erase(const string_view_type& name)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                break;
            case json_storage_kind::object:
                cast<object_storage>().value().erase(name);
                break;
            default:
                JSONCONS_THROW(not_an_object(name.data(),name.length()));
                break;
            }
        }

        template <typename T>
        std::pair<object_iterator,bool> insert_or_assign(const string_view_type& name, T&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                {
                    create_object_implicitly();
                    auto result = cast<object_storage>().value().insert_or_assign(name, std::forward<T>(val));
                    return std::make_pair(object_iterator(result.first), result.second);
                }
                case json_storage_kind::object:
                {
                    auto result = cast<object_storage>().value().insert_or_assign(name, std::forward<T>(val));
                    return std::make_pair(object_iterator(result.first), result.second);
                }
                default:
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
                }
        }

        template <typename ... Args>
        std::pair<object_iterator,bool> try_emplace(const string_view_type& name, Args&&... args)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                {
                    create_object_implicitly();
                    auto result = cast<object_storage>().value().try_emplace(name, std::forward<Args>(args)...);
                    return std::make_pair(object_iterator(result.first),result.second);
                }
                case json_storage_kind::object:
                {
                    auto result = cast<object_storage>().value().try_emplace(name, std::forward<Args>(args)...);
                    return std::make_pair(object_iterator(result.first),result.second);
                }
                default:
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
                }
        }

        // merge

        void merge(const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(source.cast<object_storage>().value());
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                        }
                    break;
               default:
                   JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
           }
        }

        void merge(basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(std::move(source.cast<object_storage>().value()));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                    break;
               default:
                   JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
           }
        }

        void merge(object_iterator hint, const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(hint, source.cast<object_storage>().value());
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                     break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge(object_iterator hint, basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        // merge_or_update

        void merge_or_update(const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(source.cast<object_storage>().value());
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                    case json_storage_kind::empty_object:
                        create_object_implicitly();
                        cast<object_storage>().value().merge_or_update(std::move(source.cast<object_storage>().value()));
                        break;
                    case json_storage_kind::object:
                        cast<object_storage>().value().merge_or_update(std::move(source.cast<object_storage>().value()));
                        break;
                    default:
                        JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(object_iterator hint, const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(hint, source.cast<object_storage>().value());
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(object_iterator hint, basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        template <typename T>
        object_iterator insert_or_assign(object_iterator hint, const string_view_type& name, T&& val)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                create_object_implicitly();
                return object_iterator(cast<object_storage>().value().insert_or_assign(hint, name, std::forward<T>(val)));
            case json_storage_kind::object:
                return object_iterator(cast<object_storage>().value().insert_or_assign(hint, name, std::forward<T>(val)));
            default:
                JSONCONS_THROW(not_an_object(name.data(),name.length()));
            }
        }

        template <typename ... Args>
        object_iterator try_emplace(object_iterator hint, const string_view_type& name, Args&&... args)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                create_object_implicitly();
                return object_iterator(cast<object_storage>().value().try_emplace(hint, name, std::forward<Args>(args)...));
            case json_storage_kind::object:
                return object_iterator(cast<object_storage>().value().try_emplace(hint, name, std::forward<Args>(args)...));
            default:
                JSONCONS_THROW(not_an_object(name.data(),name.length()));
            }
        }

        template <typename T>
        array_iterator insert(const_array_iterator pos, T&& val)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().insert(pos, std::forward<T>(val));
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename InputIt>
        array_iterator insert(const_array_iterator pos, InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().insert(pos, first, last);
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                create_object_implicitly();
                cast<object_storage>().value().insert(first, last);
                break;
            case json_storage_kind::object:
                cast<object_storage>().value().insert(first, last);
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an object"));
            }
        }

        template <typename InputIt>
        void insert(sorted_unique_range_tag tag, InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                create_object_implicitly();
                cast<object_storage>().value().insert(tag, first, last);
                break;
            case json_storage_kind::object:
                cast<object_storage>().value().insert(tag, first, last);
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an object"));
            }
        }

        template <typename... Args> 
        array_iterator emplace(const_array_iterator pos, Args&&... args)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().emplace(pos, std::forward<Args>(args)...);
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename... Args> 
        basic_json& emplace_back(Args&&... args)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                return cast<array_storage>().value().emplace_back(std::forward<Args>(args)...);
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        friend void swap(basic_json& a, basic_json& b) noexcept
        {
            a.swap(b);
        }

        template <typename T>
        void push_back(T&& val)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                cast<array_storage>().value().push_back(std::forward<T>(val));
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        void push_back(basic_json&& val)
        {
            switch (storage_kind())
            {
            case json_storage_kind::array:
                cast<array_storage>().value().push_back(std::move(val));
                break;
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        std::basic_string<char_type> to_string() const noexcept
        {
            using string_type2 = std::basic_string<char_type>;
            string_type2 s;
            basic_compact_json_encoder<char_type, jsoncons::string_sink<string_type2>> encoder(s);
            dump(encoder);
            return s;
        }

        template <typename InputIterator>
        basic_json(InputIterator first, InputIterator last, const Allocator& alloc = Allocator())
            : basic_json(json_array_arg,first,last,alloc)
        {
        }

        object_range_type object_range()
        {
            switch (storage_kind())
            {
            case json_storage_kind::empty_object:
                return object_range_type(object_iterator(), object_iterator());
            case json_storage_kind::object:
                return object_range_type(object_iterator(cast<object_storage>().value().begin()),
                                              object_iterator(cast<object_storage>().value().end()));
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        const_object_range_type object_range() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return const_object_range_type(const_object_iterator(), const_object_iterator());
                case json_storage_kind::object:
                    return const_object_range_type(const_object_iterator(cast<object_storage>().value().begin()),
                                                        const_object_iterator(cast<object_storage>().value().end()));
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->object_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        array_range_type array_range()
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return array_range_type(cast<array_storage>().value().begin(),
                        cast<array_storage>().value().end());
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

        const_array_range_type array_range() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return const_array_range_type(cast<array_storage>().value().begin(),
                        cast<array_storage>().value().end());
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->array_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

    private:

        void dump_noflush(basic_json_visitor<char_type>& visitor, std::error_code& ec) const
        {
            const ser_context context{};
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    visitor.string_value(as_string_view(), tag(), context, ec);
                    break;
                case json_storage_kind::byte_str:
                    if (tag() == semantic_tag::ext)
                    {
                        visitor.byte_string_value(as_byte_string_view(), ext_tag(), context, ec);
                    }
                    else
                    {
                        visitor.byte_string_value(as_byte_string_view(), tag(), context, ec);
                    }
                    break;
                case json_storage_kind::half_float:
                    visitor.half_value(cast<half_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::float64:
                    visitor.double_value(cast<double_storage>().value(), 
                                         tag(), context, ec);
                    break;
                case json_storage_kind::int64:
                    visitor.int64_value(cast<int64_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::uint64:
                    visitor.uint64_value(cast<uint64_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::boolean:
                    visitor.bool_value(cast<bool_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::null:
                    visitor.null_value(tag(), context, ec);
                    break;
                case json_storage_kind::empty_object:
                    visitor.begin_object(0, tag(), context, ec);
                    visitor.end_object(context, ec);
                    break;
                case json_storage_kind::object:
                {
                    bool more = visitor.begin_object(size(), tag(), context, ec);
                    const object& o = cast<object_storage>().value();
                    for (auto it = o.begin(); more && it != o.end(); ++it)
                    {
                        visitor.key(string_view_type((it->key()).data(),it->key().length()), context, ec);
                        it->value().dump_noflush(visitor, ec);
                    }
                    if (more)
                    {
                        visitor.end_object(context, ec);
                    }
                    break;
                }
                case json_storage_kind::array:
                {
                    bool more = visitor.begin_array(size(), tag(), context, ec);
                    const array& o = cast<array_storage>().value();
                    for (const_array_iterator it = o.begin(); more && it != o.end(); ++it)
                    {
                        it->dump_noflush(visitor, ec);
                    }
                    if (more)
                    {
                        visitor.end_array(context, ec);
                    }
                    break;
                }
                case json_storage_kind::const_json_pointer:
                    return cast<json_const_pointer_storage>().value()->dump_noflush(visitor, ec);
                default:
                    break;
            }
        }

        friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const basic_json& o)
        {
            o.dump(os);
            return os;
        }

        friend std::basic_istream<char_type>& operator>>(std::basic_istream<char_type>& is, basic_json& o)
        {
            json_decoder<basic_json> visitor;
            basic_json_reader<char_type,stream_source<char_type>> reader(is, visitor);
            reader.read_next();
            reader.check_done();
            if (!visitor.is_valid())
            {
                JSONCONS_THROW(json_runtime_error<std::runtime_error>("Failed to parse json stream"));
            }
            o = visitor.get_result();
            return is;
        }

        friend basic_json deep_copy(const basic_json& other)
        {
            switch (other.storage_kind())
            {
                case json_storage_kind::array:
                {
                    basic_json j(json_array_arg, other.tag(), other.get_allocator());
                    j.reserve(other.size());

                    for (const auto& item : other.array_range())
                    {
                        j.push_back(deep_copy(item));
                    }
                    return j;
                }
                case json_storage_kind::object:
                {
                    basic_json j(json_object_arg, other.tag(), other.get_allocator());
                    j.reserve(other.size());

                    for (const auto& item : other.object_range())
                    {
                        j.try_emplace(item.key(), deep_copy(item.value()));
                    }
                    return j;
                }
                case json_storage_kind::const_json_pointer:
                    return deep_copy(*(other.cast<json_const_pointer_storage>().value()));
                default:
                    return other;
            }
        }
    };

    // operator==

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator==(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator==(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator==(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator==(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator==(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) == 0;
    }

    // operator!=

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator!=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator!=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator!=(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator!=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator!=(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) != 0;
    }

    // operator<

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator<(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator<(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator<(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) > 0;
    }

    // operator<=

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator<=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator<=(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator<=(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) >= 0;
    }

    // operator>

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator>(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator>(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator>(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) < 0;
    }

    // operator>=

    template <typename Json>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value,bool>::type
    operator>=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator>=(const Json& lhs, const T& rhs) 
    {
        return lhs.evaluate().compare(rhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<extension_traits::is_basic_json<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<is_proxy<Json>::value && !is_proxy<T>::value && !extension_traits::is_basic_json<T>::value && std::is_convertible<T,typename Json::proxied_type>::value,bool>::type
    operator>=(const T& lhs, const Json& rhs) 
    {
        return rhs.evaluate().compare(lhs) <= 0;
    }

    // swap

    template <typename Json>
    void swap(typename Json::key_value_type& a,typename Json::key_value_type& b) noexcept
    {
        a.swap(b);
    }

    using json = basic_json<char,sorted_policy,std::allocator<char>>;
    using wjson = basic_json<wchar_t,sorted_policy,std::allocator<char>>;
    using ojson = basic_json<char, order_preserving_policy, std::allocator<char>>;
    using wojson = basic_json<wchar_t, order_preserving_policy, std::allocator<char>>;

    inline namespace literals {

    inline 
    jsoncons::json operator "" _json(const char* s, std::size_t n)
    {
        return jsoncons::json::parse(jsoncons::json::string_view_type(s, n));
    }

    inline 
    jsoncons::wjson operator "" _json(const wchar_t* s, std::size_t n)
    {
        return jsoncons::wjson::parse(jsoncons::wjson::string_view_type(s, n));
    }

    inline
    jsoncons::ojson operator "" _ojson(const char* s, std::size_t n)
    {
        return jsoncons::ojson::parse(jsoncons::ojson::string_view_type(s, n));
    }

    inline
    jsoncons::wojson operator "" _ojson(const wchar_t* s, std::size_t n)
    {
        return jsoncons::wojson::parse(jsoncons::wojson::string_view_type(s, n));
    }

    } // inline namespace literals

    #if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
    namespace pmr {
        template< typename CharT,typename Policy>
        using basic_json = jsoncons::basic_json<CharT, Policy, std::pmr::polymorphic_allocator<char>>;
        using json = basic_json<char,sorted_policy>;
        using wjson = basic_json<wchar_t,sorted_policy>;
        using ojson = basic_json<char, order_preserving_policy>;
        using wojson = basic_json<wchar_t, order_preserving_policy>;
    } // namespace pmr
    #endif

} // namespace jsoncons

#endif
