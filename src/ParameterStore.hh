#pragma once

#ifndef PARAMETERSTORE_HH
#define PARAMETERSTORE_HH

#include <stdexcept> // For exceptions
#include <string>
#include <unordered_map>
#include <utility> // For std::move

/**
 * @class ParameterStore
 * @brief A type-safe, heterogeneous key-value store.
 *
 * This class stores values of different types associated with string keys.
 * It manages the memory of the stored objects, allocating on insertion
 * and deallocating when the store is destroyed or a value is overwritten.
 * The templated `add` and `get` methods provide a type-safe interface
 * for the end-user.
 *
 * Example usage:
 * ```cpp
 *   ParameterStore store;
 *   store.add<int>("param1", 42);
 *
 *  int value = store.get<int>("param1");
 *  ```
 */
class ParameterStore
{
    public:
        /**
         * @brief Default constructor.
         */
        ParameterStore() = default;

        /**
         * @brief Destructor.
         *
         * Cleans up all the memory allocated by the store.
         */
        ~ParameterStore()
        {
            for (auto &pair : m_store)
            {
                delete pair.second;
            }
        }

        // --- Rule of Five: Disable copying and moving ---
        // This prevents shallow copies which would lead to double-free errors.
        // A proper implementation would require deep copying, which is complex.
        // For most use cases, a central store like this should not be copied.
        ParameterStore(const ParameterStore &) = delete;
        ParameterStore &operator=(const ParameterStore &) = delete;
        ParameterStore(ParameterStore &&) = delete;
        ParameterStore &operator=(ParameterStore &&) = delete;

        /**
         * @brief Adds or updates a parameter in the store.
         * @tparam T The type of the value to store.
         * @param key The string key to associate with the value.
         * @param value The value to store. The value is moved into the store.
         */
        template <typename T> void add(const std::string &key, T value)
        {
            // Check if the key already exists to clean up the old value first.
            auto it = m_store.find(key);
            if (it != m_store.end())
            {
                delete it->second; // Free old memory before overwriting
            }

            // Allocate new memory for the value in its type-erased holder.
            m_store[key] = new ValueHolder<T>(std::move(value));
        }

        /**
         * @brief Retrieves a parameter from the store.
         * @tparam T The type of the value to retrieve.
         * @param key The key of the value to retrieve.
         * @return A mutable reference to the stored value.
         * @throw std::out_of_range if the key does not exist.
         * @throw std::bad_cast if the key exists but is of a different type.
         */
        template <typename T> T &get(const std::string &key)
        {
            auto it = m_store.find(key);
            if (it == m_store.end())
            {
                throw std::out_of_range("Parameter with key '" + key + "' not found.");
            }

            // Safely downcast the base pointer to the specific derived pointer.
            ValueHolder<T> *holder = dynamic_cast<ValueHolder<T> *>(it->second);
            if (holder == nullptr)
            {
                // This means the key exists, but you requested the wrong type.
                throw std::bad_cast();
            }

            return holder->m_data;
        }

        /**
         * @brief Retrieves a constant parameter from the store.
         * @tparam T The type of the value to retrieve.
         * @param key The key of the value to retrieve.
         * @return A constant reference to the stored value.
         * @throw std::out_of_range if the key does not exist.
         * @throw std::bad_cast if the key exists but is of a different type.
         */
        template <typename T> const T &get(const std::string &key) const
        {
            auto it = m_store.find(key);
            if (it == m_store.end())
            {
                throw std::out_of_range("Parameter with key '" + key + "' not found.");
            }

            const ValueHolder<T> *holder = dynamic_cast<const ValueHolder<T> *>(it->second);
            if (holder == nullptr)
            {
                throw std::bad_cast();
            }

            return holder->m_data;
        }

        /**
         * @brief Checks if a parameter with a given key exists.
         * @param key The key to check.
         * @return True if the key exists, false otherwise.
         */
        bool exists(const std::string &key) const
        {
            return m_store.count(key) > 0;
        }

    private:
        // --- Type Erasure Implementation ---

        /**
         * @struct ValueBase
         * @brief Abstract base class for the type erasure container.
         * The virtual destructor is essential for correct memory cleanup.
         */
        struct ValueBase
        {
                virtual ~ValueBase() = default;
        };

        /**
         * @struct ValueHolder
         * @brief Templated derived class that holds the actual data.
         */
        template <typename T> struct ValueHolder : public ValueBase
        {
                T m_data;

                // Constructor moves the data into the holder
                explicit ValueHolder(T data) : m_data(std::move(data))
                {
                }
        };

        // The map stores string keys and pointers to the base class.
        std::unordered_map<std::string, ValueBase *> m_store;
};

#endif // PARAMETERSTORE_HH
