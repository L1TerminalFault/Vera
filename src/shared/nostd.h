#pragma once

#include <cstring>
#include <istream>

// ============================================================================
// LIGHTWEIGHT PRIMITIVES
// ============================================================================

namespace nostd {

template <typename T>
class Vector {
    T* m_data = nullptr;
    size_t m_capacity = 0;
    size_t m_size = 0;

   public:
    Vector() = default;

    explicit Vector(size_t initialCapacity) { reserve(initialCapacity); }

    ~Vector() {
        clear();
        ::operator delete(m_data);
    }

    Vector(const Vector& other) {
        reserve(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i) {
            push_back(other.m_data[i]);
        }
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            clear();
            reserve(other.m_size);
            for (size_t i = 0; i < other.m_size; ++i) {
                push_back(other.m_data[i]);
            }
        }
        return *this;
    }

    // 1. Initializer list constructor (allows: Vector<T> v = { ... })
    Vector(std::initializer_list<T> init) {
        reserve(init.size());
        for (const auto& item : init) {
            push_back(item);
        }
    }

    // 2. Initializer list assignment operator (allows: v = { ... })
    Vector& operator=(std::initializer_list<T> init) {
        clear();
        reserve(init.size());
        for (const auto& item : init) {
            push_back(item);
        }
        return *this;
    }

    Vector(Vector&& other) noexcept
        : m_data(other.m_data),
          m_capacity(other.m_capacity),
          m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clear();
            ::operator delete(m_data);
            m_data = other.m_data;
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_capacity = 0;
            other.m_size = 0;
        }
        return *this;
    }

    /// NOLINTBEGIN(readability-identifier-naming)
    void push_back(const T& val) {
        if (m_size >= m_capacity) {
            T tmp = val;  // Copy to survive reallocation
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
            new (m_data + m_size) T(static_cast<T&&>(tmp));
        } else {
            new (m_data + m_size) T(val);
        }
        ++m_size;
    }

    void push_back(T&& val) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        new (m_data + m_size) T(static_cast<T&&>(val));
        ++m_size;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 4 : m_capacity * 2);
        }
        new (m_data + m_size) T(static_cast<Args&&>(args)...);
        return m_data[m_size++];
    }

    /// Access the first element (undefined behavior if empty)
    T& front() noexcept { return m_data[0]; }

    /// Access the first element as const (undefined behavior if empty)
    const T& front() const noexcept { return m_data[0]; }

    /// Access the last element (undefined behavior if empty)
    T& back() noexcept { return m_data[m_size - 1]; }

    /// Access the last element as const (undefined behavior if empty)
    const T& back() const noexcept { return m_data[m_size - 1]; }

    void reserve(size_t newCap) {
        if (newCap <= m_capacity) return;
        T* newData = static_cast<T*>(::operator new(newCap * sizeof(T)));
        for (size_t i = 0; i < m_size; ++i) {
            new (newData + i) T(static_cast<T&&>(m_data[i]));
            m_data[i].~T();
        }
        ::operator delete(m_data);
        m_data = newData;
        m_capacity = newCap;
    }

    template <typename Predicate>
    void erase_if(Predicate pred) {
        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < m_size; ++read_idx) {
            if (pred(m_data[read_idx])) {
                m_data[read_idx].~T();
            } else {
                if (write_idx != read_idx) {
                    new (m_data + write_idx)
                        T(static_cast<T&&>(m_data[read_idx]));
                    m_data[read_idx].~T();
                }
                ++write_idx;
            }
        }
        m_size = write_idx;
    }

    /// Resizes the container to contain new_size elements (default-constructed)
    void resize(size_t new_size) {
        if (new_size < m_size) {
            // Shrink: Explicitly call destructors on removed elements
            for (size_t i = new_size; i < m_size; ++i) {
                m_data[i].~T();
            }
        } else if (new_size > m_size) {
            // Grow: Expand capacity if necessary
            if (new_size > m_capacity) {
                reserve(new_size);
            }
            // Placement-new default construct new elements
            for (size_t i = m_size; i < new_size; ++i) {
                new (&m_data[i]) T();
            }
        }
        m_size = new_size;
    }

    /// Resizes the container to contain new_size elements filled with `value`
    void resize(size_t new_size, const T& value) {
        if (new_size < m_size) {
            for (size_t i = new_size; i < m_size; ++i) m_data[i].~T();
        } else if (new_size > m_size) {
            T tmp = value;  // Copy to survive potential reserve() realloc
            if (new_size > m_capacity) reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i) new (m_data + i) T(tmp);
        }
        m_size = new_size;
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            m_data[m_size].~T();
        }
    }
    /// NOLINTEND(readability-identifier-naming)

    void clear() {
        for (size_t i = 0; i < m_size; ++i) {
            m_data[i].~T();
        }
        m_size = 0;
    }

    size_t size() const noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0; }

    T& operator[](size_t idx) noexcept { return m_data[idx]; }
    const T& operator[](size_t idx) const noexcept { return m_data[idx]; }

    T* data() noexcept { return m_data; }
    const T* data() const noexcept { return m_data; }

    T* begin() noexcept { return m_data; }
    const T* begin() const noexcept { return m_data; }
    T* end() noexcept { return m_data + m_size; }
    const T* end() const noexcept { return m_data + m_size; }
};

