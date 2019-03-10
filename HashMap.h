#ifndef MY_HASHMAP
#define MY_HASHMAP

#include <stdexcept>
#include <utility>
#include <vector>

namespace myHashMap {
    template <class KeyType, class ValueType, class Hasher = std::hash<KeyType>>
    class HashMap {
     private:
        size_t capacity;
        size_t presentSize;
        size_t filled;
        Hasher hasher;
        std::vector<std::pair<KeyType, ValueType>> data;
        std::vector<bool> isEmpty;
        std::vector<bool> isRemoved;

        bool need_rehash_() const {
            return (filled + 1) * 2 > capacity;
        }

        size_t get_index_(const KeyType& key, size_t size) const {
            if (size == 0) return 0;
            return hasher(key) % size;
        }

        bool insert_to_(std::vector<std::pair<KeyType, ValueType>>& destination,
            std::vector<bool>& isEmpty, std::vector<bool>& isRemoved,
            const KeyType& key, const ValueType& value) {
                size_t indexToInsert = get_index_(key, destination.size());
                while (!isEmpty[indexToInsert]) {
                    if (isRemoved[indexToInsert] && destination[indexToInsert].first == key) {
                        isRemoved[indexToInsert] = false;
                        destination[indexToInsert].second = value;
                        return true;
                    }
                    if (destination[indexToInsert].first == key) {
                        return false;
                    }
                    indexToInsert++;
                    if (indexToInsert == destination.size())
                        indexToInsert = 0;
                }
                isEmpty[indexToInsert] = false;
                isRemoved[indexToInsert] = false;
                destination[indexToInsert] = std::pair<KeyType, ValueType>(key, value);
                return true;
            }

        size_t find_(const KeyType& key) const {
            if (empty()) return data.size();
            size_t indexInData = get_index_(key, data.size());
            while (!isEmpty[indexInData]) {
                if (data[indexInData].first == key) {
                    if (isRemoved[indexInData])
                        return data.size();
                    return indexInData;
                }
                indexInData++;
                if (indexInData == data.size())
                    indexInData = 0;
            }
            return data.size();
        }

        bool erase_(const KeyType& key) {
            size_t removeIndex = find_(key);
            if (removeIndex == data.size())
                return false;
            isRemoved[removeIndex] = true;
            return true;
        }

        void rehash_() {
            size_t newCapacity = capacity == 0 ? 2 : (capacity << 1);
            std::vector<std::pair<KeyType, ValueType>> dataBuffer(newCapacity);
            std::vector<bool> isEmptyBuffer(newCapacity, true);
            std::vector<bool> isRemovedBuffer(newCapacity, false);
            for (size_t index = 0; index != data.size(); ++index) {
                if (!isEmpty[index] && !isRemoved[index]) {
                    insert_to_(dataBuffer, isEmptyBuffer, isRemovedBuffer,
                        data[index].first, data[index].second);
                }
            }
            std::swap(data, dataBuffer);
            std::swap(isEmpty, isEmptyBuffer);
            std::swap(isRemoved, isRemovedBuffer);
            capacity = newCapacity;
        }

        void insert_(const KeyType& key, const ValueType& value) {
            if (need_rehash_())
                rehash_();
            if (insert_to_(data, isEmpty, isRemoved, key, value)) {
                filled++;
                presentSize++;
            }
        }

        template <class AnchorType>
        class iterator_base_ {
        friend class HashMap;

         protected:
            size_t index;
            AnchorType* anchor;

            void iterate_forward_() {
                while (index < anchor->data.size() &&
                    (anchor->isEmpty[index] || anchor->isRemoved[index]))
                        index++;
                if (index >= anchor->data.size())
                    index = anchor->data.size();
            }

         public:
            iterator_base_(): index(0), anchor(nullptr) {}

            iterator_base_(AnchorType& anchor_, size_t index_):
                index(index_), anchor(&anchor_) {}

            iterator_base_& operator ++() {
                index++;
                iterate_forward_();
                return *this;
            }

            iterator_base_ operator ++(int) {
                auto tmp(*this);
                index++;
                iterate_forward_();
                return tmp;
            }

