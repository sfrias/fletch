// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:_fletch_system' as fletch;

const patch = "patch";

@patch class LinkedHashMap<K, V> {
  @patch factory LinkedHashMap({ bool equals(K key1, K key2),
                                 int hashCode(K key),
                                 bool isValidKey(potentialKey) }) {
    if (equals != null ||
        hashCode != null ||
        isValidKey != null) {
      throw new UnsupportedError("LinkedHashMap arguments are not implemented");
    }
    return new fletch.LinkedHashMapImpl<K, V>();
  }
}

@patch class HashMap<K, V> {
  @patch factory HashMap({ bool equals(K key1, K key2),
                           int hashCode(K key),
                           bool isValidKey(potentialKey) }) {
    if (equals != null ||
        hashCode != null ||
        isValidKey != null) {
      throw new UnsupportedError("HashMap arguments are not implemented");
    }
    return new _HashMap<K, V>();
  }
}

@patch class LinkedHashSet<E> {
  @patch factory LinkedHashSet({ bool equals(E key1, E key2),
                                 int hashCode(E key),
                                 bool isValidKey(potentialKey) }) {
    if (equals != null ||
        hashCode != null ||
        isValidKey != null) {
      throw new UnsupportedError("LinkedHashMap arguments are not implemented");
    }
    return new fletch.LinkedHashSetImpl<E>();
  }
}

@patch class HashSet<E> {
  @patch factory HashSet({ bool equals(E key1, E key2),
                           int hashCode(E key),
                           bool isValidKey(potentialKey) }) {
    if (equals != null ||
        hashCode != null ||
        isValidKey != null) {
      throw new UnsupportedError("LinkedHashMap arguments are not implemented");
    }
    return new _HashSet<E>();
  }
}

class _HashMap<K, V> implements HashMap<K, V> {
  static const int _INITIAL_SIZE = 8;

  List _buckets;
  int _elements = 0;

  // TODO(ager): Other versions of constructors. Parameterization with
  // comparison etc.
  _HashMap() : _buckets = new List(_INITIAL_SIZE);

  _HashMap._(int buckets) : _buckets = new List(buckets);

  _HashMapNode<K, V> _lookup(K key) {
    var hash = key.hashCode.abs();
    var index = hash % _buckets.length;
    var node = _buckets[index];
    while (node != null) {
      if (node.key == key) return node;
      node = node.next;
    }
    return null;
  }

  bool containsValue(Object value) {
    for (var v in values) {
      if (v == value) return true;
    }
    return false;
  }

  bool containsKey(Object key) => _lookup(key) != null;

  V operator[](Object key) {
    var node = _lookup(key);
    if (node == null) return null;
    return node.value;
  }

  void _resizeIfNeeded() {
    var bucketCount = _buckets.length;
    if (_elements > (bucketCount - (bucketCount >> 2))) {
      var rehashed = new _HashMap<K, V>._(bucketCount * 2);
      rehashed.addAll(this);
      _buckets = rehashed._buckets;
      bucketCount = bucketCount * 2;
    }
  }

  void operator[]=(K key, V value) {
    _resizeIfNeeded();
    var hash = key.hashCode.abs();
    var index = hash % _buckets.length;
    var node = _buckets[index];
    while (node != null) {
      if (node.key == key) break;
      node = node.next;
    }
    if (node != null) {
      node.value = value;
    } else {
      var node = new _HashMapNode(key, value);
      node.next = _buckets[index];
      _buckets[index] = node;
      ++_elements;
    }
  }

  V putIfAbsent(K key, V ifAbsent()) {
    var node = _lookup(key);
    if (node != null) return node.value;
    var value = ifAbsent();
    return this[key] = value;
  }

  void addAll(Map<K, V> other) {
    for (var k in other.keys) {
      this[k] = other[k];
    }
  }

  V remove(Object key) {
    var hash = key.hashCode.abs();
    var index = hash % _buckets.length;
    var node = _buckets[index];
    var previous = null;
    while (node != null) {
      if (node.key == key) {
        if (previous == null) {
          _buckets[index] = node.next;
        } else {
          previous.next = node.next;
        }
        --_elements;
        return node.value;
      }
      previous = node;
      node = node.next;
    }
    return null;
  }

  void clear() {
    _buckets = new List(_INITIAL_SIZE);
    _elements = 0;
  }

  void forEach(void f(K key, V value)) {
    for (var k in this.keys) {
      f(k, this[k]);
    }
  }

  Iterable<K> get keys => new _KeyIterable(this);

  Iterable<V> get values => new _ValueIterable(this);

  int get length => _elements;

  bool get isEmpty => _elements == 0;

  bool get isNotEmpty => !isEmpty;
}

class _HashMapNode<K, V> {
  final K key;
  V value;
  _HashMapNode next;

  _HashMapNode(this.key, this.value);
}

class _HashNodeIterator<K, V> {
  final _HashMap<K, V> _map;
  int _index = -1;
  _HashMapNode<K, V> _current;

  _HashNodeIterator(this._map);