template <typename Err>
struct Unexpected {
    Err val;
};

template <typename Type, typename Err>
class Expected {
    bool m_hasVal;
    union {
        Type val;
        Err err;
    };

   public:
    // Exact value constructors
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(const Type& v) : m_hasVal(true), val(v) {}

    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(Type&& v) : m_hasVal(true), val(std::move(v)) {}

    // Converting Value Constructor (e.g., unique_ptr<Derived> ->
    // unique_ptr<Base>)
    template <typename U,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<U>, Expected> &&
                  !std::is_same_v<std::decay_t<U>, Unexpected<Err>> &&
                  std::is_constructible_v<Type, U&&>>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(U&& v) : m_hasVal(true), val(std::forward<U>(v)) {}

    // Unexpected constructors
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(const Unexpected<Err>& u) : m_hasVal(false), err(u.val) {}

    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(Unexpected<Err>&& u) : m_hasVal(false), err(std::move(u.val)) {}

    // Converting Unexpected Constructor
    template <typename E,
              typename = std::enable_if_t<std::is_constructible_v<Err, E&&>>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(const Unexpected<E>& u) : m_hasVal(false), err(u.val) {}

    template <typename E,
              typename = std::enable_if_t<std::is_constructible_v<Err, E&&>>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Expected(Unexpected<E>&& u) : m_hasVal(false), err(std::move(u.val)) {}

    // Move Constructor & Assignment
    Expected(Expected&& other) noexcept(
        std::is_nothrow_move_constructible_v<Type> &&
        std::is_nothrow_move_constructible_v<Err>)
        : m_hasVal(other.m_hasVal) {
        if (m_hasVal) {
            new (&val) Type(std::move(other.val));
        } else {
            new (&err) Err(std::move(other.err));
        }
    }

    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            this->~Expected();
            m_hasVal = other.m_hasVal;
            if (m_hasVal) {
                new (&val) Type(std::move(other.val));
            } else {
                new (&err) Err(std::move(other.err));
            }
        }
        return *this;
    }

    // Copy Constructor & Assignment
    Expected(const Expected& other) : m_hasVal(other.m_hasVal) {
        if (m_hasVal) {
            new (&val) Type(other.val);
        } else {
            new (&err) Err(other.err);
        }
    }

    Expected& operator=(const Expected& other) {
        if (this != &other) {
            this->~Expected();
            m_hasVal = other.m_hasVal;
            if (m_hasVal) {
                new (&val) Type(other.val);
            } else {
                new (&err) Err(other.err);
            }
        }
        return *this;
    }

    ~Expected() {
        if (m_hasVal) {
            val.~Type();
        } else {
            err.~Err();
        }
    }

    /// NOLINTBEGIN(readability-identifier-naming)
    bool has_value() const { return m_hasVal; }
    /// NOLINTEND(readability-identifier-naming)
    explicit operator bool() const { return m_hasVal; }

    Type& value() { return val; }
    const Type& value() const { return val; }
    Type& operator*() { return val; }
    const Type& operator*() const { return val; }
    Type* operator->() { return &val; }
    const Type* operator->() const { return &val; }

    Err& error() { return err; }
    const Err& error() const { return err; }
};

// Lightweight Optional Replacement
template <typename T>
struct Optional {
    bool m_hasVal = false;
    T m_val{};

    constexpr Optional() = default;
    constexpr explicit Optional(const T& v) : m_hasVal(true), m_val(v) {}
    constexpr explicit Optional(T&& v) : m_hasVal(true), m_val(std::move(v)) {}

