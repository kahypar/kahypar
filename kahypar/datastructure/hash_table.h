#pragma once

#include <limits>
#include <vector>

namespace std {

template <typename T, typename V>
struct numeric_limits<pair<T, V>>
{
    static constexpr pair<T, V> max() noexcept { return std::make_pair(numeric_limits<T>::max(), numeric_limits<V>::max()); }
};

}

template <typename T>
struct SimpleHash
{
    size_t operator() (const T& x) const
    {
        return x;
    }
};


template <>
struct SimpleHash<std::pair<int, int>>
{
    uint64_t operator() (const std::pair<int, int>& x) const
    {
        return ((uint64_t)(x.first) << 32) + x.second;
    }
};


template <typename HashTable>
class HashTableIterator
{
private:
    using Hm = HashTable;
    using Elem = typename Hm::Elem;
    using Pos = typename Hm::Pos ;

public:
    HashTableIterator(const Hm& hm, Pos offset)
            :   _hm(hm)
            ,   _offset(offset)
            ,   _specials(0)
            ,   _numSpecials(_hm.getNumSpecials())
    {
    }

    const Elem& operator* ()
    {
        const Elem* res = _hm.getSpecialElem(_specials);
        if (res) {
            return *res;
        } else {
            return _hm._ht[_hm._poses[_offset - _specials]];
        }
    }

    void operator++ ()
    {
        ++_offset;
        if (_specials < _numSpecials)
            ++_specials;
    }

    bool operator==(const HashTableIterator& it)
    {
        return _offset == it._offset;
    }

    bool operator!=(const HashTableIterator& it)
    {
        return !(*this == it);
    }

private:
    const Hm& _hm;
    Pos _offset;
    Pos _specials;
    Pos _numSpecials;
};