            bool operator == (const iterator_base_& other) const {
                return index == other.index && (anchor) == (other.anchor);
            }

            bool operator != (const iterator_base_& other) const {
                return !(*this == other);
            }
        };

     public:
        explicit HashMap(Hasher hasher = Hasher()):
            capacity(0), presentSize(0), filled(0), hasher(hasher) {}

        template <typename ForwardIterator>
        HashMap(ForwardIterator begin, ForwardIterator end, Hasher hasher = Hasher()):
            HashMap(hasher) {
            while (begin != end) {
                insert_((*begin).first, (*begin).second);
                ++begin;
            }
        }

        HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& list,
            Hasher hasher = Hasher()): HashMap(list.begin(), list.end(), hasher) {}

        class iterator : public iterator_base_<HashMap<KeyType, ValueType, Hasher>> {
        friend class HashMap;
        using iterator_base_<HashMap<KeyType, ValueType, Hasher>>::anchor;
        using iterator_base_<HashMap<KeyType, ValueType, Hasher>>::index;
        using iterator_base_<HashMap<KeyType, ValueType, Hasher>>::iterator_base_;

         public:
            std::pair<const KeyType, ValueType>& operator *() {
                return reinterpret_cast<std::pair<const KeyType, ValueType>&>(anchor->data[index]);
            }

            std::pair<const KeyType, ValueType>* operator ->() {
                return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&anchor->data[index]);
            }
        };

        class const_iterator : public iterator_base_<const HashMap<KeyType, ValueType, Hasher>> {
        friend class HashMap;
        using iterator_base_<const HashMap<KeyType, ValueType, Hasher>>::anchor;
        using iterator_base_<const HashMap<KeyType, ValueType, Hasher>>::index;
        using iterator_base_<const HashMap<KeyType, ValueType, Hasher>>::iterator_base_;

         public:
            const std::pair<const KeyType, ValueType>& operator *() {
                return reinterpret_cast
                    <const std::pair<const KeyType, ValueType>&>(anchor->data[index]);
            }

            const std::pair<const KeyType, ValueType>* operator ->() {
                return reinterpret_cast
                    <const std::pair<const KeyType, ValueType>*>(&anchor->data[index]);
            }
        };

        void insert(const std::pair<KeyType, ValueType>& element) {
            insert_(element.first, element.second);
        }

        void erase(const KeyType& key) {
            if (erase_(key)) {
                presentSize--;
            }
        }

        Hasher hash_function() const {
            return hasher;
        }

        size_t size() const {
            return presentSize;
        }

        size_t empty() const {
            return size() == 0;
        }

        ValueType& operator[](const KeyType& key) {
            insert(std::pair<KeyType, ValueType>(key, ValueType()));
            size_t indexInData = find_(key);
            return data[indexInData].second;
        }

        const ValueType& at(const KeyType& key) const {
            size_t indexInData = find_(key);
            if (indexInData == data.size())
                throw std::out_of_range("Key is invalid");
            return data[indexInData].second;
        }

        iterator begin() {
            auto pseudo_begin = iterator(*this, 0);
            pseudo_begin.iterate_forward_();
            return pseudo_begin;
        }

        iterator end() {
            return iterator(*this, data.size());
        }

        const_iterator begin() const {
            auto pseudo_begin = const_iterator(*this, 0);
            pseudo_begin.iterate_forward_();
            return pseudo_begin;
        }

        const_iterator end() const {
            return const_iterator(*this, data.size());
        }

        iterator find(const KeyType& key) {
            size_t index = find_(key);
            return iterator(*this, index);
        }

        const_iterator find(const KeyType& key) const {
            size_t index = find_(key);
            return const_iterator(*this, index);
        }

        void clear() {
            capacity = 0;
            filled = 0;
            presentSize = 0;
            isEmpty.clear();
            isRemoved.clear();
            data.clear();
        }
    };
}

template <class KeyType, class ValueType, class Hasher = std::hash<KeyType>>
using HashMap = myHashMap::HashMap<KeyType, ValueType, Hasher>;

#endif
