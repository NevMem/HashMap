#include <stdexcept>
#include <utility>
#include <vector>

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

    bool __need_rehash() const {
        return (filled + 1) * 2 > capacity;
    }

    bool __insert(std::vector<std::pair<KeyType, ValueType>>& destination,
        std::vector<bool>& isEmpty, std::vector<bool>& isRemoved,
        const KeyType& key, const ValueType& value) {
            size_t keyHash = hasher(key);
            size_t indexToInsert = keyHash % destination.size();
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

    size_t __find(const KeyType& key) const {
        if (empty()) return data.size();
        size_t keyHash = hasher(key);
        size_t indexInData = keyHash % data.size();
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

    bool __erase(const KeyType& key) {
        size_t removeIndex = __find(key);
        if (removeIndex == data.size())
            return false;
        isRemoved[removeIndex] = true;
        return true;
    }

    void __rehash() {
        size_t newCapacity = capacity == 0 ? 2 : (capacity << 1);
        std::vector<std::pair<KeyType, ValueType>> dataBuffer(newCapacity);
        std::vector<bool> isEmptyBuffer(newCapacity, true);
        std::vector<bool> isRemovedBuffer(newCapacity, false);
        for (size_t index = 0; index != data.size(); ++index) {
            if (!isEmpty[index] && !isRemoved[index]) {
                __insert(dataBuffer, isEmptyBuffer, isRemovedBuffer,
                    data[index].first, data[index].second);
            }
        }
        std::swap(data, dataBuffer);
        std::swap(isEmpty, isEmptyBuffer);
        std::swap(isRemoved, isRemovedBuffer);
        capacity = newCapacity;
    }

    void __insert(const KeyType& key, const ValueType& value) {
        if (__need_rehash())
            __rehash();
        if (__insert(data, isEmpty, isRemoved, key, value)) {
            filled++;
            presentSize++;
        }
    }

 public:
    HashMap(Hasher hasher = Hasher()): capacity(0), presentSize(0), filled(0), hasher(hasher) {}

    template <typename Iter>
    HashMap(Iter begin, Iter end, Hasher hasher = Hasher()): HashMap(hasher) {
        while (begin != end) {
            __insert((*begin).first, (*begin).second);
            ++begin;
        }
    }

    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& list,
        Hasher hasher = Hasher()): HashMap(list.begin(), list.end(), hasher) {}

    class iterator {
     friend class HashMap<KeyType, ValueType, Hasher>;
     private:
        HashMap<KeyType, ValueType, Hasher>* anchor;
        size_t index;

        void __iterate_forward() {
            while (index < anchor->data.size() &&
                (anchor->isEmpty[index] || anchor->isRemoved[index]))
                    index++;
            if (index >= anchor->data.size())
                index = anchor->data.size();
        }

     public:
        iterator(): anchor(nullptr), index(0) {}

        iterator(HashMap<KeyType, ValueType, Hasher>& anchor, size_t index):
            anchor(&anchor), index(index) {}

        std::pair<const KeyType, ValueType>& operator *() {
            return reinterpret_cast<std::pair<const KeyType, ValueType>&>(anchor->data[index]);
        }

        std::pair<const KeyType, ValueType>* operator ->() {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&anchor->data[index]);
        }

        iterator& operator ++() {
            index++;
            __iterate_forward();
            return *this;
        }

        iterator operator ++(int) {
            auto tmp(*this);
            index++;
            __iterate_forward();
            return tmp;
        }

        bool operator == (const iterator& other) const {
            return index == other.index && (anchor) == (other.anchor);
        }

        bool operator != (const iterator& other) const {
            return !(*this == other);
        }
    };

    class const_iterator {
     friend class HashMap<KeyType, ValueType, Hasher>;
     private:
        const HashMap<KeyType, ValueType, Hasher>* anchor;
        size_t index;

        void __iterate_forward() {
            while (index < anchor->data.size() &&
                (anchor->isEmpty[index] || anchor->isRemoved[index]))
                    index++;
            if (index >= anchor->data.size())
                index = anchor->data.size();
        }

     public:
        const_iterator(): anchor(nullptr), index(0) {}

        const_iterator(const HashMap<KeyType, ValueType, Hasher>& anchor, size_t index):
            anchor(&anchor), index(index) {}

        const_iterator(const const_iterator& other):
            anchor(other.anchor), index(other.index) {}

        const std::pair<const KeyType, ValueType>& operator *() {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>&>(
                anchor->data[index]);
        }

        const std::pair<const KeyType, ValueType>* operator ->() {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>*>(
                &anchor->data[index]);
        }

        const_iterator& operator ++() {
            index++;
            __iterate_forward();
            return *this;
        }

        const_iterator operator ++(int) {
            auto tmp(*this);
            index++;
            __iterate_forward();
            return tmp;
        }

        bool operator == (const const_iterator& other) const {
            return index == other.index && (anchor) == (other.anchor);
        }

        bool operator != (const const_iterator& other) const {
            return !(*this == other);
        }
    };

    void insert(const std::pair<KeyType, ValueType>& element) {
        __insert(element.first, element.second);
    }

    void erase(const KeyType& key) {
        if (__erase(key)) {
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
        size_t indexInData = __find(key);
        return data[indexInData].second;
    }

    const ValueType& at(const KeyType& key) const {
        size_t indexInData = __find(key);
        if (indexInData == data.size())
            throw std::out_of_range("Key is invalid");
        return data[indexInData].second;
    }

    iterator begin() {
        auto pseudo_begin = iterator(*this, 0);
        pseudo_begin.__iterate_forward();
        return pseudo_begin;
    }

    iterator end() {
        return iterator(*this, data.size());
    }

    const_iterator begin() const {
        auto pseudo_begin = const_iterator(*this, 0);
        pseudo_begin.__iterate_forward();
        return pseudo_begin;
    }

    const_iterator end() const {
        return const_iterator(*this, data.size());
    }

    iterator find(const KeyType& key) {
        size_t index = __find(key);
        return iterator(*this, index);
    }

    const_iterator find(const KeyType& key) const {
        size_t index = __find(key);
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