template <typename Key, typename Value, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class hash_map
{
public:
    using Elem = std::pair<Key, Value>;
    using key_type = Key ;
    using mapped_type = Value;

private:
    typedef hash_map<Key, Value, Hash, Cache, SizeFactor> TSelf;

    using Pos = uint32_t;

    const Pos _numSpecials = 2;

    bool _emptyElemKey;
    bool _deletedElemKey;
    Elem _emptyElem;
    Elem _deletedElem;
    uint64_t _htSize;
    uint64_t _maxSize;
    std::vector<Elem> _ht;
    std::vector<Pos> _poses;
    std::vector<uint32_t> _posInPos;
    Hash _hash;
    Key _lastKey;
    Pos _lastPos;

public:
    typedef HashTableIterator<TSelf> Iterator;

    friend Iterator;

    explicit hash_map(uint64_t maxSize = 0)
            :   _emptyElemKey(false)
            ,   _deletedElemKey(false)
            ,   _emptyElem(std::numeric_limits<Key>::max(), Value())
            ,   _deletedElem(_emptyElem.first - 1, Value())
            ,   _htSize(maxSize * SizeFactor)
            ,   _maxSize(maxSize)
            ,   _ht(_htSize + _maxSize * 1.1, std::make_pair(_emptyElem.first, Value()))
            ,   _posInPos(_htSize + _maxSize * 1.1)
            ,   _lastKey(_emptyElem.first)
            ,   _lastPos(0)
    {
        _poses.reserve(maxSize);
    }

    void reserve(uint32_t maxSize)
    {
        _htSize = maxSize * SizeFactor;
        _maxSize = maxSize;
        _ht.resize(_htSize + _maxSize * 1.1, std::make_pair(_emptyElem.first, Value()));
        _posInPos.resize(_htSize + _maxSize * 1.1);
    }

    size_t size() const
    {
        return _poses.size() + _emptyElemKey + _deletedElemKey;
    }

    Iterator begin() const
    {
        return Iterator(*this, 0);
    }

    Iterator end() const
    {
        return Iterator(*this, size());
    }

    bool empty() const
    {
        return size() == 0;
    }

    void erase(const Key& key) {
        if (key == _emptyElem.first) {
            _emptyElemKey = false;
            return;
        }

        if (key == _deletedElem.first) {
            _deletedElemKey = false;
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first != _emptyElem.first) {
            _ht[pos].first = _deletedElem.first;
            uint32_t posOfDeleted = _posInPos[pos];
            _posInPos[_poses.back()] = posOfDeleted;
            std::swap(_poses[posOfDeleted], _poses.back());
            _poses.resize(_poses.size() - 1);
        }
    }

    Value& operator[] (const Key& key)
    {
        if (key == _emptyElem.first) {
            if (!_emptyElemKey) {
                _emptyElemKey = true;
                _emptyElem.second = Value();
            }
            return _emptyElem.second;
        }

        if (key == _deletedElem.first) {
            if (!_deletedElemKey) {
                _deletedElemKey = true;
                _deletedElem.second = Value();
            }
            return _deletedElem.second;
        }

        Pos pos = findPos(key);

        if (_ht[pos].first == _emptyElem.first)
        {
            _ht[pos].first = key;
            _ht[pos].second = Value();
            _poses.push_back(pos);
            _posInPos[pos] = _poses.size() - 1;
        }

        return _ht[pos].second;
    }

    bool contains(const Key& key)
    {
        if (key == _emptyElem.first) {
            return _emptyElemKey;
        }

        if (key == _deletedElem.first) {
            return _deletedElemKey;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first == _emptyElem.first)
            return false;
        return true;
    }

    void insert(const Elem& elem)
    {
        insertImpl(elem.first, elem.second);
    }

    void insert(const Key& key, const Value& value)
    {
        insertImpl(key, value);
    }

    void clear()
    {
        for (auto pos : _poses)
            _ht[pos].first = _emptyElem.first;

        _poses.clear();

        _lastKey = _emptyElem.first;
        _lastPos = 0;

        _emptyElemKey = false;
        _deletedElemKey = false;
    }

    void swap(TSelf& hashMap)
    {
        std::swap(_htSize, hashMap._htSize);
        std::swap(_maxSize, hashMap._maxSize);
        _ht.swap(hashMap._ht);
        _poses.swap(hashMap._poses);
        std::swap(_lastKey, hashMap._lastKey);
        std::swap(_lastPos, hashMap._lastPos);
        _posInPos.swap(hashMap._posInPos);
        _emptyElem.swap(hashMap._emptyElem);
        _deletedElem.swap(hashMap._deletedElem);
        std::swap(_emptyElemKey, hashMap._emptyElemKey);
        std::swap(_deletedElemKey, hashMap._deletedElemKey);
    }

private:
    Pos getNumSpecials() const {
        return _emptyElemKey + _deletedElemKey;
    }

    const Elem* getSpecialElem(Pos offset) const {
        if (_emptyElemKey && offset == 0) {
            return &_emptyElem;
        }
        if (_deletedElemKey && offset == 1) {
            return &_deletedElem;
        }
        return nullptr;
    }

    void insertImpl(const Key& key, const Value& value)
    {
        if (key == _emptyElem.first) {
            if (!_emptyElemKey) {
                _emptyElemKey = true;
                _emptyElem.second = value;
            }
            return;
        }

        if (key == _deletedElem.first) {
            if (!_deletedElemKey) {
                _deletedElemKey = true;
                _deletedElem.second = value;
            }
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first == _emptyElem.first) {
            _ht[pos].first = key;
            _ht[pos].second = value;
            _poses.push_back(pos);
            _posInPos[pos] = _poses.size() - 1;
        }
    }

    Pos findPos(const Key& key)
    {
        if (Cache && key == _lastKey)
            return _lastPos;

        Pos startPos = _hash(key) % _htSize;

        Pos pos;
        for (pos = startPos; pos < _ht.size(); ++pos)
        {
            if (_ht[pos].first == _emptyElem.first || _ht[pos].first == key)
            {
                if (Cache) {
                    _lastKey = key;
                    _lastPos = pos;
                }
                return pos;
            }
        }

        throw std::logic_error("_hash table is oveflowed");
    }
};

