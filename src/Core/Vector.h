//
// Created by Orgest on 10/4/2024.
//

#pragma once


#ifdef USE_SMALL_VECTOR
    using vecSizeType = uint32_t;  // Use u32 for smaller vectors
#else
    using vecSizeType = size_t;  // Default to size_t for larger vectors
#endif

template <typename T>
class Vector
{
protected:
    vecSizeType size     = 0;
    vecSizeType capacity = 0;
    T* data              = nullptr;

    void resize(const vecSizeType newCapacity)
    {
        T* newData = new T[newCapacity];
        for (vecSizeType i = 0; i < size; i++)
        {
            newData[i] = data[i];  // Copy elements
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    Vector()
    {
        size = 0;
        capacity = 10;
        data = new T[capacity];
    }

    // Size and initial value
    explicit Vector(vecSizeType n, const T& value = T())
    {
        size = n;
        capacity = n * 2;
        data = new T[capacity];
        for (vecSizeType i = 0; i < size; i++)
        {
            data[i] = value;
        }
    }

    // Copy constructor
    Vector(const Vector& other)
    {
        size = other.size;
        capacity = other.capacity;
        data = new T[capacity];
        for (vecSizeType i = 0; i < size; i++)
        {
            data[i] = other.data[i];
        }
    }

    // Destructor
    ~Vector()
    {
        delete[] data;
    }

    [[nodiscard]] vecSizeType Size() const
    {
        return size;
    }

    [[nodiscard]] vecSizeType Capacity() const
    {
        return capacity;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return size == 0;
    }

    T& operator[](vecSizeType idx)
    {
        return data[idx];
    }

    const T& operator[](vecSizeType idx) const
    {
        return data[idx];
    }

    // Assignment operator
    Vector& operator=(const Vector& other)
    {
        if (this != &other)  // Self-assignment check
        {
            delete[] data;
            size = other.size;
            capacity = other.capacity;
            data = new T[capacity];
            for (vecSizeType i = 0; i < size; i++)
            {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    void PushBack(const T& object)
    {
        if (size == capacity)
        {
            resize(capacity * 2);  // Resize if capacity is reached
        }
        data[size] = object;
        size++;
    }

    void PopBack()
    {
        if (size > 0)
        {
            size--;
            // Shrink capacity if size is much smaller
            if (size < capacity / 4)
            {
                resize(capacity / 2);
            }
        }
    }

    void Erase(const vecSizeType position)
    {
        if (position >= size) return;  // Out of bounds check
        for (vecSizeType i = position; i < size - 1; i++)
        {
            data[i] = data[i + 1];
        }
        size--;
    }

    void Insert(vecSizeType idx, const T& object)
    {
        if (idx > size) return;  // Out of bounds check
        if (size == capacity)
        {
            resize(capacity * 2);  // Resize if capacity is reached
        }
        for (vecSizeType i = size; i > idx; i--)
        {
            data[i] = data[i - 1];
        }
        data[idx] = object;
        size++;
    }

    void Clear()
    {
        size = 0;
    }

    // Returns a pointer to the underlying array
    T* Data()
    {
        return data;
    }

    // Returns a pointer to the first element (for iteration)
    T* Begin()
    {
        return data;
    }

    // Returns a pointer just past the last element (for iteration)
    T* End()
    {
        return data + size;
    }

    // Const versions of Data, Begin, and End for read-only access
    const T* Data() const
    {
        return data;
    }

    const T* Begin() const
    {
        return data;
    }

    const T* End() const
    {
        return data + size;
    }
};