    /// NOLINTBEGIN(readability-identifier-naming)
    constexpr bool has_value() const noexcept { return m_hasVal; }
    /// NOLINTEND(readability-identifier-naming)
    constexpr explicit operator bool() const noexcept { return m_hasVal; }
    constexpr T& value() noexcept { return m_val; }
    constexpr const T& value() const noexcept { return m_val; }
    constexpr T& operator*() noexcept { return m_val; }
    constexpr const T& operator*() const noexcept { return m_val; }
    constexpr T* operator->() noexcept { return &m_val; }
    constexpr const T* operator->() const noexcept { return &m_val; }
    /// NOLINTBEGIN(readability-identifier-naming)
    constexpr T value_or(T def) const noexcept {
        /// NOLINTEND(readability-identifier-naming)
        return m_hasVal ? m_val : def;
    }
};

// Lightweight nostd::Function Replacement (48-byte inline buffer, zero heap
// allocations)

template <typename Signature>
class Function;

template <typename Ret, typename... Args>
class Function<Ret(Args...)> {
   private:
    struct Concept {
        virtual ~Concept() = default;
        virtual Ret invoke(Args... args) = 0;
        virtual Concept* clone() const = 0;
    };

    template <typename F>
    struct Model final : Concept {
        F m_func;

        template <typename Functor>
        explicit Model(Functor&& f) : m_func(std::forward<Functor>(f)) {}

        Ret invoke(Args... args) override {
            if constexpr (std::is_void_v<Ret>) {
                m_func(std::forward<Args>(args)...);
            } else {
                return m_func(std::forward<Args>(args)...);
            }
        }

        Concept* clone() const override { return new Model<F>(m_func); }
    };

    Concept* m_concept = nullptr;

   public:
    Function() noexcept = default;
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Function(std::nullptr_t) noexcept {}

    // Copy Constructor (Deep-copy via clone)
    Function(const Function& other) {
        if (other.m_concept) {
            m_concept = other.m_concept->clone();
        }
    }

    // Move Constructor
    Function(Function&& other) noexcept : m_concept(other.m_concept) {
        other.m_concept = nullptr;
    }

    // Generic Callable / Functor Constructor
    template <typename F, typename = std::enable_if_t<
                              !std::is_same_v<std::decay_t<F>, Function> &&
                              !std::is_same_v<std::decay_t<F>, std::nullptr_t>>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    Function(F&& f) {
        using DecayedF = std::decay_t<F>;
        m_concept = new Model<DecayedF>(std::forward<F>(f));
    }

    ~Function() { delete m_concept; }

    // Copy Assignment Operator
    Function& operator=(const Function& other) {
        if (this != &other) {
            delete m_concept;
            m_concept = other.m_concept ? other.m_concept->clone() : nullptr;
        }
        return *this;
    }

    // Move Assignment Operator
    Function& operator=(Function&& other) noexcept {
        if (this != &other) {
            delete m_concept;
            m_concept = other.m_concept;
            other.m_concept = nullptr;
        }
        return *this;
    }

    // Nullptr Assignment
    Function& operator=(std::nullptr_t) noexcept {
        delete m_concept;
        m_concept = nullptr;
        return *this;
    }