template <typename Key, typename Value, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class hash_map_no_erase
{
public:
    typedef std::pair<Key, Value> Elem;
    typedef Key key_type;
    typedef Value mapped_type;

private:
    typedef hash_map_no_erase<Key, Value, Hash, Cache, SizeFactor> TSelf;

    typedef uint32_t Pos;

    const Pos _numSpecials = 1;

    bool _emptyElemKey;
    Elem _emptyElem;
    uint64_t _htSize;
    uint64_t _maxSize;
    std::vector<Elem> _ht;
    std::vector<Pos> _poses;
    Hash _hash;
    Key _lastKey;
    Pos _lastPos;

public:
    typedef HashTableIterator<TSelf> Iterator;

    friend Iterator;

    explicit hash_map_no_erase(uint64_t maxSize = 0)
            :   _emptyElemKey(false)
            ,   _emptyElem(std::numeric_limits<Key>::max(), Value())
            ,   _htSize(maxSize * SizeFactor)
            ,   _maxSize(maxSize)
            ,   _ht(_htSize + _maxSize * 1.1, std::make_pair(_emptyElem.first, Value()))
            ,   _lastKey(_emptyElem.first)
            ,   _lastPos(0)
    {
        _poses.reserve(maxSize);
    }

    void reserve(uint32_t maxSize)
    {
        _htSize = maxSize * SizeFactor;
        _maxSize = maxSize;
        _ht.resize(_htSize + _maxSize * 1.1, std::make_pair(_emptyElem, Value()));
    }

    uint64_t size() const
    {
        return _poses.size() + _emptyElemKey;
    }

    Iterator begin() const
    {
        return Iterator(*this, 0);
    }

    Iterator end() const
    {
        return Iterator(*this, size());
    }

    bool empty() const
    {
        return size() == 0;
    }

    Value& operator[] (const Key& key)
    {
        if (key == _emptyElem.first) {
            if (!_emptyElemKey) {
                _emptyElemKey = true;
                _emptyElem.second = Value();
            }
            return _emptyElem.second;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first == _emptyElem.first)
        {
            _ht[pos].first = key;
            _ht[pos].second = Value();
            _poses.push_back(pos);
        }

        return _ht[pos].second;
    }

    bool contains(const Key& key)
    {
        if (key == _emptyElem.first) {
            return _emptyElemKey;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first == _emptyElem.first)
            return false;
        return true;
    }

    void insert(const Elem& elem)
    {
        insertImpl(elem.first, elem.second);
    }

    void insert(const Key& key, const Value& value)
    {
        insertImpl(key, value);
    }

    void clear()
    {
        for (auto pos : _poses)
            _ht[pos].first = _emptyElem;

        _poses.clear();

        _lastKey = _emptyElem;
        _lastPos = 0;

        _emptyElemKey = false;
    }

    void swap(TSelf& hashMap)
    {
        std::swap(_htSize, hashMap._htSize);
        std::swap(_maxSize, hashMap._maxSize);
        _ht.swap(hashMap._ht);
        _poses.swap(hashMap._poses);
        std::swap(_lastKey, hashMap._lastKey);
        std::swap(_lastPos, hashMap._lastPos);
        _emptyElem.swap(hashMap._emptyElem);
        std::swap(_emptyElemKey, hashMap._emptyElemKey);
    }

private:
    Pos getNumSpecials() const {
        return _emptyElemKey;
    }

    const Elem* getSpecialElem(Pos offset) const {
        if (_emptyElemKey && offset == 0) {
            return &_emptyElem;
        }
        return nullptr;
    }

    void insertImpl(const Key& key, const Value& value)
    {
        if (key == _emptyElem.first) {
            if (!_emptyElemKey) {
                _emptyElemKey = true;
                _emptyElem.second = value;
            }
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos].first == _emptyElem.first) {
            _ht[pos].first = key;
            _ht[pos].second = value;
            _poses.push_back(pos);
        }
    }

    Pos findPos(const Key& key)
    {
        if (Cache && key == _lastKey)
            return _lastPos;

        Pos startPos = _hash(key) % _htSize;

        Pos pos;
        for (pos = startPos; pos < _ht.size(); ++pos)
        {
            if (_ht[pos].first == _emptyElem.first || _ht[pos].first == key)
            {
                if (Cache) {
                    _lastKey = key;
                    _lastPos = pos;
                }
                return pos;
            }
        }

        throw std::logic_error("_hash table is oveflowed");
    }
};

