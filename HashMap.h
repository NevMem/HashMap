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
                    if (destination[indexToInsert].first == key) {
                        if (isRemoved[indexToInsert]) {
                            isRemoved[indexToInsert] = false;
                            destination[indexToInsert].second = value;
                            return true;
                        }
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
            bool wasInsertion = insert_to_(data, isEmpty, isRemoved, key, value);
            if (wasInsertion) {
                filled++;
                presentSize++;
            }
        }

        template <class DataArrayType>
        class iterator_base_ {
         protected:
            size_t index_;
            DataArrayType* data_;
            const std::vector<bool>* empty_;
            const std::vector<bool>* removed_;

            void iterate_forward_() {
                while (index_ < data_->size() &&
                    ((*empty_)[index_] || (*removed_)[index_]))
                        index_++;
                if (index_ >= data_->size())
                    index_ = data_->size();
            }

         public:
            iterator_base_():
                index_(0), data_(nullptr), empty_(nullptr), removed_(nullptr) {}

            iterator_base_(DataArrayType& data, size_t index,
                const std::vector<bool>& empty, const std::vector<bool>& removed):
                index_(index), data_(&data), empty_(&empty), removed_(&removed) {
                    iterate_forward_();
                }

            iterator_base_& operator ++() {
                index_++;
                iterate_forward_();
                return *this;
            }

            iterator_base_ operator ++(int) {
                auto tmp(*this);
                index_++;
                iterate_forward_();
                return tmp;
            }

            bool operator == (const iterator_base_& other) const {
                return index_ == other.index_ && data_ == other.data_
                    && empty_ == other.empty_ && removed_ == other.removed_;
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

        typedef std::vector<std::pair<KeyType, ValueType>> DataArrayType;
        class iterator : public iterator_base_<DataArrayType> {
        friend class HashMap;
        using iterator_base_<DataArrayType>::data_;
        using iterator_base_<DataArrayType>::index_;
        using iterator_base_<DataArrayType>::iterator_base_;

         public:
            std::pair<const KeyType, ValueType>& operator *() {
                return reinterpret_cast<std::pair<const KeyType, ValueType>&>((*data_)[index_]);
            }

            std::pair<const KeyType, ValueType>* operator ->() {
                return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&(*data_)[index_]);
            }
        };

        typedef const std::vector<std::pair<KeyType, ValueType>> ConstDataArrayType;
        class const_iterator : public iterator_base_<ConstDataArrayType> {
        friend class HashMap;
        using iterator_base_<ConstDataArrayType>::data_;
        using iterator_base_<ConstDataArrayType>::index_;
        using iterator_base_<ConstDataArrayType>::iterator_base_;

         public:
            const std::pair<const KeyType, ValueType>& operator *() {
                return reinterpret_cast
                    <const std::pair<const KeyType, ValueType>&>((*data_)[index_]);
            }

            const std::pair<const KeyType, ValueType>* operator ->() {
                return reinterpret_cast
                    <const std::pair<const KeyType, ValueType>*>(&(*data_)[index_]);
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
            insert_(key, ValueType());
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
            return iterator(data, 0, isEmpty, isRemoved);
        }

        iterator end() {
            return iterator(data, data.size(), isEmpty, isRemoved);
        }

        const_iterator begin() const {
            return const_iterator(data, 0, isEmpty, isRemoved);
        }

        const_iterator end() const {
            return const_iterator(data, data.size(), isEmpty, isRemoved);
        }

        iterator find(const KeyType& key) {
            return iterator(data, find_(key), isEmpty, isRemoved);
        }

        const_iterator find(const KeyType& key) const {
            return const_iterator(data, find_(key), isEmpty, isRemoved);
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