    // Call Operator
    Ret operator()(Args... args) const {
        if (m_concept) {
            return m_concept->invoke(std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    }

    explicit operator bool() const noexcept { return m_concept != nullptr; }
};

// =============================================================================
// nostd::Function<Signature>
// =============================================================================
// template <typename Signature>
// class Function;
//
// template <typename R, typename... Args>
// class Function<R(Args...)> {
//    private:
//     struct ICallable {
//         virtual ~ICallable() = default;
//         virtual R invoke(Args... args) = 0;
//         virtual ICallable* clone() const = 0;
//     };
//
//     template <typename F>
//     struct CallableModel : ICallable {
//         F functor;
//
//         CallableModel(const F& f) : functor(f) {}
//         CallableModel(F&& f) : functor(std::move(f)) {}
//
//         R invoke(Args... args) override {
//             return functor(std::forward<Args>(args)...);
//         }
//
//         ICallable* clone() const override {
//             return new CallableModel<F>(functor);
//         }
//     };
//
//    public:
//     Function() noexcept : m_callable(nullptr) {}
//     Function(std::nullptr_t) noexcept : m_callable(nullptr) {}
//
//     template <typename F>
//     Function(F functor)
//         : m_callable(new CallableModel<F>(std::move(functor))) {}
//
//     Function(const Function& other)
//         : m_callable(other.m_callable ? other.m_callable->clone() : nullptr)
//         {}
//
//     Function(Function&& other) noexcept : m_callable(other.m_callable) {
//         other.m_callable = nullptr;
//     }
//
//     ~Function() { delete m_callable; }
//
//     Function& operator=(std::nullptr_t) noexcept {
//         delete m_callable;
//         m_callable = nullptr;
//         return *this;
//     }
//
//     Function& operator=(const Function& other) {
//         if (this != &other) {
//             delete m_callable;
//             m_callable = other.m_callable ? other.m_callable->clone() :
//             nullptr;
//         }
//         return *this;
//     }
//
//     Function& operator=(Function&& other) noexcept {
//         if (this != &other) {
//             delete m_callable;
//             m_callable = other.m_callable;
//             other.m_callable = nullptr;
//         }
//         return *this;
//     }
//
//     R operator()(Args... args) const {
//         return m_callable->invoke(std::forward<Args>(args)...);
//     }
//
//     explicit operator bool() const noexcept { return m_callable != nullptr; }
//
//    private:
//     ICallable* m_callable{nullptr};
// };

// =============================================================================
// nostd::UniquePtr
// =============================================================================
template <typename T>
class UniquePtr {
   public:
    constexpr UniquePtr() noexcept : m_ptr(nullptr) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr UniquePtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}
    explicit UniquePtr(T* ptr) noexcept : m_ptr(ptr) {}

    ~UniquePtr() { reset(); }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return m_ptr; }
    T* operator->() const noexcept { return m_ptr; }
    T& operator*() const noexcept { return *m_ptr; }
    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    T* release() noexcept {
        T* temp = m_ptr;
        m_ptr = nullptr;
        return temp;
    }

    void reset(T* ptr = nullptr) noexcept {
        T* old = m_ptr;
        m_ptr = ptr;
        if (old) {
            delete old;
        }
    }

   private:
    T* m_ptr{nullptr};
};

template <typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

// =============================================================================
// nostd::Optional
// =============================================================================
struct NullOpt {};
/// NOLINTBEGIN(readability-identifier-naming)
inline constexpr NullOpt nullopt{};
/// NOLINTEND(readability-identifier-naming)

// template <typename T>
// class Optional {
//    public:
//     Optional() noexcept : m_hasValue(false) {}
//     Optional(NullOpt) noexcept : m_hasValue(false) {}
//
//     Optional(const T& value) : m_hasValue(true) { new (storage()) T(value); }
//
//     Optional(T&& value) : m_hasValue(true) {
//         new (storage()) T(std::move(value));
//     }
//
//     Optional(const Optional& other) : m_hasValue(other.m_hasValue) {
//         if (m_hasValue) {
//             new (storage()) T(*other.asT());
//         }
//     }
//
//     Optional(Optional&& other) noexcept : m_hasValue(other.m_hasValue) {
//         if (m_hasValue) {
//             new (storage()) T(std::move(*other.asT()));
//             other.reset();
//         }
//     }
//
//     ~Optional() { reset(); }
//
//     Optional& operator=(NullOpt) noexcept {
//         reset();
//         return *this;
//     }
//
//     Optional& operator=(const Optional& other) {
//         if (this != &other) {
//             if (other.m_hasValue) {
//                 *this = *other.asT();
//             } else {
//                 reset();
//             }
//         }
//         return *this;
//     }
//
//     Optional& operator=(Optional&& other) noexcept {
//         if (this != &other) {
//             if (other.m_hasValue) {
//                 *this = std::move(*other.asT());
//                 other.reset();
//             } else {
//                 reset();
//             }
//         }
//         return *this;
//     }
//
//     bool hasValue() const noexcept { return m_hasValue; }
//     explicit operator bool() const noexcept { return m_hasValue; }
//
//     T& value() { return *asT(); }
//
//     const T& value() const { return *asT(); }
//
//     T valueOr(T&& defaultValue) const {
//         return m_hasValue ? *asT() : std::forward<T>(defaultValue);
//     }
//
//     T* operator->() { return asT(); }
//     const T* operator->() const { return asT(); }
//     T& operator*() { return *asT(); }
//     const T& operator*() const { return *asT(); }
//
//     void reset() noexcept {
//         if (m_hasValue) {
//             asT()->~T();
//             m_hasValue = false;
//         }
//     }
//
//    private:
//     void* storage() noexcept { return static_cast<void*>(m_buff); }
//     const void* storage() const noexcept {
//         return static_cast<const void*>(m_buff);
//     }
//     T* asT() noexcept { return reinterpret_cast<T*>(m_buff); }
//     const T* asT() const noexcept { return reinterpret_cast<const
//     T*>(m_buff); }
//
//     alignas(T) char m_buff[sizeof(T)];
//     bool m_hasValue{false};
// };