template <typename Key, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class hash_set
{
public:
    typedef Key Elem;

private:
    typedef hash_set<Key, Hash, Cache, SizeFactor> TSelf;

    typedef uint32_t Pos;

    const Pos _numSpecials = 2;

    bool _emptyElemKey;
    bool _deletedElemKey;
    const Key _emptyElem;
    const Key _deletedElem;
    uint64_t _htSize;
    uint64_t _maxSize;
    std::vector<Elem> _ht;
    std::vector<Pos> _poses;
    std::vector<uint32_t> _posInPos;
    Hash _hash;
    Key _lastKey;
    Pos _lastPos;

public:
    typedef HashTableIterator<TSelf> Iterator;

    friend Iterator;

    explicit hash_set(uint64_t maxSize = 0)
            :   _emptyElemKey(false)
            ,   _deletedElemKey(false)
            ,   _emptyElem(std::numeric_limits<Key>::max())
            ,   _deletedElem(_emptyElem - 1)
            ,   _htSize(maxSize * SizeFactor)
            ,   _maxSize(maxSize)
            ,   _ht(_htSize + _maxSize * 1.1, _emptyElem)
            ,   _posInPos(_htSize + _maxSize * 1.1)
            ,   _lastKey(_emptyElem)
            ,   _lastPos(0)
    {
        _poses.reserve(maxSize);
    }

    void reserve(uint32_t maxSize)
    {
        _htSize = maxSize * SizeFactor;
        _maxSize = maxSize;
        _ht.resize(_htSize + _maxSize * 1.1, _emptyElem);
        _posInPos.resize(_htSize + _maxSize * 1.1);
    }

    uint64_t size() const
    {
        return _poses.size() + _emptyElemKey + _deletedElemKey;
    }

    Iterator begin() const
    {
        return Iterator(*this, 0);
    }

    Iterator end() const
    {
        return Iterator(*this, size());
    }

    bool empty() const
    {
        return size() == 0;
    }

    void erase(const Key& key) {
        if (key == _emptyElem) {
            _emptyElemKey = false;
            return;
        }

        if (key == _deletedElem) {
            _deletedElemKey = false;
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos] != _emptyElem) {
            _ht[pos] = _deletedElem;
            uint32_t posOfDeleted = _posInPos[pos];
            _posInPos[_poses.back()] = posOfDeleted;
            std::swap(_poses[posOfDeleted], _poses.back());
            _poses.resize(_poses.size() - 1);
        }
    }

    bool contains(const Key& key)
    {
        if (key == _emptyElem) {
            return _emptyElemKey;
        }

        if (key == _deletedElem) {
            return _deletedElemKey;
        }

        Pos pos = findPos(key);
        if (_ht[pos] == _emptyElem)
            return false;
        return true;
    }

    void insert(const Key& key)
    {
        insertImpl(key);
    }

    void clear()
    {
        for (auto pos : _poses)
            _ht[pos] = _emptyElem;

        _poses.clear();

        _lastKey = _emptyElem;
        _lastPos = 0;

        _emptyElemKey = false;
        _deletedElemKey = false;
    }

    void swap(TSelf& hashMap)
    {
        std::swap(_htSize, hashMap._htSize);
        std::swap(_maxSize, hashMap._maxSize);
        _ht.swap(hashMap._ht);
        _poses.swap(hashMap._poses);
        std::swap(_lastKey, hashMap._lastKey);
        std::swap(_lastPos, hashMap._lastPos);
        _posInPos.swap(hashMap._posInPos);
        _emptyElem.swap(hashMap._emptyElem);
        _deletedElem.swap(hashMap._deletedElem);
        std::swap(_emptyElemKey, hashMap._emptyElemKey);
        std::swap(_deletedElemKey, hashMap._deletedElemKey);
    }

private:
    Pos getNumSpecials() const {
        return _emptyElemKey + _deletedElemKey;
    }

    const Elem* getSpecialElem(Pos offset) const {
        if (_emptyElemKey && offset == 0) {
            return &_emptyElem;
        }
        if (_deletedElemKey && offset == 1) {
            return &_deletedElem;
        }
        return nullptr;
    }

    void insertImpl(const Key& key)
    {
        if (key == _emptyElem) {
            if (!_emptyElemKey) {
                _emptyElemKey = true;
            }
            return;
        }

        if (key == _deletedElem) {
            if (!_deletedElemKey) {
                _deletedElemKey = true;
            }
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos] == _emptyElem) {
            _ht[pos] = key;
            _poses.push_back(pos);
            _posInPos[pos] = _poses.size() - 1;
        }
    }

    Pos findPos(const Key& key)
    {
        if (Cache && key == _lastKey)
            return _lastPos;

        Pos startPos = _hash(key) % _htSize;

        Pos pos;
        for (pos = startPos; pos < _ht.size(); ++pos)
        {
            if (_ht[pos] == _emptyElem || _ht[pos] == key)
            {
                if (Cache) {
                    _lastKey = key;
                    _lastPos = pos;
                }
                return pos;
            }
        }

        throw std::logic_error("_hash table is oveflowed");
    }
};