  bool moveNext() {
    if (_current != null) {
      _current = _current.next;
      if (_current != null) return true;
    }
    _index++;
    int limit = _map._buckets.length;
    for (; _index < limit; _index++) {
      if (_map._buckets[_index] != null) {
        _current = _map._buckets[_index];
        return true;
      }
    }
    return false;
  }
}

class _KeyIterable<K, V> extends IterableBase<K> implements Iterable<K> {
  final HashMap<K, V> _map;
  _KeyIterable(this._map);
  Iterator<K> get iterator => new _KeyIterator<K, V>(_map);
}

class _KeyIterator<K, V>
    extends _HashNodeIterator<K, V> implements Iterator<K> {
  _KeyIterator(HashMap<K, V> map) : super(map);
  K get current => (_current != null) ? _current.key : null;
}

class _ValueIterable<K, V> extends IterableBase<V> implements Iterable<V> {
  final HashMap<K, V> _map;
  _ValueIterable(this._map);
  Iterator<V> get iterator => new _ValueIterator<K, V>(_map);
}

class _ValueIterator<K, V>
    extends _HashNodeIterator<K, V> implements Iterator<V> {
  _ValueIterator(HashMap<K, V> map) : super(map);
  V get current => (_current != null) ? _current.value : null;
}

class _HashSet<E> extends IterableBase<E> implements HashSet<E> {
  static const int _INITIAL_SIZE = 8;

  List _buckets;
  int _elements = 0;

  _HashSet() : _buckets = new List(_INITIAL_SIZE);

  _HashSet._(int buckets) : _buckets = new List(buckets);

  Iterator<E> get iterator => new _HashSetIterator(this);

  Set<E> toSet() => new _HashSet.from(this);

  int get length => _elements;

  bool get isEmpty => _elements == 0;

  bool get isNotEmpty => _elements != 0;

  bool add(E value) {
    var bucketCount = _buckets.length;
    if (_elements > (bucketCount - (bucketCount >> 2))) {
      var rehashed = new _HashSet<E>._(bucketCount * 2);
      rehashed.addAll(this);
      _buckets = rehashed._buckets;
      bucketCount = bucketCount * 2;
    }
    var hash = value.hashCode.abs();
    var index = hash % bucketCount;
    var node = _buckets[index];
    while (node != null) {
      if (node.value == value) {
        return false;
      }
      node = node.next;
    }
    node = new _HashSetNode(value);
    node.next = _buckets[index];
    _buckets[index] = node;
    ++_elements;
    return true;
  }

  void addAll(Iterable<E> elements) {
    elements.forEach((E each) {
      add(each);
    });
  }

  bool remove(Object value) {
    var hash = value.hashCode.abs();
    var index = hash % _buckets.length;
    var node = _buckets[index];
    var previous = null;
    while (node != null) {
      if (node.value == value) {
        if (previous == null) {
          _buckets[index] = node.next;
        } else {
          previous.next = node.next;
        }
        --_elements;
        return true;
      }
      previous = node;
      node = node.next;
    }
    return false;
  }

  E lookup(Object object) {
    var hash = object.hashCode.abs();
    var index = hash % _buckets.length;
    var node = _buckets[index];
    while (node != null) {
      if (node.value == object) return node.value;
      node = node.next;
    }
    return null;
  }

  bool contains(Object object) => lookup(object) != null;

  void removeAll(Iterable<Object> elements) {
    elements.forEach((E each) {
      remove(each);
    });
  }

  void retainAll(Iterable<Object> elements) {
    throw new UnimplementedError("HashSet.retainAll");
  }

  void removeWhere(bool test(E element)) {
    throw new UnimplementedError("HashSet.removeWhere");
  }

  void retainWhere(bool test(E element)) {
    throw new UnimplementedError("HashSet.retainWhere");
  }

  bool containsAll(Iterable<Object> other) {
    throw new UnimplementedError("HashSet.containsAll");
  }

  Set<E> intersection(Set<Object> other) {
    throw new UnimplementedError("HashSet.intersection");
  }

  Set<E> union(Set<E> other) {
    throw new UnimplementedError("HashSet.union");
  }

  Set<E> difference(Set<E> other) {
    throw new UnimplementedError("HashSet.difference");
  }

  void clear() {
    _buckets = new List(_INITIAL_SIZE);
    _elements = 0;
  }
}

class _HashSetNode<E> {
  final E value;
  _HashSetNode next;

  _HashSetNode(this.value);
}

class _HashSetIterator<E> implements Iterator<E> {
  final _HashSet _set;

  int _index = -1;
  _HashSetNode<E> _current;

  _HashSetIterator(this._set);

  bool moveNext() {
    if (_current != null) {
      _current = _current.next;
      if (_current != null) return true;
    }
    _index++;
    int limit = _set._buckets.length;
    for (; _index < limit; _index++) {
      if (_set._buckets[_index] != null) {
        _current = _set._buckets[_index];
        return true;
      }
    }
    return false;
  }

  E get current => (_current != null) ? _current.value : null;
}