// =============================================================================
// nonostd::String
// =============================================================================
/// NOLINTBEGIN(readability-identifier-naming)
class String {
   public:
    static constexpr size_t npos = static_cast<size_t>(-1);
    // Iterators for range-based for loops (for (char& c : str))
    char* begin() noexcept { return m_data; }
    char* end() noexcept { return m_data + m_size; }
    const char* begin() const noexcept { return m_data; }
    const char* end() const noexcept { return m_data + m_size; }

    // Size / Length aliases
    size_t size() const noexcept { return m_size; }
    size_t length() const noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0; }

    String substr(size_t pos = 0, size_t count = npos) const {
        if (pos >= m_size) return String();
        size_t actualCount =
            (count == npos || pos + count > m_size) ? (m_size - pos) : count;
        return String(m_data + pos, actualCount);
    }

    // Resets string to empty state
    void clear() noexcept {
        m_size = 0;
        if (m_data) {
            m_data[0] = '\0';
        }
    }

    // Appends a single character
    void push_back(char ch) {
        if (m_size + 1 >= m_capacity) {
            reserve(m_capacity == 0 ? 16 : m_capacity * 2);
        }
        m_data[m_size++] = ch;
        m_data[m_size] = '\0';
    }

    // Character append operator
    String& operator+=(char ch) {
        push_back(ch);
        return *this;
    }

    // Truncates string starting at pos
    void erase(size_t pos) noexcept {
        if (pos < m_size) {
            m_size = pos;
            m_data[m_size] = '\0';
        }
    }

    // Finds last character not present in set
    size_t find_last_not_of(const char* set) const noexcept {
        if (m_size == 0 || !set) return npos;
        for (size_t i = m_size; i > 0; --i) {
            if (std::strchr(set, m_data[i - 1]) == nullptr) {
                return i - 1;
            }
        }
        return npos;
    }

    // Access last character
    char& back() noexcept { return m_data[m_size - 1]; }

    const char& back() const noexcept { return m_data[m_size - 1]; }

    // Remove last character
    void pop_back() noexcept {
        if (m_size > 0) {
            --m_size;
            m_data[m_size] = '\0';
        }
    }

    String() noexcept {
        m_data = new char[1];
        m_data[0] = '\0';
        m_size = 0;
        m_capacity = 0;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    String(const char* str) {
        if (!str) {
            m_size = 0;
            m_capacity = 0;
            m_data = new char[1]{'\0'};
            return;
        }
        m_size = std::strlen(str);
        m_capacity = m_size;
        m_data = new char[m_size + 1];
        std::memcpy(m_data, str, m_size + 1);
    }

    String(const char* str, size_t count) {
        m_size = count;
        m_capacity = count;
        m_data = new char[count + 1];
        if (str && count > 0) {
            std::memcpy(m_data, str, count);
        }
        m_data[count] = '\0';
    }

    String(const String& other)
        : m_size(other.m_size), m_capacity(other.m_size) {
        m_data = new char[m_size + 1];
        std::memcpy(m_data, other.m_data, m_size + 1);
    }

    String(String&& other) noexcept
        : m_data(other.m_data),
          m_size(other.m_size),
          m_capacity(other.m_capacity) {
        other.m_data = new char[1]{'\0'};
        other.m_size = 0;
        other.m_capacity = 0;
    }

    ~String() { delete[] m_data; }

    String& operator=(const String& other) {
        if (this != &other) {
            char* newData = new char[other.m_size + 1];
            std::memcpy(newData, other.m_data, other.m_size + 1);
            delete[] m_data;
            m_data = newData;
            m_size = other.m_size;
            m_capacity = other.m_size;
        }
        return *this;
    }

    String& operator=(String&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;

            other.m_data = new char[1]{'\0'};
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    const char* c_str() const noexcept { return m_data; }
    char* data() noexcept { return m_data; }

    void reserve(size_t newCap) {
        if (newCap <= m_capacity) return;
        char* newData = new char[newCap + 1];
        std::memcpy(newData, m_data, m_size + 1);
        delete[] m_data;
        m_data = newData;
        m_capacity = newCap;
    }

    String& operator+=(const String& other) {
        append(other.c_str(), other.size());
        return *this;
    }

    String& operator+=(const char* str) {
        if (str) append(str, std::strlen(str));
        return *this;
    }

    void append(const char* str, size_t len) {
        if (!str || len == 0) return;
        if (m_size + len > m_capacity) {
            reserve((m_size + len) * 2);
        }
        std::memcpy(m_data + m_size, str, len);
        m_size += len;
        m_data[m_size] = '\0';
    }

    size_t find(const char* substr, size_t pos = 0) const noexcept {
        if (!substr || pos >= m_size) return npos;
        const char* found = std::strstr(m_data + pos, substr);
        if (!found) return npos;
        return static_cast<size_t>(found - m_data);
    }

    size_t find(char ch, size_t pos = 0) const noexcept {
        if (pos >= m_size) return npos;
        const char* found = static_cast<const char*>(
            std::memchr(m_data + pos, ch, m_size - pos));
        if (!found) return npos;
        return static_cast<size_t>(found - m_data);
    }

    bool operator==(const String& other) const noexcept {
        if (m_size != other.m_size) return false;
        return std::memcmp(m_data, other.m_data, m_size) == 0;
    }

    bool operator==(const char* str) const noexcept {
        if (!str) return false;
        return std::strcmp(m_data, str) == 0;
    }

   private:
    char* m_data{nullptr};
    size_t m_size{0};
    size_t m_capacity{0};
};

inline String to_string(int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return String(buf);
}

inline std::istream& getline(std::istream& is, String& str, char delim = '\n') {
    str.clear();
    char ch;
    while (is.get(ch)) {
        if (ch == delim) {
            break;
        }
        str.push_back(ch);
    }
    return is;
}

// Global Concatenation Operators
inline String operator+(const String& lhs, const String& rhs) {
    String res = lhs;
    res += rhs;
    return res;
}

inline std::ostream& operator<<(std::ostream& os, const String& str) {
    return os << str.c_str();
}

inline String operator+(const String& lhs, const char* rhs) {
    String res = lhs;
    res += rhs;
    return res;
}

inline String operator+(const char* lhs, const String& rhs) {
    String res(lhs);
    res += rhs;
    return res;
}

// =============================================================================
// nostd::Path
// =============================================================================
class Path {
   public:
    Path() = default;
    // NOLINTNEXTLINE(google-explicit-constructor)
    Path(const char* pathStr) : m_path(pathStr) { normalize(); }
    // NOLINTNEXTLINE(google-explicit-constructor)
    Path(const String& pathStr) : m_path(pathStr) { normalize(); }

    const String& string() const noexcept { return m_path; }
    const char* c_str() const noexcept { return m_path.c_str(); }
    bool empty() const noexcept { return m_size_check(); }

    Path& operator/=(const Path& other) { return appendPath(other.string()); }
    Path& operator/=(const String& other) { return appendPath(other); }
    Path& operator/=(const char* other) { return appendPath(String(other)); }

    private:
    bool m_size_check() const noexcept { return m_path.empty(); }

    Path& appendPath(const String& otherStr) {
        if (otherStr.empty()) return *this;
        if (m_path.empty()) {
            m_path = otherStr;
            return *this;
        }

        bool hasTrailing = (m_path.c_str()[m_path.size() - 1] == '/');
        bool hasLeading = (otherStr.c_str()[0] == '/');

        if (!hasTrailing && !hasLeading) {
            m_path += "/";
            m_path += otherStr;
        } else if (hasTrailing && hasLeading) {
            m_path.append(otherStr.c_str() + 1, otherStr.size() - 1);
        } else {
            m_path += otherStr;
        }

        return *this;
    }

    void normalize() {
        while (m_path.size() > 1 && m_path.c_str()[m_path.size() - 1] == '/') {
            m_path = String(m_path.c_str(), m_path.size() - 1);
        }
    }

    String m_path;
};
/// NOLINTEND(readability-identifier-naming)

// Path Division Operators
inline Path operator/(const Path& lhs, const Path& rhs) {
    Path res = lhs;
    res /= rhs;
    return res;
}

inline Path operator/(const Path& lhs, const String& rhs) {
    Path res = lhs;
    res /= rhs;
    return res;
}

inline Path operator/(const Path& lhs, const char* rhs) {
    Path res = lhs;
    res /= rhs;
    return res;
}

}  // namespace nostd

////////////////////////////////////////////////////////////////////////////////////////////////////////////