template <typename Key, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class hash_set_no_erase
{
public:
    typedef Key Elem;

private:
    typedef hash_set_no_erase<Key, Hash, Cache, SizeFactor> TSelf;

    typedef uint32_t Pos;

    const Pos _numSpecials = 2;

    bool _emptyElemKey;
    const Key _emptyElem;
    uint64_t _htSize;
    uint64_t _maxSize;
    std::vector<Elem> _ht;
    std::vector<Pos> _poses;
    Hash _hash;
    Key _lastKey;
    Pos _lastPos;

public:
    typedef HashTableIterator<TSelf> Iterator;

    friend Iterator;

    explicit hash_set_no_erase(uint64_t maxSize = 0)
            :   _emptyElemKey(false)
            ,   _emptyElem(std::numeric_limits<Key>::max())
            ,   _htSize(maxSize * SizeFactor)
            ,   _maxSize(maxSize)
            ,   _ht(_htSize + _maxSize * 1.1, _emptyElem)
            ,   _lastKey(_emptyElem)
            ,   _lastPos(0)
    {
        _poses.reserve(maxSize);
    }

    void reserve(uint32_t maxSize)
    {
        _htSize = maxSize * SizeFactor;
        _maxSize = maxSize;
        _ht.resize(_htSize + _maxSize * 1.1, _emptyElem);
    }

    uint64_t size() const
    {
        return _poses.size() + _emptyElemKey;
    }

    Iterator begin() const
    {
        return Iterator(*this, 0);
    }

    Iterator end() const
    {
        return Iterator(*this, size());
    }

    bool empty() const
    {
        return size() == 0;
    }

    bool contains(const Key& key)
    {
        if (key == _emptyElem) {
            return _emptyElemKey;
        }
        Pos pos = findPos(key);
        if (_ht[pos] == _emptyElem)
            return false;
        return true;
    }

    void insert(const Key& key)
    {
        insertImpl(key);
    }

    void clear()
    {
        for (auto pos : _poses)
            _ht[pos] = _emptyElem;

        _poses.clear();

        _lastKey = _emptyElem;
        _lastPos = 0;

        _emptyElemKey = false;
    }

    void swap(TSelf& hashMap)
    {
        std::swap(_htSize, hashMap._htSize);
        std::swap(_maxSize, hashMap._maxSize);
        _ht.swap(hashMap._ht);
        _poses.swap(hashMap._poses);
        std::swap(_lastKey, hashMap._lastKey);
        std::swap(_lastPos, hashMap._lastPos);
        std::swap(_emptyElemKey, hashMap._emptyElemKey);
    }

private:
    Pos getNumSpecials() const {
        return _emptyElemKey;
    }

    const Elem* getSpecialElem(Pos offset) const {
        if (_emptyElemKey && offset == 0) {
            return &_emptyElem;
        } else {
            return nullptr;
        }
    }

    void insertImpl(const Key& key)
    {
        if (key == _emptyElem && !_emptyElemKey) {
            _emptyElemKey = true;
            return;
        }

        Pos pos = findPos(key);
        if (_ht[pos] == _emptyElem) {
            _ht[pos] = key;
            _poses.push_back(pos);
        }
    }

    Pos findPos(const Key& key)
    {
        if (Cache && key == _lastKey)
            return _lastPos;

        Pos startPos = _hash(key) % _htSize;

        Pos pos;
        for (pos = startPos; pos < _ht.size(); ++pos)
        {
            if (_ht[pos] == _emptyElem || _ht[pos] == key)
            {
                if (Cache) {
                    _lastKey = key;
                    _lastPos = pos;
                }
                return pos;
            }
        }

        throw std::logic_error("_hash table is oveflowed");
    }
};
